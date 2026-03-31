#ifndef PMC_LLM_WEBSOCKET_SERVER_HPP
#define PMC_LLM_WEBSOCKET_SERVER_HPP

#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include <functional>
#include <vector>
#include <utility>
#include <atomic>

#include "LLMClient.hpp"
#include "net/WebSocketServer.hpp"

namespace pmc::llm {

class LLMWebSocketServer {
public:
    LLMWebSocketServer(const std::string& api_key, unsigned short port = 8080)
        : api_key_(api_key)
        , port_(port)
        , running_(false) {
        
        // 创建WebSocket服务器
        ws_server_ = std::make_unique<pmc::net::WebSocketServer>(port);
    }
    
    ~LLMWebSocketServer() {
        stop();
    }
    
    void start() {
        if (running_) {
            return;
        }
        
        std::cout << "启动LLM WebSocket服务器，端口: " << port_ << std::endl;
        std::cout << "API密钥: " << (api_key_.empty() ? "未设置" : "已设置") << std::endl;
        
        // 设置WebSocket回调
        ws_server_->onConnect([this](const std::string& client_id, const std::string& remote_address) {
            handle_connect(client_id, remote_address);
        });
        
        ws_server_->onMessage("/", [this](const std::string& client_id,
                                         const std::string& message,
                                         std::function<void(const std::string&)> send_callback) {
            // 这里我们使用异步发送，所以忽略send_callback参数
            handle_message(client_id, message);
        });
        
        ws_server_->onDisconnect([this](const std::string& client_id) {
            handle_disconnect(client_id);
        });
        
        // 启动WebSocket服务器
        ws_server_->start();
        
        running_ = true;
        
        std::cout << "服务器已启动，等待客户端连接..." << std::endl;
        std::cout << "按Ctrl+C停止服务器" << std::endl;
    }
    
    void stop() {
        if (!running_) {
            return;
        }
        
        std::cout << "正在停止服务器..." << std::endl;
        ws_server_->stop();
        running_ = false;
    }
    
    bool is_running() const {
        return running_;
    }

private:
    // 处理连接
    void handle_connect(const std::string& client_id, const std::string& remote_address) {
        std::cout << "客户端连接: " << client_id << " (" << remote_address << ")" << std::endl;
        
        // 使用异步线程发送欢迎消息，确保客户端已准备好
        std::thread([this, client_id]() {
            // 等待一小段时间，确保客户端连接完全建立
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // 发送欢迎消息
            ws_server_->sendToClient(client_id, "欢迎连接到LLM WebSocket服务器！");
            ws_server_->sendToClient(client_id, "请发送您的消息，我将为您提供AI助手服务。");
        }).detach();
    }
    
    // 处理断开连接
    void handle_disconnect(const std::string& client_id) {
        std::cout << "客户端断开: " << client_id << std::endl;
    }
    
    // 处理消息
    void handle_message(const std::string& client_id, const std::string& message) {
        std::cout << "收到来自客户端 " << client_id << " 的消息: " << message << std::endl;
        
        // 创建LLM客户端
        LLMClient llm_client(api_key_);
        
        // 构建消息历史
        std::vector<std::pair<std::string, std::string>> messages = {
            {"user", message}
        };
        
        // 发送流式请求
        llm_client.stream_request(messages, [this, client_id](const std::string& chunk) {
            // 将每个数据块发送给客户端
            ws_server_->sendToClient(client_id, chunk);
        });
        
        // 发送处理完成标记
        ws_server_->sendToClient(client_id, "[处理完成]");
    }

private:
    std::string api_key_;
    unsigned short port_;
    bool running_;
    std::unique_ptr<pmc::net::WebSocketServer> ws_server_;
};

} // namespace pmc::llm

#endif // PMC_LLM_WEBSOCKET_SERVER_HPP