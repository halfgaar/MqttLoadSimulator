#ifndef THREADLOOPDRIFTGUAGE_H
#define THREADLOOPDRIFTGUAGE_H

#include <QObject>
#include <QTimer>
#include <chrono>

#define HEARTBEAT 1000

class ThreadLoopDriftGuage : public QObject
{
    Q_OBJECT

    QTimer timer;
    int mainLoopDrift = 0;
    std::chrono::time_point<std::chrono::steady_clock> prevCountWhen = std::chrono::steady_clock::now();

private slots:
    void onTimout();
public:
    explicit ThreadLoopDriftGuage(QObject *parent = nullptr);

    void start();
    int getMainLoopDrift() const;

signals:

};

#endif // THREADLOOPDRIFTGUAGE_H
