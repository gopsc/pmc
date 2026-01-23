#include <functional>
#include "th/ITask.hpp"
#include "th/Thread.hpp"
/*
 * Http Server Task
 * 超文本传输服务器任务
 */
namespace qing{
class HttpTask: public qing::ITask {
public:
	/* 调用回调函数，设置http服务器 */
	HttpTask(const std::string& addr, const int port, std::function<void(Http&)> f)
	: http_addr(addr), http_port(port), callback(f) {

		init_thread();
	}

	/* 任务不可复制 */
	HttpTask(const HttpTask& ) = delete;

	/* 关闭线程、关闭服务 */
	~HttpTask() {
		if (http) http->stop();
		if (th) th->WaitClose();
	}

	/*----------------------------------
         * 这四个重写的控制方法可以控制线程的状态，分别是停止、启动、运行、关闭
         *
         * 这四个控制方法，调用时如果http正在运行，则会被关闭
         *
         * 如果是在运行态再次运行，这样http就会死亡，会退出运行
         *
         * 会形成死循环， 故在run()中不关闭http服务器
	 *
	 */


	/* 从外部停止任务 */
	void stop() override {
		if (http) http->stop();
		th->WaitClose();
	}

	/* 唤醒线程 */
	void start() override {
		th->Activate(); /* 对于已关闭的线程 */
		th->WaitStart();
	}

	/* 是否在运行 */
	bool isRunning() override {
		auto stat = th->check();
		return stat == Fsm::Stat::START
			|| stat == Fsm::Stat::RUNNING;
	}
private:
	std::unique_ptr<Thread> th;
	std::function<void(Http&)> callback;
	std::unique_ptr<Http> http;
	std::string http_addr;
	int http_port;

	/* 初始化线程 */
	void init_thread();
}; //HttpTask
} // qing
