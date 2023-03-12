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

#include "counters.h"

void Counters::operator+=(const Counters &rhs)
{
    received += rhs.received;
    publish += rhs.publish;
    connect += rhs.connect;
    disconnect += rhs.disconnect;
    error += rhs.error;
}

Counters Counters::operator-(const Counters &rhs) const
{
    Counters r;
    r.received = received - rhs.received;
    r.publish = publish - rhs.publish;
    r.connect = connect - rhs.connect;
    r.disconnect = disconnect - rhs.disconnect;
    r.error = error - rhs.error;
    return r;
}

void Counters::normalizeToPerSecond(std::chrono::milliseconds period)
{
    if (period.count() == 0)
        return;

    double factor = 1000.0 / static_cast<double>(period.count());
    received *= factor;
    publish *= factor;
    connect *= factor;
    disconnect *= factor;
    error *= factor;
}
