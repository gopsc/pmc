#ifndef PMC_NET_WEBSOCKET_CLIENT_HPP
#define PMC_NET_WEBSOCKET_CLIENT_HPP

#include "WebSocketClientBase.hpp"

namespace pmc {
namespace net {

/**
 * @brief 基于 Boost.Beast 的异步 WebSocket 客户端（非 SSL 版本）
 */
class WebSocketClient : public WebSocketClientBase {
public:
    /**
     * @brief 构造函数
     * @param host 服务器主机名或 IP 地址
     * @param port 服务器端口号
     * @param path WebSocket 路径（默认 "/"）
     */
    WebSocketClient(const std::string& host, const std::string& port, const std::string& path = "/");
    
    /**
     * @brief 析构函数
     */
    ~WebSocketClient();

    // 实现 WebSocketClientBase 接口
    void setMessageCallback(MessageCallback callback) override;
    void setErrorCallback(ErrorCallback callback) override;
    void setConnectCallback(ConnectCallback callback) override;
    void setCloseCallback(CloseCallback callback) override;

    bool connect(int timeout_ms = 5000) override;
    void disconnect() override;
    bool isConnected() const override;

    bool send(const std::string& message) override;
    bool sendBinary(const void* data, size_t size) override;

    void setAutoReconnect(bool enable, int interval_ms = 3000) override;
    void setHeartbeatInterval(int interval_ms = 30000) override;

    std::string getHost() const override;
    std::string getPort() const override;
    std::string getPath() const override;

    // SSL/TLS 相关（非 SSL 版本返回默认值）
    bool isSecure() const override { return false; }
    void setVerifyCertificate(bool verify) override { /* 忽略 */ }
    void setCertificateFile(const std::string& cert_file) override { /* 忽略 */ }
    void setPrivateKeyFile(const std::string& key_file) override { /* 忽略 */ }
    void setCertificateAuthorityFile(const std::string& ca_file) override { /* 忽略 */ }

private:
    // 前向声明实现类
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace net
} // namespace pmc

#endif // PMC_NET_WEBSOCKET_CLIENT_HPP