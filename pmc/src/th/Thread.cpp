#include "th/Thread.hpp"
namespace qing{

	Thread::Thread(f_t& stop_callback, f_t& wake_callback, f_t& loop_callback, f_t& clear_callback)
	{
		StopEvent = std::make_unique<f_t>(stop_callback);
		WakeEvent = std::make_unique<f_t>(wake_callback);
		LoopEvent = std::make_unique<f_t>(loop_callback);
		ClearEvent = std::make_unique<f_t>(clear_callback);
	};

    /* 线程主函数 */
    void Thread::main() {

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

}
