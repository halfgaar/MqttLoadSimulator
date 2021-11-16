#ifndef POOLARGUMENTS_H
#define POOLARGUMENTS_H

#include <QString>

struct PoolArguments
{
    QString hostname;
    QString hostnameList;
    quint16 port;
    QString username;
    QString password;
    bool pub_and_sub = false;
    int amount = 0;
    QString clientIdPart;
    uint delay = 0;
    bool ssl = false;
    int burst_interval = 0;
    uint burst_spread = 0;
    int burst_size = 0;
    int overrideReconnectInterval = -1;
    bool incrementTopicPerPublish = false;
    QString topic;
    uint qos;
};

#endif // POOLARGUMENTS_H
