#ifndef HTTPSERVER_HPP
#define HTTPSERVER_HPP

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <memory>
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <thread>
#include <atomic>

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
using tcp = boost::asio::ip::tcp;

namespace pmc {
namespace net {

/**
 * @brief HTTP请求处理器类型定义
 * 
 * 处理HTTP请求并返回响应
 */
using HttpRequestHandler = std::function<http::response<http::string_body>(
    const http::request<http::string_body>&,
    const std::unordered_map<std::string, std::string>&)>;

/**
 * @brief 中间件处理器类型定义
 * 
 * 用于处理请求链的中间件
 */
using MiddlewareHandler = std::function<bool(
    const http::request<http::string_body>&,
    http::response<http::string_body>&,
    const std::unordered_map<std::string, std::string>&)>;

/**
 * @brief HTTP服务器类（合并增强版）
 * 
 * 基于boost::asio和boost::beast的异步HTTP服务器
 * 结合了原始版本的易用性和修复版本的功能完整性
 */
class HttpServer {
public:
    /**
     * @brief 构造函数
     * @param port 监听端口
     * @param threads 工作线程数
     */
    explicit HttpServer(unsigned short port, unsigned int threads = 1);
    
    /**
     * @brief 析构函数
     */
    ~HttpServer();
    
    /**
     * @brief 启动服务器
     */
    void start();
    
    /**
     * @brief 停止服务器
     */
    void stop();
    
    /**
     * @brief 注册GET请求处理器
     * @param path 请求路径
     * @param handler 请求处理器
     */
    void get(const std::string& path, HttpRequestHandler handler);
    
    /**
     * @brief 注册POST请求处理器
     * @param path 请求路径
     * @param handler 请求处理器
     */
    void post(const std::string& path, HttpRequestHandler handler);
    
    /**
     * @brief 注册PUT请求处理器
     * @param path 请求路径
     * @param handler 请求处理器
     */
    void put(const std::string& path, HttpRequestHandler handler);
    
    /**
     * @brief 注册DELETE请求处理器
     * @param path 请求路径
     * @param handler 请求处理器
     */
    void del(const std::string& path, HttpRequestHandler handler);
    
    /**
     * @brief 注册中间件
     * @param middleware 中间件处理器
     */
    void use(MiddlewareHandler middleware);
    
    /**
     * @brief 设置静态文件目录
     * @param path 静态文件目录路径
     */
    void setStaticDirectory(const std::string& path);
    
    /**
     * @brief 获取服务器状态
     * @return 服务器是否正在运行
     */
    bool isRunning() const;
    
    /**
     * @brief 获取服务器端口
     * @return 服务器监听端口
     */
    unsigned short getPort() const;
    
    /**
     * @brief 运行服务器（阻塞调用）- 从原始版本继承
     */
    void run();
    
    /**
     * @brief 显示服务器状态信息 - 从原始版本继承
     */
    void showStatus() const;

private:
    // 内部类声明
    class Session;
    class Listener;
    
    // 服务器配置
    unsigned short port_;
    unsigned int threads_;
    std::atomic<bool> running_{false};
    
    // Boost.Asio上下文
    std::unique_ptr<asio::io_context> ioc_;
    
    // 网络组件
    std::shared_ptr<Listener> listener_;
    std::vector<std::thread> worker_threads_;
    
    // 路由表
    std::unordered_map<std::string, HttpRequestHandler> get_handlers_;
    std::unordered_map<std::string, HttpRequestHandler> post_handlers_;
    std::unordered_map<std::string, HttpRequestHandler> put_handlers_;
    std::unordered_map<std::string, HttpRequestHandler> delete_handlers_;
    
    // 中间件链
    std::vector<MiddlewareHandler> middlewares_;
    
    // 静态文件目录
    std::string static_directory_;
    
    // 内部方法
    std::unordered_map<std::string, std::string> parseQueryParams(const std::string& query);
    
    // 请求处理
    http::response<http::string_body> handleRequest(const http::request<http::string_body>& req);
};

} // namespace net
} // namespace pmc

#endif // HTTPSERVER_HPP