#pragma once
#include <string>
#include <iostream>
#include <unistd.h>
#include <fcntl.h> /* open() */
#include <sys/stat.h> /* mkfifo() */
#include <sys/select.h>

namespace qing {
class Pipe{  /* 20260309(qing): 该类不会抛出异常 */
public:
	static const int CREATOR = 1;
        static const int USER =2;
	static const ssize_t PIPE_MAX_LEN = 2048;
	Pipe(const std::string& path, int mode)
	: fd(-1), isCreator(mode == CREATOR) {
		std::string real_path = "/tmp/pst-";
		real_path += path;
		fifo_path = real_path;
		if (isCreator && mkfifo(fifo_path.c_str(), 0666) == -1 && errno != EEXIST)  /* wr_wr_wr */
			perror("mkfifo");  /* 文件已存在是正常情况，其他错误才需要处理 */
	}

	~Pipe() {
		/* 如果打开了文件，则关闭文件 */
		if (fd != -1) close(fd);
		/**********************/
		/* 如果管道由该对象创建，就要负责清理 */
		if (isCreator) unlink(fifo_path.c_str()); /* 创建者负责清理 */
	}
	Pipe(const Pipe&) = delete;
	Pipe& operator=(const Pipe&) = delete;

	/* 这里才是打开文件的指令 - 以读模式打开，可以非阻塞 */
	bool openForRead(bool nonblock = false) {

		/* 设置是否以非阻塞方式打开文件 */
		int flags = O_RDONLY;
		if (nonblock) {
			flags |= O_NONBLOCK;
		}

		fd = open(fifo_path.c_str(), flags);

		if (fd == -1) {
			perror("open for read");
			return false;
		}

		else {
			FD_ZERO(&read_fds);
			FD_SET(fd, &read_fds);
			if (fd > max_fd) max_fd = fd;
			return true;
		}

	}

	/* 以写模式打开文件 */
	bool openForWrite(/*bool nonblock = falsei*/) {

		/* 写入管道是不需要阻塞/非阻塞的？ */
		int flags = O_WRONLY;
		//if (nonblock) {
		//	flags |= O_NONBLOCK;
		//}
		fd = open(fifo_path.c_str(), flags);
		if (fd == -1) {
			perror("open for write");
			return false;
		}

		else {
			FD_ZERO(&write_fds);
			FD_SET(fd, &write_fds);
			if (fd > max_fd) max_fd = fd;
			return true;
		}

	}

	/* 写入管道，如果失败返回-1。成功返回写入的字符数。 */
	ssize_t writePipe(const std::string &data) {
		if (fd == -1) {
			std::cerr << "Pipe not opened for writing" << std::endl;
			return -1;
		}
		else {
			return write(fd, data.c_str(), data.size());
		}
	}

	/* 将探测与读取解耦，使用者可以自行决定要不要阻塞 */
	bool waitForRead(int timeout_sec = 0, int timeout_usec = 0) {

		/* 如果文件没打开，等待是失败的 */
		if (fd == -1) return false;

		fd_set temp_fds = read_fds;
		struct timeval timeout = {timeout_sec, timeout_usec};

		int ret = select(max_fd + 1, &temp_fds, nullptr, nullptr, &timeout);

		if (ret == -1) {
			perror("select");
			return false;
		}

		else {
			return ret > 0 && FD_ISSET(fd, &temp_fds);
		}
	}

	/* 读取数据到字符串，返回实际读取的字节数 */
	ssize_t readPipe(std::string *output) {

		if (fd == -1) {
			std::cerr << "Pipe not opened for reading" << std::endl;
			return -1;
		}

		else
		
		{
		
			char buffer[PIPE_MAX_LEN];
			ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
			if (n >0) {
				buffer[n] = '\0';
				*output = buffer;
			}

			else if (n == 0) {
				/* 管道写入端关闭 */
				*output = "";
			}

			else {
				perror("read");
			}

			return n;
		
		}
	}
private:
	std::string fifo_path;
	int fd;
	bool isCreator;
	fd_set read_fds;
	fd_set write_fds;
	fd_set except_fds;
	int max_fd;
};
}
