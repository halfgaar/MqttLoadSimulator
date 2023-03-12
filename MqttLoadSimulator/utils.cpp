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

#include "utils.h"

#include "sys/random.h"

#include "vector"

QString GetRandomString()
{
   const QString possibleCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
   const int randomStringLength = 12; // assuming you want random strings of 12 characters

   std::vector<unsigned char> buf(randomStringLength);
   ssize_t actual_len = getrandom(buf.data(), randomStringLength, 0);

   if (actual_len < 0 || actual_len != randomStringLength)
   {
       throw std::runtime_error("Error requesting random data");
   }

   QString randomString;
   for(const unsigned char c : buf)
   {
       const int index = c % possibleCharacters.length();
       QChar nextChar = possibleCharacters.at(index);
       randomString.append(nextChar);
   }
   return randomString;
}

/**
 * @brief seedQtrand qsrand() is per thread, so call this per thread.
 */
void seedQtrand()
{
    const size_t s = sizeof (uint);
    std::vector<unsigned char> buf(s);
    ssize_t actual_len = getrandom(buf.data(), s, 0);

    if (actual_len < 0 || actual_len != s)
    {
        throw std::runtime_error("Error requesting random data");
    }

    uint result = 0;
    uint shift = 0;

    for (unsigned char c : buf)
    {
        if (shift > 32)
            break;

        result |= c << shift;
        shift += 8;
    }

    qsrand(result);
}

std::string formatString(const std::string str, ...)
{
    char buf[512];

    va_list valist;
    va_start(valist, str);
    vsnprintf(buf, 512, str.c_str(), valist);
    va_end(valist);

    size_t len = strlen(buf);
    std::string result(buf, len);

    return result;
}

ArgumentException::ArgumentException(const std::string &s) : std::runtime_error(s)
{

}
