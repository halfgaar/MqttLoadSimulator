#include "oneclient.h"
#include "utils.h"
#include "iostream"
#include <QSslConfiguration>


// Hack, not thread safe, but we don't need that (for now)
bool OneClient::dnsDone = false;
QHostInfo OneClient::targetHostInfo;

OneClient::OneClient(QString &hostname, quint16 port, QString &username, QString &password, bool pub_and_sub, int clientNr, QString &clientIdPart,
                     bool ssl, QString clientPoolRandomId, const int totalClients, const int delay, QObject *parent) :
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

    client->setClientId(client_id);
    client->setUsername(username);
    client->setPassword(password.toLatin1());

    int keepAlive = 60;
    client->setKeepAlive(keepAlive);

    connect(client, &QMQTT::Client::connected, this, &OneClient::connected);
    connect(client, &QMQTT::Client::disconnected, this, &OneClient::onDisconnect);
    connect(client, &QMQTT::Client::error, this, &OneClient::onClientError);
    connect(client, &QMQTT::Client::received, this, &OneClient::onReceived);

    int interval = (qrand() % 3000) + 1000;

    publishTimer.setInterval(interval);
    publishTimer.setSingleShot(false);
    connect(&publishTimer, &QTimer::timeout, this, &OneClient::onPublishTimerTimeout);

    const int totalConnectionDuration = ((totalClients + 1) / (1000.0 / (delay + 1))) * 1000;
    const int reconnectInterval = 5000 + (qrand() % totalConnectionDuration);
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
    if (error == QMQTT::SocketTimeoutError)
        errStr = "Socket timeout";

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
        this->publishCount++;
    }
}

void OneClient::onReceived(const QMQTT::Message &message)
{
    Q_UNUSED(message)
    this->receivedCount++;
    //std::cout << qPrintable(message.payload()) << std::endl;
}
