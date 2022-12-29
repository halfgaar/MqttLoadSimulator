#ifndef CLIENTPOOL_H
#define CLIENTPOOL_H

#include <QObject>
#include <oneclient.h>
#include <QStack>
#include <QVector>

#include "counters.h"
#include "poolarguments.h"

class ClientPool : public QObject
{
    Q_OBJECT

    QVector<OneClient*> clients;
    QStack<OneClient*> clientsToConnect;
    QTimer connectNextBatchTimer;
    QTimer publishTimer;
    uint delay;
    QString clientPoolRandomId;
public:
    explicit ClientPool(const PoolArguments &args);
    ~ClientPool();

    Counters getTotalCounters() const;
    int getClientCount() const;

signals:

public slots:
    void startClients();

private slots:
    void publishNextRound();
};

#endif // CLIENTPOOL_H
