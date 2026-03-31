/* 有限状态机（Finite State Machine，FSM）是一种用于建模系统行为的抽象数学模型
 * 它的核心思想是，系统在任何给定时刻都处于一个有限、预定义的“状态”中
 * 并且会根据接收到的输入或事件，从一个状态切换到另一个状态
 */

#pragma once
#include <mutex>
#include <condition_variable>
namespace qing {
	/* FiniteStateMachine有限状态机
	 * 每个数字代表一个状态
	 */
	class Fsm {
	public:

		/* 状态枚举
		 * 优化建议：多准备一些状态
		 */
		enum class Stat : int {
			SHUT = -1,
			STOP = 0,
			START = 1,
			RUNNING = 2
		};

		// 默认构造函数
		Fsm() = default;

		// 检查当前的状态，返回snum Stat
		inline Stat check() {
			//进入临界区
			std::lock_guard<std::mutex> lk(mtx);
			return stat;
		}// check()

		// 在不上锁的情况下设置一个状态
		// 别在这里加条件唤醒
		[[noreturn]] void set_without_lock(Stat stat) {
			this->stat = stat;
		}// set_without_lock()

		// 带锁和条件唤醒的状态设置（正式）
		[[noreturn]] void set(Stat stat) {
			{
				std::lock_guard<std::mutex> lk(mtx);
				set_without_lock(stat);
				ready = true;
			}
			cv.notify_all();
		} // set()

	//protected:

		// 阻塞线程，等待事件唤醒
		[[noreturn]] void suspend() {
			std::unique_lock<std::mutex> lk(mtx);
			ready = false;
			cv.wait(lk, [this] { return ready; });
		}// suspend()

	private:
		
		// 锁使得状态机线程安全
		std::mutex mtx;

		// 用于条件变量的阻塞唤醒
		bool ready = false;

		// 条件变量使得线程能够等待状态的改变
		std::condition_variable cv;

		// 主菜，状态机的状态变量
		enum Stat stat = Stat::STOP;

	};
}