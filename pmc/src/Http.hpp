#pragma once
#include <httplib.h> // libcpp-httplib-dev
#include <nlohmann/json.hpp> // 
#include <iostream>
#include <filesystem> // 
#include <stdexcept>
namespace fs  = std::filesystem;

// 这里的类型是HTTP服务器要设置的回调函数的类型
using Handler = std::function<void(const httplib::Request&, httplib::Response&)>;

namespace qing {
    /*
     * 超文本传输协议（HTTP）服务器类
     *
     * 该类主要是为了简化Http服务器的搭建
     * 有点过度封装的嫌疑
     */
    class Http {
        public:

            // 初始化各个变量
            Http(std::string addr, int port): 
                addr(addr), port(port) {} 

            
            // 运行服务器，失败抛出异常
            void run() {
                if (!svr.listen(addr, port)) {
                    throw StartServerFailed("");
                }
            }

            // 终止服务器
            void stop() {
                svr.stop();
            }


            // 设置路由和对应的处理函数 GET
            void get(std::string path, Handler handler) {
                svr.Get(path, handler);
            }

            // POST
            void post(std::string path, Handler handler) {
                svr.Post(path, handler);
            }

            // 打印地址
            //
            // 修改意见：在run()中调用
            std::string printAddr()  {
                return std::string("Listenning on this address: http://")
                    + addr
                    + ":"
                    + std::to_string(port);
            }

            // 获取当前执行文件目录
            static std::string getCurrentDirectory() {
                fs::path currentPath = fs::absolute(__FILE__);
                return currentPath.parent_path().string();
            }

            // 读取文本文件的所有内容
            static std::string readFile(const std::string& filePath) {
                std::ifstream file(filePath); // 创建一个输入文件流对象，用于读取文件
                if (!file) {
                    std::cerr << "无法打开文件：" << filePath << std::endl;
                    return "";
                }
                //使用迭代器读取文件内容到字符串
                std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                file.close();
                return content;
            }

            // HTTP 异常类
            class Http_Error: public std::exception {
                private:
                    std::string message;
                public:
                    explicit Http_Error(const std::string& msg): message(msg) {}
                    const char* what() const noexcept override {
                        return message.c_str();
                    }
            };

            // 开始服务失败
            class StartServerFailed: public Http_Error {
            public:
                StartServerFailed(const std::string& msg): Http_Error(msg) {}
            };


        private:

            // 创建HTTP服务器对象（如果需要HTTPS，则使用SSLServer
            httplib::Server svr;

            // 服务配置字段
            //
            // 地址
            std::string addr;

            // 端口
            int port;

    };
}
