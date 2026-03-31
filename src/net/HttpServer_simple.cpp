#include "net/HttpServer.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>

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
    : port_(port), threads_(threads) {
}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    std::cout << "HttpServer started on port " << port_ << std::endl;
    std::cout << "Note: This is a minimal implementation for testing" << std::endl;
}

void HttpServer::stop() {
    std::cout << "HttpServer stopped" << std::endl;
}

void HttpServer::get(const std::string& path, HttpRequestHandler handler) {
    get_handlers_[path] = handler;
    std::cout << "Registered GET handler for path: " << path << std::endl;
}

void HttpServer::post(const std::string& path, HttpRequestHandler handler) {
    post_handlers_[path] = handler;
    std::cout << "Registered POST handler for path: " << path << std::endl;
}

void HttpServer::put(const std::string& path, HttpRequestHandler handler) {
    put_handlers_[path] = handler;
    std::cout << "Registered PUT handler for path: " << path << std::endl;
}

void HttpServer::del(const std::string& path, HttpRequestHandler handler) {
    delete_handlers_[path] = handler;
    std::cout << "Registered DELETE handler for path: " << path << std::endl;
}

void HttpServer::use(MiddlewareHandler middleware) {
    middlewares_.push_back(middleware);
    std::cout << "Registered middleware" << std::endl;
}

void HttpServer::setStaticDirectory(const std::string& path) {
    static_directory_ = path;
    std::cout << "Set static directory to: " << path << std::endl;
}

bool HttpServer::isRunning() const {
    return true;
}

unsigned short HttpServer::getPort() const {
    return port_;
}

void HttpServer::run() {
    std::cout << "HttpServer is running (minimal implementation)" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    
    // 简单的运行循环
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

} // namespace net
} // namespace pmc