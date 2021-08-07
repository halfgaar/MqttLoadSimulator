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

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    seedQtrand();

#ifdef Q_OS_LINUX
    rlim_t rlim = 1000000;
    printf("Setting ulimit nofile to %ld.\n", rlim);
    struct rlimit v = { rlim, rlim };
    if (setrlimit(RLIMIT_NOFILE, &v) != 0)
    {
        fputs(qPrintable("WARNING: Changing ulimit failed.\n"), stderr);
    }
#endif

    QCommandLineParser parser;
    parser.setApplicationDescription("MQTT load simulator. The active clients subscribe to the topics of every previous active clients.\nThe passive "
                                     "clients connect and subscribe to a random path that likely doesn't exist."
                                     "\n\n"
                                     "The active clients publish <msg-per-burst> topics, every <burst-interval> mseconds (with randomization added)."
                                     "\n\n"
                                     "It's useful to note that clients with high publish volume are heavier for servers, because it entails more\n"
                                     "topic parsing, subscriber lookup, etc."
                                     "\n\n"
                                     "Also, primarily because Qt has one event loop on the main thread, this app is single threaded and not highly\n"
                                     "efficient. But, probalby not less than other stress testers I've seen.");
    parser.addHelpOption();

    QCommandLineOption hostnameOption("hostname", "Hostname of target. Required.", "hostname");
    parser.addOption(hostnameOption);

    QCommandLineOption portOption("port", "Target port. Default: 1883|8883", "port", "1883");
    parser.addOption(portOption);

    QCommandLineOption sslOption("ssl", "Enable SSL. Always insecure mode.");
    parser.addOption(sslOption);

    QCommandLineOption amountActiveOption("amount-active", "Amount of active clients. Required.", "amount");
    parser.addOption(amountActiveOption);

    QCommandLineOption amountPassiveOption("amount-passive", "Amount of passive clients with one silent subscription. Required.", "amount");
    parser.addOption(amountPassiveOption);

    QCommandLineOption usernameOption("username", "Username. DEFAULT: user", "username", "user");
    parser.addOption(usernameOption);

    QCommandLineOption passwordOption("password", "Password. DEFAULT: password", "password", "user");
    parser.addOption(passwordOption);

    QCommandLineOption clientStartupDelayOption("delay", "Wait <ms> milliseconds between each connecting client", "ms", "0");
    parser.addOption(clientStartupDelayOption);

    QCommandLineOption clientBurstIntervaltOption("burst-interval", "Publish <msg-per-burst> messages per <burst interval> (+/- random spread). DEFAULT: 3000", "ms", "3000");
    parser.addOption(clientBurstIntervaltOption);

    QCommandLineOption clientMessageCountPerBurstOption("msg-per-burst", "Publish x messages per <burst interval>. Default: 25", "amount", "25");
    parser.addOption(clientMessageCountPerBurstOption);

    parser.process(a);

    if (!parser.isSet(hostnameOption))
    {
        fputs(qPrintable("Hostname is required\n"), stderr);
        return 1;
    }

    if (!parser.isSet(amountActiveOption))
    {
        fputs(qPrintable("Amount of active clients is required\n"), stderr);
        return 1;
    }

    if (!parser.isSet(amountPassiveOption))
    {
        fputs(qPrintable("Amount of passive clients is required\n"), stderr);
        return 1;
    }

    bool parsed = false;
    quint16 port = static_cast<quint16>(parser.value(portOption).toInt(&parsed));
    if (!parsed)
    {
        fputs(qPrintable("Port is not a number\n"), stderr);
        return 1;
    }

    bool ssl = false;

    if (parser.isSet(sslOption))
    {
        ssl = true;

        if (!parser.isSet(portOption))
        {
            port = 8883;
        }
    }

    int amountActive = parser.value(amountActiveOption).toInt(&parsed);
    if (!parsed)
    {
        fputs(qPrintable("amount-active is not a number\n"), stderr);
        return 1;
    }

    int amountPassive = parser.value(amountPassiveOption).toInt(&parsed);
    if (!parsed)
    {
        fputs(qPrintable("amount-active is not a number\n"), stderr);
        return 1;
    }

    uint delay = 0;

    if (parser.isSet(clientStartupDelayOption))
    {
        delay = parser.value(clientStartupDelayOption).toUInt(&parsed);
        if (!parsed)
        {
            fputs(qPrintable("delay is not a positive number\n"), stderr);
            return 1;
        }

    }

    int burstInterval = parser.value(clientBurstIntervaltOption).toInt(&parsed);
    if (!parsed)
    {
        fputs(qPrintable("--burst-interval is not a number\n"), stderr);
        return 1;
    }

    int burstSize = parser.value(clientMessageCountPerBurstOption).toInt(&parsed);
    if (!parsed)
    {
        fputs(qPrintable("--msg-per-burst is not a number\n"), stderr);
        return 1;
    }

    QString hostname = parser.value(hostnameOption);

    ClientPool poolActive(hostname, port, parser.value(usernameOption), parser.value(passwordOption), true, amountActive, "active", delay, ssl, burstInterval, burstSize, &a);
    poolActive.startClients();

    ClientPool poolPassive(hostname, port, parser.value(usernameOption), parser.value(passwordOption), false, amountPassive, "passive", delay, ssl, burstInterval, burstSize, &a);
    poolPassive.startClients();

    return a.exec();
}
