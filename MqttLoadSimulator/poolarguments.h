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

#ifndef POOLARGUMENTS_H
#define POOLARGUMENTS_H

#include <QString>

struct PoolArguments
{
    QString hostname;
    QString hostnameList;
    quint16 port;
    QString username;
    QString password;
    bool pub_and_sub = false;
    int amount = 0;
    QString clientIdPart;
    uint delay = 0;
    bool ssl = false;
    QString clientCertificatePath;
    QString clientPrivateKeyPath;
    int burst_interval = 0;
    uint burst_spread = 0;
    int burst_size = 0;
    int overrideReconnectInterval = -1;
    bool incrementTopicPerBurst = false;
    QString topic;
    uint qos;
    bool retain = false;
    QString clientid;
    bool cleanSession = true;
    bool deferPublishing = false;
    QString payloadFormat;
    int payload_max_value = 100;
};

#endif // POOLARGUMENTS_H
