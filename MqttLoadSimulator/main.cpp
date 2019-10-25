#include <QtCore>
#include "clientpool.h"
#include <iostream>
#include <QTimer>
#include <QCommandLineParser>
#include <QProcess>

#ifdef Q_OS_LINUX
#include <sys/resource.h>
#endif

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

#ifdef Q_OS_LINUX
    puts("Setting ulimit nofile to 100000.\n");
    struct rlimit v = { 1000000, 1000000 };
    if (setrlimit(RLIMIT_NOFILE, &v) != 0)
    {
        fputs(qPrintable("Can't set ulimit nofile to 1000000\n"), stderr);
        return 1;
    }
#endif

    QCommandLineParser parser;
    parser.setApplicationDescription("MQTT load simulator. The active clients subscribe to the topics of every previous active clients.\nThe passive "
                                     "clients connect and subscribe to a random path that likely doesn't exist."
                                     "\n\n"
                                     "The active clients publish 25 topics, every 1-4 seconds (randomly picked per client).");
    parser.addHelpOption();

    QCommandLineOption hostnameOption("hostname", "Hostname of target. Required.", "hostname");
    parser.addOption(hostnameOption);

    QCommandLineOption portOption("port", "Target port. Default: 1883", "port", "1883");
    parser.addOption(portOption);

    QCommandLineOption amountActiveOption("amount-active", "Amount of active clients. Required.", "amount");
    parser.addOption(amountActiveOption);

    QCommandLineOption amountPassiveOption("amount-passive", "Amount of passive clients with one silent subscription. Required.", "amount");
    parser.addOption(amountPassiveOption);

    QCommandLineOption usernameOption("username", "Username. DEFAULT: user", "username", "user");
    parser.addOption(usernameOption);

    QCommandLineOption passwordOption("password", "Password. DEFAULT: password", "username", "user");
    parser.addOption(passwordOption);

    QCommandLineOption clientStartupDelayOption("delay", "Wait <ms> milliseconds between each connecting client", "ms", "0");
    parser.addOption(clientStartupDelayOption);

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

    qsrand(static_cast<uint>(QDateTime::currentMSecsSinceEpoch()));

    QString hostname = parser.value(hostnameOption);

    ClientPool poolActive(hostname, port, parser.value(usernameOption), parser.value(passwordOption), true, amountActive, "active", delay, &a);
    poolActive.startClients();

    ClientPool poolPassive(hostname, port, parser.value(usernameOption), parser.value(passwordOption), false, amountPassive, "passive", delay, &a);
    poolPassive.startClients();

    return a.exec();
}
