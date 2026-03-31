#ifndef PMC_HTTP_CLIENT_HPP
#define PMC_HTTP_CLIENT_HPP

#include <string>
#include <functional>
#include <curl/curl.h>
#include <iostream>

namespace pmc::net {

class HttpClient {
public:
    HttpClient() {
        std::cout << "HttpClient构造函数: 开始初始化" << std::endl;
        curl_ = curl_easy_init();
        if (!curl_) {
            throw std::runtime_error("Failed to initialize CURL");
        }
        std::cout << "HttpClient构造函数: 初始化完成" << std::endl;
    }
    
    ~HttpClient() {
        std::cout << "HttpClient析构函数: 开始清理" << std::endl;
        if (curl_) {
            curl_easy_cleanup(curl_);
        }
        if (headers_) {
            curl_slist_free_all(headers_);
        }
        std::cout << "HttpClient析构函数: 清理完成" << std::endl;
    }
    
    void set_header(const std::string& header) {
        std::cout << "set_header: 添加header: " << header << std::endl;
        headers_ = curl_slist_append(headers_, header.c_str());
    }
    
    // 同步流式请求
    void post_stream_sync(const std::string& url, const std::string& data, std::function<void(const std::string&)> callback) {
        std::cout << "post_stream_sync: 开始发送流式请求到 " << url << std::endl;
        
        if (!curl_) {
            throw std::runtime_error("CURL not initialized");
        }
        
        // 重置CURL句柄
        curl_easy_reset(curl_);
        
        // 设置URL
        curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
        
        // 设置POST请求
        curl_easy_setopt(curl_, CURLOPT_POST, 1L);
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, data.c_str());
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, data.size());
        
        // 设置请求头
        curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers_);
        
        // 设置流式回调函数
        std::string buffer;
        auto data_pair = std::make_pair(&buffer, &callback);
        std::cout << "post_stream_sync: 数据对创建完成" << std::endl;
        
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, stream_write_callback);
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &data_pair);
        
        // 设置超时和连接选项
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, 10L);
        
        // 执行请求
        std::cout << "post_stream_sync: 执行请求" << std::endl;
        CURLcode res = curl_easy_perform(curl_);
        std::cout << "post_stream_sync: 数据对清理完成" << std::endl;
        
        if (res != CURLE_OK) {
            std::cout << "post_stream_sync: CURL错误: " << curl_easy_strerror(res) << std::endl;
            throw std::runtime_error(std::string("CURL error: ") + curl_easy_strerror(res));
        }
        
        std::cout << "post_stream_sync: 请求成功" << std::endl;
    }

private:
    CURL* curl_ = nullptr;
    struct curl_slist* headers_ = nullptr;
    
    static size_t stream_write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
        // userdata指向一个std::pair<std::string*, std::function<void(const std::string&)>*>
        auto* data_pair = static_cast<std::pair<std::string*, std::function<void(const std::string&)>*>*>(userdata);
        if (!data_pair || !data_pair->first || !data_pair->second) {
            std::cout << "stream_write_callback: 无效的用户数据" << std::endl;
            return 0;
        }
        
        std::string& buffer = *(data_pair->first);
        std::function<void(const std::string&)>& callback = *(data_pair->second);
        size_t total_size = size * nmemb;
        
        std::cout << "stream_write_callback: 收到 " << total_size << " 字节数据" << std::endl;
        
        // 将数据追加到缓冲区
        buffer.append(ptr, total_size);
        
        // 打印原始数据（用于调试）
        std::string raw_data(ptr, total_size);
        std::cout << "stream_write_callback: 原始数据: " << raw_data << std::endl;
        
        // 处理流式数据（按行处理Server-Sent Events格式）
        size_t pos = 0;
        size_t newline_pos;
        
        while ((newline_pos = buffer.find('\n', pos)) != std::string::npos) {
            std::string line = buffer.substr(pos, newline_pos - pos);
            pos = newline_pos + 1;
            
            std::cout << "stream_write_callback: 处理行: " << line << std::endl;
            
            // 处理SSE格式的数据（以"data: "开头）
            if (line.rfind("data: ", 0) == 0) {
                std::string json_str = line.substr(6);
                
                if (json_str == "[DONE]") {
                    std::cout << "stream_write_callback: 收到 [DONE]" << std::endl;
                    callback("[[流式传输完成]]");
                    continue;
                }
                
                // 调用回调函数处理JSON数据
                std::cout << "stream_write_callback: 调用回调，数据长度: " << json_str.length() << std::endl;
                callback(json_str);
            } else if (!line.empty()) {
                // 如果不是SSE格式，直接传递给回调（可能是错误消息）
                std::cout << "stream_write_callback: 非SSE数据，直接传递: " << line << std::endl;
                callback(line);
            }
        }
        
        // 移除已处理的数据
        if (pos > 0) {
            buffer.erase(0, pos);
        }
        
        return total_size;
    }
};

} // namespace pmc::net

#endif // PMC_HTTP_CLIENT_HPP