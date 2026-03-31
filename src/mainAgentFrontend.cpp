#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <memory>
#include <fstream>
#include <filesystem>
#include <regex>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = boost::asio::ip::tcp;

// 获取 MIME 类型
std::string get_mime_type(const std::string& path) {
    if (path.ends_with(".html") || path.ends_with(".htm")) return "text/html";
    if (path.ends_with(".css")) return "text/css";
    if (path.ends_with(".js")) return "application/javascript";
    if (path.ends_with(".json")) return "application/json";
    if (path.ends_with(".png")) return "image/png";
    if (path.ends_with(".jpg") || path.ends_with(".jpeg")) return "image/jpeg";
    if (path.ends_with(".gif")) return "image/gif";
    if (path.ends_with(".svg")) return "image/svg+xml";
    if (path.ends_with(".ico")) return "image/x-icon";
    return "text/plain";
}

// 读取文件内容
std::string read_file(const std::string& path) {
    try {
        std::ifstream file(path, std::ios::binary);
        if (!file) return "";
        
        std::string content;
        file.seekg(0, std::ios::end);
        content.resize(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(&content[0], content.size());
        return content;
    } catch (...) {
        return "";
    }
}

// WebSocket 客户端，用于连接LLM后端
class WebSocketClient : public std::enable_shared_from_this<WebSocketClient> {
public:
    WebSocketClient(net::io_context& ioc, const std::string& host, const std::string& port)
        : resolver_(net::make_strand(ioc))
        , ws_(net::make_strand(ioc))
        , host_(host)
        , port_(port)
        , connected_(false) {}
    
    void connect() {
        resolver_.async_resolve(host_, port_,
            beast::bind_front_handler(&WebSocketClient::on_resolve, shared_from_this()));
    }
    
    void send(const std::string& message) {
        if (!connected_) {
            std::cerr << "Cannot send: not connected to backend" << std::endl;
            return;
        }
        
        // 将消息放入队列
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            send_queue_.push(message);
        }
        
        // 如果当前没有在发送，开始发送
        if (!sending_) {
            send_next();
        }
    }
    
    void set_message_handler(std::function<void(const std::string&)> handler) {
        message_handler_ = handler;
    }
    
    bool is_connected() const { return connected_; }
    
private:
    
    void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
        if (ec) {
            std::cerr << "Resolve error: " << ec.message() << std::endl;
            return;
        }
        
        beast::get_lowest_layer(ws_).async_connect(results,
            beast::bind_front_handler(&WebSocketClient::on_connect, shared_from_this()));
    }
    
    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type) {
        if (ec) {
            std::cerr << "Connect error: " << ec.message() << std::endl;
            return;
        }
        
        ws_.async_handshake(host_, "/",
            beast::bind_front_handler(&WebSocketClient::on_handshake, shared_from_this()));
    }
    
    void on_handshake(beast::error_code ec) {
        if (ec) {
            std::cerr << "Handshake error: " << ec.message() << std::endl;
            return;
        }
        
        std::cout << "Connected to LLM backend at " << host_ << ":" << port_ << std::endl;
        connected_ = true;
        
        // 开始接收消息
        do_read();
        
        // 发送欢迎消息
        send_welcome();
    }
    
    void send_welcome() {
        std::string welcome_msg = R"({"type": "welcome", "message": "Connected to PMC LLM Backend"})";
        send(welcome_msg);
    }
    
    void do_read() {
        ws_.async_read(buffer_,
            beast::bind_front_handler(&WebSocketClient::on_read, shared_from_this()));
    }
    
    void on_read(beast::error_code ec, std::size_t bytes_transferred) {
        if (ec == websocket::error::closed) {
            std::cout << "WebSocket connection closed" << std::endl;
            connected_ = false;
            return;
        }
        
        if (ec) {
            std::cerr << "Read error: " << ec.message() << std::endl;
            connected_ = false;
            return;
        }
        
        std::string message = beast::buffers_to_string(buffer_.data());
        buffer_.consume(buffer_.size());
        
        // 处理接收到的消息
        if (message_handler_) {
            message_handler_(message);
        }
        
        // 继续读取
        do_read();
    }
    
    void send_next() {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        
        if (send_queue_.empty()) {
            sending_ = false;
            return;
        }
        
        sending_ = true;
        std::string message = send_queue_.front();
        send_queue_.pop();
        
        ws_.async_write(net::buffer(message),
            beast::bind_front_handler(&WebSocketClient::on_write, shared_from_this()));
    }
    
    void on_write(beast::error_code ec, std::size_t bytes_transferred) {
        if (ec) {
            std::cerr << "Write error: " << ec.message() << std::endl;
            connected_ = false;
            return;
        }
        
        // 发送下一个消息
        send_next();
    }
    
    tcp::resolver resolver_;
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
    std::string host_;
    std::string port_;
    std::atomic<bool> connected_;
    
    std::queue<std::string> send_queue_;
    std::mutex queue_mutex_;
    std::atomic<bool> sending_{false};
    
    std::function<void(const std::string&)> message_handler_;
};

// HTTP 请求处理器
class HttpSession : public std::enable_shared_from_this<HttpSession> {
public:
    HttpSession(tcp::socket socket, const std::string& static_dir, 
                std::shared_ptr<WebSocketClient> ws_client)
        : socket_(std::move(socket)), static_dir_(static_dir), ws_client_(ws_client) {}
    
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
        
        // 检查是否为 WebSocket 升级请求
        if (websocket::is_upgrade(request_)) {
            handle_websocket_upgrade();
            return;
        }
        
        // 只处理 GET 请求
        if (request_.method() != http::verb::get) {
            response_.result(http::status::bad_request);
            response_.set(http::field::content_type, "text/plain");
            response_.body() = "Invalid method\n";
            write_response();
            return;
        }
        
        // 获取请求路径
        std::string target = request_.target();
        if (target.empty() || target == "/") {
            target = "/llm_chat.html";  // 默认加载聊天界面
        }
        
        // 防止路径遍历攻击
        if (target.find("..") != std::string::npos) {
            response_.result(http::status::forbidden);
            response_.set(http::field::content_type, "text/plain");
            response_.body() = "Forbidden\n";
            write_response();
            return;
        }
        
        // 构建文件路径
        std::string file_path = static_dir_ + target;
        
        // 读取文件
        std::string content = read_file(file_path);
        
        if (content.empty()) {
            // 文件不存在，返回 404
            if (target == "/llm_chat.html") {
                // 如果聊天界面不存在，返回默认页面
                response_.result(http::status::ok);
                response_.set(http::field::content_type, "text/html");
                response_.body() = "<html><body><h1>PMC LLM Chat Interface</h1><p>Chat interface not found. Please check static directory.</p></body></html>";
            } else {
                response_.result(http::status::not_found);
                response_.set(http::field::content_type, "text/plain");
                response_.body() = "File not found: " + target + "\n";
            }
        } else {
            // 文件存在，返回内容
            response_.result(http::status::ok);
            response_.set(http::field::content_type, get_mime_type(file_path));
            response_.body() = content;
        }
        
        write_response();
    }
    
    void handle_websocket_upgrade() {
        try {
            // 创建 WebSocket 会话
            auto ws = std::make_shared<websocket::stream<beast::tcp_stream>>(std::move(socket_));
            
            // 接受 WebSocket 握手
            ws->async_accept(request_,
                [self = shared_from_this(), ws](beast::error_code ec) {
                    if (!ec) {
                        self->handle_websocket_session(ws);
                    }
                });
        } catch (const std::exception& e) {
            std::cerr << "WebSocket upgrade error: " << e.what() << std::endl;
        }
    }
    
    void handle_websocket_session(std::shared_ptr<websocket::stream<beast::tcp_stream>> ws) {
        std::cout << "New WebSocket connection established" << std::endl;
        
        // 发送欢迎消息
        std::string welcome_msg = R"({"type": "welcome", "message": "Welcome to PMC LLM Chat Interface!"})";
        ws->write(net::buffer(welcome_msg));
        
        // 开始读取消息
        do_websocket_read(ws);
    }
    
    void do_websocket_read(std::shared_ptr<websocket::stream<beast::tcp_stream>> ws) {
        auto buffer = std::make_shared<beast::flat_buffer>();
        
        ws->async_read(*buffer,
            [self = shared_from_this(), ws, buffer](beast::error_code ec, std::size_t bytes_transferred) {
                if (ec == websocket::error::closed) {
                    std::cout << "WebSocket connection closed by client" << std::endl;
                    return;
                }
                
                if (ec) {
                    std::cerr << "WebSocket read error: " << ec.message() << std::endl;
                    return;
                }
                
                // 处理接收到的消息
                std::string message = beast::buffers_to_string(buffer->data());
                self->handle_websocket_message(ws, message);
                
                // 继续读取
                self->do_websocket_read(ws);
            });
    }
    
    void handle_websocket_message(std::shared_ptr<websocket::stream<beast::tcp_stream>> ws, 
                                  const std::string& message) {
        try {
            std::cout << "Received WebSocket message: " << message << std::endl;
            
            // 解析 JSON 消息
            // 这里简化处理，实际应该使用 JSON 库
            if (message.find("\"type\":\"hello\"") != std::string::npos || 
                message.find("hello") != std::string::npos) {
                // 发送欢迎消息
                std::string response = R"({"type": "welcome", "message": "Hello! I'm PMC LLM Assistant. How can I help you today?"})";
                ws->write(net::buffer(response));
            } else if (message.find("\"type\":\"message\"") != std::string::npos ||
                      message.find("\"type\":\"chat\"") != std::string::npos) {
                // 聊天消息，转发到 LLM 后端
                if (ws_client_ && ws_client_->is_connected()) {
                    // 发送思考指示
                    std::string thinking_msg = R"({"type": "thinking"})";
                    ws->write(net::buffer(thinking_msg));
                    
                    // 转发到 LLM 后端
                    ws_client_->send(message);
                    
                    // 设置消息处理器，将后端响应转发回客户端
                    ws_client_->set_message_handler([this, ws](const std::string& backend_response) {
                        // 将后端响应转发给 WebSocket 客户端
                        std::string response = R"({"type": "response", "content": ")" + 
                                              escape_json(backend_response) + "\"}";
                        ws->write(net::buffer(response));
                    });
                } else {
                    // LLM 后端未连接，返回模拟响应
                    std::string response = R"({"type": "response", "content": "LLM backend is not connected. This is a simulated response."})";
                    ws->write(net::buffer(response));
                }
            } else if (message.find("\"type\":\"change_model\"") != std::string::npos) {
                // 处理模型切换请求
                std::string response = R"({"type": "system", "message": "Model change request received"})";
                ws->write(net::buffer(response));
            } else {
                // 默认处理：直接回复
                std::string response = R"({"type": "response", "content": "I received your message: )" + 
                                      escape_json(message) + "\"}";
                ws->write(net::buffer(response));
            }
        } catch (const std::exception& e) {
            std::cerr << "Error handling WebSocket message: " << e.what() << std::endl;
            std::string error_msg = R"({"type": "error", "message": "Error processing message"})";
            ws->write(net::buffer(error_msg));
        }
    }
    
    std::string escape_json(const std::string& input) {
        std::string output;
        output.reserve(input.length());
        
        for (char c : input) {
            switch (c) {
                case '"': output += "\\\""; break;
                case '\\': output += "\\\\"; break;
                case '\b': output += "\\b"; break;
                case '\f': output += "\\f"; break;
                case '\n': output += "\\n"; break;
                case '\r': output += "\\r"; break;
                case '\t': output += "\\t"; break;
                default: output += c; break;
            }
        }
        
        return output;
    }
    
    void write_response() {
        auto self = shared_from_this();
        
        response_.content_length(response_.body().size());
        
        http::async_write(socket_, response_,
            [self](beast::error_code ec, std::size_t bytes_transferred) {
                // 连接关闭后自动清理
            });
    }
    
    tcp::socket socket_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> request_;
    http::response<http::string_body> response_;
    std::string static_dir_;
    std::shared_ptr<WebSocketClient> ws_client_;
};

// HTTP 服务器
class HttpServer {
public:
    HttpServer(net::io_context& ioc, tcp::endpoint endpoint, 
               const std::string& static_dir, const std::string& backend_host, 
               const std::string& backend_port)
        : acceptor_(ioc)
        , static_dir_(static_dir)
        , backend_host_(backend_host)
        , backend_port_(backend_port) {
        
        beast::error_code ec;
        
        // 打开 acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            std::cerr << "Open error: " << ec.message() << std::endl;
            return;
        }
        
        // 设置地址重用
        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        if (ec) {
            std::cerr << "Set option error: " << ec.message() << std::endl;
            return;
        }
        
        // 绑定到地址
        acceptor_.bind(endpoint, ec);
        if (ec) {
            std::cerr << "Bind error: " << ec.message() << std::endl;
            return;
        }
        
        // 开始监听
        acceptor_.listen(net::socket_base::max_listen_connections, ec);
        if (ec) {
            std::cerr << "Listen error: " << ec.message() << std::endl;
            return;
        }
        
        std::cout << "HTTP server listening on " << endpoint.address().to_string() 
                  << ":" << endpoint.port() << std::endl;
        
        // 创建 WebSocket 客户端连接到 LLM 后端
        ws_client_ = std::make_shared<WebSocketClient>(ioc, backend_host_, backend_port_);
        
        // 尝试连接到后端
        std::cout << "Connecting to LLM backend at " << backend_host_ << ":" << backend_port_ << std::endl;
        ws_client_->connect();
        
        // 开始接受连接
        do_accept();
    }
    
private:
    void do_accept() {
        acceptor_.async_accept(
            [this](beast::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<HttpSession>(std::move(socket), static_dir_, ws_client_)->start();
                }
                
                // 继续接受新连接
                do_accept();
            });
    }
    
    tcp::acceptor acceptor_;
    std::string static_dir_;
    std::string backend_host_;
    std::string backend_port_;
    std::shared_ptr<WebSocketClient> ws_client_;
};

int main(int argc, char* argv[]) {
    try {
        // 默认配置
        std::string host = "0.0.0.0";
        unsigned short port = 8080;
        std::string static_dir = "./static";
        std::string backend_host = "localhost";
        std::string backend_port = "8081";
        
        // 解析命令行参数
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--host" && i + 1 < argc) {
                host = argv[++i];
            } else if (arg == "--port" && i + 1 < argc) {
                port = static_cast<unsigned short>(std::stoi(argv[++i]));
            } else if (arg == "--static" && i + 1 < argc) {
                static_dir = argv[++i];
            } else if (arg == "--backend-host" && i + 1 < argc) {
                backend_host = argv[++i];
            } else if (arg == "--backend-port" && i + 1 < argc) {
                backend_port = argv[++i];
            } else if (arg == "--help") {
                std::cout << "Usage: " << argv[0] << " [options]\n"
                          << "Options:\n"
                          << "  --host <address>        HTTP server host (default: 0.0.0.0)\n"
                          << "  --port <port>          HTTP server port (default: 8080)\n"
                          << "  --static <dir>         Static files directory (default: ./static)\n"
                          << "  --backend-host <host>  LLM backend host (default: localhost)\n"
                          << "  --backend-port <port>  LLM backend port (default: 8081)\n"
                          << "  --help                 Show this help message\n";
                return 0;
            }
        }
        
        // 检查静态目录
        if (!std::filesystem::exists(static_dir)) {
            std::cout << "Static directory '" << static_dir << "' does not exist. Creating it..." << std::endl;
            std::filesystem::create_directories(static_dir);
            
            // 创建默认的 llm_chat.html 文件
            std::string chat_html_path = static_dir + "/llm_chat.html";
            if (!std::filesystem::exists(chat_html_path)) {
                std::cout << "Creating default chat interface..." << std::endl;
                // 这里可以添加默认的 HTML 内容
            }
        }
        
        std::cout << "PMC LLM Frontend Server\n";
        std::cout << "=======================\n";
        std::cout << "HTTP Server: " << host << ":" << port << "\n";
        std::cout << "Static Directory: " << static_dir << "\n";
        std::cout << "LLM Backend: " << backend_host << ":" << backend_port << "\n";
        std::cout << "==============================\n\n";
        
        // 创建 I/O 上下文
        net::io_context ioc;
        
        // 创建 HTTP 服务器
        tcp::endpoint endpoint(net::ip::make_address(host), port);
        HttpServer server(ioc, endpoint, static_dir, backend_host, backend_port);
        
        // 运行 I/O 上下文
        ioc.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}