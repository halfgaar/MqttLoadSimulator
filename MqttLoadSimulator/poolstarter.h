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
