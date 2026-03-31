#include "net/WebSocketClient.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>

namespace pmc {
namespace net {

namespace beast = boost::beast;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

// 内部实现结构体
struct WebSocketClient::Impl {
    // 配置参数
    std::string host;
    std::string port;
    std::string path;
    
    // Boost 对象
    asio::io_context ioc;
    tcp::resolver resolver;
    beast::websocket::stream<beast::tcp_stream> ws;
    
    // 线程管理
    std::thread io_thread;
    std::atomic<bool> running{false};
    
    // 回调函数
    WebSocketClientBase::MessageCallback message_callback;
    WebSocketClientBase::ErrorCallback error_callback;
    WebSocketClientBase::ConnectCallback connect_callback;
    WebSocketClientBase::CloseCallback close_callback;
    
    // 消息队列
    std::queue<std::string> send_queue;
    std::mutex queue_mutex;
    std::condition_variable queue_cv;
    std::atomic<bool> sending{false};
    
    // 状态标志
    std::atomic<bool> connected{false};
    std::atomic<bool> connecting{false};
    std::atomic<bool> stopping{false};
    
    // 配置选项
    bool auto_reconnect{false};
    int reconnect_interval_ms{3000};
    int heartbeat_interval_ms{30000};
    
    // 构造函数
    Impl(const std::string& host, const std::string& port, const std::string& path)
        : host(host)
        , port(port)
        , path(path)
        , resolver(asio::make_strand(ioc))
        , ws(asio::make_strand(ioc))
    {
    }
    
    // 析构函数
    ~Impl() {
        stop();
    }
    
    // 启动IO线程
    void start() {
        if (!running) {
            running = true;
            io_thread = std::thread([this]() {
                while (running) {
                    try {
                        ioc.run();
                        break;
                    } catch (const std::exception& e) {
                        std::cerr << "IO上下文异常: " << e.what() << std::endl;
                    }
                }
            });
        }
    }
    
    // 停止IO线程
    void stop() {
        running = false;
        stopping = true;
        
        if (io_thread.joinable()) {
            ioc.stop();
            io_thread.join();
        }
    }
    
    // 执行连接
    void doConnect() {
        if (connecting || connected) {
            return;
        }
        
        connecting = true;
        connected = false;
        
        // 解析主机名
        resolver.async_resolve(host, port,
            [this](beast::error_code ec, tcp::resolver::results_type results) {
                onResolve(ec, results);
            });
    }
    
    // 解析完成回调
    void onResolve(beast::error_code ec, tcp::resolver::results_type results) {
        if (ec) {
            onError("解析失败: " + ec.message());
            return;
        }
        
        // 连接到解析出的端点
        beast::get_lowest_layer(ws).async_connect(results,
            [this](beast::error_code ec, tcp::resolver::results_type::endpoint_type) {
                onConnect(ec);
            });
    }
    
    // 连接完成回调
    void onConnect(beast::error_code ec) {
        if (ec) {
            onError("连接失败: " + ec.message());
            return;
        }
        
        // 设置WebSocket选项
        ws.set_option(beast::websocket::stream_base::timeout::suggested(
            beast::role_type::client));
        
        // 执行WebSocket握手
        ws.async_handshake(host, path,
            [this](beast::error_code ec) {
                onHandshake(ec);
            });
    }
    
    // 握手完成回调
    void onHandshake(beast::error_code ec) {
        connecting = false;
        
        if (ec) {
            onError("握手失败: " + ec.message());
            return;
        }
        
        connected = true;
        
        // 调用连接成功回调
        if (connect_callback) {
            connect_callback(true);
        }
        
        // 开始读取消息
        doRead();
        
        // 开始发送队列中的消息
        doSend();
    }
    
    // 读取消息
    void doRead() {
        if (!connected) {
            return;
        }
        
        ws.async_read(buffer,
            [this](beast::error_code ec, std::size_t bytes_transferred) {
                onRead(ec, bytes_transferred);
            });
    }
    
    // 读取完成回调
    void onRead(beast::error_code ec, std::size_t bytes_transferred) {
        if (ec) {
            if (ec == beast::websocket::error::closed) {
                // 正常关闭
                onClose();
            } else {
                onError("读取失败: " + ec.message());
            }
            return;
        }
        
        // 处理接收到的消息
        std::string message = beast::buffers_to_string(buffer.data());
        buffer.consume(bytes_transferred);
        
        if (message_callback) {
            message_callback(message);
        }
        
        // 继续读取
        doRead();
    }
    
    // 发送消息
    void doSend() {
        if (!connected || sending) {
            return;
        }
        
        std::unique_lock<std::mutex> lock(queue_mutex);
        if (send_queue.empty()) {
            return;
        }
        
        sending = true;
        std::string message = std::move(send_queue.front());
        send_queue.pop();
        lock.unlock();
        
        ws.async_write(asio::buffer(message),
            [this](beast::error_code ec, std::size_t bytes_transferred) {
                onWrite(ec, bytes_transferred);
            });
    }
    
    // 写入完成回调
    void onWrite(beast::error_code ec, std::size_t bytes_transferred) {
        sending = false;
        
        if (ec) {
            onError("写入失败: " + ec.message());
            return;
        }
        
        // 继续发送队列中的消息
        doSend();
    }
    
    // 错误处理
    void onError(const std::string& error) {
        connecting = false;
        connected = false;
        
        std::cerr << "WebSocket错误: " << error << std::endl;
        
        if (error_callback) {
            error_callback(error);
        }
        
        // 自动重连
        if (auto_reconnect && !stopping) {
            std::this_thread::sleep_for(std::chrono::milliseconds(reconnect_interval_ms));
            asio::post(ioc, [this]() { doConnect(); });
        }
    }
    
    // 连接关闭
    void onClose() {
        connected = false;
        
        if (close_callback) {
            close_callback();
        }
        
        // 自动重连
        if (auto_reconnect && !stopping) {
            std::this_thread::sleep_for(std::chrono::milliseconds(reconnect_interval_ms));
            asio::post(ioc, [this]() { doConnect(); });
        }
    }
    
private:
    beast::flat_buffer buffer;
};

// WebSocketClient 实现
WebSocketClient::WebSocketClient(const std::string& host, const std::string& port, const std::string& path)
    : impl_(std::make_unique<Impl>(host, port, path))
{
    impl_->start();
}

WebSocketClient::~WebSocketClient() {
    disconnect();
}

void WebSocketClient::setMessageCallback(MessageCallback callback) {
    impl_->message_callback = callback;
}

void WebSocketClient::setErrorCallback(ErrorCallback callback) {
    impl_->error_callback = callback;
}

void WebSocketClient::setConnectCallback(ConnectCallback callback) {
    impl_->connect_callback = callback;
}

void WebSocketClient::setCloseCallback(CloseCallback callback) {
    impl_->close_callback = callback;
}

bool WebSocketClient::connect(int timeout_ms) {
    if (impl_->connecting || impl_->connected) {
        return false;
    }
    
    impl_->doConnect();
    return true;
}

void WebSocketClient::disconnect() {
    if (!impl_->connected && !impl_->connecting) {
        return;
    }
    
    impl_->stopping = true;
    
    asio::post(impl_->ioc, [this]() {
        beast::error_code ec;
        impl_->ws.close(beast::websocket::close_code::normal, ec);
        impl_->connected = false;
        impl_->connecting = false;
    });
}

bool WebSocketClient::isConnected() const {
    return impl_->connected;
}

bool WebSocketClient::send(const std::string& message) {
    if (!impl_->connected) {
        return false;
    }
    
    {
        std::lock_guard<std::mutex> lock(impl_->queue_mutex);
        impl_->send_queue.push(message);
    }
    
    impl_->queue_cv.notify_one();
    asio::post(impl_->ioc, [this]() { impl_->doSend(); });
    
    return true;
}

bool WebSocketClient::sendBinary(const void* data, size_t size) {
    if (!impl_->connected) {
        return false;
    }
    
    std::string message(static_cast<const char*>(data), size);
    return send(message);
}

void WebSocketClient::setAutoReconnect(bool enable, int interval_ms) {
    impl_->auto_reconnect = enable;
    impl_->reconnect_interval_ms = interval_ms;
}

void WebSocketClient::setHeartbeatInterval(int interval_ms) {
    impl_->heartbeat_interval_ms = interval_ms;
}

std::string WebSocketClient::getHost() const {
    return impl_->host;
}

std::string WebSocketClient::getPort() const {
    return impl_->port;
}

std::string WebSocketClient::getPath() const {
    return impl_->path;
}

} // namespace net
} // namespace pmc