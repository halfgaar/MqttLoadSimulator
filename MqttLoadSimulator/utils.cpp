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
