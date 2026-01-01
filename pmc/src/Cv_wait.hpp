/*
 * 问：什么是条件变量？
 *
 * 答：条件变量（Condition Variable）是多线程编程中用于线程间同步的核心机制，它允许线程在特定条件不满足时主动阻塞（休眠），并在条件可能满足时被其他线程唤醒，从而避免忙等待（Busy-Waiting）造成的CPU浪费。
 */

#include <thread>
#include <atomic> // 也许将字段flag创建为原子变量？
#include <mutex>
#include <condition_variable>
namespace qing {
    // 用于等待的条件变量类
    // 对同一个对象进行操作能够阻塞当前线程，或者唤醒被阻塞的线程
    class Cv_wait {
    private:

        // 用于控制条件变量的锁
        std::mutex mtx{};

        // 条件变量
        std::condition_variable cv{};

        // 用于解锁条件变量的标志
        bool flag = false;

    public:

        // 线程进入等待
        void Wait() {
            auto lock = std::unique_lock<std::mutex>(mtx);
            flag = false;
            cv.wait(lock, [this] { return flag; });
        }

        // 唤醒线程
        void WakeCv() {
            {
                std::lock_guard<std::mutex> lcok(mtx);
                flag = true;
            }
            cv.notify_all();
        }
    };
}
