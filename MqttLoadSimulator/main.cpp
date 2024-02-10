/*
This file is part of MqttLoadSimulator
Copyright (C) 2023  Wiebe Cazemier

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.
*/

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
    a.setApplicationVersion(APPLICATION_VERSION);
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

    QCommandLineOption clientCertificateOption("client-certificate", "Client certificate to be presented to the server for authentication", "path");
    parser.addOption(clientCertificateOption);

    QCommandLineOption clientPrivateKeyOption("client-private-key", "Private key belonging to the client certificate above.", "path");
    parser.addOption(clientPrivateKeyOption);

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

    QCommandLineOption payload_format("payload-format", "Override the payload string. Placeholders are available. "
                                                        "Possible placeholders: %%utc_time%% for an UTC time string, %%value%% for a random int. Also "
                                                        "include %%latency%% to put in the data required for latency calculation.", "format");
    parser.addOption(payload_format);

    QCommandLineOption payload_max_value("payload-max-value", "The maximum value of the %%value%% placeholder from the payload format. Default: 100", "value", "100");
    parser.addOption(payload_max_value);

    QCommandLineOption qosOption("qos", "QoS of publish and subscribe. Default: 0", "qos", "0");
    parser.addOption(qosOption);

    QCommandLineOption retainOption("retain", "Set retain flag on messages");
    parser.addOption(retainOption);

    QCommandLineOption clientidOption("client-id", "Fixed client ID. Because MQTT is forced to disconnect on existing client ID, you can use this to "
                                                   "brute-force test session hand-over/destruction. Default: random", "clientid");
    parser.addOption(clientidOption);

    QCommandLineOption disableCleanSessionOption("disable-clean-session", "Duh.");
    parser.addOption(disableCleanSessionOption);

    QCommandLineOption topicModuloOption("topic-modulo", "When using --topic, the counter modulo for '%1'. Default: 1000", "modulo", "1000");
    parser.addOption(topicModuloOption);

    QCommandLineOption incrementTopicPerBurst("increment-topic-per-burst", "Use the '%1' in --topic to increment per publish burst.");
    parser.addOption(incrementTopicPerBurst);

    QCommandLineOption deferPublishing("defer-publishing", "Defer publishing (within thread) until all clients are connected. Helps the 'recv - sent' stat.");
    parser.addOption(deferPublishing);

    QCommandLineOption verboseOption("verbose", "Print debugging info. Warning: ugly.");
    parser.addOption(verboseOption);

    QCommandLineOption versionOption("version", "Show version.");
    parser.addOption(versionOption);

    QCommandLineOption licenseOption("license", "Show license.");
    parser.addOption(licenseOption);

    parser.process(a);

    try
    {
        if (parser.isSet(versionOption))
        {
            printf("MqttLoadSimulator Version: %s\n", qPrintable(QCoreApplication::applicationVersion()));
            return 0;
        }

        if (parser.isSet(licenseOption))
        {
            puts("Copyright (C) 2019,2020,2021,2022,2023 Wiebe Cazemier.");
            puts("License GPLv2: GNU GPL version 2. <https://www.gnu.org/licenses/old-licenses/gpl-2.0.html>.");
            return 0;
        }

        quint16 port = parseIntOption<quint16>(parser, portOption);
        const int amountActive = parseIntOption<int>(parser, amountActiveOption);
        const int amountPassive = parseIntOption<int>(parser, amountPassiveOption);
        const int burstInterval = parseIntOption<int>(parser, clientBurstIntervaltOption);
        const uint burst_spread = parseIntOption<uint>(parser, clientBurstIntervalSpreadOption);
        const int burstSize = parseIntOption<int>(parser, clientMessageCountPerBurstOption);
        const int overrideReconnectInterval = parseIntOption<int>(parser, overrideReconnectIntervalOption);
        const uint delay = parseIntOption<uint>(parser, clientStartupDelayOption);
        const uint modulo = parseIntOption<uint>(parser, topicModuloOption);
        const uint qos = parseIntOption<uint>(parser, qosOption);

        if (burstInterval <= 0)
            throw ArgumentException("Burst interval must be > 0");

        if (qos > 2)
            throw ArgumentException("QoS must be <= 2");

        bool ssl = false;
        if (parser.isSet(sslOption))
        {
            ssl = true;
            if (!parser.isSet(portOption))
                port = 8883;
        }

        if (parser.isSet(clientCertificateOption) ^ parser.isSet(clientPrivateKeyOption))
        {
            const QStringList cnames = clientCertificateOption.names();
            const QStringList knames = clientPrivateKeyOption.names();

            throw ArgumentException(QString("When you specify '%1', you also have to specify '%2'").
                                    arg(cnames.first(), knames.first()).toStdString());
        }

        const QString clientCertPath = parser.value(clientCertificateOption);
        const QString clientPrivateKeyPath = parser.value(clientPrivateKeyOption);

        if (parser.isSet(clientCertificateOption) || parser.isSet(clientPrivateKeyOption))
        {
            QFile clientCertFile(clientCertPath);
            QFile clientPrivateKeyFile(clientPrivateKeyPath);

            if (!clientCertFile.open(QFile::ReadOnly) || !clientPrivateKeyFile.open(QFile::ReadOnly))
                throw ArgumentException("Can't read client cert or private key");
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
        activePoolArgs.password = parser.value(passwordOption);
        activePoolArgs.pub_and_sub = true;
        activePoolArgs.amount = amountActive;
        activePoolArgs.clientIdPart = "active";
        activePoolArgs.delay = delay;
        activePoolArgs.ssl = ssl;
        activePoolArgs.clientCertificatePath = clientCertPath;
        activePoolArgs.clientPrivateKeyPath = clientPrivateKeyPath;
        activePoolArgs.burst_interval = burstInterval;
        activePoolArgs.burst_spread = burst_spread;
        activePoolArgs.burst_size = burstSize;
        activePoolArgs.overrideReconnectInterval = overrideReconnectInterval;
        activePoolArgs.topic = parser.value(topic);
        activePoolArgs.qos = qos;
        activePoolArgs.retain = parser.isSet(retainOption);
        activePoolArgs.incrementTopicPerBurst = parser.isSet(incrementTopicPerBurst);
        activePoolArgs.clientid = parser.value(clientidOption);
        activePoolArgs.cleanSession = !parser.isSet(disableCleanSessionOption);
        activePoolArgs.deferPublishing = parser.isSet(deferPublishing);
        activePoolArgs.payload_max_value = parseIntOption<int>(parser, payload_max_value);

        if (parser.isSet(payload_format))
        {
            activePoolArgs.payloadFormat = parser.value(payload_format);
        }

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
