#ifndef DOSSER_H
#define DOSSER_H

#include <qmqtt.h>
#include <QTimer>
#include <QHostInfo>
#include <QHash>

#include "counters.h"

class OneClient : public QObject
{
    Q_OBJECT

    QString client_id;
    int clientNr = 0;
    bool pub_and_sub = false;
    QTimer publishTimer;

    QMQTT::Client *client = nullptr;
    QTimer reconnectTimer;
    QString clientPoolRandomId;

    const int burstSize;
    const QString topicBase;
    QString publishTopic;
    QString subscribeTopic;
    QString payloadBase;
    const uint qos;
    const bool retain;

    Counters counters;

    thread_local static QHash<QString, QHostInfo> dnsCache;

    bool _connected = false;

    QString usernameBase;
    QString passwordBase;
    bool regenRandomUsername = false;
    bool regenRandomPassword = false;
    const bool incrementTopicPerBurst;

    quint16 packetid = 0;

private:
    quint16 getNextPacketPacketID();

private slots:

    void connected();
    void onDisconnect();
    void onClientError(const QMQTT::ClientError error);
    void onPublishTimerTimeout();
    void onReceived(const QMQTT::Message& message);
public:
    OneClient(const QString &hostname, quint16 port, const QString &username, const QString &password, bool pub_and_sub, int clientNr, const QString &clientIdPart,
              bool ssl, const QString &clientPoolRandomId, const int totalClients, const int delay, int burst_interval, const uint burst_spread,
              int burst_size, int overrideReconnectInterval, const QString &topic, uint qos, bool retain, bool incrementTopicPerBurst,
              const QString &clientid, bool cleanSession, const QString &clientCertPath, const QString &clientPrivateKeyPath, QObject *parent = nullptr);
    ~OneClient();

    Counters getCounters() const;

public slots:
    void connectToHost();
};

#endif // DOSSER_H
