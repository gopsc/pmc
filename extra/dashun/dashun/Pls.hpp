/* 进程（Process）是计算机中正在执行的程序的一个实例，是
 * 操作系统进行资源分配和调度的基本单位
 * 
 * 你可以将其理解为程序的一次动态执行过程——程序
 * 是静态的代码文件，而进程是程序运行时的动态实体。
 */

#pragma once
#ifdef _WIN32
#include <WinSock2.h>
#include <Windows.h>
#endif
#include <boost/process/process.hpp>
#include <boost/process/environment.hpp>
#include <boost/asio/io_context.hpp>
#include <list>
#include <memory>
#include <iostream>
namespace qing {
	/* 进程列表类，这个类封装了一个
	 * 容纳Pls::subp的列表
	 * 
	 * 以及基本的进程操作
	 */
	class Pls {
	public:

		// 命名子进程
		struct subp {
			std::unique_ptr<boost::process::process> p;
			std::string name;
		};

		/* 创建子进程执行一个命令，并且
		 * 添加到进程列表。
		 * 
		 * 接受一个std::string类型的文件路径，以及
		 * std::vector<std::string> 类型的参数列表（缺省）
		 * 
		 * 优化方案：返回错误信息
		 */
		void exec(const std::string& path, const std::vector<std::string> &args) 
		try {

			// 检查文件是否存在（好像不会在环境变量中寻找）
			if (!boost::filesystem::exists(path)) {
				std::cerr << "Error: File not found - " << path << std::endl;
				return;
			}
			
			// 输入输出上下文
			boost::asio::io_context io_ctx;

			// 继承当前进程的所有环境变量（旧版）
			//auto env = boost::process::environment::current;

			// 错误码
			boost::system::error_code ec;

			// 构造子进程
			subp p = { std::move(std::make_unique<boost::process::process>( io_ctx, path, args/*, env, ec*/)), mergeCmd(path, args)};
			if (ec) {
				std::cerr << "Failed to start process: " << ec.message() << std::endl;
				return ;
			}
			else {
				// 推入向量
				vec.push_back(std::move(p));
			}

		}
		catch (std::exception &e) {

			std::cout << e.what() << std::endl;

		}


		/* 序列化进程列表 */
		inline std::string initialize() {
			std::string ret{ "" };
			for (auto& item : vec) {
				std::ostringstream oss;
				ret += (ret == "") ? "" : "\n";
#ifdef _WIN32
				DWORD pid = GetProcessId(item.p->native_handle());
				oss << pid;
#else
				oss << item.p->native_handle();
#endif
				ret += oss.str();
				ret += "\t";
				ret += (item.p->running()) ? "Alive" : "Dead";
				ret += "\t";
				ret += item.name;
			}
			return ret;
		}


		// 杀死一个进程
		[[noreturn]] void kill(int pid)
		try {
			for (auto it = vec.begin(); it != vec.end(); ++it)
				if ((*it).p->id() == pid && (*it).p->running()) {
					(*it).p->terminate();
					//(*it).p->wait(); // 可能杀不死？
					//vec.erase(it); // 清除
					break;
				}
		}
		catch (std::exception &ex) {
			std::cout << ex.what() << std::endl;
		}

		// 清理死掉的进程
		[[noreturn]] void clear() {
			for (auto it = vec.begin(); it != vec.end();)
				if (!(*it).p->running()) {
					it = vec.erase(it);
				}
				else {
					++it;
				}
		}

		// 关闭所有进程
		[[noreturn]] void closeAll() {
			for (auto& item : vec)
				if (item.p->running())
					item.p->terminate();

			// 加上这个似乎更清爽一些
			clear();
		}

	private:

		// 容纳子进程的链表
		std::list<subp> vec{};

		// 用于操作链表的锁
		//std::mutex mtx{};

		/* 将一个启动脚本地址，和一个
		 * 字符串向量组成的参数列表，组装 
		 * 成一条完整的启动命令
		 */
		inline static std::string mergeCmd(const std::string& path, const std::vector<std::string>& args) {
			std::string ret = path;
			for (const auto& item : args) {
				ret += " ";
				ret += item;
			}
			return ret;
		}

	};
}