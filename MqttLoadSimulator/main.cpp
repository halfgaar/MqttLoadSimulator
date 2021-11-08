#include <QtCore>
#include "clientpool.h"
#include <iostream>
#include <QTimer>
#include <QCommandLineParser>
#include <QProcess>

#ifdef Q_OS_LINUX
#include <sys/resource.h>
#endif

#include "utils.h"

#include "loadsimulator.h"
#include "globals.h"
#include "poolarguments.h"
#include "clientnumberpool.h"

int main(int argc, char *argv[])
{
    LoadSimulator a(argc, argv);
    seedQtrand();



    QCommandLineParser parser;
    parser.setApplicationDescription("MQTT load simulator. The active clients subscribe to the topics of every previous active clients.\nThe passive "
                                     "clients connect and subscribe to a random path that likely doesn't exist."
                                     "\n\n"
                                     "The active clients publish <msg-per-burst> topics, every <burst-interval> mseconds (with randomization added)."
                                     "\n\n"
                                     "It's useful to note that clients with high publish volume are heavier for servers, because it entails more\n"
                                     "topic parsing, subscriber lookup, etc.");
    parser.addHelpOption();

    QCommandLineOption hostnameOption("hostname", "Hostname of target. Default: localhost.", "hostname", "localhost");
    parser.addOption(hostnameOption);

    QCommandLineOption hostnameListOption("hostname-list", "Comma-separated list of hostnames clients will pick round-robin. You need to give a server "
                                                           "multiple IPs and use this when testing more than 30k-ish connections, to work around the TCP "
                                                           "ephemeral port limit.", "list");
    parser.addOption(hostnameListOption);

    QCommandLineOption portOption("port", "Target port. Default: 1883|8883", "port", "1883");
    parser.addOption(portOption);

    QCommandLineOption sslOption("ssl", "Enable SSL. Always insecure mode.");
    parser.addOption(sslOption);

    QCommandLineOption amountActiveOption("amount-active", "Amount of active clients. Default: 1.", "amount", "1");
    parser.addOption(amountActiveOption);

    QCommandLineOption amountPassiveOption("amount-passive", "Amount of passive clients with one silent subscription. Default: 1.", "amount", "1");
    parser.addOption(amountPassiveOption);

    QCommandLineOption usernameOption("username", "Username. DEFAULT: user. Any occurance of %1 will be replaced by a random string, also on each reconnect. "
                                                  "Useful for stress-testing the auth mechanism of a server.", "username", "user");
    parser.addOption(usernameOption);

    QCommandLineOption passwordOption("password", "Password. DEFAULT: password. Any occurance of %1 will be replaced by a random string, also on each reconnect. "
                                                  "Useful for stress-testing the auth mechanism of a server.", "password", "user");
    parser.addOption(passwordOption);

    QCommandLineOption clientStartupDelayOption("delay", "Wait <ms> milliseconds between each connecting client", "ms", "0");
    parser.addOption(clientStartupDelayOption);

    QCommandLineOption clientBurstIntervaltOption("burst-interval", "Publish <msg-per-burst> messages per <burst interval>, per client. DEFAULT: 3000", "ms", "3000");
    parser.addOption(clientBurstIntervaltOption);

    QCommandLineOption clientBurstIntervalSpreadOption("burst-spread", "Add (burst_spread/2 - (RAND() % burst_spread)) to burst-interval. Default: 1000", "ms", "1000");
    parser.addOption(clientBurstIntervalSpreadOption);

    QCommandLineOption clientMessageCountPerBurstOption("msg-per-burst", "Publish x messages per <burst interval>, per client. Default: 25", "amount", "25");
    parser.addOption(clientMessageCountPerBurstOption);

    QCommandLineOption overrideReconnectIntervalOption("reconnect-interval", "Time between reconnect on error. Default: dynamic.", "ms", "-1");
    parser.addOption(overrideReconnectIntervalOption);

    QCommandLineOption topic("topic", "Topic for passive clients to subscribe to and active clients to publish to. Any occurance of %1 is "
                                      "replaced by a number per client, modulo <topic-modulo>. Default: random per client", "topic");
    parser.addOption(topic);

    QCommandLineOption topicModuloOption("topic-modulo", "When using --topic, the counter modulo for '%1'. Default: 1000", "modulo", "1000");
    parser.addOption(topicModuloOption);

    QCommandLineOption incrementTopicPerPublish("increment-topic-per-publish", "Use the '%1' in --topic to increment per publish.");
    parser.addOption(incrementTopicPerPublish);

    QCommandLineOption verboseOption("verbose", "Print debugging info. Warning: ugly.");
    parser.addOption(verboseOption);

    parser.process(a);

    try
    {
        quint16 port = parseIntOption<quint16>(parser, portOption);
        const int amountActive = parseIntOption<int>(parser, amountActiveOption);
        const int amountPassive = parseIntOption<int>(parser, amountPassiveOption);
        const int burstInterval = parseIntOption<int>(parser, clientBurstIntervaltOption);
        const uint burst_spread = parseIntOption<uint>(parser, clientBurstIntervalSpreadOption);
        const int burstSize = parseIntOption<int>(parser, clientMessageCountPerBurstOption);
        const int overrideReconnectInterval = parseIntOption<int>(parser, overrideReconnectIntervalOption);
        const uint delay = parseIntOption<uint>(parser, clientStartupDelayOption);
        const uint modulo = parseIntOption<uint>(parser, topicModuloOption);

        if (burstInterval <= 0)
            throw ArgumentException("Burst interval must be > 0");

        bool ssl = false;
        if (parser.isSet(sslOption))
        {
            ssl = true;
            if (!parser.isSet(portOption))
                port = 8883;
        }

        if (parser.isSet(verboseOption))
        {
            Globals::verbose = true;
        }

        ClientNumberPool::setModulo(modulo);

#ifdef Q_OS_LINUX
        rlim_t rlim = 1000000;
        if (Globals::verbose)
            printf("Setting ulimit nofile to %ld.\n", rlim);
        struct rlimit v = { rlim, rlim };
        if (setrlimit(RLIMIT_NOFILE, &v) != 0)
        {
            fputs(qPrintable("WARNING: Changing ulimit failed.\n"), stderr);
        }
#endif

        PoolArguments activePoolArgs;
        activePoolArgs.hostname = parser.value(hostnameOption);
        activePoolArgs.hostnameList = parser.value(hostnameListOption);
        activePoolArgs.port = port;
        activePoolArgs.username = parser.value(usernameOption);
        activePoolArgs.pub_and_sub = true;
        activePoolArgs.amount = amountActive;
        activePoolArgs.clientIdPart = "active";
        activePoolArgs.delay = delay;
        activePoolArgs.ssl = ssl;
        activePoolArgs.burst_interval = burstInterval;
        activePoolArgs.burst_spread = burst_spread;
        activePoolArgs.burst_size = burstSize;
        activePoolArgs.overrideReconnectInterval = overrideReconnectInterval;
        activePoolArgs.topic = parser.value(topic);
        activePoolArgs.incrementTopicPerPublish = parser.isSet(incrementTopicPerPublish);
        a.createPoolsBasedOnArgument(activePoolArgs);

        PoolArguments passivePoolArgs(activePoolArgs);
        passivePoolArgs.pub_and_sub = false;
        passivePoolArgs.amount = amountPassive;
        passivePoolArgs.clientIdPart = "passive";
        a.createPoolsBasedOnArgument(passivePoolArgs);

        return a.exec();
    }
    catch (std::exception &ex)
    {
        fprintf(stderr, "%s\n", ex.what());
        return 1;
    }
}
