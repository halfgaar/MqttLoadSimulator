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

#ifndef CLIENTPOOL_H
#define CLIENTPOOL_H

#include <QObject>
#include <oneclient.h>
#include <QStack>
#include <QVector>

#include "counters.h"
#include "poolarguments.h"

class ClientPool : public QObject
{
    Q_OBJECT

    QVector<OneClient*> clients;
    QStack<OneClient*> clientsToConnect;
    QTimer connectNextBatchTimer;
    QTimer publishTimer;
    uint delay;
    bool deferPublishing;
    QString clientPoolRandomId;
public:
    explicit ClientPool(const PoolArguments &args);
    ~ClientPool();

    Counters getTotalCounters() const;
    int getClientCount() const;
    std::vector<LatencyValues> getAllLatencies() const;

signals:

public slots:
    void startClients();

private slots:
    void publishNextRound();
};

#endif // CLIENTPOOL_H
