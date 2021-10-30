#ifndef LOADSIMULATOR_H
#define LOADSIMULATOR_H

#include <QtCore>
#include <QObject>
#include <QTimer>
#include <chrono>

#include "clientpool.h"
#include "counters.h"

/**
 * @brief The LoadSimulator class is a bit of a hack to make the client pools available to timer events. A better way would be to move everything from main() in here.
 */
class LoadSimulator : public QCoreApplication
{
    Q_OBJECT

    QTimer statsTimer;
    Counters prevCounts;
    std::chrono::time_point<std::chrono::steady_clock> prevCountWhen = std::chrono::steady_clock::now();

    QList<QSharedPointer<ClientPool>> clientsPools;

    std::string getMainLoopDriftColor(int drift) const;
private slots:
    void onStatsTimeout();
public:
    explicit LoadSimulator(int &argc, char **argv);
    void addClientPool(QSharedPointer<ClientPool> c);

signals:

};

#endif // LOADSIMULATOR_H
