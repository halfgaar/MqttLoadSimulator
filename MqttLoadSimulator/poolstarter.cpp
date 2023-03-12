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
