#ifndef CLIENTPOOL_H
#define CLIENTPOOL_H

#include <QObject>
#include <oneclient.h>
#include <QStack>

#include "counters.h"

class ClientPool : public QObject
{
    Q_OBJECT

    QList<OneClient*> clients;
    QStack<OneClient*> clientsToConnect;
    QTimer connectNextBatchTimer;
    uint delay;
    QString clientPoolRandomId;
public:
    explicit ClientPool(QString hostname, quint16 port, QString username, QString password, bool pub_and_sub, int amount, QString clientIdPart,
                        uint delay, bool ssl, int burst_interval, int burst_size, int overrideReconnectInterval, const QString &subscribeTopic,
                        QObject *parent = nullptr);
    ~ClientPool();

    Counters getTotalCounters() const;
    int getClientCount() const;

signals:

public slots:
    void startClients();
};

#endif // CLIENTPOOL_H
