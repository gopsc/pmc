#include <functional>
#include "ITask.hpp"
#include "Thread.hpp"
/*
 * Http Server Task
 * 超文本传输服务器任务
 */
namespace qing{
class HttpServerTask: public qing::ITask {
public:
	/* 调用回调函数，设置http服务器 */
	HttpServerTask(const std::string& addr, const int port, std::function<void(Http&)> f)
	: http_addr(addr), http_port(port), callback(f) {
		init_thread();
	}
	/*  */
	HttpServerTask(const HttpServerTask& ) = delete;
	/* 关闭线程、关闭服务 */
	~HttpServerTask() {
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

	inline void init_thread() {

		/**/
		f_t StopEvent = [](Thread& th) {
			th.suspend();
		};

		/* 线程唤醒事件 */
		f_t WakeEvent = [this](Thread& th) {
		
			this->http=std::make_unique<Http>(http_addr, http_port);
			
			this->callback(*http);

			if (th.check() == Fsm::Stat::START) {  /* 只跑一次 */
				th.run(); /* 标记为启动 */
				http->run(); /* 这个会堵塞线程以进行监听 */
			}

			/* HTTP服务器异常关闭 */
			if (th.check() == Fsm::Stat::RUNNING) {
				th.stop();
			}


		};

		f_t LoopEvent = [](Thread& _) {
			throw std::runtime_error("Task  not support this Event");
		};

		f_t ClearEvent = [this](Thread& _) {
			if (this->http)
				this->http.reset(); /*  */
		};

		th = std::make_unique<Thread> (StopEvent, WakeEvent, LoopEvent, ClearEvent);
	}
}; //HttpServerTask
} // qing
