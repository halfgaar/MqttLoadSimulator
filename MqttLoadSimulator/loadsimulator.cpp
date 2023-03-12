/*
This file is part of MqttLoadSimulator
Copyright (C) 2023  Wiebe Cazemier

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.
*/

#include "loadsimulator.h"
#include "stdio.h"
#include "utils.h"
#include "poolarguments.h"
#include "cassert"

#define STATS_INTERVAL 1000

LoadSimulator::LoadSimulator(int &argc, char **argv) : QCoreApplication(argc, argv),
    statsTimer()
{
    statsTimer.setInterval(STATS_INTERVAL);

    connect(&statsTimer, &QTimer::timeout, this, &LoadSimulator::onStatsTimeout);
    statsTimer.start();

    for(int i = 0; i < QThread::idealThreadCount(); i++)
    {
        std::unique_ptr<QThread> t(new QThread());
        t->start();
        t->setObjectName(QString("MqttLoadSim %1").arg(i));
        threads.push_back(std::move(t));
    }

    for(uint i = 0; i < threads.size(); i++)
    {
        std::unique_ptr<ThreadLoopDriftGuage> t(new ThreadLoopDriftGuage());
        t->moveToThread(threads[i].get());
        t->start();
        threadDriftGuages.push_back(std::move(t));
    }
}

LoadSimulator::~LoadSimulator()
{
    for(auto &t : threads)
    {
        t->quit();
        t->wait();
    }
}

/**
 * @brief LoadSimulator::createPoolsBasedOnArgument creates as many clients as specified by args, but divides them over the threads.
 * @param args
 */
void LoadSimulator::createPoolsBasedOnArgument(const PoolArguments &args)
{
    if (args.amount == 0)
        return;

    std::vector<int> subamounts(threads.size());
    const int n = args.amount / threads.size() + (args.amount % threads.size());

    int totalAmount = args.amount;
    for(int &c : subamounts)
    {
        c = std::min<int>(n, totalAmount);
        totalAmount -= n;

        if (totalAmount <= 0)
            break;
    }

    assert(std::accumulate(subamounts.begin(), subamounts.end(), 0) == args.amount);

    for (uint i = 0; i < subamounts.size(); i++)
    {
        int c = subamounts[i];
        if (c <= 0)
            continue;

        PoolArguments args2(args);
        args2.amount = c;

        std::unique_ptr<PoolStarter> ps(new PoolStarter(args2));
        ps->moveToThread(threads[i].get());
        QTimer::singleShot(0, ps.get(), &PoolStarter::makeClientPool);
        starters.push_back(std::move(ps));
    }
}

std::string LoadSimulator::getDriftString(Drift drift) const
{
    std::string col = "\033[01;32m";
    std::string status = "OK";

    if (drift.avg > 200 || drift.max > 200)
    {
        col = "\033[01;31m";
        status = "tester overload!";
    }
    else if (drift.avg > 100 || drift.max > 100)
    {
        col = "\033[01;33m";
        status = "lagging...";
    }

    std::string s = formatString("%sAvg: %.2f ms, max: %d ms (%s)\033[00m", col.c_str(), drift.avg, drift.max, status.c_str());
    return s;
}

Drift LoadSimulator::getAvgDriftLoop() const
{
    Drift result;

    std::vector<int> sizes(threadDriftGuages.size());

    for(uint i = 0; i < threadDriftGuages.size(); i++)
    {
        auto &t = threadDriftGuages[i];
        int n = t->getMainLoopDrift();
        sizes[i] = n;
        result.max = std::max<int>(n, result.max);
    }

    result.avg = std::accumulate(sizes.begin(), sizes.end(), 0.0) / sizes.size();
    return result;
}

void LoadSimulator::onStatsTimeout()
{
    Counters cnt;
    int totalClients = 0;

    for(std::unique_ptr<PoolStarter> &s : starters)
    {
        std::unique_ptr<ClientPool> &c = s->getClientPool();

        if (!c)
            return;

        cnt += c->getTotalCounters();
        totalClients += c->getClientCount();
    }

    Counters diff = cnt - prevCounts;
    std::chrono::milliseconds msSinceLastTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - prevCountWhen);
    diff.normalizeToPerSecond(msSinceLastTime);

    Drift drift = getAvgDriftLoop();

    const uint64_t diffCount = std::max(cnt.publish, cnt.received) - std::min(cnt.publish, cnt.received);

    std::string driftString = getDriftString(drift);
    std::string line = formatString("\rVersion: %s. \033[01mClients\033[00m: %d on %d threads. "
                                    "\033[01mSent\033[00m: %ld (\033[01;36m%ld/s\033[00m). "
                                    "\033[01mRecv\033[00m: %ld (\033[01;36m%ld/s\033[00m). "
                                    "\033[01mRecv-Sent\033[00m: %ld. "
                                    "\033[01mConnects\033[00m: %ld (\033[01;36m%ld/s\033[00m). "
                                    "\033[01mDisconnects\033[00m: %ld (\033[01;36m%ld/s\033[00m). "
                                    "\033[01mErrors\033[00m: %ld (\033[01;36m%ld/s\033[00m). "
                                    "\nThread loop drift: %s",
                                    applicationVersion().toStdString().c_str(),
                                    totalClients, threads.size(), cnt.publish, diff.publish, cnt.received, diff.received, diffCount, cnt.connect, diff.connect,
                                    cnt.disconnect, diff.disconnect, cnt.error, diff.error, driftString.c_str());

    if (firstTimePrinted)
        fputs("\033[2K\033[A1\033[2K", stdout); // clear two lines in VT100 codes
    firstTimePrinted = true;
    fputs(line.c_str(), stdout);
    fflush(stdout);

    prevCounts = cnt;
    prevCountWhen = std::chrono::steady_clock::now();
}
