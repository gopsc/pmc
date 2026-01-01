/*
 * 问：什么是线程？
 *
 * 答：线程（Thread）是操作系统能够进行运算调度的最小单元，被包含在进程（Process）之中，是进程中的实际运作单元。
 *
 * 你可以将它理解为一条在代码中独立执行的路径。
 *
 */

#pragma once
#include "Fsm.hpp" // 有限状态机使得线程能够控制自己的状态
#include <thread> // CPP标准库线程类
#include <memory> // 内存管理
#include <chrono> // 用于线程休息
namespace qing {
	/*
	 * 包含一个简易的多线程控制机制。
	 * 需要共享的变量和函数声明为成员。
	 * 线程主函数包括了流程控制
     *
     * 要使用需要实现其中的各个事件方法
     *
	 * 
	 * 已经将fsm有限状态机做成通用状态机
	 * 20250621: 从派生改为持有
     *
	 * 
	 * 优化建议：将整个类做一个事件驱动的版本
	 *
     *
	 */
	class Thread {
	public:

        // 没有C原生的数据结构，可以直接使用缺省构造函数
		Thread() = default;
        
        // 删除复制构造函数。
        //
        // 因为持有线程类，不要轻举妄动。
		Thread(const Thread&) = delete;


        // 虚析构函数
        //
        // 具体有什么用，忘了
		virtual ~Thread() {}

        //-----------------------------------
		//-----------------------------------
		// 检查状态的方法
        //
        // 做成虚函数，万一子类有特别要求呢
		virtual Fsm::Stat check() {
			return fsm.check();
		}

        //-----------------------------------
		//-----------------------------------
		// 类的控制流方法
        //
        // 上面这是什么意思？
        //
        // 更正，这是让线程能够阻塞直到新的状态被赋予的方法
		virtual void suspend() {
			fsm.suspend();
		}

        //-----------------------------------
		//-----------------------------------
		// 状态改变的方法

		// 进入唤醒状态 
		// 1. 这里的锁和条件变量是这个类中的、
		// 2. 唤醒函数不可以进行该类中的锁操作，否则会造成死锁 
        //（为啥忘了，可能跟该函数的调用位置有关）
		//
		virtual void wake() {
			//{
				//std::lock_guard<std::mutex> lk(mtx_th);
				fsm.set(Fsm::Stat::START);
				//ready = true;
			//}
			// 可能会出现多个线程在等待的情况
			//cv.notify_all();
		} //wake()

		// 进入静止状态
        //
        // 上锁、唤醒阻塞的线程
		virtual void stop() {
			{
				std::lock_guard<std::mutex> lk(mtx_th);
				fsm.set(Fsm::Stat::STOP);
				ready = true;
			}
			// 可能会出现多个线程在等待的情况
			cv.notify_all();
		}

		// 进入关闭状态
        //
        // 上锁，唤醒阻塞的线程
		virtual void shut() {
			{
				std::lock_guard<std::mutex> lk(mtx_th);
				fsm.set(Fsm::Stat::SHUT);
				ready = true;
			}
			// 可能会出现多个线程在等待的情况
			cv.notify_all();
		} //shut()

		// 进入运行状态
        //
        //上锁，唤醒阻塞的线程
		virtual void run() {
			{
				std::lock_guard<std::mutex> lk(mtx_th);
				fsm.set(Fsm::Stat::RUNNING);
				ready = true;
			}
			// 可能会出现多个线程在等待的情况 
			cv.notify_all();
		}//run()

        //-----------------------------------
		//-----------------------------------
		// 事件虚函数
        //
        // 优化建议：计划下一次将其改为事件驱动的


		// 静止事件
		virtual void StopEvent() = 0;

		// 唤醒事件
		virtual void WakeEvent() = 0;

		// 循环事件
		virtual void LoopEvent() = 0;

		// 清除事件
		virtual void ClearEvent() = 0;

		//-----------------------------------
		// 线程相关方法


		// 激活线程，申请线程对象
		//
		// 该函数不能放在构造函数内，因为需要确保对象已经被构造了
        // （？为什么）
		void Activate() {

            // 防止重复定义
			if (th) return; 

			// 申请线程资源
            th = std::make_unique<std::thread>(&Thread::main, this);

		}

		// 尝试唤醒线程并等待它开始 该函数有问题可能导致异常
        // 
        // （太可惜了）
		void WaitStart() {
			//
			// 等待线程开始，wake()不可以加入，否则会造成死锁
			//
			// 不加入这里的锁会死等
			//
			std::unique_lock<std::mutex> lk(mtx_th);
			wake();
			ready = false;
			cv.wait(lk, [this] { return ready; });
		}

		// 等待线程关闭
		void WaitClose() {
			shut();
			if (th->joinable())
				th->join();
		}

		//-----------------------------------

		void main() {

			// 需要等待对象完成构造 现在已经改为放在对象构造完之后激活
            //
            // 之前留下的
            //
			//std::this_thread::sleep_for(std::chrono::milliseconds(10L));

			// 回字形循环
			while (check() != Fsm::Stat::SHUT) {

				// 静止状态
				while (check() == Fsm::Stat::STOP) {
					StopEvent();
				}

				// 开始事件
				if (check() == Fsm::Stat::START) {
					WakeEvent();
				}

				// 从设置阶段进入运行态
				while (check() == Fsm::Stat::RUNNING) {
					LoopEvent();
				}

				// 清理事件
				//（记得检查成员变量有没有被设置）
				ClearEvent();

			}
		}

	private:

        // 持有状态机类型
		qing::Fsm fsm{};

        // 持有线程类型
		std::unique_ptr<std::thread> th;

        // 持有锁类型
		std::mutex mtx_th;

        // 这里持有条件变量，用于阻塞以等待线程的关闭
		std::condition_variable cv;

        // 辅助条件变量的逻辑型
		bool ready;

	};
}
