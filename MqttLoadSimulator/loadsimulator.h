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

#ifndef LOADSIMULATOR_H
#define LOADSIMULATOR_H

#include <QtCore>
#include <QObject>
#include <QTimer>
#include <chrono>
#include <QThread>
#include <memory>

#include "clientpool.h"
#include "counters.h"
#include "poolstarter.h"
#include "threadloopdriftguage.h"

struct Drift
{
    double avg = 0;
    int max = 0;
};

/**
 * @brief The LoadSimulator class is a bit of a hack to make the client pools available to timer events. A better way would be to move everything from main() in here.
 */
class LoadSimulator : public QCoreApplication
{
    Q_OBJECT

    QTimer statsTimer;
    Counters prevCounts;
    bool firstTimePrinted = false;
    std::chrono::time_point<std::chrono::steady_clock> prevCountWhen = std::chrono::steady_clock::now();

    std::vector<std::unique_ptr<PoolStarter>> starters;
    std::vector<std::unique_ptr<QThread>> threads;

    std::vector<std::unique_ptr<ThreadLoopDriftGuage>> threadDriftGuages;

    std::string getDriftString(Drift drift) const;
    Drift getAvgDriftLoop() const;
private slots:
    void onStatsTimeout();
public:
    explicit LoadSimulator(int &argc, char **argv);
    ~LoadSimulator();
    void createPoolsBasedOnArgument(const PoolArguments &args);

signals:

};

#endif // LOADSIMULATOR_H
