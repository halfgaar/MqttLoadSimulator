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
