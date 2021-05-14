#include "clientpool.h"
#include <QDateTime>
#include <QThread>
#include <utils.h>

ClientPool::ClientPool(QString hostname, quint16 port, QString username, QString password, bool pub_and_sub, int amount, QString clientIdPart,
                       uint delay, bool ssl, QObject *parent) : QObject(parent), delay(delay)
{
    //qsrand(static_cast<uint>(QDateTime::currentMSecsSinceEpoch()));
    this->clientPoolRandomId = GetRandomString();

    connectNextBatchTimer.setSingleShot(true);
    connectNextBatchTimer.setInterval(static_cast<int>(delay));
    connect(&connectNextBatchTimer, &QTimer::timeout, this, &ClientPool::startClients);

    for (int i = 0; i < amount; i++)
    {
        OneClient *oneClient = new OneClient(hostname, port, username, password, pub_and_sub, i, clientIdPart, ssl, this->clientPoolRandomId, amount, delay, this);
        clients.append(oneClient);
        clientsToConnect.push(oneClient);
    }
}

void ClientPool::startClients()
{
    int i = 0;
    while(!this->clientsToConnect.empty())
    {
        OneClient *client = this->clientsToConnect.pop();
        client->connectToHost();

        // Doing this with the timer to avoid blocking the event loop and allowing other events to be processed first.
        if (this->delay > 0)
        {
            connectNextBatchTimer.start();
            break;
        }
        else
        {
            if (i++ > 100)
            {
                connectNextBatchTimer.setInterval(0);
                connectNextBatchTimer.start();
                break;
            }
        }
    }
}
