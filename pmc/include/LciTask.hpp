#include <iostream>
#include <memory>
#include "ITask.hpp"
#include "lci/Term.h"
#include "lci/Rectangle.h"
#include "lci/Drawable.h"
#include "lci/FrameBuf.h"
#include "lci/KeyboardEvent.h"
#include "lci/CmdArea.h"
#include "lci/p_th.h"
#define TARGET_TTY 8
#define ORIGIN_TTY 1
namespace qing {
class LciTask: public ITask{
public:
	LciTask(const std::string& cmd, const int masterfd, const int command_pipe_send, const int rotate, const int fontsize)
        : exec_cmd(cmd), masterfd(masterfd), command_pipe_send(command_pipe_send),
	rotate(rotate), fontsize(fontsize) {

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
	std::unique_ptr<Term> term;
	std::unique_ptr<FrameBuf> fb;
	std::unique_ptr<KeyboardEvent> kbe;
	std::unique_ptr<CmdArea> videoArea;
	std::unique_ptr<CmdArea> logArea;
	std::unique_ptr<CmdArea> cmdArea;
	std::unique_ptr<lci::p_th> printTask;

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
	void init_thread() {

		/* 静止事件 */
		f_t StopEvent  = [](Thread& th) {
			th.suspend();
		};

		/* 启动事件 */
		f_t StartEvent = [this] (Thread& th) {
		
			this->term = std::make_unique<Term>(TARGET_TTY, ORIGIN_TTY);
			//std::vector<std::unique_ptr<Drawable>> v;
			this->fb = std::make_unique<FrameBuf>("/dev/fb0", rotate, fontsize);
			this->kbe = std::make_unique<KeyboardEvent>();

			Rectangle rect = fb->get_size(); /* 屏幕大小 */
			set_terminal_size(masterfd, rect.h / (fontsize/2), rect.w / (fontsize/2) - 1);

			
			int video_margin_left = 50;
			int video_margin_top  = 50 - 14;
			int video_w = (rect.w - 3*50) / 2;
			int video_h = (rect.h  - 3*50) /2;

			int cmd_margin_left = 50 + video_w + 50;
			int cmd_margin_top  = 50 - 14;
			int cmd_w = (rect.w - 3 * 50) / 2;
			int cmd_h = (rect.h - 3 * 50) / 2;

			int log_margin_left = 50;	/* 位置 */
			int log_margin_top = video_margin_top + video_h + 50 + 14;
			int log_w = rect.w - 2 * 50;/* 大小 */
			int log_h = (rect.h - 3 * 50) / 2;

			this->videoArea  = std::make_unique<CmdArea> (fb.get(), video_w, video_h, video_margin_left, video_margin_top, 0, fontsize*2);
			fb->flush(this->videoArea.get(), true);

			this->cmdArea = std::make_unique<CmdArea>(fb.get(), cmd_w, cmd_h, cmd_margin_left, cmd_margin_top, 0, fontsize, true);
			fb->flush(cmdArea.get(), true);

			this->videoArea->test();
			this->logArea = std::make_unique<CmdArea> (fb.get(), log_w, log_h, log_margin_left, log_margin_top, 0, fontsize);
			fb->flush(this->logArea.get(), true);

			this->videoArea->test1();
			this->logArea->print((char*)"192.168.43.12\n192.168.43.13\n", 0);

			/* NOTE: 子进程收到命令会执行 */
			write(command_pipe_send, exec_cmd.c_str(), exec_cmd.size());
			const char* sig = "\n";
			write(command_pipe_send, sig, strlen(sig));

			this->printTask  = std::make_unique<lci::p_th>( /* 创建打印线程 */
				"Printer",
				masterfd,
				fb.get(),
				cmdArea.get()
			);
			printTask->wake();
			printTask->WaitStart(10000);  /* FIXME: 使用新版的线程类 */
		
			th.run();
		};

		
		f_t LoopEvent = [this](Thread& th) {
		
			try {

				struct input_event ev = kbe->get();	/* 获取输入事件 */
				std::cout << "Key Press:\t";
				std::cout << ev.code << "," << ev.value << std::endl;

				if (ev.type == EV_KEY && ev.value == 1 && ev.code == 1)
				{ /* ESC 退出键 */
					th.stop();
				}
				
				else if (ev.type == EV_KEY && ev.value == 1 && ev.code == 14)
				{ /* DELETE 删除键 */
					cmdArea->delete_input();
					cmdArea->clearBox();
					cmdArea->update_input();
				}
				
				else if (ev.type == EV_KEY && ev.value == 1 && ev.code == 28)
				{ /* ENTER 回车 */
					auto input = cmdArea->get_input_and_clear();
					cmdArea->clearBox();
					cmdArea->update_input();
					if (input == "") {
						input = "\n";
					}
					const char *msg = input.c_str();
					write(masterfd, msg, input.length());
				}
				
				else if (ev.type == EV_KEY && ev.code == 42)
				{  /* Shift */
					shift = (ev.value) ? true : false;
				}
			       
				else if (ev.type == EV_KEY && ev.value == 1)
				{ /* 其他有效按键 */

					char c = '\0';  /* 功能变换 */
					switch (ev.code) {
						case 2:
							c = (shift) ? '!' : '1';
							break;
						case 3:
							c = (shift) ? '@' : '2';
							break;
						case 4:
							c = (shift) ? '#' : '3';
							break;
						case 5:
							c = (shift) ? '$' : '4';
							break;
						case 6:
							c = (shift) ? '%' : '5';
							break;
						case 7:
							c = (shift) ? '^' : '6';
							break;
						case 8:
							c = (shift) ? '&' : '7';
							break;
						case 9:
							c = (shift) ? '*' : '8';
							break;
						case 10:
							c = (shift) ? '(' : '9';
							break;
						case 11:
							c = (shift) ? ')' : '0';
							break;
						case KEY_A:
							c = (shift) ? 'A' : 'a';
							break;
						case KEY_B:
							c = (shift) ? 'B' : 'b';
							break;
						case KEY_C:
							c = (shift) ? 'C' : 'c';
							break;
						case KEY_D:
							c = (shift) ? 'D' : 'd';
							break;
						case KEY_E:
							c = (shift) ? 'E' : 'e';
							break;
						case KEY_F:
							c = (shift) ? 'F' : 'f';
							break;
						case KEY_G:
							c = (shift) ? 'G' : 'g';
							break;
						case KEY_H:
							c = (shift) ? 'H' : 'h';
							break;
						case KEY_I:
							c = (shift) ? 'I' : 'i';
							break;
						case KEY_J:
							c = (shift) ? 'J' : 'j';
							break;
						case KEY_K:
							c = (shift) ? 'K' : 'k';
							break;
						case KEY_L:
							c = (shift) ? 'L' : 'l';
							break;
						case KEY_M:
							c = (shift) ? 'M' : 'm';
							break;
						case KEY_N:
							c = (shift) ? 'N' : 'n';
							break;
						case KEY_O:
							c = (shift) ? 'O' : 'o';
							break;
						case KEY_P:
							c = (shift) ? 'P' : 'p';
							break;
						case KEY_Q:
							c = (shift) ? 'Q' : 'q';
							break;
						case KEY_R:
							c = (shift) ? 'R' : 'r';
							break;
						case KEY_S:
							c = (shift) ? 'S' : 's';
							break;
						case KEY_T:
							c = (shift) ? 'T' : 't';
							break;
						case KEY_U:
							c = (shift) ? 'U' : 'u';
							break;
						case KEY_V:
							c = (shift) ? 'V' : 'v';
							break;
						case KEY_W:
							c = (shift) ? 'W' : 'w';
							break;
						case KEY_X:
							c = (shift) ? 'X' : 'x';
							break;
						case KEY_Y:
							c = (shift) ? 'Y' : 'y';
							break;
						case KEY_Z:
							c = (shift) ? 'Z' : 'z';
							break;
						case KEY_SPACE:
							c = ' ';
							break;
						case 51:
							c = (shift) ? '<' : ',';
							break;
						case 52:
							c = (shift) ? '>' : '.';
							break;
						case 53:
							c = (shift) ? '?' : '/';
							break;
						case 12:
							c = (shift) ? '_' : '-';
							break;
						case 43:
							c = (shift) ? '|' : '\\';
							break;

					}

					if (c != '\0') /* 如果点击了有效按键 */
					{
						cmdArea->Input(c);
						cmdArea->clearBox();
						cmdArea->update_input();
					}

				} else if (ev.type == EV_REL && ev.code == REL_X) {  /* 这两个是鼠标移动事件 */
					std::cout << "X:\t\t" << ev.value << std::endl;
				} else if (ev.type == EV_REL && ev.code == REL_Y) {
					std::cout << "Y:\t\t" << ev.value << std::endl;
				}

			} catch( KeyboardEvent::NoEvent _ ) {
				;
			}
		
		};


		f_t ClearEvent = [this](Thread& th) {
		
			fb.reset();
			kbe.reset();
			cmdArea.reset();
			printTask.reset();
			term.reset();
			
		};

		this->th = std::make_unique<Thread>(
			StopEvent, StartEvent, LoopEvent, ClearEvent
		);



	}

};
}
