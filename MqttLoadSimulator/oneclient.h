#ifndef DOSSER_H
#define DOSSER_H

#include <qmqtt.h>
#include <QTimer>
#include <QHostInfo>

class OneClient : public QObject
{
    Q_OBJECT

    QString client_id;
    int publish_counter = 0;
    int clientNr = 0;
    bool pub_and_sub = false;
    QTimer publishTimer;

    QMQTT::Client *client;
    QTimer reconnectTimer;
    QString clientPoolRandomId;

    uint receivedCount = 0;
    uint publishCount = 0;

    static bool dnsDone;
    static QHostInfo targetHostInfo;

private slots:

    void connected();
    void onDisconnect();
    void onClientError(const QMQTT::ClientError error);
    void onPublishTimerTimeout();
    void onReceived(const QMQTT::Message& message);
public:
    OneClient(QString &hostname, quint16 port, QString &username, QString &password, bool pub_and_sub, int clientNr, QString &clientIdPart,
              bool ssl, QString clientPoolRandomId, QObject *parent = nullptr);
    ~OneClient();

public slots:
    void connectToHost();
};

#endif // DOSSER_H
