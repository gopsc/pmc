// WebSocketServer.hpp - WebSocket服务器头文件
// 基于Boost.Beast和Boost.Asio

#ifndef PMC_NET_WEBSOCKETSERVER_HPP
#define PMC_NET_WEBSOCKETSERVER_HPP

#include <functional>
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <vector>
#include <thread>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace pmc::net {

/**
 * @brief WebSocket连接处理器类型定义
 * 
 * 处理WebSocket连接建立后的消息收发
 */
using WebSocketMessageHandler = std::function<void(
    const std::string& clientId,
    const std::string& message,
    std::function<void(const std::string&)> sendCallback)>;

/**
 * @brief WebSocket连接回调类型定义
 */
using WebSocketConnectHandler = std::function<void(
    const std::string& clientId,
    const std::string& remoteAddress)>;

/**
 * @brief WebSocket断开连接回调类型定义
 */
using WebSocketDisconnectHandler = std::function<void(
    const std::string& clientId)>;

/**
 * @brief WebSocket服务器类
 * 
 * 基于Boost.Beast和Boost.Asio实现的WebSocket服务器
 */
class WebSocketServer {
public:
    /**
     * @brief 构造函数
     * @param port 监听端口
     * @param threads 线程数（默认2）
     */
    WebSocketServer(unsigned short port, unsigned int threads = 2);
    
    /**
     * @brief 析构函数
     */
    ~WebSocketServer();
    
    /**
     * @brief 启动服务器
     */
    void start();
    
    /**
     * @brief 停止服务器
     */
    void stop();
    
    /**
     * @brief 注册消息处理器
     * @param path 路径（用于路由）
     * @param handler 消息处理器
     */
    void onMessage(const std::string& path, WebSocketMessageHandler handler);
    
    /**
     * @brief 注册连接处理器
     * @param handler 连接处理器
     */
    void onConnect(WebSocketConnectHandler handler);
    
    /**
     * @brief 注册断开连接处理器
     * @param handler 断开连接处理器
     */
    void onDisconnect(WebSocketDisconnectHandler handler);
    
    /**
     * @brief 向指定客户端发送消息
     * @param clientId 客户端ID
     * @param message 消息内容
     * @return 是否发送成功
     */
    bool sendToClient(const std::string& clientId, const std::string& message);
    
    /**
     * @brief 向所有客户端广播消息
     * @param message 消息内容
     * @return 成功发送的客户端数量
     */
    size_t broadcast(const std::string& message);
    
    /**
     * @brief 断开指定客户端连接
     * @param clientId 客户端ID
     */
    void disconnectClient(const std::string& clientId);
    
    /**
     * @brief 获取当前连接的客户端数量
     * @return 客户端数量
     */
    size_t getClientCount() const;
    
    /**
     * @brief 获取服务器是否正在运行
     * @return 运行状态
     */
    bool isRunning() const { return running_; }
    
    /**
     * @brief 获取服务器监听端口
     * @return 端口号
     */
    unsigned short getPort() const { return port_; }

private:
    // 内部Session类
    class Session;
    
    // 内部Listener类
    class Listener;
    
    // 内部实现结构体
    struct Impl {
        boost::asio::io_context ioc_;
        std::shared_ptr<Listener> listener_;
        std::vector<std::thread> threads_;
    };
    
    // 服务器配置
    unsigned short port_;
    unsigned int threads_;
    std::atomic<bool> running_{false};
    
    // 内部实现
    std::unique_ptr<Impl> impl_;
    
    // 生成客户端ID
    std::string generateClientId() const;
    
    // 添加客户端
    void addClient(const std::string& clientId, std::shared_ptr<Session> session);
    
    // 移除客户端
    void removeClient(const std::string& clientId);
    
    // 获取客户端Session
    std::shared_ptr<Session> getClient(const std::string& clientId);
    
    // 回调函数
    std::unordered_map<std::string, WebSocketMessageHandler> messageHandlers_;
    WebSocketConnectHandler connectHandler_;
    WebSocketDisconnectHandler disconnectHandler_;
    
    // 客户端管理
    mutable std::mutex clientsMutex_;
    std::unordered_map<std::string, std::shared_ptr<Session>> clients_;
};

} // namespace pmc::net

#endif // PMC_NET_WEBSOCKETSERVER_HPP