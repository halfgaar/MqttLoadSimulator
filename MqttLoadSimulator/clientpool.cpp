#include "clientpool.h"
#include <QDateTime>

ClientPool::ClientPool(QString hostname, quint16 port, QString username, QString password, bool pub_and_sub, int amount, QString clientIdPart, QObject *parent) : QObject(parent)
{
    //qsrand(static_cast<uint>(QDateTime::currentMSecsSinceEpoch()));

    for (int i = 0; i < amount; i++)
    {
        OneClient *oneClient = new OneClient(hostname, port, username, password, pub_and_sub, i, clientIdPart, parent);
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
    }
}
