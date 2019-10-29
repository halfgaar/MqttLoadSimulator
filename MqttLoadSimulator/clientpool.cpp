#include "clientpool.h"
#include <QDateTime>
#include <QThread>

ClientPool::ClientPool(QString hostname, quint16 port, QString username, QString password, bool pub_and_sub, int amount, QString clientIdPart,
                       uint delay, bool ssl, QObject *parent) : QObject(parent), delay(delay)
{
    //qsrand(static_cast<uint>(QDateTime::currentMSecsSinceEpoch()));

    for (int i = 0; i < amount; i++)
    {
        OneClient *oneClient = new OneClient(hostname, port, username, password, pub_and_sub, i, clientIdPart, ssl, parent);
        clients.append(oneClient);
    }
}

ClientPool::~ClientPool()
{
    qDeleteAll(clients);
}

void ClientPool::startClients()
{
    foreach (OneClient *client, clients)
    {
        client->connectToHost();
        if (delay > 0)
            QThread::msleep(static_cast<unsigned long>(delay));
    }
}
