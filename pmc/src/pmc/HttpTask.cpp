#include "net/Http.hpp"
#include "pmc/HttpTask.hpp"
namespace qing {

	void HttpTask::init_thread() {

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

}
