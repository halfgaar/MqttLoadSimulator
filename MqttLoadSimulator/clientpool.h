#ifndef CLIENTPOOL_H
#define CLIENTPOOL_H

#include <QObject>
#include <oneclient.h>
#include <QStack>

#include "counters.h"
#include "poolarguments.h"

class ClientPool : public QObject
{
    Q_OBJECT

    QList<OneClient*> clients;
    QStack<OneClient*> clientsToConnect;
    QTimer connectNextBatchTimer;
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
};

#endif // CLIENTPOOL_H
