#ifndef DOSSER_H
#define DOSSER_H

#include <qmqtt.h>
#include <QTimer>
#include <QHostInfo>

#include "counters.h"

class OneClient : public QObject
{
    Q_OBJECT

    QString client_id;
    int clientNr = 0;
    bool pub_and_sub = false;
    QTimer publishTimer;

    QMQTT::Client *client;
    QTimer reconnectTimer;
    QString clientPoolRandomId;

    const int burstSize;
    QString topicString;

    Counters counters;

    static bool dnsDone;
    static QHostInfo targetHostInfo;

    bool _connected = false;

    QString usernameBase;
    QString passwordBase;
    bool regenRandomUsername = false;
    bool regenRandomPassword = false;

    const QString &subscribeTopic;

private slots:

    void connected();
    void onDisconnect();
    void onClientError(const QMQTT::ClientError error);
    void onPublishTimerTimeout();
    void onReceived(const QMQTT::Message& message);
public:
    OneClient(QString &hostname, quint16 port, QString &username, QString &password, bool pub_and_sub, int clientNr, QString &clientIdPart,
              bool ssl, QString clientPoolRandomId, const int totalClients, const int delay, int burst_interval, int burst_size, int overrideReconnectInterval,
              const QString &subscribeTopic, QObject *parent = nullptr);
    ~OneClient();

    Counters getCounters() const;

public slots:
    void connectToHost();
};

#endif // DOSSER_H
