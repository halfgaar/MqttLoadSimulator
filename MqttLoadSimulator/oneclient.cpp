#include "oneclient.h"
#include "utils.h"
#include "iostream"
#include <QSslConfiguration>


// Hack, not thread safe, but we don't need that (for now)
bool OneClient::dnsDone = false;
QHostInfo OneClient::targetHostInfo;

OneClient::OneClient(QString &hostname, quint16 port, QString &username, QString &password, bool pub_and_sub, int clientNr, QString &clientIdPart,
                     bool ssl, QString clientPoolRandomId, const int totalClients, const int delay, int burst_interval, int burst_size, int overrideReconnectInterval,
                     const QString &subscribeTopic, QObject *parent) :
    QObject(parent),
    client_id(QString("mqtt_load_tester_%1_%2_%3").arg(clientIdPart).arg(clientNr).arg(GetRandomString())),
    clientNr(clientNr),
    pub_and_sub(pub_and_sub),
    publishTimer(this),
    clientPoolRandomId(clientPoolRandomId),
    burstSize(burst_size),
    topicString(QString("/loadtester/clientpool_%1/%2/hellofromtheloadtester").arg(this->clientPoolRandomId).arg(this->clientNr)),
    subscribeTopic(subscribeTopic)
{
    if (ssl)
    {
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);

        this->client = new QMQTT::Client(hostname, port, sslConfig, true);
    }
    else
    {
        QHostAddress targetHost;

        if (!this->dnsDone)
        {
            this->targetHostInfo = QHostInfo::fromName(hostname);
            this->dnsDone = true;
        }

        // Ehm, why the difference in QMTT::Client's overloaded constructors for SSL and non-SSL?
        targetHost = targetHostInfo.addresses().first();
        this->client = new QMQTT::Client(targetHost, port);
    }

    if (username.contains("%1"))
    {
        this->usernameBase = username;
        regenRandomUsername = true;
        username = QString(this->usernameBase).arg(GetRandomString());
    }

    if (password.contains("%1"))
    {
        this->passwordBase = password;
        regenRandomPassword = true;
        password = QString(this->passwordBase).arg(GetRandomString());
    }

    client->setClientId(client_id);
    client->setUsername(username);
    client->setPassword(password.toLatin1());

    int keepAlive = 60;
    client->setKeepAlive(keepAlive);

    connect(client, &QMQTT::Client::connected, this, &OneClient::connected);
    connect(client, &QMQTT::Client::disconnected, this, &OneClient::onDisconnect);
    connect(client, &QMQTT::Client::error, this, &OneClient::onClientError);
    connect(client, &QMQTT::Client::received, this, &OneClient::onReceived);

    int interval = (qrand() % burst_interval) + 1000;

    publishTimer.setInterval(interval);
    publishTimer.setSingleShot(false);
    connect(&publishTimer, &QTimer::timeout, this, &OneClient::onPublishTimerTimeout);

    const int totalConnectionDuration = ((totalClients + 1) / (1000.0 / (delay + 1))) * 1000;
    const int reconnectInterval = overrideReconnectInterval >= 0 ? overrideReconnectInterval : 5000 + (qrand() % totalConnectionDuration);
    reconnectTimer.setInterval(reconnectInterval);
    reconnectTimer.setSingleShot(true);
    connect(&reconnectTimer, &QTimer::timeout, this, &OneClient::connectToHost);
}

OneClient::~OneClient()
{
    client->disconnectFromHost();
    delete client;
}

void OneClient::connectToHost()
{
    if (!_connected) // client->isConnectedToHost() checks the wrong thing (whether socket is connected), and is true when SSL is still being negotiated.
    {
        std::cout << "Connecting...\n";
        client->connectToHost();
    }
}

void OneClient::connected()
{
    _connected = true;

    //QMQTT::Client *sender = static_cast<QMQTT::Client *>(this->sender());
    std::cout << "Connected.\n";

    if (this->pub_and_sub)
    {
        QString topic = QString("/loadtester/clientpool_%1/%2/#").arg(this->clientPoolRandomId).arg(this->clientNr - 1);
        std::cout << qPrintable(QString("Subscribing to '%1'\n").arg(topic));
        client->subscribe(topic);
        publishTimer.start();
    }
    else
    {
        QString topic = this->subscribeTopic;

        if (topic.isEmpty())
        {
            QString ran = GetRandomString();
            topic = QString("/silentpath/%1/#").arg(ran);
        }
        std::cout << qPrintable(QString("Subscribing to '%1'\n").arg(topic));
        client->subscribe(topic);
    }
}

void OneClient::onDisconnect()
{
    _connected = false;

    QString msg = QString("Client %1 disconnected\n").arg(this->client_id);
    std::cout << msg.toLatin1().toStdString().data();
}

void OneClient::onClientError(const QMQTT::ClientError error)
{
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

    QString msg = QString("Client %1 error code: %2 (%3). Initiated delayed reconnect.\n").arg(this->client_id).arg(error).arg(errStr);
    std::cerr << msg.toLatin1().toStdString().data();

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

    //QTimer *sender = static_cast<QTimer*>(this->sender());

    for (int i = 0; i < burstSize; i++)
    {
        QString payload = QString("Client %1 publish counter: %2").arg(client_id).arg(this->publish_counter++);
        QMQTT::Message msg(0, topicString, payload.toUtf8());
        client->publish(msg);
        this->publishCount++;
    }
}

void OneClient::onReceived(const QMQTT::Message &message)
{
    Q_UNUSED(message)
    this->receivedCount++;
    //std::cout << qPrintable(message.payload()) << std::endl;
}
