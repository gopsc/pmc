/* 线程（Thread）是操作系统能够进行运算调度的最小单元，被
 * 包含在进程（Process）之中，是进程中的实际运作单元。
 * 你可以将它理解为一条在啊代码中独立执行的路径。
 */
#pragma once
#include "Fsm.hpp"
#include <thread>
#include <memory>
#include <chrono>
#include <functional>
namespace qing {
	/* 包含一个简易的多线程控制机制，需要共享的变量
	 * 和函数声明为成员。线程主函数包括了流程控制。
	 * 要使用需要实现其中的各个事件方法
	 * 
	 * ?: 已经将fsm改为通用的
	 * 20250621: 从派生改为持有
	 */

	class Th {
	public:

		// 没有C原生的数据结构，可以直接使用缺省构造函数
		Th() = default;

		// 删除复制构造函数。因为
		// 持有线程类，不要轻举妄动。
		Th(const Th&) = delete;

		// 虚析构函数。具体
		// 有什么用忘了，好像是
		// 通过析构函数关闭线程时用的
		virtual ~Th() = default;

		/////////////////////////////////////
		// 检查状态，做成
		// 虚函数，万一子类有特别要求呢？
		virtual inline Fsm::Stat check() {
			return fsm.check();
		}
		/////////////////////////////////////
		// 变质——状态改变的方法
		//
		// 进入唤醒状态
		// 1.这里的锁和条件变量是这个类中的。
		// 2.唤醒函数不可以进行该类中的锁操作，否则会造成死锁。（忘了为啥，可能跟调用位置相关）
		[[noreturn]] virtual void wake() {
			fsm.set(Fsm::Stat::START);
		} // wake()

		// 进入静止状态
		[[noreturn]] virtual void stop() {
			{
				std::lock_guard<std::mutex> lk(mtx_th);
				fsm.set(Fsm::Stat::STOP);
				ready = true;
			}

			// 可能会出现多个线程在等待的情况
			cv.notify_all();
		}// stop()

		// 进入关闭状态
		[[noreturn]] virtual void shut() {
			{
				std::lock_guard<std::mutex> lk(mtx_th);
				fsm.set(Fsm::Stat::SHUT);
				ready = true;
			}
			cv.notify_all();
		}

		// 进入运行状态
		[[noreturn]] virtual void run() {
			{
				std::lock_guard<std::mutex> lk(mtx_th);
				fsm.set(Fsm::Stat::RUNNING);
				ready = true;
			}
			cv.notify_all();
		}

		/////////////////////////////////////
		//控制
		//
		// 尝试唤醒线程并等待它开始，据说该函数有问题
		[[noreturn]] void WaitStart() {
			// 等待线程开始，wake()不可以加锁，否则会死锁
			std::unique_lock<std::mutex> lk(mtx_th);
			wake();
			ready = false;
			cv.wait(lk, [this] { return ready; });
		}

		// 等待线程关闭
		[[noreturn]] void WaitClose() {
			shut();
			if (th->joinable())
				th->join();
		}

		// 仅仅等待线程结束
		[[noreturn]] void Wait() {
			if (th->joinable())
				th->join();
		}

		// 激活线程，申请线程对象，因为
		// 需要确保对象已经被构造了，该函数不能放在构造函数内
		[[noreturn]] void Activate() {
			// 防止重复激活
			if (th) return;
			// 申请线程资源
			th = std::make_unique<std::thread>(&Th::main, this);
		}

	protected:

		/////////////////////////////////////
		// 事件
		//
		//
		// 静止事件
		std::function<void()> StopEvent = [this] { fsm.suspend(); };

		// 唤醒事件
		std::function<void()> WakeEvent = [this] { this->run(); };

		// 循环事件
		std::function<void()> LoopEvent = [] {
			std::this_thread::sleep_for(std::chrono::milliseconds(10L));
		};

		// 清理事件
		std::function<void()> ClearEvent = [] {};


		/////////////////////////////////////
		// 循环
		[[noreturn]] virtual void main() {

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

				// （从设置阶段）进入运行态
				while (check() == Fsm::Stat::RUNNING) {
					LoopEvent();
				}

				// 清理事件，记得检查
				// 成员变量有没有获取资源
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
		bool ready = false;

	};
}