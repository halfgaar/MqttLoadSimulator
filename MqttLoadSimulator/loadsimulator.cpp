#include "loadsimulator.h"
#include "stdio.h"
#include "utils.h"

#define STATS_INTERVAL 500

LoadSimulator::LoadSimulator(int &argc, char **argv) : QCoreApplication(argc, argv),
    statsTimer()
{
    statsTimer.setInterval(STATS_INTERVAL);

    connect(&statsTimer, &QTimer::timeout, this, &LoadSimulator::onStatsTimeout);
    statsTimer.start();
}

void LoadSimulator::addClientPool(QSharedPointer<ClientPool> c)
{
    clientsPools.append(c);
}

std::string LoadSimulator::getMainLoopDriftColor(int drift) const
{
    if (drift > 100)
        return "\033[01;31m";
    if (drift > 50)
        return "\033[01;33m";
    return "\033[01;32m";
}

void LoadSimulator::onStatsTimeout()
{
    Counters cnt;
    int totalClients = 0;

    for(QSharedPointer<ClientPool> c : clientsPools)
    {
        cnt += c->getTotalCounters();
        totalClients += c->getClientCount();
    }

    Counters diff = cnt - prevCounts;
    std::chrono::milliseconds msSinceLastTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - prevCountWhen);
    diff.normalizeToPerSecond(msSinceLastTime);

    const int mainLoopDrift = std::abs(STATS_INTERVAL - msSinceLastTime.count());
    std::string driftColor = getMainLoopDriftColor(mainLoopDrift);
    std::string line = formatString("\rClients: %d. Sent: %ld (%ld/s). Recv: %ld (%ld/s). Connects: %ld (%ld/s). Disconnects: %ld (%ld/s). Errors: %ld (%ld/s). "
                                    "Main loop drift: %s%d ms\033[00m",
                                    totalClients, cnt.publish, diff.publish, cnt.received, diff.received, cnt.connect, diff.connect,
                                    cnt.disconnect, diff.disconnect, cnt.error, diff.error, driftColor.c_str(), mainLoopDrift);

    fputs("\033[2K\r", stdout); // clear line
    fputs(line.c_str(), stdout);
    fflush(stdout);

    prevCounts = cnt;
    prevCountWhen = std::chrono::steady_clock::now();
}
