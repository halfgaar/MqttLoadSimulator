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
