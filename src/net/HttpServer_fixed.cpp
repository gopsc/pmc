#include "net/HttpServer.hpp"
#include <fstream>
#include <sstream>
#include <regex>
#include <filesystem>

namespace pmc {
namespace net {

// 辅助函数：获取文件内容类型
static std::string getContentType(const std::string& path) {
    if (path.ends_with(".html") || path.ends_with(".htm")) return "text/html";
    if (path.ends_with(".css")) return "text/css";
    if (path.ends_with(".js")) return "application/javascript";
    if (path.ends_with(".json")) return "application/json";
    if (path.ends_with(".png")) return "image/png";
    if (path.ends_with(".jpg") || path.ends_with(".jpeg")) return "image/jpeg";
    if (path.ends_with(".gif")) return "image/gif";
    if (path.ends_with(".svg")) return "image/svg+xml";
    if (path.ends_with(".txt")) return "text/plain";
    if (path.ends_with(".xml")) return "application/xml";
    return "application/octet-stream";
}

HttpServer::HttpServer(unsigned short port, unsigned int threads)
    : port_(port), threads_(threads), ioc_(std::make_unique<net::io_context>()) {
}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    if (running_) return;
    
    running_ = true;
    
    // 创建监听器
    tcp::endpoint endpoint(tcp::v4(), port_);
    listener_ = std::make_shared<Listener>(*ioc_, endpoint, this);
    listener_->run();
    
    // 创建工作线程
    for (unsigned int i = 0; i < threads_; ++i) {
        worker_threads_.emplace_back([this]() {
            ioc_->run();
        });
    }
    
    std::cout << "HttpServer started on port " << port_ << " with " << threads_ << " threads" << std::endl;
}

void HttpServer::stop() {
    if (!running_) return;
    
    running_ = false;
    
    // 停止io_context
    ioc_->stop();
    
    // 等待所有工作线程结束
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    worker_threads_.clear();
    listener_.reset();
    
    std::cout << "HttpServer stopped" << std::endl;
}

void HttpServer::get(const std::string& path, HttpRequestHandler handler) {
    get_handlers_[path] = handler;
}

void HttpServer::post(const std::string& path, HttpRequestHandler handler) {
    post_handlers_[path] = handler;
}

void HttpServer::put(const std::string& path, HttpRequestHandler handler) {
    put_handlers_[path] = handler;
}

void HttpServer::del(const std::string& path, HttpRequestHandler handler) {
    delete_handlers_[path] = handler;
}

void HttpServer::use(MiddlewareHandler middleware) {
    middlewares_.push_back(middleware);
}

void HttpServer::setStaticDirectory(const std::string& path) {
    static_directory_ = path;
}

bool HttpServer::isRunning() const {
    return running_;
}

unsigned short HttpServer::getPort() const {
    return port_;
}

void HttpServer::run() {
    ioc_->run();
}

http::response<http::string_body> HttpServer::handleRequest(const http::request<http::string_body>& req) {
    // 查找对应的处理器
    std::string path = std::string(req.target());
    
    switch (req.method()) {
        case http::verb::get: {
            auto it = get_handlers_.find(path);
            if (it != get_handlers_.end()) {
                return it->second(req, parseQueryParams(std::string(req.target())));
            }
            break;
        }
        case http::verb::post: {
            auto it = post_handlers_.find(path);
            if (it != post_handlers_.end()) {
                return it->second(req, parseQueryParams(std::string(req.target())));
            }
            break;
        }
        case http::verb::put: {
            auto it = put_handlers_.find(path);
            if (it != put_handlers_.end()) {
                return it->second(req, parseQueryParams(std::string(req.target())));
            }
            break;
        }
        case http::verb::delete_: {
            auto it = delete_handlers_.find(path);
            if (it != delete_handlers_.end()) {
                return it->second(req, parseQueryParams(std::string(req.target())));
            }
            break;
        }
        default:
            break;
    }
    
    // 返回404响应
    http::response<http::string_body> res{http::status::not_found, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/plain");
    res.body() = "404 Not Found";
    res.prepare_payload();
    return res;
}

std::unordered_map<std::string, std::string> HttpServer::parseQueryParams(const std::string& query) {
    std::unordered_map<std::string, std::string> params;
    
    size_t pos = query.find('?');
    if (pos == std::string::npos) {
        return params;
    }
    
    std::string query_str = query.substr(pos + 1);
    std::regex pattern("([^&=]+)=([^&]*)");
    std::sregex_iterator it(query_str.begin(), query_str.end(), pattern);
    std::sregex_iterator end;
    
    for (; it != end; ++it) {
        params[(*it)[1]] = (*it)[2];
    }
    
    return params;
}

// Session类实现
HttpServer::Session::Session(tcp::socket socket, HttpServer* server)
    : socket_(std::move(socket)), server_(server) {
}

void HttpServer::Session::start() {
    doRead();
}

void HttpServer::Session::doRead() {
    req_ = {};
    buffer_.clear();
    
    http::async_read(socket_, buffer_, req_,
        [self = shared_from_this()](beast::error_code ec, std::size_t bytes_transferred) {
            self->onRead(ec, bytes_transferred);
        });
}

void HttpServer::Session::onRead(beast::error_code ec, std::size_t bytes_transferred) {
    if (ec == http::error::end_of_stream) {
        socket_.shutdown(tcp::socket::shutdown_send, ec);
        return;
    }
    
    if (ec) {
        std::cerr << "Read error: " << ec.message() << std::endl;
        return;
    }
    
    processRequest();
}

void HttpServer::Session::processRequest() {
    res_ = {};
    
    // 执行中间件链
    bool continueProcessing = true;
    for (auto& middleware : server_->middlewares_) {
        if (!middleware(req_, res_, {})) {
            continueProcessing = false;
            break;
        }
    }
    
    if (!continueProcessing) {
        doWrite();
        return;
    }
    
    // 处理静态文件请求
    if (!server_->static_directory_.empty() && req_.method() == http::verb::get) {
        std::string path = std::string(req_.target());
        if (path.find("..") == std::string::npos && path.length() > 1) {
            std::string filePath = server_->static_directory_ + path;
            std::ifstream file(filePath, std::ios::binary);
            if (file) {
                std::stringstream buffer;
                buffer << file.rdbuf();
                res_.result(http::status::ok);
                res_.set(http::field::content_type, getContentType(path));
                res_.body() = buffer.str();
                res_.prepare_payload();
                doWrite();
                return;
            }
        }
    }
    
    // 查找路由处理器
    std::string method = std::string(req_.method_string());
    std::string target = std::string(req_.target());
    
    // 根据方法类型查找对应的处理器
    http::response<http::string_body> response;
    
    switch (req_.method()) {
        case http::verb::get: {
            auto it = server_->get_handlers_.find(target);
            if (it != server_->get_handlers_.end()) {
                response = it->second(req_, server_->parseQueryParams(target));
            }
            break;
        }
        case http::verb::post: {
            auto it = server_->post_handlers_.find(target);
            if (it != server_->post_handlers_.end()) {
                response = it->second(req_, server_->parseQueryParams(target));
            }
            break;
        }
        case http::verb::put: {
            auto it = server_->put_handlers_.find(target);
            if (it != server_->put_handlers_.end()) {
                response = it->second(req_, server_->parseQueryParams(target));
            }
            break;
        }
        case http::verb::delete_: {
            auto it = server_->delete_handlers_.find(target);
            if (it != server_->delete_handlers_.end()) {
                response = it->second(req_, server_->parseQueryParams(target));
            }
            break;
        }
        default:
            break;
    }
    
    if (response.result() == http::status::unknown) {
        // 没有找到处理器，返回404
        res_.result(http::status::not_found);
        res_.set(http::field::content_type, "text/plain");
        res_.body() = "404 Not Found";
        res_.prepare_payload();
    } else {
        res_ = std::move(response);
    }
    
    doWrite();
}

void HttpServer::Session::doWrite() {
    res_.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    
    http::async_write(socket_, res_,
        [self = shared_from_this()](beast::error_code ec, std::size_t bytes_transferred) {
            self->onWrite(ec, bytes_transferred);
        });
}

void HttpServer::Session::onWrite(beast::error_code ec, std::size_t bytes_transferred) {
    if (ec) {
        std::cerr << "Write error: " << ec.message() << std::endl;
        return;
    }
    
    // 检查是否需要关闭连接
    bool close = res_.need_eof();
    
    if (close) {
        socket_.shutdown(tcp::socket::shutdown_send, ec);
        return;
    }
    
    // 继续读取下一个请求
    doRead();
}

void HttpServer::Session::close() {
    beast::error_code ec;
    socket_.shutdown(tcp::socket::shutdown_send, ec);
}

// Listener类实现
HttpServer::Listener::Listener(net::io_context& ioc, tcp::endpoint endpoint, HttpServer* server)
    : ioc_(ioc), acceptor_(ioc), server_(server) {
    beast::error_code ec;
    
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        std::cerr << "Open error: " << ec.message() << std::endl;
        return;
    }
    
    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
        std::cerr << "Set option error: " << ec.message() << std::endl;
        return;
    }
    
    acceptor_.bind(endpoint, ec);
    if (ec) {
        std::cerr << "Bind error: " << ec.message() << std::endl;
        return;
    }
    
    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
        std::cerr << "Listen error: " << ec.message() << std::endl;
        return;
    }
}

void HttpServer::Listener::run() {
    doAccept();
}

void HttpServer::Listener::doAccept() {
    acceptor_.async_accept(
        net::make_strand(ioc_),
        [self = shared_from_this()](beast::error_code ec, tcp::socket socket) {
            self->onAccept(ec, std::move(socket));
        });
}

void HttpServer::Listener::onAccept(beast::error_code ec, tcp::socket socket) {
    if (ec) {
        std::cerr << "Accept error: " << ec.message() << std::endl;
        return;
    }
    
    // 创建新的Session处理连接
    std::make_shared<Session>(std::move(socket), server_)->start();
    
    // 继续接受下一个连接
    doAccept();
}

} // namespace net
} // namespace pmc