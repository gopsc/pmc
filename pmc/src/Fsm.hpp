/*
 * 问：什么是有限状态机？
 *
 *
 * 答：有限状态机（Finite State Machine，FSM）是一种用于建模系统行为的抽象数学模型。
 *
 * 它的核心思想是：系统在任何给定时刻都处于一个有限的、预定义的“状态”中，
 * 
 * 并且会根据接收到的输入或事件，从一个状态切换到另一个状态。
 *
 */

#pragma once
#include <mutex> //上锁 使得有限状态机线程安全
#include <condition_variable> // 条件变量，使得子类能够等待状态的设置
namespace qing {
    /*
     * FiniteStateMachine有限状态机
     * 我们已经将它做成每个数字代表一个状态
     * 
     * 我们已经把它做成 enum class 类型
     * 并使用static_cast<>()之类的函数去赋值
     */
    class Fsm {
    public:

        //状态
        //
        //优化建议：多准备一些状态
        enum class Stat: int {
            SHUT = -1,
            STOP = 0,
            START = 1,
            RUNNING = 2
        };//Stat


        // 构造函数，以STOP为初始状态
        Fsm() {
            
            // 使用成员方法来设置状态
            set(Stat::STOP);

        }//Fsm()


        // 检查当前的状态，返回 enum Stat
        enum Stat check() {
            //在这里上锁以获取准确的信息
            std::lock_guard<std::mutex> lk(mtx);
            return stat;
        } //check()


        // 在不上锁的情况下设置一个状态 
        // 我不知道它用来干什么
        void set_without_lock(Stat stat) {
            // 这里不能使用锁，因为我们还要在临界区里唤醒条件变量
            //std::lock_guard<std::mutex> lk(mtx);
            this->stat = stat;
        } // set()

        // 带锁的状态设置 但是该方法不唤醒条件变量
        // 
        //我不知道用来干嘛的
        //
        void set_without_notify(Stat stat) {
            std::lock_guard<std::mutex> lk(mtx);
            this->stat = stat;
        }

        // 带锁和条件唤醒的状态设置
        // （这个似乎比较正式）
        void set(Stat stat) {
            {
                std::lock_guard<std::mutex> lk(mtx);
                set_without_lock(stat);
                ready = true;
            }
            cv.notify_all();
        }


    // 我们要使得组合它的类能够调用该方法
	//protected:

		// 阻塞线程，等待事件唤醒
		void suspend() {
            std::unique_lock<std::mutex> lk(mtx);
            ready = false;
            cv.wait(lk, [this] { return ready; });
        }//suspend

        private:

            // 锁使得状态机可以线程安全
            std::mutex mtx;

            // 用于条件变量的阻塞唤醒
            bool ready = false;

            // 条件变量 使得子类/外部可以阻塞/唤醒线程
            std::condition_variable cv;

            // 主菜，状态机的状态变量
            enum Stat stat;

    };//Fsm

}//qing
