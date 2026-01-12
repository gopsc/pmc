/* 线程
 *
 *
 * 包含一个简易的多线程控制机制，需要共享的变量和函数声明为成员。
 * 
 * 线程主函数包括了流程控制，要使用需要实现其中的各个事件方法
 *
 * 20250621: 从派生改为持有
 *
 * NOTE: 在对象构造完之后应该激活
 */

#pragma once
#include "Fsm.hpp" /* 有限状态机 */
#include <thread>
#include <memory> /* 内存管理 */
#include <chrono> /* 用于线程休息 */
#include <stdexcept>
#include <functional>
namespace qing {
class Thread;
//using f_t = void(*)(Thread&);
using f_t = std::function<void(Thread&)>;
class Thread {
public:

    /*没有C原生的数据结构，可以直接使用缺省构造函数 */
    //Thread() = default;

    /* 作为可变的函数，持有回调可能比继承更好 */
    Thread(f_t& stop_callback, f_t& wake_callback, f_t& loop_callback, f_t& clear_callback)
    {
        StopEvent = std::make_unique<f_t>(stop_callback);
        WakeEvent = std::make_unique<f_t>(wake_callback);
        LoopEvent = std::make_unique<f_t>(loop_callback);
        ClearEvent = std::make_unique<f_t>(clear_callback);
    };

    /*
     * 删除复制构造函数。
     *
     * 因为持有线程类，不要轻举妄动。
     */
    Thread(const Thread&) = delete;


    /*
     * 虚析构函数
     *
     * FIXME: 具体有什么用，忘了
     */
    virtual ~Thread() {}

    /*-----------------------------------*/
    /*-----------------------------------*/
    /*
     * 检查状态的方法
     *
     * NOTE: 做成虚函数，万一子类有特别要求呢
     */
    virtual Fsm::Stat check() {
        return fsm.check();
    }

    /*-----------------------------------*/
    /*-----------------------------------*/
    /* 让线程能够阻塞直到新的状态被赋予的方法 */
    inline void suspend() {
        fsm.suspend();
    }

    /*-----------------------------------*/
    /*-----------------------------------*/
    /* 状态改变的方法 - 变异器 */

    /*
     * 进入唤醒状态 
     * 
     * 唤醒函数不可以进行该类中的锁操作，否则会造成死锁 
     */
    virtual void wake() {
        //{
            //std::lock_guard<std::mutex> lk(mtx_th);
            fsm.set(Fsm::Stat::START);
            //ready = true;
        //}
        //cv.notify_all();
    } /* wake */

    /* 进入静止状态
     *
     * 上锁、唤醒阻塞的线程
     */
    virtual void stop() {
        {
            std::lock_guard<std::mutex> lk(mtx_th);
            fsm.set(Fsm::Stat::STOP);
            ready = true;
        }
        /* 可能会出现多个线程在等待的情况 */
        cv.notify_all();
    }/* stop */

    /* 进入关闭状态 */
    virtual void shut() {
        {
            std::lock_guard<std::mutex> lk(mtx_th);
            fsm.set(Fsm::Stat::SHUT);
            ready = true;
        }
        cv.notify_all();
    } /*shut*/

    /* 进入运行状态 */
    virtual void run() {
        {
            std::lock_guard<std::mutex> lk(mtx_th);
            fsm.set(Fsm::Stat::RUNNING);
            ready = true;
        }
        cv.notify_all();
    }/*run*/

    /*-----------------------------------*/
    /*-----------------------------------*/
    /*
     * 事件虚函数
     *
     * 优化建议：计划下一次将其改为事件驱动的
     */

    /* 静止事件 */
    //virtual void StopEvent() = 0;

    /* 唤醒事件 */
    //virtual void WakeEvent() = 0;

    /* 循环事件 */
    //virtual void LoopEvent() = 0;

    /* 清除事件 */
    //virtual void ClearEvent() = 0;

    /*-----------------------------------*/
    /* 线程相关方法 */


    /*
     * 激活线程，申请线程对象
     *
     * 该函数不能放在构造函数内，因为需要确保对象已经被构造了
     */
    void Activate() {

        /* 防止重复初始化 */
        if (th) return; 

        /* 申请线程资源 */
        th = std::make_unique<std::thread>(&Thread::main, this);

    }

    /*
     * 尝试唤醒线程并等待它开始 据说该函数有问题可能导致异常
     */
    void WaitStart() {
        std::unique_lock<std::mutex> lk(mtx_th);	/* 不进入临界区可能会造成死等，在唤醒过程非常短暂的情况下 */
        wake(); 	/* 这就是为什么wake()不可以获取临界区，否则会造成死锁 */
        ready = false;	/* 这个临界区结束后，WakeEvent()中的run()会唤醒线程的堵塞 */
        cv.wait(lk, [this] { return ready; });
    }

    /* 等待线程关闭 */
    void WaitClose() {
        shut();
        if (th->joinable())
            th->join();
    }

    /*-----------------------------------*/
    /* 线程主函数 */
    void main() {

        /* 回字形循环 */
        while (check() != Fsm::Stat::SHUT) {

            try { /* 异常只会使线程静止 */

                while (check() == Fsm::Stat::STOP) {
                    (*StopEvent)(*this);	/* 静止状态 */
                }
					
                if (check() == Fsm::Stat::START) {
                    //(*ClearEvent)(*this);   /* 在开始之前做一次清理 NOTE: 比较反直觉*/
                    (*WakeEvent)(*this);	/* 开始事件 */
                }

                while (check() == Fsm::Stat::RUNNING) {
                    (*LoopEvent)(*this);	/* 从设置阶段进入运行态 */
                }

                /* 清理事件 （记得检查成员变量有没有被设置） */
                (*ClearEvent)(*this);
					
            }

            catch (std::exception& exp) {
                std::cout << "Thread throw a exception: " << exp.what() << std::endl;
                stop(); /* 因为这里的停止，线程在循环内抛出异常进程不会死亡，所以需要特别注意内存泄漏 */
            }

        }
    }

private:

    qing::Fsm fsm{};	/* 持有状态机类型 */

    std::unique_ptr<std::thread> th;	/* 持有线程类型 */

    std::mutex mtx_th;	/* 持有锁类型 */

    /*
     * 这里持有一个条件变量，用于阻塞以等待线程的关闭
     * 
     * 注意状态机中的cv是另一个变量
     */
    std::condition_variable cv;

    /* 辅助条件变量的逻辑型 */
    bool ready;

    /* 事件回调 */
    std::unique_ptr<f_t> StopEvent;
    std::unique_ptr<f_t> WakeEvent;
    std::unique_ptr<f_t> LoopEvent;
    std::unique_ptr<f_t> ClearEvent;

};
}
