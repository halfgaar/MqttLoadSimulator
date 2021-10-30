#include "threadloopdriftguage.h"

ThreadLoopDriftGuage::ThreadLoopDriftGuage(QObject *parent) : QObject(parent)
{
    connect(&timer, &QTimer::timeout, this, &ThreadLoopDriftGuage::onTimout);
    timer.setInterval(HEARTBEAT);
}

void ThreadLoopDriftGuage::start()
{
    timer.start();
}

int ThreadLoopDriftGuage::getMainLoopDrift() const
{
    return mainLoopDrift;
}

void ThreadLoopDriftGuage::onTimout()
{
    std::chrono::milliseconds msSinceLastTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - prevCountWhen);
    mainLoopDrift = std::abs(HEARTBEAT - msSinceLastTime.count());
    prevCountWhen = std::chrono::steady_clock::now();
}
