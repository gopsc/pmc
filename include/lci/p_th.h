#pragma once
#include "Area.h"
#include "CmdArea.h"
#include "CommonThread.h"
namespace qing {
namespace lci{
/* PrintThread 打印线程 */
class p_th: public CommonThread {
public:
	/**
	 * 构造函数 传入要读取的控制台设备文件描述符，背景画布，以及命令行区域
	 */
	p_th(std::string name, int masterfd, Drawable *back, CmdArea *font)
	: CommonThread(name) {
		this->masterfd = masterfd; 
		this->back = back;
		this->font = font;
	}

	/**
	 * 析构函数 关闭线程
	 * 
	 * 我在可控制线程中主要遇到的问题便是线程的自主终止，
	 * 
	 * 在 Python 中似乎是只能实现可控制线程的手动退出的，
	 * 
	 * 我在想是不是在cpp中也做成那样。
	 *
	 *
	 * FIXME(20260108): 啥意思不懂
	 */
	~p_th(){

		/* 使线程自主关闭 */
		if (this->chk() != lci::SSHUT){
			this->shut();
		}

		/* 等待线程退出 */
		this->WaitClose();

	}

	/**
	 * 暂时删除复制构造函数，因为复制出一个与原来线程无关的空白线程似乎没有什么作用
	 * 
	 * 如果让两个变量引用同一个线程，会使得它们在结束的时候出现错误。
	 */
	p_th(p_th&) = delete;


	/**
	 * 线程睡眠阶段的操作函数，线程等待。
	 * 
	 * suspend() 方法似乎不太稳定，我还得再测试测试。
	 */
	void StopEvent() override {
		this->suspend();
	}

	/**
	 * 线程苏醒之后执行的操作，线程启动。
	 * 
	 * 我不知道为什么要等待1秒。
	 */
	void WakeEvent() {

		/* 将文件描述符集合清零 */
		FD_ZERO(&read_fds);

		/* 清空缓存字符串 */
		output = "";

		/* 设置完毕，线程进入运行态 */
		this->run();

	}

	/**
	 * 线程运行阶段的循环体，线程运行。
	 * 
	 * 使用 SELECT 进行缓冲区的探测。
	 */
	void LoopEvent() {

		/* 对可读集合进行清零 */
		//FD_ZERO(&read_fds);

		/* 设置可读集合 */
		//FD_SET(masterfd, &read_fds);

		/**
		 * 仅在集合中不存在控制台描述符时，才往其中添加该描述符
		 * 
		 * 在这个集合中，应该只会存在这一个文件描述符吧
		 */
		if (!FD_ISSET(masterfd, &read_fds))
			FD_SET(masterfd, &read_fds);

		/* 设置超时时间 */
		timeout.tv_sec = 0;
		timeout.tv_usec = 10000;

		int ret = select(masterfd + 1, &read_fds, NULL, NULL, &timeout);
		if (ret == -1) {
			//throw std::runtime_error("select");
			//this->stop();
			return;    /* qing(20260108): 为什么不停止 */
		} else if (ret == 0) { /* No data to read 无值可读 */
			if (output == "") return; /* 在空闲时进行打印*/
			font->print((char*)output.c_str(), 0); /* 按次为单位进行刷新应该可以提高效率 20260108(qing): ？*/
			back->flush(font, false);
			output = "";
		} else {
			if (FD_ISSET(masterfd, &read_fds)) { /* 有值可读 */
				nread = read(masterfd, buffer, sizeof(buffer));
				if (nread > 0) {
					buffer[nread] = '\0';
					output += buffer;
				}
			}
		}

	}

	/**
	 * 线程执行过后的清理函数，清理资源。
	 * 
	 * 什么也不做。我们在唤醒时并未申请资源。
	 */
	void ClearEvent() {
			;
	}


private:

	int masterfd;	/* 对象中存储的要读取的控制台设备文件描述符。 */

	Drawable *back;	/* 对象中存储的背景画布指针 */

	CmdArea *font;	/* 对象中存储的命令行区域指针 */

	std::string output;	/* 从终端设备中读取的进程输出 */

	//--------------------------------------------
	/* 下面的变量是为了提高速度，避免重复申请 */

	fd_set read_fds;	/* 文件描述符集合，用于存储 SELECT 返回的可读区域 */

	struct timeval timeout;	/* 时间结构体 用于传入 SELECT 函数的超时时间 */

	ssize_t nread;		/* 这一次从控制台中所读取大小 */

	char buffer[1024];	/* 存储从控制台中读取字符的缓冲区，一次最多读这么多 */

};

}
}
