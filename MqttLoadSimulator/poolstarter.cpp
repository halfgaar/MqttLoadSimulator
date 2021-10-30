#include "poolstarter.h"
#include "utils.h"

std::mutex PoolStarter::clientCreateMutex;

PoolStarter::PoolStarter(const PoolArguments &args) :
    args(args)
{

}

std::unique_ptr<ClientPool> &PoolStarter::getClientPool()
{
    return c;
}

void PoolStarter::makeClientPool()
{
    seedQtrand();

    /* This proved necessary to avoid:
     *
     * "Type conversion already registered from type QSharedPointer<QNetworkSession> to type QObject*"
     *
     * I didn't see any real deleterious effects, but still better safe than sorry.
     */
    std::lock_guard<std::mutex> locker(clientCreateMutex);

    c.reset(new ClientPool(this->args));
    QTimer::singleShot(0, c.get(), &ClientPool::startClients);
}
