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

#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QCommandLineParser>
#include <type_traits>

class ArgumentException : public std::runtime_error
{
public:
    ArgumentException(const std::string &s);
};

QString GetRandomString();
void seedQtrand();
std::string formatString(const std::string str, ...);

template<class T>
typename std::enable_if<std::is_signed<T>::value, T>::type
parseIntOption(QCommandLineParser &parser, QCommandLineOption &option)
{
    bool parsed = false;

    T val = parser.value(option).toInt(&parsed);
    if (!parsed)
    {
        throw ArgumentException(formatString("Option %s is not an int", qPrintable(option.names().first())));
    }
    return val;
}

template<class T>
typename std::enable_if<std::is_unsigned<T>::value, T>::type
parseIntOption(QCommandLineParser &parser, QCommandLineOption &option)
{
    bool parsed = false;

    T val = parser.value(option).toUInt(&parsed);
    if (!parsed)
    {
        throw ArgumentException(formatString("Option %s is not an uint", qPrintable(option.names().first())));
    }
    return val;
}

#endif // UTILS_H
