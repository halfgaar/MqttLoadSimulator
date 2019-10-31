#include "oneclient.h"
#include "utils.h"
#include "iostream"
#include <QSslConfiguration>
#include <QHostInfo>

OneClient::OneClient(QString &hostname, quint16 port, QString &username, QString &password, bool pub_and_sub, int clientNr, QString &clientIdPart,
                     bool ssl, QString clientPoolRandomId, QObject *parent) :
    QObject(parent),
    client_id(QString("mqtt_load_tester_%1_%2_%3").arg(clientIdPart).arg(clientNr).arg(GetRandomString())),
    clientNr(clientNr),
    pub_and_sub(pub_and_sub),
    publishTimer(this),
    clientPoolRandomId(clientPoolRandomId)
{
    if (ssl)
    {
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);

        this->client = new QMQTT::Client(hostname, port, sslConfig, true);
    }
    else
    {
        // Ehm, why the difference in QMTT::Client's overloaded constructors for SSL and non-SSL?
        QHostInfo info = QHostInfo::fromName(hostname);
        QHostAddress targetHost = info.addresses().first();
        this->client = new QMQTT::Client(targetHost, port);
    }

    client->setClientId(client_id);
    client->setUsername(username);
    client->setPassword(password.toLatin1());

    connect(client, &QMQTT::Client::connected, this, &OneClient::connected);
    connect(client, &QMQTT::Client::disconnected, this, &OneClient::onDisconnect);
    connect(client, &QMQTT::Client::error, this, &OneClient::onClientError);

    int interval = (qrand() % 3000) + 1000;

    publishTimer.setInterval(interval);
    publishTimer.setSingleShot(false);
    connect(&publishTimer, &QTimer::timeout, this, &OneClient::onPublishTimerTimeout);

    reconnectTimer.setInterval(interval + 1000);
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
    if (!client->isConnectedToHost())
    {
        std::cout << "Connecting...\n";
        client->connectToHost();
    }
}

void OneClient::connected()
{
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
        QString ran = GetRandomString();
        QString topic = QString("/silentpath/%1/#").arg(ran);
        std::cout << qPrintable(QString("Subscribing to '%1'\n").arg(topic));
        client->subscribe(topic);
    }
}

void OneClient::onDisconnect()
{
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

    QString msg = QString("Client %1 error code: %2 (%3). Initiated delayed reconnect.\n").arg(this->client_id).arg(error).arg(errStr);
    std::cerr << msg.toLatin1().toStdString().data();

    this->reconnectTimer.start();

}

void OneClient::onPublishTimerTimeout()
{
    //QTimer *sender = static_cast<QTimer*>(this->sender());

    for (int i = 0; i < 25; i++)
    {
        QString payload = QString("Client %1 publish counter: %2").arg(client_id).arg(this->publish_counter++);
        QMQTT::Message msg(0, QString("/loadtester/clientpool_%1/%2/hellofromtheloadtester").arg(this->clientPoolRandomId).arg(this->clientNr), payload.toUtf8());
        client->publish(msg);
    }
}
