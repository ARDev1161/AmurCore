#ifndef WORKER_H
#define WORKER_H

#include <QObject>

// Template Method pattern: defines lifecycle hooks onStart/onTick/onStop
class WorkerBase : public QObject
{
    Q_OBJECT

public:
    WorkerBase();
    ~WorkerBase() override;

public slots:
    void process();
    void stop();

signals:
    void finished();

protected:
    virtual void onStart() {}
    virtual void onTick() {}
    virtual void onStop() {}

private:
    bool running {true};
};

#endif // WORKER_H
