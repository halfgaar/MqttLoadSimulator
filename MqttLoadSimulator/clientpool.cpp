#include "clientpool.h"
#include <QDateTime>
#include <QThread>
#include <utils.h>

#define PUBLISH_INTERVAL 10

ClientPool::ClientPool(const PoolArguments &args) : QObject(nullptr),
    delay(args.delay)
{
    this->clientPoolRandomId = GetRandomString();

    connectNextBatchTimer.setSingleShot(delay == 0);
    connectNextBatchTimer.setInterval(static_cast<int>(delay));
    connect(&connectNextBatchTimer, &QTimer::timeout, this, &ClientPool::startClients);

    if (delay > 0)
        connectNextBatchTimer.start();

    QStringList hostnameList = args.hostnameList.split(",", QString::SplitBehavior::SkipEmptyParts);

    if (hostnameList.isEmpty() && !args.hostname.isEmpty())
        hostnameList.append(args.hostname);

    this->clients.reserve(args.amount);

    for (int i = 0; i < args.amount; i++)
    {
        const QString &hostname = hostnameList[i % hostnameList.size()];
        OneClient *oneClient = new OneClient(hostname, args.port, args.username, args.password, args.pub_and_sub, i, args.clientIdPart, args.ssl, this->clientPoolRandomId,
                                             args.amount, args.delay, args.burst_interval, args.burst_spread, args.burst_size, args.overrideReconnectInterval, args.topic,
                                             args.qos, args.retain, args.incrementTopicPerBurst, args.clientid, args.cleanSession, args.clientCertificatePath,
                                             args.clientPrivateKeyPath);
        clients.append(oneClient);
        clientsToConnect.push(oneClient);
    }

    publishTimer.setInterval(PUBLISH_INTERVAL);
    connect(&publishTimer, &QTimer::timeout, this, &ClientPool::publishNextRound);
    publishTimer.start();
}

ClientPool::~ClientPool()
{
    qDeleteAll(clients);
}

Counters ClientPool::getTotalCounters() const
{
    Counters total;

    for(const OneClient *c : clients)
    {
        total += c->getCounters();
    }

    return total;
}

int ClientPool::getClientCount() const
{
    return clients.size();
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

void ClientPool::publishNextRound()
{
    // TODO: ordered map and break if next client in future?

    auto now = std::chrono::steady_clock::now();

    for(OneClient *c : clients)
    {
        c->publishIfIntervalExpired(now);
    }
}
