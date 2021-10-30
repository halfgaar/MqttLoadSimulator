#ifndef POOLSTARTER_H
#define POOLSTARTER_H

#include <QObject>
#include <poolarguments.h>
#include <clientpool.h>
#include <memory>
#include <mutex>


/**
 * @brief The PoolStarter class is a wrapper to construct a ClientPool, to be able to run it in a thread, and therefore create all the new objects in a thread.
 */
class PoolStarter : public QObject
{
    PoolArguments args;
    std::unique_ptr<ClientPool> c;

    static std::mutex clientCreateMutex;

public:
    PoolStarter(const PoolArguments &args);
    std::unique_ptr<ClientPool> &getClientPool();

public slots:
    void makeClientPool();
};

#endif // POOLSTARTER_H
