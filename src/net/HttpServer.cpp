#include "net/HttpServer.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <thread>
#include <chrono>

namespace pmc {
namespace net {

HttpServer::HttpServer(unsigned short port, unsigned int threads)
    : port_(port), threads_(threads) {
    std::cout << "HttpServer created on port " << port << " with " << threads << " threads" << std::endl;
}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::start() {
    if (running_) {
        std::cout << "HttpServer is already running" << std::endl;
        return;
    }
    
    running_ = true;
    std::cout << "HttpServer started on port " << port_ << std::endl;
    std::cout << "Registered handlers:" << std::endl;
    std::cout << "  GET: " << get_handlers_.size() << " handlers" << std::endl;
    std::cout << "  POST: " << post_handlers_.size() << " handlers" << std::endl;
    std::cout << "  PUT: " << put_handlers_.size() << " handlers" << std::endl;
    std::cout << "  DELETE: " << delete_handlers_.size() << " handlers" << std::endl;
    std::cout << "  Middlewares: " << middlewares_.size() << " middlewares" << std::endl;
    
    if (!static_directory_.empty()) {
        std::cout << "  Static directory: " << static_directory_ << std::endl;
    }
}

void HttpServer::stop() {
    if (!running_) {
        std::cout << "HttpServer is not running" << std::endl;
        return;
    }
    
    running_ = false;
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

void HttpServer::showStatus() const {
    std::cout << "\n=== HttpServer Status ===" << std::endl;
    std::cout << "Port: " << port_ << std::endl;
    std::cout << "Threads: " << threads_ << std::endl;
    std::cout << "Running: " << (running_ ? "Yes" : "No") << std::endl;
    
    std::cout << "\nRegistered handlers:" << std::endl;
    std::cout << "  GET: " << get_handlers_.size() << " handlers" << std::endl;
    if (!get_handlers_.empty()) {
        for (const auto& [path, _] : get_handlers_) {
            std::cout << "    " << path << std::endl;
        }
    }
    
    std::cout << "  POST: " << post_handlers_.size() << " handlers" << std::endl;
    if (!post_handlers_.empty()) {
        for (const auto& [path, _] : post_handlers_) {
            std::cout << "    " << path << std::endl;
        }
    }
    
    std::cout << "  PUT: " << put_handlers_.size() << " handlers" << std::endl;
    if (!put_handlers_.empty()) {
        for (const auto& [path, _] : put_handlers_) {
            std::cout << "    " << path << std::endl;
        }
    }
    
    std::cout << "  DELETE: " << delete_handlers_.size() << " handlers" << std::endl;
    if (!delete_handlers_.empty()) {
        for (const auto& [path, _] : delete_handlers_) {
            std::cout << "    " << path << std::endl;
        }
    }
    
    std::cout << "  Middlewares: " << middlewares_.size() << " middlewares" << std::endl;
    
    if (!static_directory_.empty()) {
        std::cout << "\nStatic directory: " << static_directory_ << std::endl;
    }
    
    std::cout << "=========================\n" << std::endl;
}

void HttpServer::run() {
    start();
    
    std::cout << "\nHttpServer is running in minimal mode" << std::endl;
    std::cout << "This is a stub implementation for testing purposes" << std::endl;
    std::cout << "Press Ctrl+C to stop the server" << std::endl;
    std::cout << "\nServer would be listening on: http://localhost:" << port_ << std::endl;
    
    if (!get_handlers_.empty()) {
        std::cout << "\nAvailable GET endpoints:" << std::endl;
        for (const auto& [path, _] : get_handlers_) {
            std::cout << "  http://localhost:" << port_ << path << std::endl;
        }
    }
    
    if (!post_handlers_.empty()) {
        std::cout << "\nAvailable POST endpoints:" << std::endl;
        for (const auto& [path, _] : post_handlers_) {
            std::cout << "  http://localhost:" << port_ << path << std::endl;
        }
    }
    
    // 简单的运行循环
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

} // namespace net
} // namespace pmc