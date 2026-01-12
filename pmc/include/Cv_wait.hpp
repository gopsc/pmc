/*
 * 利用条件变量进行等待
 *
 * FIXME: deepseek 认为有一个设计缺陷：它只能用于单次等待-唤醒的场景。
 * FIXME: 可能导致永久堵塞，应使用带超时的等待
 * FIXME: 使用标准库的 future/promise
 */

#include <thread>
#include <atomic> /* 也许将字段flag创建为原子变量 */
#include <mutex>
#include <condition_variable>
namespace qing {
class Cv_wait {  /* 用于等待的条件变量类，对同一个对象进行操作能够阻塞当前线程，或者唤醒被阻塞的线程 */
private:
    std::mutex mtx{};   /* 用于控制条件变量的锁 */
    std::condition_variable cv{};
    bool flag = false;  /* 用于解锁条件变量的标志 */
public:

    inline void Wait()   /* 线程进入等待 */
    {
        auto lock = std::unique_lock<std::mutex>(mtx);
        flag = false;
        cv.wait(lock, [this] { return flag; });
    }

    inline void WakeCv() /* 唤醒线程 */
    {
        {
            std::lock_guard<std::mutex> lcok(mtx);
            flag = true;
        }
        cv.notify_all();
    }

};
}
