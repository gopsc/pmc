#include "net/WebSocketClientSSL.hpp"
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/post.hpp>
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
namespace ssl = asio::ssl;
using tcp = asio::ip::tcp;
using websocket = beast::websocket::stream<beast::ssl_stream<beast::tcp_stream>>;

// 内部实现结构体
struct WebSocketClientSSL::Impl {
    // 配置参数
    std::string host;
    std::string port;
    std::string path;
    
    // SSL 配置
    bool verify_certificate{true};
    std::string cert_file;
    std::string key_file;
    std::string ca_file;
    std::string ciphers;
    std::string sni_hostname;
    long ssl_options{ssl::context::default_workarounds | 
                     ssl::context::no_sslv2 | 
                     ssl::context::no_sslv3};
    
    // Boost 对象
    asio::io_context ioc;
    ssl::context ssl_ctx{ssl::context::tlsv12_client};
    tcp::resolver resolver;
    websocket ws;
    asio::steady_timer heartbeat_timer;
    asio::steady_timer reconnect_timer;
    
    // 回调函数
    MessageCallback message_callback;
    ErrorCallback error_callback;
    ConnectCallback connect_callback;
    CloseCallback close_callback;
    
    // 状态管理
    std::atomic<bool> connected{false};
    std::atomic<bool> connecting{false};
    std::atomic<bool> stopping{false};
    std::atomic<bool> auto_reconnect{false};
    std::atomic<int> reconnect_interval{3000};
    std::atomic<int> heartbeat_interval{30000};
    
    // 消息队列
    struct QueuedMessage {
        std::string data;
        bool is_binary;
    };
    std::queue<QueuedMessage> send_queue;
    std::mutex queue_mutex;
    std::condition_variable queue_cv;
    std::atomic<bool> writing{false};
    
    // 接收缓冲区
    beast::flat_buffer read_buffer;
    
    // 线程
    std::thread io_thread;
    
    Impl(const std::string& h, const std::string& p, const std::string& pt)
        : host(h)
        , port(p)
        , path(pt)
        , resolver(asio::make_strand(ioc))
        , ws(asio::make_strand(ioc), ssl_ctx)
        , heartbeat_timer(asio::make_strand(ioc))
        , reconnect_timer(asio::make_strand(ioc)) {
        
        // 配置 SSL 上下文
        configureSSLContext();
    }
    
    ~Impl() {
        stopping = true;
        if (io_thread.joinable()) {
            ioc.stop();
            io_thread.join();
        }
    }
    
    void configureSSLContext() {
        try {
            // 设置 SSL 选项
            ssl_ctx.set_options(ssl_options);
            
            // 设置密码列表
            if (!ciphers.empty()) {
                SSL_CTX_set_cipher_list(ssl_ctx.native_handle(), ciphers.c_str());
            }
            
            // 加载证书文件
            if (!cert_file.empty()) {
                ssl_ctx.use_certificate_file(cert_file, ssl::context::pem);
            }
            
            // 加载私钥文件
            if (!key_file.empty()) {
                ssl_ctx.use_private_key_file(key_file, ssl::context::pem);
            }
            
            // 加载 CA 证书
            if (!ca_file.empty()) {
                ssl_ctx.load_verify_file(ca_file);
            } else {
                // 使用系统默认的 CA 证书
                ssl_ctx.set_default_verify_paths();
            }
            
            // 设置验证模式
            if (verify_certificate) {
                ssl_ctx.set_verify_mode(ssl::verify_peer | ssl::verify_fail_if_no_peer_cert);
            } else {
                ssl_ctx.set_verify_mode(ssl::verify_none);
            }
            
        } catch (const std::exception& e) {
            std::cerr << "SSL 上下文配置错误: " << e.what() << std::endl;
        }
    }
    
    void setSNI() {
        // 设置 SNI 主机名
        if (!sni_hostname.empty()) {
            if (!SSL_set_tlsext_host_name(ws.next_layer().native_handle(), sni_hostname.c_str())) {
                beast::error_code ec{static_cast<int>(::ERR_get_error()), asio::error::get_ssl_category()};
                throw beast::system_error{ec};
            }
        } else if (!host.empty()) {
            // 使用主机名作为 SNI
            if (!SSL_set_tlsext_host_name(ws.next_layer().native_handle(), host.c_str())) {
                beast::error_code ec{static_cast<int>(::ERR_get_error()), asio::error::get_ssl_category()};
                throw beast::system_error{ec};
            }
        }
    }
};

// 构造函数
WebSocketClientSSL::WebSocketClientSSL(const std::string& host, const std::string& port, const std::string& path)
    : impl_(std::make_unique<Impl>(host, port, path)) {
}

// 析构函数
WebSocketClientSSL::~WebSocketClientSSL() {
    disconnect();
}

// 回调函数设置
void WebSocketClientSSL::setMessageCallback(MessageCallback callback) {
    impl_->message_callback = std::move(callback);
}

void WebSocketClientSSL::setErrorCallback(ErrorCallback callback) {
    impl_->error_callback = std::move(callback);
}

void WebSocketClientSSL::setConnectCallback(ConnectCallback callback) {
    impl_->connect_callback = std::move(callback);
}

void WebSocketClientSSL::setCloseCallback(CloseCallback callback) {
    impl_->close_callback = std::move(callback);
}

// SSL/TLS 配置
void WebSocketClientSSL::setVerifyCertificate(bool verify) {
    impl_->verify_certificate = verify;
    impl_->configureSSLContext();
}

void WebSocketClientSSL::setCertificateFile(const std::string& cert_file) {
    impl_->cert_file = cert_file;
    impl_->configureSSLContext();
}

void WebSocketClientSSL::setPrivateKeyFile(const std::string& key_file) {
    impl_->key_file = key_file;
    impl_->configureSSLContext();
}

void WebSocketClientSSL::setCertificateAuthorityFile(const std::string& ca_file) {
    impl_->ca_file = ca_file;
    impl_->configureSSLContext();
}

void WebSocketClientSSL::setSSLContextOptions(long options) {
    impl_->ssl_options = options;
    impl_->configureSSLContext();
}

void WebSocketClientSSL::setCiphers(const std::string& ciphers) {
    impl_->ciphers = ciphers;
    impl_->configureSSLContext();
}

void WebSocketClientSSL::setSNIHostname(const std::string& sni_hostname) {
    impl_->sni_hostname = sni_hostname;
}

// 连接函数
bool WebSocketClientSSL::connect(int timeout_ms) {
    if (impl_->connecting || impl_->connected) {
        return false;
    }
    
    impl_->connecting = true;
    impl_->stopping = false;
    
    // 启动 IO 线程
    if (!impl_->io_thread.joinable()) {
        impl_->io_thread = std::thread([this]() { 
            try {
                impl_->ioc.run();
            } catch (const std::exception& e) {
                if (impl_->error_callback) {
                    impl_->error_callback(std::string("IO context error: ") + e.what());
                }
            }
        });
    }
    
    // 异步连接
    asio::post(impl_->ioc, [this]() { 
        try {
            impl_->setSNI();
            doConnect();
        } catch (const std::exception& e) {
            handleError("SSL setup failed", beast::error_code{}, e.what());
        }
    });
    
    // 等待连接完成
    auto start = std::chrono::steady_clock::now();
    while (impl_->connecting && !impl_->stopping) {
        if (timeout_ms > 0) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
            if (elapsed > timeout_ms) {
                impl_->connecting = false;
                if (impl_->error_callback) {
                    impl_->error_callback("Connection timeout");
                }
                return false;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return impl_->connected;
}

// 私有连接方法
void WebSocketClientSSL::doConnect() {
    impl_->resolver.async_resolve(
        impl_->host,
        impl_->port,
        [this](beast::error_code ec, tcp::resolver::results_type results) {
            onResolve(ec, std::move(results));
        });
}

void WebSocketClientSSL::onResolve(beast::error_code ec, tcp::resolver::results_type results) {
    if (ec) {
        handleError("Resolve failed", ec);
        return;
    }
    
    // 设置超时
    beast::get_lowest_layer(impl_->ws).expires_after(std::chrono::seconds(30));
    
    // 异步连接
    beast::get_lowest_layer(impl_->ws).async_connect(
        results,
        [this](beast::error_code ec, tcp::resolver::results_type::endpoint_type endpoint) {
            onConnect(ec, endpoint);
        });
}

void WebSocketClientSSL::onConnect(beast::error_code ec, tcp::resolver::results_type::endpoint_type endpoint) {
    if (ec) {
        handleError("Connect failed", ec);
        return;
    }
    
    // SSL 握手
    impl_->ws.next_layer().async_handshake(
        ssl::stream_base::client,
        [this](beast::error_code ec) {
            onSSLHandshake(ec);
        });
}

void WebSocketClientSSL::onSSLHandshake(beast::error_code ec) {
    if (ec) {
        handleError("SSL handshake failed", ec);
        return;
    }
    
    // 关闭 TCP 超时设置
    beast::get_lowest_layer(impl_->ws).expires_never();
    
    // 设置 WebSocket 选项
    impl_->ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
    impl_->ws.read_message_max(10 * 1024 * 1024); // 10MB 限制
    
    // WebSocket 握手
    impl_->ws.async_handshake(
        impl_->host + ":" + impl_->port,
        impl_->path,
        [this](beast::error_code ec) {
            onHandshake(ec);
        });
}

void WebSocketClientSSL::onHandshake(beast::error_code ec) {
    impl_->connecting = false;
    
    if (ec) {
        handleError("WebSocket handshake failed", ec);
        return;
    }
    
    impl_->connected = true;
    
    // 开始读取消息
    doRead();
    
    // 开始心跳
    if (impl_->heartbeat_interval > 0) {
        startHeartbeat();
    }
    
    // 通知连接成功
    if (impl_->connect_callback) {
        impl_->connect_callback(true);
    }
    
    // 检查是否有待发送的消息
    if (hasMessages()) {
        doWrite();
    }
}

// 断开连接
void WebSocketClientSSL::disconnect() {
    impl_->stopping = true;
    impl_->auto_reconnect = false;
    
    if (impl_->connected) {
        asio::post(impl_->ioc, [this]() {
            beast::error_code ec;
            impl_->ws.close(websocket::close_code::normal, ec);
            if (ec && impl_->error_callback) {
                impl_->error_callback("Error closing WebSocket: " + ec.message());
            }
            impl_->connected = false;
            stopHeartbeat();
            
            if (impl_->close_callback) {
                impl_->close_callback();
            }
        });
    }
}

// 发送消息
bool WebSocketClientSSL::send(const std::string& message) {
    return pushMessage(message, false);
}

bool WebSocketClientSSL::sendBinary(const void* data, size_t size) {
    std::string message(static_cast<const char*>(data), size);
    return pushMessage(message, true);
}

// 状态检查
bool WebSocketClientSSL::isConnected() const {
    return impl_->connected;
}

std::string WebSocketClientSSL::getHost() const {
    return impl_->host;
}

std::string WebSocketClientSSL::getPort() const {
    return impl_->port;
}

std::string WebSocketClientSSL::getPath() const {
    return impl_->path;
}

// 自动重连和心跳设置
void WebSocketClientSSL::setAutoReconnect(bool enable, int interval_ms) {
    impl_->auto_reconnect = enable;
    impl_->reconnect_interval = interval_ms;
}

void WebSocketClientSSL::setHeartbeatInterval(int interval_ms) {
    impl_->heartbeat_interval = interval_ms;
    if (impl_->connected && interval_ms > 0) {
        startHeartbeat();
    }
}

// ============ 私有方法实现 ============

void WebSocketClientSSL::doRead() {
    impl_->ws.async_read(
        impl_->read_buffer,
        [this](beast::error_code ec, std::size_t bytes_transferred) {
            onRead(ec, bytes_transferred);
        });
}

void WebSocketClientSSL::onRead(beast::error_code ec, std::size_t bytes_transferred) {
    if (ec) {
        if (ec == websocket::error::closed) {
            // 正常关闭
            impl_->connected = false;
            stopHeartbeat();
            
            if (impl_->close_callback) {
                impl_->close_callback();
            }
            
            // 自动重连
            if (impl_->auto_reconnect && !impl_->stopping) {
                impl_->reconnect_timer.expires_after(std::chrono::milliseconds(impl_->reconnect_interval));
                impl_->reconnect_timer.async_wait(
                    [this](beast::error_code ec) {
                        if (!ec && !impl_->stopping) {
                            doConnect();
                        }
                    });
            }
        } else {
            handleError("Read failed", ec);
        }
        return;
    }
    
    // 处理接收到的消息
    if (impl_->message_callback) {
        std::string message = beast::buffers_to_string(impl_->read_buffer.data());
        impl_->message_callback(message);
    }
    
    // 清空缓冲区并继续读取
    impl_->read_buffer.consume(bytes_transferred);
    doRead();
}

void WebSocketClientSSL::doWrite() {
    if (impl_->writing || !hasMessages()) {
        return;
    }
    
    impl_->writing = true;
    auto message = popMessage();
    
    impl_->ws.async_write(
        asio::buffer(message.first),
        [this, is_binary = message.second](beast::error_code ec, std::size_t bytes_transferred) {
            impl_->writing = false;
            onWrite(ec, bytes_transferred);
            
            // 如果是二进制消息，设置二进制模式
            if (!ec && is_binary) {
                impl_->ws.binary(true);
            } else if (!ec) {
                impl_->ws.text(true);
            }
        });
}

void WebSocketClientSSL::onWrite(beast::error_code ec, std::size_t bytes_transferred) {
    (void)bytes_transferred; // 消除未使用参数警告
    
    if (ec) {
        handleError("Write failed", ec);
        return;
    }
    
    // 继续发送队列中的下一条消息
    if (hasMessages()) {
        doWrite();
    }
}

void WebSocketClientSSL::handleError(const std::string& message, beast::error_code ec, const std::string& extra) {
    std::string error_msg = message;
    if (ec) {
        error_msg += ": " + ec.message();
    }
    if (!extra.empty()) {
        error_msg += " (" + extra + ")";
    }
    
    impl_->connecting = false;
    impl_->connected = false;
    stopHeartbeat();
    
    if (impl_->error_callback) {
        impl_->error_callback(error_msg);
    }
    
    if (impl_->connect_callback) {
        impl_->connect_callback(false);
    }
    
    // 自动重连
    if (impl_->auto_reconnect && !impl_->stopping) {
        impl_->reconnect_timer.expires_after(std::chrono::milliseconds(impl_->reconnect_interval));
        impl_->reconnect_timer.async_wait(
            [this](beast::error_code ec) {
                if (!ec && !impl_->stopping) {
                    doConnect();
                }
            });
    }
}

void WebSocketClientSSL::startHeartbeat() {
    if (impl_->heartbeat_interval <= 0) {
        return;
    }
    
    impl_->heartbeat_timer.expires_after(std::chrono::milliseconds(impl_->heartbeat_interval));
    impl_->heartbeat_timer.async_wait(
        [this](beast::error_code ec) {
            heartbeatTimerCallback(ec);
        });
}

void WebSocketClientSSL::stopHeartbeat() {
    beast::error_code ec;
    impl_->heartbeat_timer.cancel(ec);
}

void WebSocketClientSSL::heartbeatTimerCallback(beast::error_code ec) {
    if (ec || impl_->stopping || !impl_->connected) {
        return;
    }
    
    // 发送 ping 消息
    beast::error_code ping_ec;
    impl_->ws.ping(beast::websocket::ping_data{}, ping_ec);
    
    if (ping_ec) {
        handleError("Heartbeat ping failed", ping_ec);
        return;
    }
    
    // 重新设置定时器
    startHeartbeat();
}

// 线程安全队列操作
bool WebSocketClientSSL::pushMessage(const std::string& message, bool is_binary) {
    if (!impl_->connected || impl_->stopping) {
        return false;
    }
    
    {
        std::lock_guard<std::mutex> lock(impl_->queue_mutex);
        impl_->send_queue.push({message, is_binary});
    }
    
    impl_->queue_cv.notify_one();
    
    // 触发写入
    asio::post(impl_->ioc, [this]() {
        if (!impl_->writing && hasMessages()) {
            doWrite();
        }
    });
    
    return true;
}

std::pair<std::string, bool> WebSocketClientSSL::popMessage() {
    std::lock_guard<std::mutex> lock(impl_->queue_mutex);
    if (impl_->send_queue.empty()) {
        return {"", false};
    }
    
    auto msg = std::move(impl_->send_queue.front());
    impl_->send_queue.pop();
    return {std::move(msg.data), msg.is_binary};
}

bool WebSocketClientSSL::hasMessages() const {
    std::lock_guard<std::mutex> lock(impl_->queue_mutex);
    return !impl_->send_queue.empty();
}

} // namespace net
} // namespace pmc