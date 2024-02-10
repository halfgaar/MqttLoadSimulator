/*
This file is part of MqttLoadSimulator
Copyright (C) 2023  Wiebe Cazemier

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.
*/

#include "oneclient.h"
#include "utils.h"
#include "iostream"
#include <QSslConfiguration>
#include <QFile>
#include <QSslKey>
#include <iostream>

#include "globals.h"
#include "clientnumberpool.h"


thread_local QHash<QString, QHostInfo> OneClient::dnsCache;

OneClient::OneClient(const QString &hostname, quint16 port, const QString &username, const QString &password, bool pub_and_sub, int clientNr, const QString &clientIdPart,
                     bool ssl, const QString &clientPoolRandomId, const int totalClients, const int delay, int burst_interval, const uint burst_spread,
                     int burst_size, int overrideReconnectInterval, const QString &topic, uint qos, bool retain, bool incrementTopicPerBurst,
                     const QString &clientid, bool cleanSession, const QString &clientCertPath, const QString &clientPrivateKeyPath, QObject *parent) :
    QObject(parent),
    client_id(!clientid.isEmpty() ? clientid : QString("%1_%2_%3_%4").arg(QHostInfo::localHostName()).arg(clientIdPart).arg(clientNr).arg(GetRandomString())),
    clientNr(clientNr),
    pub_and_sub(pub_and_sub),
    clientPoolRandomId(clientPoolRandomId),
    burstSize(burst_size),
    topicBase(topic),
    payloadBase(QString("Client %1 publish counter: %2. current_steady_time:%3").arg(client_id)),
    qos(qos),
    retain(retain),
    incrementTopicPerBurst(incrementTopicPerBurst)
{
    if (ssl)
    {
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);

        if (!clientCertPath.isEmpty() || !clientPrivateKeyPath.isEmpty())
        {
            QFile fcert(clientCertPath);
            if (!fcert.open(QFile::ReadOnly))
                throw std::runtime_error("Error reading client certificate");
            const QByteArray fcertData  = fcert.readAll();
            QSslCertificate _cert(fcertData);

            QFile fkey(clientPrivateKeyPath);
            if (!fkey.open(QFile::ReadOnly))
                throw std::runtime_error("Error reading private key");
            const QByteArray keyData = fkey.readAll();
            QSslKey _sslKey(keyData, QSsl::KeyAlgorithm::Rsa);

            sslConfig.setLocalCertificate(_cert);
            sslConfig.setPrivateKey(_sslKey);
        }

        this->client = new QMQTT::Client(hostname, port, sslConfig, true);
    }
    else
    {
        if (!dnsCache.contains(hostname))
        {
            dnsCache[hostname] = QHostInfo::fromName(hostname);
        }

        auto addresses = dnsCache[hostname].addresses();

        if (addresses.empty())
        {
            std::cerr << "Hostname '" << hostname.toStdString() << "' doesn't resolve to anything" << std::endl;

            // Just making a dummy client because I can't throw exceptions because we're calling this from slots.
            this->client = new QMQTT::Client("dummy", 1883, false, true, nullptr);

            return;
        }
        else
        {
            const int ran = qrand() % addresses.length();

            // Ehm, why the difference in QMTT::Client's overloaded constructors for SSL and non-SSL?
            this->client = new QMQTT::Client(addresses.at(ran), port);
        }
    }

    if (!topic.isEmpty())
    {
        if (topic.contains("%1"))
        {
            int nr = ClientNumberPool::getClientNr();
            subscribeTopic = QString(topic).arg(nr);

            if (pub_and_sub)
                publishTopic = subscribeTopic;
        }
        else
        {
            subscribeTopic = topic;
            publishTopic = topic;
        }
    }
    else
    {
        if (pub_and_sub)
        {
            publishTopic = QString("loadtester/clientpool_%1/%2/hellofromtheloadtester").arg(this->clientPoolRandomId).arg((this->clientNr + 1) % totalClients);
            subscribeTopic = QString("loadtester/clientpool_%1/%2/#").arg(this->clientPoolRandomId).arg(this->clientNr);
        }
        else
        {
            QString ran = GetRandomString();
            subscribeTopic = QString("silentpath/%1/#").arg(ran);
        }
    }

    QString u = username;
    if (username.contains("%1"))
    {
        this->usernameBase = username;
        regenRandomUsername = true;
        u = QString(this->usernameBase).arg(GetRandomString());
    }

    QString p = password;
    if (password.contains("%1"))
    {
        this->passwordBase = password;
        regenRandomPassword = true;
        p = QString(this->passwordBase).arg(GetRandomString());
    }

    client->setClientId(client_id);
    client->setUsername(u);
    client->setPassword(p.toUtf8());
    client->setCleanSession(cleanSession);

    int keepAlive = 60;
    client->setKeepAlive(keepAlive);

    connect(client, &QMQTT::Client::connected, this, &OneClient::connected);
    connect(client, &QMQTT::Client::disconnected, this, &OneClient::onDisconnect);
    connect(client, &QMQTT::Client::error, this, &OneClient::onClientError);
    connect(client, &QMQTT::Client::received, this, &OneClient::onReceived);

    int spread = burst_spread/2 - (qrand() % burst_spread);
    int interval = burst_interval + spread;
    interval = std::max<int>(1, interval);

    this->publishInterval = std::chrono::milliseconds(interval);
    this->nextPublish = std::chrono::steady_clock::now() + std::chrono::milliseconds(interval);

    const int totalConnectionDuration = ((totalClients + 1) / (1000.0 / (delay + 1))) * 1000;
    const int reconnectInterval = overrideReconnectInterval >= 0 ? overrideReconnectInterval : 5000 + (qrand() % totalConnectionDuration);
    reconnectTimer.setInterval(reconnectInterval);
    reconnectTimer.setSingleShot(true);
    connect(&reconnectTimer, &QTimer::timeout, this, &OneClient::connectToHost);
}

OneClient::~OneClient()
{
    if (client)
    {
        client->disconnectFromHost();
        delete client;
        client = nullptr;
    }
}

Counters OneClient::getCounters() const
{
    return counters;
}

void OneClient::publishIfIntervalExpired(std::chrono::time_point<std::chrono::steady_clock> now)
{
    if (!_connected)
        return;

    if (!startPublishing)
        return;

    if (this->nextPublish > now)
        return;

    this->nextPublish = now + this->publishInterval;

    onPublishTimerTimeout();

}

LatencyValues OneClient::getLatencies()
{
    LatencyValues result(this->latencies);
    return result;
}

bool OneClient::getPubAndSub() const
{
    return this->pub_and_sub;
}

void OneClient::setPayloadFormat(const QString &s, int max_value)
{
    this->payloadBase = s;
    this->payloadMaxValue = max_value;
}

void OneClient::connectToHost()
{
    if (!_connected) // client->isConnectedToHost() checks the wrong thing (whether socket is connected), and is true when SSL is still being negotiated.
    {
        if (Globals::verbose)
            std::cout << "Connecting...\n";
        client->connectToHost();
    }
}

/**
 * @brief OneClient::getNextPacketPacketID gets the next packet ID
 * @return
 *
 * We have to do this ourselves, because QMQTT has a bug, in which is wraps around back to use the invalid value of 0.
 */
quint16 OneClient::getNextPacketPacketID()
{
    this->packetid++;
    if (this->packetid == 0)
        this->packetid++;
    return this->packetid;
}

void OneClient::parseLatency(const QMQTT::Message &message)
{
    const QByteArray payload = message.payload();
    const int time_index = payload.indexOf("current_steady_time:");

    if (time_index < 0)
        return;

    const QByteArray time_section = payload.mid(time_index, -1);
    QString s = QString::fromUtf8(time_section);
    QStringList fields = s.split(':');

    if (fields.size() < 2)
        return;

    QString timestamp_numbers;
    const QString string_tail = fields.at(1);

    int end = 0;
    for (QChar c : string_tail)
    {
        if (c.isDigit())
            end++;
        else
            break;
    }

    timestamp_numbers = string_tail.left(end);

    bool ok = false;
    long timestamp = timestamp_numbers.toLong(&ok);

    if (!ok)
        return;

    auto published_at = std::chrono::time_point<std::chrono::steady_clock>() + std::chrono::microseconds(timestamp);
    std::chrono::microseconds latency = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - published_at);
    latencies[latency_index++ % latencies.size()] = latency;
}

void OneClient::connected()
{
    _connected = true;
    counters.connect++;

    if (Globals::verbose)
        std::cout << "Connected.\n";

    if (this->pub_and_sub)
    {
        if (Globals::verbose)
        {
            std::cout << qPrintable(QString("Subscribing to '%1'\n").arg(subscribeTopic));

            if (incrementTopicPerBurst && topicBase.contains("%1"))
                std::cout << qPrintable(QString("Publishing to '%1' (and increasing number per publish)\n").arg(publishTopic));
            else
                std::cout << qPrintable(QString("Publishing to '%1'\n").arg(publishTopic));
        }
        client->subscribe(subscribeTopic, this->qos);
        startPublishing = true;
    }
    else
    {
        if (Globals::verbose)
            std::cout << qPrintable(QString("Subscribing to '%1'\n").arg(subscribeTopic));
        client->subscribe(subscribeTopic);
    }
}

void OneClient::onDisconnect()
{
    _connected = false;
    counters.disconnect++;

    if (Globals::verbose)
    {
        QString msg = QString("Client %1 disconnected\n").arg(this->client_id);
        std::cout << msg.toLatin1().toStdString().data();
    }
}

void OneClient::onClientError(const QMQTT::ClientError error)
{
    counters.error++;

    // TODO: arg, doesn't qmqtt have a better way for this?
    QString errStr = QString("unknown error");
    if (error == QMQTT::SocketConnectionRefusedError)
        errStr = "Connection refused";
    if (error == QMQTT::SocketRemoteHostClosedError)
        errStr = "Remote host closed";
    if (error == QMQTT::SocketHostNotFoundError)
        errStr = "Remote host not found";
    if (error == QMQTT::MqttBadUserNameOrPasswordError)
        errStr = "MQTT bad user or password";
    if (error == QMQTT::MqttNotAuthorizedError)
        errStr = "MQTT not authorized";
    if (error == QMQTT::SocketResourceError)
        errStr = "Socket resource error. Is your OS limiting you? Ulimit, etc?";
    if (error == QMQTT::SocketSslInternalError)
        errStr = "Socket SSL internal error.";
    if (error == QMQTT::SocketTimeoutError)
        errStr = "Socket timeout";

    if (Globals::verbose)
    {
        QString msg = QString("Client %1 error code: %2 (%3). Initiated delayed reconnect.\n").arg(this->client_id).arg(error).arg(errStr);
        std::cerr << msg.toLatin1().toStdString().data();
    }

    if (regenRandomUsername)
    {
        const QString newUsername = QString(this->usernameBase).arg(GetRandomString());
        client->setUsername(newUsername);
    }

    if (regenRandomPassword)
    {
        const QString newPassword = QString(this->passwordBase).arg(GetRandomString());
        client->setPassword(newPassword.toLatin1());
    }

    this->reconnectTimer.start();

}

void OneClient::onPublishTimerTimeout()
{
    // https://github.com/emqx/qmqtt/issues/230
    if (!_connected)
        return;

    if (this->publishTopic.isEmpty())
        return;

    for (int i = 0; i < burstSize; i++)
    {
        const long stamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        QString payload;

        if (payloadBase.contains("%1") || payloadBase.contains("%2"))
        {
            payload = payloadBase.arg(counters.publish).arg(stamp);
        }
        else
        {
            int value = rand() % this->payloadMaxValue;
            payload = payloadBase;
            payload.replace("%%utc_time%%", QString::fromStdString(utc_time()));
            payload.replace("%%random_value%%", QString::number(value));

            QString latency_stamp = QString("current_steady_time:%1").arg(stamp);
            payload.replace("%%latency%%", latency_stamp);
        }

        QMQTT::Message msg(getNextPacketPacketID(), publishTopic, payload.toUtf8(), this->qos, this->retain);
        client->publish(msg);
        counters.publish++;
    }

    if (incrementTopicPerBurst)
    {
        const int nr = ClientNumberPool::getClientNr();
        publishTopic = QString(topicBase).arg(nr);
    }
}

void OneClient::onReceived(const QMQTT::Message &message)
{
    Q_UNUSED(message)
    counters.received++;

    parseLatency(message);
}












