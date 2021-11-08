#include "clientnumberpool.h"

uint ClientNumberPool::modulo = 0;
uint ClientNumberPool::count = 0;
std::mutex ClientNumberPool::mut;

void ClientNumberPool::setModulo(uint m)
{
    modulo = m;
}

uint ClientNumberPool::getClientNr()
{
    std::lock_guard<std::mutex> locker(mut);
    return count++ % modulo;
}
