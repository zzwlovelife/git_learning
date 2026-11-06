#ifndef BTHREAD_EXAMPLE_THREADS_H
#define BTHREAD_EXAMPLE_THREADS_H

#include "business_thread.h"
#include <QTimer>
#include <QRandomGenerator>

namespace bthread {

// =============================================================================
// 示例业务线程：演示如何扩展 BusinessThread 基类
// 流水线：DataAcquire -> DataProcess -> DataDistribute
//   采集线程每秒产生一个随机数，广播 "data_acquired"
//   处理线程收到后做 *2+1，广播 "data_processed"
//   分发线程收到后打印（模拟上报）
// =============================================================================

// 数据采集线程：定时生成模拟数据并广播
class DataAcquireThread : public BusinessThread {
    Q_OBJECT
public:
    explicit DataAcquireThread(const QString& name = "acquire", QObject* parent = nullptr)
        : BusinessThread(name, parent) {}

protected:
    // 初始化：创建定时器（不设 parent，因为在子线程创建，需手动 delete）
    bool onInit() override {
        timer_ = new QTimer();
        timer_->setInterval(1000);  // 每秒采集一次
        connect(timer_, &QTimer::timeout, this, [this]() {
            int value = QRandomGenerator::global()->bounded(100);  // 模拟采集值
            logInfo("acquired value=" + QString::number(value));
            broadcast("data_acquired", value);  // 广播给所有订阅线程
        });
        return true;
    }
    void onStart() override { timer_->start(); }   // 进入事件循环前启动定时器
    void onStop() override { timer_->stop(); }      // 事件循环退出后停止
    void onCleanup() override { delete timer_; }    // 最终释放

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
    // 收到消息：按 type 过滤，仅处理 "data_acquired"
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
            // 实际项目中可在此调用通信库（netclient）上报数据
        }
    }
};

} // namespace bthread

#endif // BTHREAD_EXAMPLE_THREADS_H
