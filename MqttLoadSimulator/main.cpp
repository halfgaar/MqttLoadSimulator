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

    QCommandLineOption clientBurstIntervaltOption("burst-interval", "Publish <msg-per-burst> messages per <burst interval> (+/- random spread). DEFAULT: 3000", "ms", "3000");
    parser.addOption(clientBurstIntervaltOption);

    QCommandLineOption clientMessageCountPerBurstOption("msg-per-burst", "Publish x messages per <burst interval>. Default: 25", "amount", "25");
    parser.addOption(clientMessageCountPerBurstOption);

    QCommandLineOption overrideReconnectIntervalOption("reconnect-interval", "Time between reconnect on error. Default: dynamic.", "ms", "-1");
    parser.addOption(overrideReconnectIntervalOption);

    QCommandLineOption passiveSubscribeTopic("subscribe-topic", "Topic for passive clients to subscribe to. Default: random per client", "topic");
    parser.addOption(passiveSubscribeTopic);

    QCommandLineOption verboseOption("verbose", "Print debugging info. Warning: ugly.");
    parser.addOption(verboseOption);

    parser.process(a);

    try
    {
        quint16 port = parseIntOption<quint16>(parser, portOption);
        const int amountActive = parseIntOption<int>(parser, amountActiveOption);
        const int amountPassive = parseIntOption<int>(parser, amountPassiveOption);
        const int burstInterval = parseIntOption<int>(parser, clientBurstIntervaltOption);
        const int burstSize = parseIntOption<int>(parser, clientMessageCountPerBurstOption);
        const int overrideReconnectInterval = parseIntOption<int>(parser, overrideReconnectIntervalOption);
        const uint delay = parseIntOption<uint>(parser, clientStartupDelayOption);

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
        activePoolArgs.burst_size = burstSize;
        activePoolArgs.overrideReconnectInterval = overrideReconnectInterval;
        activePoolArgs.subscribeTopic = parser.value(passiveSubscribeTopic);
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
