#ifndef CLIENTNUMBERPOOL_H
#define CLIENTNUMBERPOOL_H

#include <mutex>

class ClientNumberPool
{
    static uint count;
    static uint modulo;
    static std::mutex mut;
public:

    static void setModulo(uint m);
    static uint getClientNr();
};

#endif // CLIENTNUMBERPOOL_H
