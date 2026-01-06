/*
 * 这是cpp使用linux C手段读写串口的实现。
 *
 * 作者在其中加入了“沟通”手段，这种手段主要是针对COPS设备。
 *
 * COPS设备的通信手段主要是上位机问，下位机答，一问一答的形式。
 *
 * （！）还未进行对于热插拔的支持。
 *
 * 20250621 暂时不用了
 */
#pragma once
#include <iostream>
#include <vector>
#include <regex>
#include <chrono>
#include <thread>
#include <cstdio>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>
namespace qing {
    /*
     * 串口类
     *
     * 长连接的速度会更快
     */
    class Serial {
        private:
            /* 串口文件描述符 */
            int fd = 0;
        public:
            /* 输入设备文件地址，进行构造。打开后通常需要等待一下 */
            Serial(const std::string& path, double wait = 2) {
                fd = open(path.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
                if (fd == -1) {
                    throw DeviceOpenErroring();
                }


                std::this_thread::sleep_for(std::chrono::milliseconds((long)(wait*1000))); /* 等待串口 */
            }
            //释放
            ~Serial() {
                if (fd != -1) close(fd);
            }

            /* 设置串口 */
            void Set() {
                struct termios options;
                tcgetattr(fd, &options);
                cfsetispeed(&options, B115200); /* 设置波特率 */
                cfsetospeed(&options, B115200);
                options.c_cflag |= (CLOCAL | CREAD);// 启用接收和本地模式
                options.c_cflag &= ~PARENB;// 8位数据位，无校验，1位停止位
                options.c_cflag &= ~CSTOPB;
                options.c_cflag &= ~CSIZE;
                options.c_cflag |= CS8;
                options.c_cflag &= ~CRTSCTS;// 禁用硬件流控
                options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);// 原始输入模式
                options.c_oflag &= ~OPOST;// 原始输出模式
                options.c_cc[VMIN] = 0;// 设置超时 - 1秒
                options.c_cc[VTIME] = 10;
                tcsetattr(fd, TCSANOW, &options);
                tcflush(fd, TCIOFLUSH); /* 清空缓冲区 */
            }

            /* 写数据 每次写之前清空缓冲区数据*/
            void Write(const std::string& msg) {
                tcflush(fd, TCIOFLUSH); /* 清空缓冲区 */
                write(fd, msg.c_str(), msg.size());
            }

            /* 读数据 */
            std::string CircleRead() {
                char buffer[1024] = {0};
                std::string ret = "";
                ssize_t n;
                do {
                    n = read(fd, buffer, sizeof(buffer));
                    if (n == -1 && ret == "") {
                        throw NoResponse();
                    } else if (n == -1 && ret != "") {
                        return ret;
                    } else if (n == 0) {
                        return ret;
                    } else if (n > 0) {
                        buffer[n] = '\0';
                        ret += buffer;
                    }
                } while (true);
            }

            /* 读数据 */
            std::string Read() {
                char buffer[1024] = {0};
                std::string ret = "";
                ssize_t n;
                n = read(fd, buffer, sizeof(buffer));
                if (n == -1 && ret == "") {
                    throw NoResponse();
                } else if (n > 0) {
                    buffer[n] = '\0';
                    ret += buffer;
                }
                return ret;
            }


            /* 发送然后获取 */
            std::string Send_Wait_And_Get(const std::string& snd, double wait) {
                Write(snd);
                std::this_thread::sleep_for(std::chrono::milliseconds((long)(wait*1000)));
                return Read();
            }

            /* 一问一答，尝试一定次数 */
            std::string Try_Comm(const std::string& snd, double wait, int times = 10, int drop_tail = 0) {
                int num = 0;
                while (true) {
                    try {
                        num ++;
                        if (num > times) break;
                        std::string msg = Send_Wait_And_Get(snd, wait);
                        DropTail(msg, drop_tail);
                        return msg;
                    } catch (qing::Serial::Serial_Error e) {
                        std::cerr << e.what() << std::endl;
                    }
                }
                throw NoResponse();
            }

             /* 去掉字符串末尾一定数量的字符 */
             static void DropTail(std::string& msg, int n) {
                for (int i=0; i<n; i++) {
                    msg.pop_back();
                }
            }

            /* 串口异常类 */
            class Serial_Error: public std::exception {
                private:
                    std::string message;
                public:
                    explicit Serial_Error(const std::string& msg): message(msg) {}
                    const char* what() const noexcept override {
                        return message.c_str();
                    }
            };

            /* 打开设备异常 */
            class DeviceOpenErroring: public Serial_Error {
                public:
                    DeviceOpenErroring(): Serial_Error("Device Open Erroring") {}
            };

            /* select异常 */
            class SelectErroring: public Serial_Error {
                public:
                    SelectErroring(): Serial_Error("Select Erroring") {}
            };

            /* 无响应 */
            class NoResponse: public Serial_Error {
                public:
                    NoResponse(): Serial_Error(" No Response") {}
            };

            /* 无响应异常 */
            class SerialReadError: public Serial_Error {
                public:
                    SerialReadError(): Serial_Error("Serial Read Error") {}
            };

	    /* 获取所有的串口 */
            static std::vector<std::string> GetPorts() {
                std::vector<std::string> ports;
                //Linux实现
		DIR *dir;
                struct dirent *ent;
                const std::regex serial_regex("(ttyS|ttyUSB|ttyACM)[0-9]+");
                if ((dir = opendir("/dev")) != NULL) {
		    while((ent = readdir(dir)) != NULL) {
			std::string name = ent->d_name;
			if (std::regex_match(name, serial_regex)) {
			    ports.push_back("/dev/"+name);
			}
		    }
		    closedir(dir);
                }
		return ports;
            }
	    
	    /* 以线程不安全的方式打印所有的串口 */
	    static void UnsafePrintPorts() {
		auto v = GetPorts();
		for (const auto& i: v) {
		    std::cout << i << std::endl;
		}
	    }

    };
}
