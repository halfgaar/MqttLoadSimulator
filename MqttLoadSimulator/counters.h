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

#ifndef COUNTERS_H
#define COUNTERS_H

#include <stdint.h>
#include <chrono>

struct Counters
{
    uint64_t received = 0;
    uint64_t publish = 0;
    uint64_t connect = 0;
    uint64_t disconnect = 0;
    uint64_t error = 0;

    void operator+=(const Counters &rhs);
    Counters operator-(const Counters &rhs) const;
    void normalizeToPerSecond(std::chrono::milliseconds period);
};

#endif // COUNTERS_H
