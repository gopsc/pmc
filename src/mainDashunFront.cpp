#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <thread>
#include <memory>
#include <regex>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
namespace fs = std::filesystem;

// 获取MIME类型
std::string get_mime_type(const std::string& path) {
    if (path.ends_with(".html") || path.ends_with(".htm")) return "text/html";
    if (path.ends_with(".css")) return "text/css";
    if (path.ends_with(".js")) return "application/javascript";
    if (path.ends_with(".json")) return "application/json";
    if (path.ends_with(".png")) return "image/png";
    if (path.ends_with(".jpg") || path.ends_with(".jpeg")) return "image/jpeg";
    if (path.ends_with(".gif")) return "image/gif";
    if (path.ends_with(".svg")) return "image/svg+xml";
    if (path.ends_with(".txt")) return "text/plain";
    if (path.ends_with(".ico")) return "image/x-icon";
    return "application/octet-stream";
}

// 读取文件内容
std::string read_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "无法打开文件: " << path << std::endl;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// 处理HTTP请求
class http_connection : public std::enable_shared_from_this<http_connection> {
public:
    http_connection(tcp::socket socket, const std::string& static_dir)
        : socket_(std::move(socket)), static_dir_(static_dir) {}
    
    void start() {
        read_request();
    }
    
private:
    void read_request() {
        auto self = shared_from_this();
        
        http::async_read(socket_, buffer_, request_,
            [self](beast::error_code ec, std::size_t bytes_transferred) {
                if (!ec) {
                    self->process_request();
                }
            });
    }
    
    void process_request() {
        response_.version(request_.version());
        response_.keep_alive(false);
        
        std::string target = std::string(request_.target());
        
        // 防止路径遍历攻击
        if (target.find("..") != std::string::npos) {
            response_.result(http::status::forbidden);
            response_.set(http::field::content_type, "text/plain");
            response_.body() = "禁止访问";
            send_response();
            return;
        }
        
        // 处理根路径
        if (target == "/" || target == "/index.html") {
            serve_file("dashun_index.html");
            return;
        }
        
        // 处理健康检查
        if (target == "/health") {
            response_.result(http::status::ok);
            response_.set(http::field::content_type, "text/plain");
            response_.body() = "大顺脚本前端服务运行正常";
            send_response();
            return;
        }
        
        // 处理API请求
        if (target.starts_with("/api/")) {
            handle_api_request(target);
            return;
        }
        
        // 处理静态文件
        serve_file(target.substr(1)); // 移除开头的'/'
    }
    
    void handle_api_request(const std::string& target) {
        response_.result(http::status::ok);
        response_.set(http::field::content_type, "application/json");
        
        if (target == "/api/status") {
            response_.body() = R"({
                "status": "running",
                "version": "1.0.0",
                "uptime": "0",
                "requests_served": 0
            })";
        } else if (target == "/api/info") {
            response_.body() = R"({
                "name": "大顺脚本前端服务",
                "description": "为pmc项目提供Web界面",
                "author": "大顺",
                "repository": "https://github.com/your-repo/pmc"
            })";
        } else {
            response_.result(http::status::not_found);
            response_.set(http::field::content_type, "application/json");
            response_.body() = R"({"error": "API endpoint not found"})";
        }
        
        send_response();
    }
    
    void serve_file(const std::string& filename) {
        std::string filepath = static_dir_ + "/" + filename;
        
        // 安全检查：确保文件在静态目录内
        fs::path requested_path = fs::absolute(filepath);
        fs::path static_path = fs::absolute(static_dir_);
        
        if (!fs::equivalent(requested_path.parent_path(), static_path) &&
            !requested_path.string().starts_with(static_path.string() + "/")) {
            response_.result(http::status::forbidden);
            response_.set(http::field::content_type, "text/plain");
            response_.body() = "禁止访问";
            send_response();
            return;
        }
        
        if (!fs::exists(filepath)) {
            response_.result(http::status::not_found);
            response_.set(http::field::content_type, "text/plain");
            response_.body() = "文件不存在: " + filename;
            send_response();
            return;
        }
        
        std::string content = read_file(filepath);
        if (content.empty()) {
            response_.result(http::status::internal_server_error);
            response_.set(http::field::content_type, "text/plain");
            response_.body() = "读取文件失败";
            send_response();
            return;
        }
        
        response_.result(http::status::ok);
        response_.set(http::field::content_type, get_mime_type(filename));
        response_.body() = content;
        send_response();
    }
    
    void send_response() {
        auto self = shared_from_this();
        
        response_.content_length(response_.body().size());
        
        http::async_write(socket_, response_,
            [self](beast::error_code ec, std::size_t) {
                self->socket_.shutdown(tcp::socket::shutdown_send, ec);
            });
    }
    
    tcp::socket socket_;
    beast::flat_buffer buffer_{8192};
    http::request<http::string_body> request_;
    http::response<http::string_body> response_;
    std::string static_dir_;
};

// HTTP服务器
class http_server {
public:
    http_server(net::io_context& ioc, tcp::endpoint endpoint, const std::string& static_dir)
        : acceptor_(ioc), static_dir_(static_dir) {
        beast::error_code ec;
        
        // 打开acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            throw beast::system_error{ec};
        }
        
        // 设置地址重用
        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        if (ec) {
            throw beast::system_error{ec};
        }
        
        // 绑定到端点
        acceptor_.bind(endpoint, ec);
        if (ec) {
            throw beast::system_error{ec};
        }
        
        // 开始监听
        acceptor_.listen(net::socket_base::max_listen_connections, ec);
        if (ec) {
            throw beast::system_error{ec};
        }
    }
    
    void start_accept() {
        acceptor_.async_accept(
            [this](beast::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<http_connection>(std::move(socket), static_dir_)->start();
                }
                start_accept();
            });
    }
    
private:
    tcp::acceptor acceptor_;
    std::string static_dir_;
};

int main() {
    try {
        // 静态文件目录
        std::string static_dir = "static";
        
        // 确保静态目录存在
        if (!fs::exists(static_dir)) {
            std::cerr << "警告: 静态目录 '" << static_dir << "' 不存在" << std::endl;
            if (!fs::create_directory(static_dir)) {
                std::cerr << "错误: 无法创建静态目录" << std::endl;
                return 1;
            }
        }
        
        // 检查主页文件是否存在
        std::string homepage = static_dir + "/dashun_index.html";
        if (!fs::exists(homepage)) {
            std::cerr << "警告: 主页文件 '" << homepage << "' 不存在" << std::endl;
            std::cerr << "请确保 dashun_index.html 文件在 static/ 目录中" << std::endl;
        }
        
        // 创建IO上下文
        net::io_context ioc{1};
        
        // 创建服务器端点（监听所有地址的8080端口）
        tcp::endpoint endpoint{tcp::v4(), 8080};
        
        // 创建HTTP服务器
        http_server server{ioc, endpoint, static_dir};
        server.start_accept();
        
        // 显示启动信息
        std::cout << "==========================================" << std::endl;
        std::cout << "大顺脚本前端服务已启动" << std::endl;
        std::cout << "监听地址: http://0.0.0.0:8080" << std::endl;
        std::cout << "静态文件目录: " << static_dir << std::endl;
        std::cout << "按 Ctrl+C 停止服务" << std::endl;
        std::cout << "==========================================" << std::endl;
        
        ioc.run();
        
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}