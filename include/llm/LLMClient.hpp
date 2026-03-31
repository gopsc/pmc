#ifndef PMC_LLM_CLIENT_HPP
#define PMC_LLM_CLIENT_HPP

#include <string>
#include <vector>
#include <utility>
#include <functional>
#include <memory>

#include <boost/json.hpp>

#include "net/HttpClient.hpp"

namespace pmc::llm {

class LLMClient {
public:
    LLMClient(const std::string& api_key) : api_key_(api_key) {}
    
    // 流式请求（使用同步版本避免线程死锁）
    void stream_request(const std::vector<std::pair<std::string, std::string>>& messages,
                       std::function<void(const std::string&)> callback) {
        // 使用智能指针确保HttpClient在回调期间保持存活
        auto http_client = std::make_shared<pmc::net::HttpClient>();
        
        // 设置请求头
        http_client->set_header("Authorization: Bearer " + api_key_);
        http_client->set_header("Content-Type: application/json");
        http_client->set_header("Accept: text/event-stream");
        
        // 构建请求JSON
        boost::json::object request;
        request["model"] = "deepseek-chat";
        request["stream"] = true;
        
        // 构建消息数组，添加系统消息要求返回Markdown格式
        boost::json::array messages_array;
        
        // 添加系统消息，要求使用Markdown格式回复
        boost::json::object system_message;
        system_message["role"] = "system";
        system_message["content"] = "请使用Markdown格式回复。使用**粗体**表示重要概念，*斜体*表示强调，`代码`表示代码片段，使用列表、标题等Markdown元素来组织内容。";
        messages_array.push_back(system_message);
        
        // 添加用户消息
        for (const auto& msg : messages) {
            boost::json::object message_obj;
            message_obj["role"] = msg.first;
            message_obj["content"] = msg.second;
            messages_array.push_back(message_obj);
        }
        request["messages"] = messages_array;
        
        // 转换为字符串
        std::string request_body = boost::json::serialize(request);
        
        // 创建包装回调，解析JSON并提取文本内容
        auto wrapped_callback = [callback](const std::string& json_str) {
            try {
                // 解析JSON
                boost::json::value json_value = boost::json::parse(json_str);
                
                // 检查是否是有效的响应
                if (json_value.is_object()) {
                    const auto& obj = json_value.as_object();
                    
                    // 检查是否有choices数组
                    if (obj.contains("choices") && obj.at("choices").is_array()) {
                        const auto& choices = obj.at("choices").as_array();
                        if (!choices.empty() && choices[0].is_object()) {
                            const auto& choice = choices[0].as_object();
                            
                            // 检查是否有delta对象
                            if (choice.contains("delta") && choice.at("delta").is_object()) {
                                const auto& delta = choice.at("delta").as_object();
                                
                                // 检查是否有content字段
                                if (delta.contains("content") && delta.at("content").is_string()) {
                                    const std::string& content = delta.at("content").as_string().c_str();
                                    callback(content);
                                    return;
                                }
                            }
                        }
                    }
                }
                
                // 如果JSON解析失败或没有有效内容，发送原始字符串（用于调试）
                callback("[" + json_str + "]");
            } catch (const std::exception& e) {
                // JSON解析失败，发送原始字符串
                callback("[" + json_str + "]");
            }
        };
        
        // 发送POST请求 - 使用post_stream_sync方法
        http_client->post_stream_sync("https://api.deepseek.com/v1/chat/completions", request_body, wrapped_callback);
    }

private:
    std::string api_key_;
};

} // namespace pmc::llm

#endif // PMC_LLM_CLIENT_HPP