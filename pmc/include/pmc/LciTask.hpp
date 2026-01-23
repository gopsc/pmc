/*
 * 图形用户界面（GUI）
 *
 * FIXME: 使用lua脚本去创建控件
 */

#include <iostream>
#include <memory>
#include <vector>
#include "th/ITask.hpp"
#include "lci/Term.h"
#include "lci/Rectangle.h"
#include "lci/Drawable.h"
#include "lci/FrameBuf.h"
#include "lci/KeyboardEvent.h"
#include "lci/CmdArea.h"
#include "lci/PoolArea.h"
#include "lci/p_th.h"
#define TARGET_TTY 8
#define ORIGIN_TTY 1
namespace qing {
class LciTask: public ITask{
public:
	LciTask(const std::string& cmd, const int masterfd, const int command_pipe_send, const int rotate, const int fontsize)
        : exec_cmd(cmd), masterfd(masterfd), command_pipe_send(command_pipe_send),
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
	int command_pipe_send;
	int masterfd;

	int rotate;
	int fontsize;
	bool shift = false;

	std::unique_ptr<Thread> th; /* 持有线程类 */
	std::unique_ptr<Term> term; /* 终端代理 */
	std::unique_ptr<FrameBuf> fb;
	std::unique_ptr<KeyboardEvent> kbe;
	std::unique_ptr<lci::p_th> printTask;
	std::unique_ptr<PoolArea> ppool; /* 持有控件池 */
	std::shared_ptr<CmdArea> cmd;

	/*
	 * 设置终端的大小
	 *
	 * 传入一个控制台设备文件的文件描述符，以及你要设定的行列值
	 */
	void set_terminal_size(int fd, int rows, int cols) {
		struct winsize sz = { .ws_row = (unsigned short)rows, .ws_col = (unsigned short)cols };
		if (ioctl(fd, TIOCSWINSZ, &sz) == -1) {
			throw std::runtime_error("ioctl");
		}
	}

	/*---------------------*/
	void init_thread();
	
};
}
