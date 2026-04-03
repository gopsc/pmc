#include "net/HttpServer.hpp"
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <sstream>

namespace pmc {
namespace net {

// Session类 - 处理单个HTTP连接
class HttpServer::Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket&& socket, HttpServer* server)
        : socket_(std::move(socket)), server_(server) {
    }
    
    void run() {
        doRead();
    }
    
private:
    void doRead() {
        auto self = shared_from_this();
        http::async_read(socket_, buffer_, request_,
            [this, self](beast::error_code ec, std::size_t) {
                if (!ec) {
                    processRequest();
                } else if (ec != http::error::end_of_stream) {
                    // 错误处理
                }
            });
    }
    
    void processRequest() {
        try {
            // 创建响应对象
            auto response = std::make_shared<http::response<http::string_body>>(
                server_->handleRequest(request_)
            );
            doWrite(response);
        } catch (const std::exception& e) {
            std::cerr << "Error processing request: " << e.what() << std::endl;
            // 发送错误响应
            auto error_response = std::make_shared<http::response<http::string_body>>();
            error_response->version(request_.version());
            error_response->result(http::status::internal_server_error);
            error_response->set(http::field::content_type, "text/plain");
            error_response->body() = "500 Internal Server Error\n";
            error_response->prepare_payload();
            doWrite(error_response);
        }
    }
    
    void doWrite(std::shared_ptr<http::response<http::string_body>> response) {
        auto self = shared_from_this();
        http::async_write(socket_, *response,
            [this, self, response](beast::error_code ec, std::size_t) {
                if (!ec) {
                    // 检查是否需要保持连接
                    if (!response->keep_alive()) {
                        socket_.shutdown(tcp::socket::shutdown_send, ec);
                    }
                }
                // 继续读取下一个请求
                doRead();
            });
    }
    
    tcp::socket socket_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> request_;
    HttpServer* server_;
};

// Listener类 - 接受新连接
class HttpServer::Listener : public std::enable_shared_from_this<Listener> {
public:
    Listener(asio::io_context& ioc, tcp::endpoint endpoint, HttpServer* server)
        : ioc_(ioc), acceptor_(ioc), server_(server) {
        beast::error_code ec;
        acceptor_.open(endpoint.protocol(), ec);
        if (!ec) {
            acceptor_.set_option(asio::socket_base::reuse_address(true), ec);
            acceptor_.bind(endpoint, ec);
            if (!ec) {
                acceptor_.listen(asio::socket_base::max_listen_connections, ec);
            }
        }
        if (ec) {
            std::cerr << "Listener error: " << ec.message() << std::endl;
        }
    }
    
    void run() {
        if (acceptor_.is_open()) {
            doAccept();
        }
    }
    
private:
    void doAccept() {
        acceptor_.async_accept(
            [this, self = shared_from_this()](beast::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<Session>(std::move(socket), server_)->run();
                }
                doAccept();
            });
    }
    
    asio::io_context& ioc_;
    tcp::acceptor acceptor_;
    HttpServer* server_;
};

// HttpServer实现
HttpServer::HttpServer(unsigned short port, unsigned int threads)
    : port_(port), threads_(threads), 
      ioc_(std::make_unique<asio::io_context>(threads)) {
    if (!ioc_) {
        throw std::runtime_error("Failed to create io_context");
    }
}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    if (running_) {
        return;
    }
    
    try {
        // 创建监听器
        tcp::endpoint endpoint(tcp::v4(), port_);
        listener_ = std::make_shared<Listener>(*ioc_, endpoint, this);
        if (!listener_) {
            throw std::runtime_error("Failed to create listener");
        }
        listener_->run();
        
        running_ = true;
        
        // 启动工作线程
        for (unsigned int i = 0; i < threads_; ++i) {
            worker_threads_.emplace_back([this]() {
                try {
                    ioc_->run();
                } catch (const std::exception& e) {
                    std::cerr << "Thread exception: " << e.what() << std::endl;
                }
            });
        }
        
        std::cout << "HTTP Server started on port " << port_ << " with " << threads_ << " threads" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to start server: " << e.what() << std::endl;
        running_ = false;
    }
}

void HttpServer::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    if (ioc_) {
        ioc_->stop();
    }
    
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    worker_threads_.clear();
    
    std::cout << "HTTP Server stopped" << std::endl;
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
    start();
    if (ioc_) {
        ioc_->run();
    }
}

void HttpServer::showStatus() const {
    std::cout << "=== HTTP Server Status ===" << std::endl;
    std::cout << "Port: " << port_ << std::endl;
    std::cout << "Threads: " << threads_ << std::endl;
    std::cout << "Running: " << (running_ ? "Yes" : "No") << std::endl;
    std::cout << "Static Directory: " << (static_directory_.empty() ? "Not set" : static_directory_) << std::endl;
    std::cout << "Registered Routes:" << std::endl;
    std::cout << "  GET: " << get_handlers_.size() << " handler(s)" << std::endl;
    for (const auto& [path, _] : get_handlers_) {
        std::cout << "    GET " << path << std::endl;
    }
    std::cout << "  POST: " << post_handlers_.size() << " handler(s)" << std::endl;
    for (const auto& [path, _] : post_handlers_) {
        std::cout << "    POST " << path << std::endl;
    }
    std::cout << "  PUT: " << put_handlers_.size() << " handler(s)" << std::endl;
    for (const auto& [path, _] : put_handlers_) {
        std::cout << "    PUT " << path << std::endl;
    }
    std::cout << "  DELETE: " << delete_handlers_.size() << " handler(s)" << std::endl;
    for (const auto& [path, _] : delete_handlers_) {
        std::cout << "    DELETE " << path << std::endl;
    }
    std::cout << "Middlewares: " << middlewares_.size() << std::endl;
    std::cout << "==========================" << std::endl;
}

std::unordered_map<std::string, std::string> HttpServer::parseQueryParams(const std::string& query) {
    std::unordered_map<std::string, std::string> params;
    
    if (query.empty()) {
        return params;
    }
    
    std::vector<std::string> pairs;
    boost::split(pairs, query, boost::is_any_of("&"));
    
    for (const auto& pair : pairs) {
        std::vector<std::string> key_value;
        boost::split(key_value, pair, boost::is_any_of("="));
        
        if (key_value.size() == 2) {
            params[key_value[0]] = key_value[1];
        } else if (key_value.size() == 1) {
            params[key_value[0]] = "";
        }
    }
    
    return params;
}

http::response<http::string_body> HttpServer::handleRequest(const http::request<http::string_body>& req) {
    // 将 target() 转换为字符串
    std::string target_path = std::string(req.target().data(), req.target().size());
    std::string query_string;
    
    // 分离路径和查询参数
    auto query_pos = target_path.find('?');
    if (query_pos != std::string::npos) {
        query_string = target_path.substr(query_pos + 1);
        target_path = target_path.substr(0, query_pos);
    }
    
    // 解析查询参数
    auto query_params = parseQueryParams(query_string);
    
    // 准备响应对象
    http::response<http::string_body> response;
    response.version(req.version());
    response.set(http::field::server, "Boost.Beast HttpServer");
    response.set(http::field::content_type, "text/plain");
    response.keep_alive(req.keep_alive());
    
    // 执行中间件链
    bool middleware_handled = false;
    for (const auto& middleware : middlewares_) {
        if (!middleware(req, response, query_params)) {
            middleware_handled = true;
            break;
        }
    }
    
    // 如果中间件处理了请求，直接返回
    if (middleware_handled) {
        response.prepare_payload();
        return response;
    }
    
    // 查找路由处理器
    HttpRequestHandler handler = nullptr;
    
    switch (req.method()) {
        case http::verb::get: {
            auto it = get_handlers_.find(target_path);
            if (it != get_handlers_.end()) {
                handler = it->second;
            }
            break;
        }
        case http::verb::post: {
            auto it = post_handlers_.find(target_path);
            if (it != post_handlers_.end()) {
                handler = it->second;
            }
            break;
        }
        case http::verb::put: {
            auto it = put_handlers_.find(target_path);
            if (it != put_handlers_.end()) {
                handler = it->second;
            }
            break;
        }
        case http::verb::delete_: {
            auto it = delete_handlers_.find(target_path);
            if (it != delete_handlers_.end()) {
                handler = it->second;
            }
            break;
        }
        default:
            break;
    }
    
    // 处理静态文件
    if (!handler && !static_directory_.empty()) {
        std::string file_path = static_directory_ + target_path;
        std::ifstream file(file_path, std::ios::binary);
        
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            response.result(http::status::ok);
            response.body() = buffer.str();
            
            // 根据文件扩展名设置Content-Type
            if (boost::algorithm::ends_with(file_path, ".html")) {
                response.set(http::field::content_type, "text/html");
            } else if (boost::algorithm::ends_with(file_path, ".css")) {
                response.set(http::field::content_type, "text/css");
            } else if (boost::algorithm::ends_with(file_path, ".js")) {
                response.set(http::field::content_type, "application/javascript");
            } else if (boost::algorithm::ends_with(file_path, ".json")) {
                response.set(http::field::content_type, "application/json");
            } else if (boost::algorithm::ends_with(file_path, ".png")) {
                response.set(http::field::content_type, "image/png");
            } else if (boost::algorithm::ends_with(file_path, ".jpg") || 
                       boost::algorithm::ends_with(file_path, ".jpeg")) {
                response.set(http::field::content_type, "image/jpeg");
            }
            
            response.prepare_payload();
            return response;
        }
    }
    
    // 如果找到处理器，调用它
    if (handler) {
        try {
            return handler(req, query_params);
        } catch (const std::exception& e) {
            std::cerr << "Handler exception: " << e.what() << std::endl;
            response.result(http::status::internal_server_error);
            response.body() = "500 Internal Server Error\n";
            response.prepare_payload();
            return response;
        }
    }
    
    // 404 Not Found
    response.result(http::status::not_found);
    response.body() = "404 Not Found\n";
    response.prepare_payload();
    return response;
}

} // namespace net
} // namespace pmc