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

#ifndef THREADLOOPDRIFTGUAGE_H
#define THREADLOOPDRIFTGUAGE_H

#include <QObject>
#include <QTimer>
#include <chrono>

#define HEARTBEAT 1000

class ThreadLoopDriftGuage : public QObject
{
    Q_OBJECT

    QTimer timer;
    int mainLoopDrift = 0;
    std::chrono::time_point<std::chrono::steady_clock> prevCountWhen = std::chrono::steady_clock::now();

private slots:
    void onTimout();
public:
    explicit ThreadLoopDriftGuage(QObject *parent = nullptr);

    void start();
    int getMainLoopDrift() const;

signals:

};

#endif // THREADLOOPDRIFTGUAGE_H
