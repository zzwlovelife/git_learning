#ifndef BTHREAD_EXAMPLE_THREADS_H
#define BTHREAD_EXAMPLE_THREADS_H

#include "business_thread.h"
#include <QTimer>
#include <QRandomGenerator>

namespace bthread {

// 数据采集线程：定时生成模拟数据并广播
class DataAcquireThread : public BusinessThread {
    Q_OBJECT
public:
    explicit DataAcquireThread(const QString& name = "acquire", QObject* parent = nullptr)
        : BusinessThread(name, parent) {}

protected:
    bool onInit() override {
        timer_ = new QTimer();
        timer_->setInterval(1000);  // 每秒采集一次
        connect(timer_, &QTimer::timeout, this, [this]() {
            int value = QRandomGenerator::global()->bounded(100);  // 模拟采集值
            logInfo("acquired value=" + QString::number(value));
            broadcast("data_acquired", value);
        });
        return true;
    }
    void onStart() override { timer_->start(); }
    void onStop() override { timer_->stop(); }
    void onCleanup() override { delete timer_; }

private:
    QTimer* timer_ = nullptr;
};

// 数据处理线程：订阅采集数据，处理后广播
class DataProcessThread : public BusinessThread {
    Q_OBJECT
public:
    explicit DataProcessThread(const QString& name = "process", QObject* parent = nullptr)
        : BusinessThread(name, parent) {}

protected:
    bool onInit() override {
        logInfo("processor ready");
        return true;
    }
    void handleMessage(const Message& msg) override {
        if (msg.type == "data_acquired") {
            int raw = msg.data.toInt();
            int processed = raw * 2 + 1;  // 模拟处理：放大+偏移
            logInfo(QString("process %1 -> %2").arg(raw).arg(processed));
            broadcast("data_processed", processed);
        }
    }
};

// 数据分发线程：订阅处理结果，模拟分发/上报
class DataDistributeThread : public BusinessThread {
    Q_OBJECT
public:
    explicit DataDistributeThread(const QString& name = "distribute", QObject* parent = nullptr)
        : BusinessThread(name, parent) {}

protected:
    bool onInit() override {
        logInfo("distributor ready");
        return true;
    }
    void handleMessage(const Message& msg) override {
        if (msg.type == "data_processed") {
            int value = msg.data.toInt();
            logInfo("distribute value=" + QString::number(value));
            // 实际项目中可在此调用通信库（netclient）上报
        }
    }
};

} // namespace bthread

#endif // BTHREAD_EXAMPLE_THREADS_H
