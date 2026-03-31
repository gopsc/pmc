#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include <functional>
#include <vector>
#include <utility>
#include <csignal>
#include <atomic>
#include <fstream>
#include <filesystem>

#include <boost/json.hpp>

#include "net/HttpClient.hpp"
#include "net/WebSocketServer.hpp"
#include "llm/LLMClient.hpp"
#include "llm/LLMWebSocketServer.hpp"

// 全局变量用于信号处理
std::atomic<bool> g_running{true};

// 从文件读取API密钥
std::string read_api_key_from_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("无法打开密钥文件: " + filepath);
    }
    
    std::string api_key;
    std::getline(file, api_key);
    
    // 去除可能的空白字符
    api_key.erase(0, api_key.find_first_not_of(" \t\n\r\f\v"));
    api_key.erase(api_key.find_last_not_of(" \t\n\r\f\v") + 1);
    
    if (api_key.empty()) {
        throw std::runtime_error("密钥文件为空: " + filepath);
    }
    
    return api_key;
}

int main(int argc, char* argv[]) {
    // 解析命令行参数
    std::string api_key;
    unsigned short port = 8080;
    std::string key_file_path = std::string(getenv("HOME")) + "/.cert/deepseek.key";
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--key-file" && i + 1 < argc) {
            key_file_path = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            port = static_cast<unsigned short>(std::stoi(argv[++i]));
        } else if (arg == "--help") {
            std::cout << "用法: " << argv[0] << " [--key-file <密钥文件路径>] [--port <端口号>]" << std::endl;
            std::cout << "默认密钥文件路径: ~/.cert/deepseek.key" << std::endl;
            return 0;
        }
    }
    
    try {
        // 从文件读取API密钥
        std::cout << "正在从文件加载API密钥: " << key_file_path << std::endl;
        api_key = read_api_key_from_file(key_file_path);
        std::cout << "API密钥加载成功" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        std::cerr << "请确保密钥文件存在且包含有效的API密钥" << std::endl;
        std::cerr << "默认路径: ~/.cert/deepseek.key" << std::endl;
        std::cerr << "或使用 --key-file 参数指定其他路径" << std::endl;
        return 1;
    }
    
    // 创建并启动服务器
    pmc::llm::LLMWebSocketServer server(api_key, port);
    
    // 设置信号处理
    signal(SIGINT, [](int) {
        g_running = false;
    });
    
    try {
        server.start();
        
        // 主循环，等待信号
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // 停止服务器
        server.stop();
        
    } catch (const std::exception& e) {
        std::cerr << "服务器错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}