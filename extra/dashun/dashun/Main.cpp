#define _CRT_SECURE_NO_WARNINGS
#include "Th.hpp"
#include "Pls.hpp"
#include "args.hxx"
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <uv.h>
#ifdef _WIN32
#include <Windows.h>
#undef UNICODE
#undef _UNICODE
#endif
#include <iostream>
#include <memory>
#include <atomic>
using namespace boost::interprocess;

// 定义分配器类型
using CharAllocator = boost::interprocess::allocator<
	char,
	boost::interprocess::managed_shared_memory::segment_manager
>;

// 定义共享内存字符串类型
using SharedString = boost::interprocess::basic_string<
	char,
	std::char_traits<char>,
	CharAllocator
>;

namespace qing {
	// 监听线程
	class LTh : public qing::Th {
	public:
		LTh(const std::string& name) {

			this->name = name;
			nameMem = name + "_MEM";
			nameMtx = name + "_MTX";
			namePtr = name + "_PTR";
			nameRet = name + "_RET";

			this->WakeEvent = [this] {

				try {

					// 先移除可能存在的旧共享内存
					this->del_shared();

					// 创建共享对象
					this->create_shared();

					// 线程进入运行态
					this->run();

				}
				catch (interprocess_exception& ex) {

					// 输出错误信息
					std::cerr << ex.what() << std::endl;

					// 线程静止
					this->stop();
				}

			};
			this->LoopEvent = [this] {


				this->recv_shared_and_action();

				// 等待
				//
				// 优化方案：使用信号，有新消息进来给沉默进程发送信号
				// 而不是休眠定长时间检查
				std::this_thread::sleep_for(std::chrono::milliseconds(10));

			};

			this->ClearEvent = [this] {

				// 移除共享对象
				this->del_shared();

			};
		}



		[[noreturn]] static auto send_shared_msg(const std::string& to, const std::string& msg) -> void {

			std::string nameMem = to + "_MEM";
			std::string nameMtx = to + "_MTX";
			std::string namePtr = to + "_PTR";
			std::string nameRet = to + "_RET";
			try {

				// 打开共享内存
				managed_shared_memory segment(open_only, nameMem.c_str());

				// 构造一个命名互斥量
				named_mutex mutex(open_or_create, nameMtx.c_str());

				// 在共享内存中创建一个字符串
				SharedString* str = segment.construct<SharedString>(namePtr.c_str())(
					msg.c_str(),  // 初始值（空字符串）
					segment.get_allocator<char>()  // 必须传入分配器
					);

				// 不延时的话可能会抢占，也许应该想一个两全其美的方法
				std::this_thread::sleep_for(std::chrono::milliseconds(100));

				{
					// 加锁
					std::lock_guard<named_mutex> lk(mutex);

					// 获取消息
					std::pair<SharedString*, std::size_t> msg = segment.find<SharedString>(nameRet.c_str());
					if (msg.first) {
						// 输出
						std::cout << std::endl << *msg.first << std::endl;

						// 销毁
						segment.destroy<SharedString>(nameRet.c_str());
					}
				}

			}
			catch (interprocess_exception& ex) {
				std::cerr << "Writer Error: " << ex.what() << std::endl;
			}
		}


	private:
		std::string name;
		std::string nameMem;
		std::string nameMtx;
		std::string namePtr;
		std::string nameRet;
		std::unique_ptr<managed_shared_memory> segment;
		std::unique_ptr<named_mutex> mutex;
		std::pair<SharedString*, std::size_t> msg;
		qing::Pls pls;

		inline void create_shared() {

			//////////////////////////////
			// 删除可能存在的共享变量
			this->del_shared();
			//////////////////////////////


			// 创建共享内存
			segment = std::make_unique<managed_shared_memory>(create_only, (nameMem).c_str(), 65536);

			// 创建互斥量
			mutex = std::make_unique<named_mutex>(create_only, (nameMtx).c_str());

		}

		inline auto del_shared() -> void {

			// 先移除可能存在的旧共享内存
			shared_memory_object::remove(nameMem.c_str());

			// 移除可能存在的互斥量
			named_mutex::remove(nameMtx.c_str());

		}

		inline auto recv_shared_and_action() -> void
		
		try {

				// 加锁
				std::lock_guard<named_mutex> lock(*mutex);

				// 查找共享字符串
				msg = segment->find<SharedString>(namePtr.c_str());
				if (msg.first) { // 找到


					std::cout << "Reader read: " << *msg.first << std::endl;

					std::string ret = this->lexer(std::string((msg.first)->data(), (msg.first)->size()));

					// 在共享内存中创建一个字符串
					SharedString* str = segment->construct<SharedString>(nameRet.c_str())(
						ret.c_str(),  // 初始值（空字符串）
						segment->get_allocator<char>()  // 必须传入分配器
					);


					// 销毁writer创建的变量
					segment->destroy<SharedString>(namePtr.c_str());

				}
			}

		catch (interprocess_exception& ex) {
			std::cerr << ex.what() << std::endl;
		}


		// 简易词法分析器
		std::string lexer(std::string tmp) {

			// 要返回的消息
			std::string ret = "OK";

			if (tmp == "list") {
				ret = pls.initialize();
			}
			else if (tmp.starts_with("kill ")) {
				pls.kill(std::stoi(tmp.substr(5)));
			}
			else if (tmp == "clear") {
				pls.clear();
			}
			//else if (tmp == "shut") {
			//	this->shut();
			//} //关闭不了程序
			else {
				// 这个分支最好返回自己是否出现了异常
				pls.exec(tmp.c_str(), {});
			}

			// 返回消息
			return ret;
		}


	};
}



















// 线程指针
std::unique_ptr<qing::Th> th;

// 全局标志，指示是否应该退出
std::atomic<bool> should_exit(false);

// 子系统的名称（全局参数）
std::string name;

// 消息
std::string msg;

// Windows异步句柄，用于信号转发
#ifdef _WIN32
uv_async_t async_handle;
SERVICE_STATUS_HANDLE hStatus;
SERVICE_STATUS status;
#endif


////////////////////////
////////////////////////
// 解析命令行参数
//
//
// 创建参数解析器
args::ArgumentParser parser("这是一个脚本托管服务。", "请使用专门的通信程序（或本程序自己）进行操作。");

// 显示帮助信息的标志
args::HelpFlag help(parser, "帮助", "显示帮助信息", { 'h', "help" });

// 启用守护进程
args::Flag daemon(parser, "守护进程", "启动守护进程", { 'd', "daemon" });

// 子系统路径
args::ValueFlag<std::string> subSys(parser, "子系统", "输入子系统文件夹的路径", { 's', "sys" });

// 要发送的子系统名字
args::ValueFlag<std::string> arg_to(parser, "发送目标", "输入要发送的子系统路径", { 't', "to" });

// 要发送的信息
args::ValueFlag<std::string> arg_msg(parser, "消息", "输入要发送的消息（<path>|list|kill <pid>| clear）", { 'm', "msg" });

[[noreturn]] static auto parse_args(int argc, char** argv) -> void {

	// 解析命令行参数
	try {
		parser.ParseCLI(argc, argv);
	}
	catch (const args::Help&) {
		std::cout << parser;
		exit(0);
	}
	catch (const args::ParseError& e) {
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		exit(1);
	}
	catch (const args::ValidationError& e) {
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		exit(1);
	}
}


////////////////////////
////////////////////////
// 信号处理回调函数
[[noreturn]] static void on_signal(uv_signal_t* handle, int signum) {
	std::cout << "\n收到退出信号 " << signum << std::endl;

	// 防止重复处理退出信号
	if (should_exit) return;
	should_exit = true;

	// 停止信号监听
	uv_signal_stop(handle);

	// 执行清理
	if (th)
		th->WaitClose();

	// 关闭事件循环
	uv_stop(uv_default_loop());
}



// Windows异步回调函数
#ifdef _WIN32
[[noreturn]] static void async_callback(uv_async_t* handle) {
	std::cout << 1 << std::endl;
	// 这里可以触发退出流程或其他需要在事件循环中处理的操作
	if (!should_exit) {
		on_signal(nullptr, SIGINT); // 模拟收到SIGINT信号
	}
}

// Windows控制台事件处理程序
static BOOL WINAPI console_handler(DWORD signal) {
	std::cout << 1 << std::endl;
	switch (signal) {
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
		// 使用异步句柄将信号转发到libuv事件循环
		uv_async_send(&async_handle);
		return TRUE;
	default:
		return FALSE;
	}
}
#endif


////////////////////////////////////
////////////////////////////////////
// 分支
//
//
// 发送消息分支
static inline auto SendMsgBranch() -> void {
	qing::LTh::send_shared_msg(name, msg);
}


// 守护进程分支
static inline auto DaemonBranch() -> void {
	th = std::make_unique<qing::LTh>(name);
	th->Activate();
	th->WaitStart();
	//th->Wait();
}

//
// 分支选择器
[[noreturn]] static auto BranchSelector(uv_idle_t* handle) {

	if (should_exit) {
		uv_idle_stop(handle);
		return;
	}

	if (th) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		return;
	}



	if (arg_to && arg_msg) { // 发送消息分支

		name = args::get(arg_to);
		msg = args::get(arg_msg);

		SendMsgBranch();

		// 退出
		should_exit = true;
		// 关闭事件循环
		uv_stop(uv_default_loop());
	}
	else if ((daemon && arg_to) || 1) { // 守护进程分支
		std::cout << "程序启动，开始运行..." << std::endl;
		std::cout << "按下 CTRL+C 退出程序" << std::endl;
		name = (arg_to) ? args::get(arg_to) : "test";

		DaemonBranch();
	}
	else {
		//输出帮助文本
		std::cout << parser;

		// 退出
		should_exit = true;

		// 关闭事件循环
		uv_stop(uv_default_loop());
	}


}

////////////////////////////////////
int MyMain(int argc, char** argv) {

	parse_args(argc, argv);


	//////////
	// 信号
	//
	// 
#ifdef _WIN32
	// 初始化Windows异步句柄
	uv_async_init(uv_default_loop(), &async_handle, async_callback);

	// 设置控制台事件处理程序
	if (!SetConsoleCtrlHandler(console_handler, TRUE)) {
		std::cerr << "无法设置控制台控制处理程序" << std::endl;
		return 1;
	}
#else
	// Unix平台：确保信号不会被忽略
	signal(SIGINT, [](int signum) { /* 留给libuv处理 */ });
	signal(SIGTERM, [](int signum) { /* 留给libuv处理 */ });
#endif


	// 初始化信号处理器
	uv_signal_t sig_int, sig_term;

	// 监听SIGINT 信号（CTRL + C）
	uv_signal_init(uv_default_loop(), &sig_int);
	uv_signal_start(&sig_int, on_signal, SIGINT);

	// 监听 SIGTERM信号
	uv_signal_init(uv_default_loop(), &sig_term);
	uv_signal_start(&sig_term, on_signal, SIGTERM);

#ifdef _WIN32
	// WINDOWS平台特有的信号
	uv_signal_t sig_break;
	uv_signal_init(uv_default_loop(), &sig_break);
	uv_signal_start(&sig_break, on_signal, SIGBREAK);
#endif

	// 设置任务
	uv_idle_t idle;
	uv_idle_init(uv_default_loop(), &idle);
	uv_idle_start(&idle, ::BranchSelector);

	// 运行事件循环
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);


	// 释放资源
	uv_close((uv_handle_t*)&sig_int, nullptr);
	uv_close((uv_handle_t*)&sig_term, nullptr);
#ifdef _WIN32
	uv_close((uv_handle_t*)&sig_break, nullptr);
	uv_close((uv_handle_t*)&async_handle, nullptr);
	// 移除控制台事件处理程序
	SetConsoleCtrlHandler(console_handler, FALSE);
#endif
	uv_close((uv_handle_t*)&idle, nullptr);
	
	// 清理事件循环
	uv_loop_close(uv_default_loop());

	std::cout << "程序已经退出" << std::endl;
	return 0;
}


#ifdef _WIN32



[[noreturn]] void ReportStatus(DWORD state) {
	status.dwCurrentState = state;
	SetServiceStatus(hStatus, &status);
}

[[noreturn]] void ServiceCtrlHandler(DWORD control) {
	if (control == SERVICE_CONTROL_STOP) {
		ReportStatus(SERVICE_STOP_PENDING);
		// 执行清理
		ReportStatus(SERVICE_STOPPED);
	}
}

[[noreturn]] void WINAPI ServiceMain(DWORD argc, LPTSTR* argv) {
	hStatus = RegisterServiceCtrlHandler(L"大顺脚本", ServiceCtrlHandler);

	status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	status.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	ReportStatus(SERVICE_RUNNING); // 告诉SCM服务已启动

	//服务主逻辑
	MyMain(argc, argv);
}

#endif