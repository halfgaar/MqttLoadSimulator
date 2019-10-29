#ifndef DOSSER_H
#define DOSSER_H

#include <qmqtt.h>
#include <QTimer>

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

private slots:

    void connected();
    void onDisconnect();
    void onClientError(const QMQTT::ClientError error);
    void onPublishTimerTimeout();
public:
    OneClient(QString &hostname, quint16 port, QString &username, QString &password, bool pub_and_sub, int clientNr, QString &clientIdPart,
              bool ssl, QObject *parent = nullptr);
    ~OneClient();

public slots:
    void connectToHost();
};

#endif // DOSSER_H