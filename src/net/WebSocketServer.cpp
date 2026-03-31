// WebSocketServer.cpp - WebSocket服务器实现
// 基于Boost.Beast和Boost.Asio

#include "net/WebSocketServer.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <deque>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

namespace pmc::net {

// 内部Session类实现
class WebSocketServer::Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, WebSocketServer* server)
        : ws_(std::move(socket))
        , server_(server) {
        
        // 获取远程地址
        auto endpoint = ws_.next_layer().socket().remote_endpoint();
        remoteAddress_ = endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
    }

    void start() {
        // 生成客户端ID
        if (server_) {
            clientId_ = server_->generateClientId();
        } else {
            // 如果server为空，使用默认ID
            static std::atomic<uint64_t> counter{0};
            clientId_ = "client_" + std::to_string(++counter);
        }
        
        // 异步接受WebSocket握手
        ws_.async_accept(
            beast::bind_front_handler(
                &Session::onAccept,
                shared_from_this()));
    }

    void send(const std::string& message) {
        // 将消息添加到发送队列
        bool write_in_progress = !sendQueue_.empty();
        sendQueue_.push_back(message);
        
        if (!write_in_progress) {
            // 开始异步写入
            doWrite();
        }
    }

    const std::string& getClientId() const { return clientId_; }
    const std::string& getRemoteAddress() const { return remoteAddress_; }

private:
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
    std::deque<std::string> sendQueue_;
    WebSocketServer* server_;
    std::string clientId_;
    std::string remoteAddress_;

    void onAccept(beast::error_code ec) {
        if (ec) {
            std::cerr << "WebSocket accept error: " << ec.message() << std::endl;
            return;
        }
        
        // 添加到服务器客户端列表
        if (server_) {
            server_->addClient(clientId_, shared_from_this());
            
            // 调用连接处理器
            if (server_->connectHandler_) {
                server_->connectHandler_(clientId_, remoteAddress_);
            }
        }
        
        // 开始读取消息
        doRead();
    }

    void doRead() {
        // 异步读取消息
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &Session::onRead,
                shared_from_this()));
    }

    void onRead(beast::error_code ec, std::size_t bytes_transferred) {
        if (ec == websocket::error::closed) {
            // 正常关闭
            if (server_) {
                server_->removeClient(clientId_);
                if (server_->disconnectHandler_) {
                    server_->disconnectHandler_(clientId_);
                }
            }
            return;
        }
        
        if (ec) {
            std::cerr << "WebSocket read error: " << ec.message() << std::endl;
            if (server_) {
                server_->removeClient(clientId_);
                if (server_->disconnectHandler_) {
                    server_->disconnectHandler_(clientId_);
                }
            }
            return;
        }
        
        // 处理接收到的消息
        std::string message = beast::buffers_to_string(buffer_.data());
        buffer_.consume(bytes_transferred);
        
        if (server_) {
            // 查找消息处理器
            for (const auto& handler : server_->messageHandlers_) {
                // 这里可以添加路径匹配逻辑
                handler.second(clientId_, message, [this](const std::string& response) {
                    send(response);
                });
            }
        }
        
        // 继续读取
        doRead();
    }

    void doWrite() {
        if (sendQueue_.empty()) {
            return;
        }
        
        // 获取队列中的第一条消息
        auto message = sendQueue_.front();
        
        // 异步写入消息
        ws_.async_write(
            asio::buffer(message),
            beast::bind_front_handler(
                &Session::onWrite,
                shared_from_this()));
    }

    void onWrite(beast::error_code ec, std::size_t bytes_transferred) {
        if (ec) {
            std::cerr << "WebSocket write error: " << ec.message() << std::endl;
            if (server_) {
                server_->removeClient(clientId_);
                if (server_->disconnectHandler_) {
                    server_->disconnectHandler_(clientId_);
                }
            }
            return;
        }
        
        // 移除已发送的消息
        sendQueue_.pop_front();
        
        // 继续发送队列中的下一条消息
        if (!sendQueue_.empty()) {
            doWrite();
        }
    }
};

// 内部Listener类实现
class WebSocketServer::Listener : public std::enable_shared_from_this<Listener> {
public:
    static std::shared_ptr<Listener> create(asio::io_context& ioc, tcp::endpoint endpoint, WebSocketServer* server) {
        return std::shared_ptr<Listener>(new Listener(ioc, endpoint, server));
    }
    
    void run() {
        if (!acceptor_.is_open()) {
            throw std::runtime_error("Acceptor is not open");
        }
        
        doAccept();
    }

private:
    Listener(asio::io_context& ioc, tcp::endpoint endpoint, WebSocketServer* server)
        : ioc_(ioc)
        , acceptor_(asio::make_strand(ioc))
        , server_(server) {
        
        beast::error_code ec;
        
        // 打开acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            std::cerr << "Failed to open acceptor: " << ec.message() << std::endl;
            throw std::runtime_error("Failed to open acceptor: " + ec.message());
        }
        
        // 设置地址重用
        acceptor_.set_option(asio::socket_base::reuse_address(true), ec);
        if (ec) {
            std::cerr << "Failed to set reuse address: " << ec.message() << std::endl;
            throw std::runtime_error("Failed to set reuse address: " + ec.message());
        }
        
        // 绑定到端点
        acceptor_.bind(endpoint, ec);
        if (ec) {
            std::cerr << "Failed to bind to endpoint: " << ec.message() << std::endl;
            throw std::runtime_error("Failed to bind to endpoint: " + ec.message());
        }
        
        // 开始监听
        acceptor_.listen(asio::socket_base::max_listen_connections, ec);
        if (ec) {
            std::cerr << "Failed to listen: " << ec.message() << std::endl;
            throw std::runtime_error("Failed to listen: " + ec.message());
        }
    }
    
    void doAccept() {
        acceptor_.async_accept(
            asio::make_strand(ioc_),
            beast::bind_front_handler(
                &Listener::onAccept,
                shared_from_this()));
    }
    
    void onAccept(beast::error_code ec, tcp::socket socket) {
        if (ec) {
            std::cerr << "Accept error: " << ec.message() << std::endl;
        } else {
            // 创建新的Session处理连接
            std::make_shared<Session>(std::move(socket), server_)->start();
        }
        
        // 继续接受新的连接
        doAccept();
    }

    asio::io_context& ioc_;
    tcp::acceptor acceptor_;
    WebSocketServer* server_;
};

WebSocketServer::WebSocketServer(unsigned short port, unsigned int threads)
    : port_(port)
    , threads_(threads)
    , impl_(std::make_unique<Impl>()) {}

WebSocketServer::~WebSocketServer() {
    stop();
}

void WebSocketServer::start() {
    if (running_) {
        return;
    }
    
    // 创建监听器
    tcp::endpoint endpoint(tcp::v4(), port_);
    impl_->listener_ = Listener::create(impl_->ioc_, endpoint, this);
    
    // 启动监听器
    impl_->listener_->run();
    
    // 启动工作线程
    for (unsigned int i = 0; i < threads_; ++i) {
        impl_->threads_.emplace_back([this]() {
            impl_->ioc_.run();
        });
    }
    
    running_ = true;
    std::cout << "WebSocketServer started on port " << port_ << " with " << threads_ << " threads" << std::endl;
}

void WebSocketServer::stop() {
    if (!running_) {
        return;
    }
    
    // 停止io_context
    impl_->ioc_.stop();
    
    // 等待所有线程结束
    for (auto& thread : impl_->threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    impl_->threads_.clear();
    running_ = false;
    std::cout << "WebSocketServer stopped" << std::endl;
}

void WebSocketServer::onMessage(const std::string& path, WebSocketMessageHandler handler) {
    messageHandlers_[path] = std::move(handler);
}

void WebSocketServer::onConnect(WebSocketConnectHandler handler) {
    connectHandler_ = std::move(handler);
}

void WebSocketServer::onDisconnect(WebSocketDisconnectHandler handler) {
    disconnectHandler_ = std::move(handler);
}

bool WebSocketServer::sendToClient(const std::string& clientId, const std::string& message) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    auto it = clients_.find(clientId);
    if (it != clients_.end()) {
        it->second->send(message);
        return true;
    }
    return false;
}

size_t WebSocketServer::broadcast(const std::string& message) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    size_t count = 0;
    for (auto& pair : clients_) {
        pair.second->send(message);
        count++;
    }
    return count;
}

void WebSocketServer::disconnectClient(const std::string& clientId) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    auto it = clients_.find(clientId);
    if (it != clients_.end()) {
        // 这里应该关闭WebSocket连接
        // 目前只是从列表中移除
        clients_.erase(it);
    }
}

size_t WebSocketServer::getClientCount() const {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    return clients_.size();
}

std::string WebSocketServer::generateClientId() const {
    static std::atomic<uint64_t> counter{0};
    return "client_" + std::to_string(++counter);
}

void WebSocketServer::addClient(const std::string& clientId, std::shared_ptr<Session> session) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    clients_[clientId] = session;
}

void WebSocketServer::removeClient(const std::string& clientId) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    clients_.erase(clientId);
}

std::shared_ptr<WebSocketServer::Session> WebSocketServer::getClient(const std::string& clientId) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    auto it = clients_.find(clientId);
    if (it != clients_.end()) {
        return it->second;
    }
    return nullptr;
}

} // namespace pmc::net