#include "clientpool.h"
#include <QDateTime>
#include <QThread>
#include <utils.h>

ClientPool::ClientPool(QString hostname, quint16 port, QString username, QString password, bool pub_and_sub, int amount, QString clientIdPart,
                       uint delay, bool ssl, int burst_interval, int burst_size, QObject *parent) : QObject(parent), delay(delay)
{
    this->clientPoolRandomId = GetRandomString();

    connectNextBatchTimer.setSingleShot(delay == 0);
    connectNextBatchTimer.setInterval(static_cast<int>(delay));
    connect(&connectNextBatchTimer, &QTimer::timeout, this, &ClientPool::startClients);

    if (delay > 0)
        connectNextBatchTimer.start();

    for (int i = 0; i < amount; i++)
    {
        OneClient *oneClient = new OneClient(hostname, port, username, password, pub_and_sub, i, clientIdPart, ssl, this->clientPoolRandomId,
                                             amount, delay, burst_interval, burst_size, parent);
        clients.append(oneClient);
        clientsToConnect.push(oneClient);
    }
}

ClientPool::~ClientPool()
{
    qDeleteAll(clients);
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

    if (clientsToConnect.empty())
    {
        connectNextBatchTimer.stop();
    }
}
