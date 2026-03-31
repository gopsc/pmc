/*
 *  并发机器（进程托管）
 *  
 *  FIXME: 尽量使用大众的库（boost）
 */
#include <filesystem>
#include <iostream>
#include <string> 
#include <vector>
#include <memory>
#include <csignal> /* 注册信号以优雅退出 */
#include <functional>
#include <sys/types.h>
#include <sys/wait.h>
#include <boost/program_options.hpp>
#include "logs/Logger.hpp"
#include "th/Thread.hpp" /* 可控线程类 */
#include "th/Cv_wait.hpp" /* 通过条件变量进行等待 */
#include "th/ITask.hpp"
#include "lci/Term.h"
#include "lci/Rectangle.h"
#include "lci/Drawable.h"
#include "lci/FrameBuf.h"
#include "lci/PoolArea.h"
#include "lci/ShapeArea.h"
#include "lci/CharShape.h"
#include "lci/Color.h"
#include "lci/图层区域.hpp"
#include "hd/mem.h"
#include "hd/cpu.h"

/* lci 临时 */
#include "lci/p_th.h"

/* 中文模式 Chinese Mode */
#include "cn/中文化.hpp"
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
namespace po = boost::program_options;

/* 版本号 */
static const char *PMC_VERSION = "0.4.x";
/* 条件变量的等待机制让主程序能够等待中断信号的产生 */
qing::Cv_wait cv =  qing::Cv_wait();






/* 捕获中断信号，结束线程并且退出 */
void signalHandler(int signum)
{
    LOG_INFO("收到终止信号，准备停止服务器...");
    cv.WakeCv();    /* 唤醒条件信号即关闭程序 */
}

/*----SIGNAL ACTION----*/
//
/* 对信号的响应行为 */
void set_signal_action() {

    /* 该结构体用于描述信号的处理方式 */
    struct sigaction sa;


    /* 绑定处理函数 */
    sa.sa_handler = signalHandler;
    
    /*
     * 清空并初始化一个信号掩码集  --------------> 用于指定哪些信号在当前或即将执行的信号处理函数期间应该被阻塞（位掩码）
     *                                “在执行这个信号处理函数时，除了当前正在处理的信号外，还希望临时阻塞其他某些信号”
     * 不阻塞其他信号
     * 确保在执行期间没有其他特定的信号会被阻塞
     * 防止处理过程中被其他信号中断
     */
    sigemptyset(&sa.sa_mask);

    /*
     * 用于设置信号处理函数的行为特性。
     * 0 - 默认选项
     * SA_NODEFER - 表示在执行信号处理函数时不允许其他信号被阻塞
     * SA_RESETHAND - 在信号处理函数返回后，将信号的处理方式重置为默认
     * SA_NOCLDSTOP - 对于子进程发送的停止信号，父进程不会停止。
     * SA_NOCLDWAIT - 父进程在等待等待子进程终止时不会因为子进程发送信号而收到通知
     * SA_SIGINFO - 使用扩展的信号信息，信号处理函数接收三个参数而不是一个 
     */
    sa.sa_flags = 0;

    /* 注册信号 */
    sigaction(SIGINT, &sa, nullptr);  /* Ctrl + C */
    sigaction(SIGTERM, &sa, nullptr); /* kill 或 systemctl stop */

}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

#define TARGET_TTY 8 /* FIXME: 目标端口设置为参数传入 */
#define ORIGIN_TTY 1
namespace qing {
class LciTask: public ITask{
public:
	LciTask(/*const std::string& cmd, const int masterfd, const int command_pipe_send, */const int rotate, const int fontsize)
        : /*exec_cmd(cmd), masterfd(masterfd), command_pipe_send(command_pipe_send),*/
	rotate(rotate), fontsize(fontsize) {
		/* 初始化线程 */
		init_thread();
	}

        /* 停止线程 */
        void stop() override {
            th->WaitClose();
        }

        /* 唤醒线程 */
        void start() override{
            th->Activate(); /* 对于已经关闭的进程 */
            th->WaitStart();
        }

        /* 线程是否在运行 */
        bool isRunning() override {
            auto stat = th->check();	
            return stat == Fsm::Stat::START;
	}

private:

	std::string exec_cmd;
	//int command_pipe_send;
	//int masterfd;

	int rotate;
	int fontsize;
	bool shift = false;

	std::unique_ptr<Thread> th; /* 持有线程类 */
	std::unique_ptr<Term> term; /* 终端代理 */
	std::unique_ptr<FrameBuf> fb;
	//std::unique_ptr<KeyboardEvent> kbe;
	//std::unique_ptr<lci::p_th> printTask;
	std::unique_ptr<PoolArea> poolArea; /* 持有控件池 */
	//std::shared_ptr<CmdArea> cmd;
	int table_w;
	int table_h;
	std::shared_ptr<PoolArea> tableArea;
	std::vector<double> cpu_vec;
	std::vector<double> mem_vec;


	/*
	 * 设置终端的大小
	 *
	 * 传入一个控制台设备文件的文件描述符，以及你要设定的行列值
	 */
	//void set_terminal_size(int fd, int rows, int cols) {
	//	struct winsize sz = { .ws_row = (unsigned short)rows, .ws_col = (unsigned short)cols };
	//	if (ioctl(fd, TIOCSWINSZ, &sz) == -1) {
	//		throw std::runtime_error("ioctl");
	//	}
	//}

	/*---------------------*/
	void init_thread();
	
};
}


int lci_init(po::variables_map&);

/* 任务池 */
auto taskPool = std::vector<std::shared_ptr<qing::ITask>>{};


/* 入口函数 */
int main(int argc, char** argv) {

    /*------------------------------*/
    /* 设置信号行为*/
    set_signal_action();

    /*------------------------------*/
    /* 解析命令行参数 */
    po::options_description desc("pmc subsystem start options");
    desc.add_options()  /* 定义选项 */
	("help,h",    "display help message")
	("version,v", "display version message")
	("lci",       "run lci task")
	/*****************/
	//("exec",     po::value<std::string>(), "lci module run commandline")
	("rotate",   po::value<int>()->default_value(0),  "lci module rotate mode(0~3)")
	("fontsize", po::value<int>()->default_value(18), "lci module fontsize option");

    /* 参数变量映射关系 */
    po::variables_map vm;

    try {  /* 开始解析 */
    	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);
	if (vm.count("help")) {
	    std::cout << desc << std::endl;
	    return 0;
	}
	if (vm.count("version")) {
	    std::cout << PMC_VERSION << std::endl;
	    return 0;
	}
    }

    catch (const std::exception& e) {
	std::cerr << "ERROR: " << e.what() << std::endl;
	std::cerr << "using --help to check options message" << std::endl;
	throw e;
    }



    /* LINUX CHINESE INTERF */
    if (vm.count("lci")) {

        if (!(vm.count("rotate") && vm.count("fontsize")/* && vm.count("exec")*/))
            throw std::runtime_error(
                    "您进入了lci模块，请输入以下选项 --rotate， --fontsize");

        lci_init(vm);
    }

    /*
     * 等待
     *
     * 触发器在信号处理那里
     */
    cv.Wait();

    /* 主线程被唤醒 */
    for (auto& task: taskPool) {
        task->stop();
    }
}


/* 初始化PMC并发机器 */
int lci_init(po::variables_map& vm) {

    //int masterfd; /* 控制台文件描述符 */
    /* 在运行到多线程之前，进行fork() */
    //if ((masterfd = posix_openpt(O_RDWR)) == -1) { /* 打开一个新的虚拟控制台 */
    //    throw std::runtime_error("posix_openpt");
    //}

    /*--------------------------------------------------*/
    /* 创建两个管道：一个用于发送命令，一个用于接收输出 */
    //int command_pipe[2]; /* [0] 是读端，[1] 是写端 */
    //if (pipe(command_pipe) == -1)
    //    throw std::runtime_error(
    //        "pipe:" + std::string(strerror(errno))
    //    );
    /* 创建成功 */
    /*--------------------------------------------------*/

//	if (grantpt(masterfd) != 0 || unlockpt(masterfd) != 0) { /* 解锁虚拟控制台？ */
//		close(masterfd);
//		throw std::runtime_error("grantpt/unlockpt");  /* FIXME: 输出错误字符串 */
//	}

//	char *slave_name = ptsname(masterfd);  /* 获取虚拟控制台pty的名字 */
//	if (slave_name == NULL) {
//		close(masterfd);
//		throw std::runtime_error("ptsname");
//	}

//	LOG_INFO(
//		std::string("新的虚拟终端运行在 ") + slave_name
//	);



	
	/* Fork中间子进程 */
//	pid_t pid = fork();

	/* pid为-1通常意味着分叉失败 */
//	if (pid == -1)
//		throw std::runtime_error(
//			std::string("fork") + std::string(strerror(errno))
//		);

	/* 如果pid为0表示这是子进程 */
//	else if (pid == 0) {
//		LOG_INFO("LCI subprocess is running...");
//		close(command_pipe[1]); /* 关闭管道写端 */
//		int slavefd = open(slave_name, O_RDWR);
//		if (slavefd == -1)
//			throw std::runtime_error(
//				std::string("open slave pty") + std::string(strerror(errno))
//			);

		//runIntermediateProcess(command_pipe[0], slavefd);
//		int command_pipe_read = command_pipe[0];
//		char buffer[1024];
//		ssize_t nread;
//		while (true) {
//			memset(buffer, 0, sizeof(buffer)); /* 清空缓冲区 */
//		    nread = read(command_pipe_read, buffer, sizeof(buffer) - 1);	/* 从命令管道读取数据 */
//			if (nread <= 0) break;  /* 没有数据可读，退出循环 */

			/* 查找命令的结束位置 */
			/* FIXME: 更换结束符号，这样我们可以运行一整个脚本 */
//			char* end_of_cmd = strchr(buffer, '\n');
//			if (end_of_cmd != nullptr)
//				*end_of_cmd = '\0';
//			else
//				continue; /* 没找到结束符号，继续读取 */

			/* 检查是否受到结束信号 */
//			if (strcmp(buffer, "END") == 0)
//				break;

			/* 输入输出重定向 */
//			if (dup2(slavefd, STDIN_FILENO) != STDIN_FILENO
//			|| dup2(slavefd, STDOUT_FILENO) != STDOUT_FILENO
//			|| dup2(slavefd, STDERR_FILENO) != STDERR_FILENO)
//				throw std::runtime_error(
//					std::string("dup2") // +
//				);

			/* ？*/
//			if (slavefd > STDERR_FILENO)
//				close(slavefd);

			/* 执行命令 */
//			execlp("/bin/sh", "/bin/sh", "-c", buffer, nullptr);

			/* 如果能运行到这里表示进程切换异常 */
//			throw std::runtime_error(
//				std::string("execlp") // + 
//			);

//        }

//        close(command_pipe[0]);
//        _exit(0);
//    }

	/* 返回其他正数表示这是主进程 */
//    else {

//        LOG_INFO("LCI main thread is running...");



        auto lciTask = std::make_shared<qing::LciTask>(
//            vm["exec"].as<std::string>(),
//            masterfd, command_pipe[1],
            vm["rotate"].as<int>(),
            vm["fontsize"].as<int>()
        );
        lciTask->start();
        taskPool.push_back(lciTask);
	

        /* 等待中间子进程终止 */
//        waitpid(pid, nullptr, 0);

        /* 将管道移动到分支外面 */
//        close(command_pipe[0]); // 关闭读端
//        close(command_pipe[1]);

//        close(masterfd);  /* 关闭虚拟终端 */
    
//    }

    return 0;
}

namespace qing {

	void LciTask::init_thread() {

		/* 静止事件 */
		f_t StopEvent  = [](Thread& th) {
			th.suspend();
		};

		/* 启动事件 */
		f_t StartEvent = [this] (Thread& th) {
		
			this->term = std::make_unique<Term>(TARGET_TTY, ORIGIN_TTY);
			this->fb = std::make_unique<FrameBuf>("/dev/fb0", rotate, fontsize);
			//this->kbe = std::make_unique<KeyboardEvent>();

			Rectangle rect = fb->get_size(); /* 屏幕大小 */
			//set_terminal_size(masterfd, rect.h / (fontsize/2), rect.w / (fontsize/2) - 1);

			
			//int cmd_margin_left = 50;
			//int cmd_margin_top  = 50;
			//int cmd_w = (rect.w - 3*50) * 0.7;
			//int cmd_h = (rect.h - 2*50) * 1.0;

			//int log_margin_left = 50 * 2 + cmd_w;
			//int log_margin_top  = 50;
			//int log_w = (rect.w - 3 * 50) * 0.3;
			//int log_h = (rect.h - 3 * 50) / 2;

			//int video_margin_left = 50 * 2 + cmd_w;	/* 位置 */
			//int video_margin_top = log_margin_top + log_h + 50;
			//int video_w = (rect.w - 3 * 50) * 0.3;/* 大小 */
			//int video_h = (rect.h - 3 * 50) / 2;
			//
			int top_margin_left = 10;
			int top_margin_top  = 10;
			int top_w = rect.w - 2 * 10;
			int top_h = 40;

			int main_margin_left = 10;
			int main_margin_top  = 10 + top_margin_top + top_h;
			int main_w = rect.w - 2 * 10;
			int main_h = rect.h - 4 * 10 - 2 * 40;

			int bottom_margin_left = 10;
			int bottom_margin_top  = 10 + main_margin_top + main_h;
			int bottom_w = rect.w - 2 * 10;
			int bottom_h = 40;


			/* 容器区域 */
			poolArea = std::make_unique<PoolArea>(fb.get(), rect.w, rect.h, 0, 0, rotate, fontsize);

			auto topArea = std::make_shared<PoolArea>(poolArea.get(), top_w, top_h, top_margin_left, top_margin_top, rotate, fontsize);
			poolArea->add("top", topArea);
			//poolArea->flush(topArea.get(), true);
			//auto topClr = green;
			//topArea->rectangle_fill(0, 0, top_w, top_h, topClr);
			
			/* 顶部内容1 */
			auto topCtt_1 = std::make_shared<ShapeArea> (topArea.get(), top_w -10, top_h -10, 5, 5, rotate, fontsize);
			topArea->add("content1", topCtt_1);

			/* 第一个字 */
			auto tctt1_c1 = std::make_shared<CharShape>(top_h-10, top_h-10, 0, 0, 'C');
			tctt1_c1->Draw(*topCtt_1, red);
			/* 第二个字 */
			auto tctt1_c2 = std::make_shared<CharShape>(top_h-10, top_h-10, top_h-10, 0, 'P');
			tctt1_c2->Draw(*topCtt_1, red);
			/* 第三个字 */
			auto tctt1_c3 = std::make_shared<CharShape>(top_h-10, top_h-10, 2*(top_h-10), 0, 'U');
			tctt1_c3->Draw(*topCtt_1, red);


			auto bottomArea = std::make_shared<PoolArea>(poolArea.get(), bottom_w, bottom_h, bottom_margin_left, bottom_margin_top, rotate, fontsize);
			poolArea->add("bottom", bottomArea);
			//poolArea->flush(bottomArea.get(), true);
			//auto bottomClr = green;
			//bottomArea->rectangle_fill(0, 0, bottom_w, bottom_h, bottomClr);

			auto mainArea = std::make_shared<PoolArea>(poolArea.get(), main_w, main_h, main_margin_left, main_margin_top, rotate, fontsize);
			poolArea->add("main", mainArea);
			poolArea->flush(mainArea.get(), false);
			auto mainClr = green;
			mainArea->line(20, 20, 20, main_h-20, mainClr);
			mainArea->line(21, 20, 21, main_h-20, mainClr);
			mainArea->line(20, main_h-18, main_w-20, main_h-18, mainClr);
			mainArea->line(20, main_h-19, main_w-20, main_h-19, mainClr);

			table_w = main_w - 41;
			table_h = main_h - 39;
			tableArea = std::make_shared<PoolArea> (mainArea.get(), table_w, table_h, 22, 20, rotate, fontsize);
			mainArea->add("table", tableArea);
			mainArea->flush(tableArea.get(), false);

			/* NOTE: 子进程收到命令会执行 */
			//write(command_pipe_send, exec_cmd.c_str(), exec_cmd.size());
			//const char* sig = "\n";
			//write(command_pipe_send, sig, strlen(sig));

			//this->printTask  = std::make_unique<lci::p_th>( /* 创建打印线程 */
			//	"Printer",
			//	masterfd,
			//	fb.get(),
			//	cmd.get()
			//);
			//printTask->wake();
			//printTask->WaitStart(10000);  /* FIXME: 使用新版的线程类 */
		
			th.run();
		};

		
		f_t LoopEvent = [this](Thread& th) {
		
			//try {

			//	struct input_event ev = kbe->get();	/* 获取输入事件 */
			//	std::cout << "Key Press:\t";
			//	std::cout << ev.code << "," << ev.value << std::endl;

			//	if (ev.type == EV_KEY && ev.value == 1 && ev.code == 1)
			//	{ /* ESC 退出键 */
			//		th.stop();
			//	}
				
			//	else if (ev.type == EV_KEY && ev.value == 1 && ev.code == 14)
			//	{ /* DELETE 删除键 */
					//cmd->delete_input();
					//cmd->clearBox();
					//cmd->update_input();
			//	}
				
			//	else if (ev.type == EV_KEY && ev.value == 1 && ev.code == 28)
			//	{ /* ENTER 回车 */
					//auto input = cmd->get_input_and_clear();
					//cmd->clearBox();
					//cmd->update_input();
					//if (input == "") {
					//	input = "\n";
					//}
					//const char *msg = input.c_str();
					//write(masterfd, msg, input.length());
			//	}
				
			//	else if (ev.type == EV_KEY && ev.code == 42)
			//	{  /* Shift */
			//		shift = (ev.value) ? true : false;
			//	}
			       
			//	else if (ev.type == EV_KEY && ev.value == 1)
			//	{ /* 其他有效按键 */

			//		char c = '\0';  /* 功能变换 */
			//		switch (ev.code) {
			//			case 2:
			//				c = (shift) ? '!' : '1';
			//				break;
			//			case 3:
			//				c = (shift) ? '@' : '2';
			//				break;
			//			case 4:
			//				c = (shift) ? '#' : '3';
			//				break;
			//			case 5:
			//				c = (shift) ? '$' : '4';
			//				break;
			//			case 6:
			//				c = (shift) ? '%' : '5';
			//				break;
			//			case 7:
			//				c = (shift) ? '^' : '6';
			//				break;
			//			case 8:
			//				c = (shift) ? '&' : '7';
			//				break;
			//			case 9:
			//				c = (shift) ? '*' : '8';
			//				break;
			//			case 10:
			//				c = (shift) ? '(' : '9';
			//				break;
			//			case 11:
			//				c = (shift) ? ')' : '0';
			//				break;
			//			case KEY_A:
			//				c = (shift) ? 'A' : 'a';
			//				break;
			//			case KEY_B:
			//				c = (shift) ? 'B' : 'b';
			//				break;
			//			case KEY_C:
			//				c = (shift) ? 'C' : 'c';
			//				break;
			//			case KEY_D:
			//				c = (shift) ? 'D' : 'd';
			//				break;
			//			case KEY_E:
			//				c = (shift) ? 'E' : 'e';
			//				break;
			//			case KEY_F:
			//				c = (shift) ? 'F' : 'f';
			//				break;
			//			case KEY_G:
			//				c = (shift) ? 'G' : 'g';
			//				break;
			//			case KEY_H:
			//				c = (shift) ? 'H' : 'h';
			//				break;
			//			case KEY_I:
			//				c = (shift) ? 'I' : 'i';
			//				break;
			//			case KEY_J:
			//				c = (shift) ? 'J' : 'j';
			//				break;
			//			case KEY_K:
			//				c = (shift) ? 'K' : 'k';
			//				break;
			//			case KEY_L:
			//				c = (shift) ? 'L' : 'l';
			//				break;
			//			case KEY_M:
			//				c = (shift) ? 'M' : 'm';
			//				break;
			//			case KEY_N:
			//				c = (shift) ? 'N' : 'n';
			//				break;
			//			case KEY_O:
			//				c = (shift) ? 'O' : 'o';
			//				break;
			//			case KEY_P:
			//				c = (shift) ? 'P' : 'p';
			//				break;
			//			case KEY_Q:
			//				c = (shift) ? 'Q' : 'q';
			//				break;
			//			case KEY_R:
			//				c = (shift) ? 'R' : 'r';
			//				break;
			//			case KEY_S:
			//				c = (shift) ? 'S' : 's';
			//				break;
			//			case KEY_T:
			//				c = (shift) ? 'T' : 't';
			//				break;
			//			case KEY_U:
			//				c = (shift) ? 'U' : 'u';
			//				break;
			//			case KEY_V:
			//				c = (shift) ? 'V' : 'v';
			//				break;
			//			case KEY_W:
			//				c = (shift) ? 'W' : 'w';
			//				break;
			//			case KEY_X:
			//				c = (shift) ? 'X' : 'x';
			//				break;
			//			case KEY_Y:
			//				c = (shift) ? 'Y' : 'y';
			//				break;
			//			case KEY_Z:
			//				c = (shift) ? 'Z' : 'z';
			//				break;
			//			case KEY_SPACE:
			//				c = ' ';
			//				break;
			//			case 51:
			//				c = (shift) ? '<' : ',';
			//				break;
			//			case 52:
			//				c = (shift) ? '>' : '.';
			//				break;
			//			case 53:
			//				c = (shift) ? '?' : '/';
			//				break;
			//			case 12:
			//				c = (shift) ? '_' : '-';
			//				break;
			//			case 43:
			//				c = (shift) ? '|' : '\\';
			//				break;

			//		}

			//		if (c != '\0') /* 如果点击了有效按键 */
			//		{
						//cmd->Input(c);
						//cmd->clearBox();
						//cmd->update_input();
			//		}

			//	} else if (ev.type == EV_REL && ev.code == REL_X) {  /* 这两个是鼠标移动事件 */
			//		std::cout << "X:\t\t" << ev.value << std::endl;
			//	} else if (ev.type == EV_REL && ev.code == REL_Y) {
			//		std::cout << "Y:\t\t" << ev.value << std::endl;
			//	}

			//} catch( KeyboardEvent::NoEvent _ ) {
			//	;
			//}


		        MemoryInfo mem = getMemoryUsageWithSysinfo();
			mem_vec.push_back(mem.usageRate);
			if (mem_vec.size() > 40) {
				mem_vec.erase(mem_vec.begin());
			}

			CpuMonitor monitor;
			double cpu_usage = monitor.get_cpu_usage();
			cpu_vec.push_back(cpu_usage);
			if (cpu_vec.size() > 40) {
				cpu_vec.erase(cpu_vec.begin());
			}


			auto bg = black;
			tableArea->rectangle_fill(0, 0, table_w, table_h, bg);

			float single_w = (float)(table_w-1) / 40.0;
			float single_h = (float)(table_h-1) / 100.0;

			for (int i = 0; i < mem_vec.size() - 1; ++i) {
				int x0 = i * single_w;
				int y0 = (100.0 - mem_vec[i]) * single_h;
				int x1 = x0 + single_w;
				int y1 = (100.0 - mem_vec[i+1]) * single_h;
				auto fg = blue;
				tableArea->line(x0, y0, x1, y1, fg);
			}

			for (int i = 0; i < cpu_vec.size() - 1; ++i) {
				int x0 = i * single_w;
				int y0 = (100.0 - cpu_vec[i]) * single_h;
				int x1 = x0 + single_w;
				int y1 = (100.0 - cpu_vec[i+1]) * single_h;
				auto fg = red;
				tableArea->line(x0, y0, x1, y1, fg);
			}


			usleep(1000000);
		};


		f_t ClearEvent = [this](Thread& th)
		{
			//printTask.reset();
			//cmd.reset();
			poolArea.reset();
			tableArea.reset();
			term.reset();
		};

		this->th = std::make_unique<Thread>(
			StopEvent, StartEvent, LoopEvent, ClearEvent
		);



	}
}
