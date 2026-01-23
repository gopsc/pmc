/* 
 * 有限状态机（Finite State Machine，FSM）
 */

#pragma once
#include <mutex>                /*上锁 使得有限状态机线程安全*/
#include <condition_variable>   /*条件变量，使得子类能够等待状态的设置*/
namespace qing {
/*
* FiniteStateMachine有限状态机
*
*   每个数字代表一个状态 使用static_cast<>()之类的函数去赋值
*  FIXME: 之前用过一种使用回调函数处理的方式
*/
class Fsm {
public:

    /* 状态枚举类 */
    enum class Stat: int {
        SHUT = -1,  /* 关闭 */
        STOP = 0,   /* 静止 */
        START = 1,  /* 启动 */
        RUNNING = 2 /* 运行 */
                    /* ... */
    };/*Stat*/


    /* 以STOP为初始状态 */
    Fsm() { set(Stat::STOP); }

    enum Stat check() { /* 检查当前的状态 */
        std::lock_guard<std::mutex> lk(mtx);
        return stat;
    } /*check*/


    /*
     * 在不上锁的情况下设置一个状态
     * 这里不能使用锁，因为我们还要在临界区里唤醒条件变量
     */
    void set_without_lock(Stat stat) { this->stat = stat; }

    /* 带锁的状态设置 但是该方法不唤醒条件变量 */
    void set_without_notify(Stat stat) {
        std::lock_guard<std::mutex> lk(mtx);
        this->stat = stat;
    }

    /* 带锁和条件唤醒的状态设置 */
    void set(Stat stat) {
        {
            std::lock_guard<std::mutex> lk(mtx);
            set_without_lock(stat);
            ready = true;
        }
        cv.notify_all();
    }

//protected:

    void suspend() {     /* 阻塞线程，等待事件唤醒 */
        std::unique_lock<std::mutex> lk(mtx);
        ready = false;
        cv.wait(lk, [this] { return ready; });
    }/*suspend */

private:
    std::mutex mtx; /* 线程安全 */
    bool ready = false; /* 用于条件变量的阻塞唤醒 */
    std::condition_variable cv; /* 条件变量 阻塞/唤醒线程 */
    enum Stat stat; /* 状态机的状态变量 */

};//Fsm

}//qing
