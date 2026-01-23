#include <memory>
#include "th/Thread.hpp"
#include "pmc/LciTask.hpp"
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
			this->kbe = std::make_unique<KeyboardEvent>();

			Rectangle rect = fb->get_size(); /* 屏幕大小 */
			set_terminal_size(masterfd, rect.h / (fontsize/2), rect.w / (fontsize/2) - 1);

			
			int video_margin_left = 50;
			int video_margin_top  = 50;
			int video_w = (rect.w - 3*50) * 0.7;
			int video_h = (rect.h - 2*50) * 1.0;

			int log_margin_left = 50 * 2 + video_w;
			int log_margin_top  = 50;
			int log_w = (rect.w - 3 * 50) * 0.3;
			int log_h = (rect.h - 3 * 50) / 2;

			int cmd_margin_left = 50 * 2 + video_w;	/* 位置 */
			int cmd_margin_top = log_margin_top + log_h + 50;
			int cmd_w = (rect.w - 3 * 50) * 0.3;/* 大小 */
			int cmd_h = (rect.h - 3 * 50) / 2;


			/* 容器区域 */
			ppool = std::make_unique<PoolArea>(fb.get(), rect.w, rect.h, 0, 0, rotate, fontsize);

			/* 图传区域 */
			auto video = std::make_shared<PoolArea>(fb.get(), video_w, video_h, video_margin_left, video_margin_top, rotate, fontsize*2);
			fb->flush(video.get(), true);
			ppool->add("video", video);
			//video->test();
			//video->test1();

			/* 终端区域 */
			auto cmdArea = std::make_shared<PoolArea>(fb.get(), cmd_w, cmd_h, cmd_margin_left, cmd_margin_top, rotate, fontsize);
			fb->flush(cmdArea.get(), true);
			ppool->add("cmd", cmdArea);

			/* 终端内容 */
			cmd = std::make_shared<CmdArea>(cmdArea.get(), cmd_w-10, cmd_h-10, 5, 5, rotate, fontsize, true);
			cmdArea->add("content", cmd);

			/* 日志区域 */
			auto logsArea = std::make_shared<PoolArea>(fb.get(), log_w, log_h, log_margin_left, log_margin_top, rotate, fontsize);
			fb->flush(logsArea.get(), true);
			ppool->add("logs", logsArea);

			/* 日志内容 */
			auto logs = std::make_shared<CmdArea> (logsArea.get(), log_w-10, log_h-10, 5, 5, rotate, fontsize);
			logsArea->add("content", logs);
			logs->print((char*)"- - - - -\n"
					"欢迎来到对讲机界面\n"  /* FIXME: 不知道为什么这里不能分开多次打印，否则会堵塞 */
					"- - - - -\n"
					"输入内容与智能体、机器人进行交互。\n", 0);

			/* NOTE: 子进程收到命令会执行 */
			write(command_pipe_send, exec_cmd.c_str(), exec_cmd.size());
			const char* sig = "\n";
			write(command_pipe_send, sig, strlen(sig));

			this->printTask  = std::make_unique<lci::p_th>( /* 创建打印线程 */
				"Printer",
				masterfd,
				fb.get(),
				cmd.get()
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
					cmd->delete_input();
					cmd->clearBox();
					cmd->update_input();
				}
				
				else if (ev.type == EV_KEY && ev.value == 1 && ev.code == 28)
				{ /* ENTER 回车 */
					auto input = cmd->get_input_and_clear();
					cmd->clearBox();
					cmd->update_input();
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
						cmd->Input(c);
						cmd->clearBox();
						cmd->update_input();
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


		f_t ClearEvent = [this](Thread& th)
		{
			printTask.reset();
			cmd.reset();
			ppool.reset();
			term.reset();
		};

		this->th = std::make_unique<Thread>(
			StopEvent, StartEvent, LoopEvent, ClearEvent
		);



	}
}
