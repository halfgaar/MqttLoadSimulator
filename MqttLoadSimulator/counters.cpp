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
