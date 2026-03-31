#ifndef PMC_NET_WEBSOCKET_CLIENT_BASE_HPP
#define PMC_NET_WEBSOCKET_CLIENT_BASE_HPP

#include <string>
#include <functional>
#include <memory>

namespace pmc {
namespace net {

/**
 * @brief WebSocket 客户端基类
 * 
 * 定义所有 WebSocket 客户端的公共接口
 */
class WebSocketClientBase {
public:
    // 回调函数类型
    using MessageCallback = std::function<void(const std::string&)>;
    using ErrorCallback = std::function<void(const std::string&)>;
    using ConnectCallback = std::function<void(bool)>;
    using CloseCallback = std::function<void()>;

    virtual ~WebSocketClientBase() = default;

    // 回调函数设置
    virtual void setMessageCallback(MessageCallback callback) = 0;
    virtual void setErrorCallback(ErrorCallback callback) = 0;
    virtual void setConnectCallback(ConnectCallback callback) = 0;
    virtual void setCloseCallback(CloseCallback callback) = 0;

    // 连接管理
    virtual bool connect(int timeout_ms = 5000) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;

    // 消息发送
    virtual bool send(const std::string& message) = 0;
    virtual bool sendBinary(const void* data, size_t size) = 0;

    // 配置选项
    virtual void setAutoReconnect(bool enable, int interval_ms = 3000) = 0;
    virtual void setHeartbeatInterval(int interval_ms = 30000) = 0;

    // 信息获取
    virtual std::string getHost() const = 0;
    virtual std::string getPort() const = 0;
    virtual std::string getPath() const = 0;

    // SSL/TLS 相关
    virtual bool isSecure() const = 0;
    virtual void setVerifyCertificate(bool verify) = 0;
    virtual void setCertificateFile(const std::string& cert_file) = 0;
    virtual void setPrivateKeyFile(const std::string& key_file) = 0;
    virtual void setCertificateAuthorityFile(const std::string& ca_file) = 0;
};

} // namespace net
} // namespace pmc

#endif // PMC_NET_WEBSOCKET_CLIENT_BASE_HPP