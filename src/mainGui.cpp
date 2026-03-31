#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <fstream>
#include <cstdlib>
#include <regex>

#include <gtk/gtk.h>

#include "llm/LLMClient.hpp"

namespace pmc::gui::example {

// 带缓冲区的Markdown转换器，累积整个响应并处理Markdown
class BufferedMarkdownConverter {
private:
    std::string buffer;  // 累积原始Markdown文本
    std::string last_processed;  // 上次处理的Pango标记
    
public:
    BufferedMarkdownConverter() {}
    
    // 重置状态
    void reset() {
        buffer.clear();
        last_processed.clear();
    }
    
    // 添加新的Markdown块到缓冲区
    void append_chunk(const std::string& chunk) {
        buffer += chunk;
    }
    
    // 获取当前缓冲区的内容
    std::string get_buffer() const {
        return buffer;
    }
    
    // 处理整个缓冲区并返回Pango标记
    std::string process_buffer() {
        std::string result = buffer;
        
        // 1. 首先转义HTML特殊字符
        result = std::regex_replace(result, std::regex("&"), "&amp;");
        result = std::regex_replace(result, std::regex("<"), "&lt;");
        result = std::regex_replace(result, std::regex(">"), "&gt;");
        
        // 2. 转换粗斜体：***text*** -> <b><i>text</i></b>
        // 先处理粗斜体，确保正确嵌套
        std::regex bold_italic_regex("\\*\\*\\*(.*?)\\*\\*\\*");
        result = std::regex_replace(result, bold_italic_regex, "<b><i>$1</i></b>");
        
        // 3. 转换粗体：**text** -> <b>text</b>（但排除已经是粗斜体的部分）
        std::regex bold_regex("\\*\\*(?!\\*)(.*?)\\*\\*");
        result = std::regex_replace(result, bold_regex, "<b>$1</b>");
        
        // 4. 转换斜体：*text* -> <i>text</i>（但排除已经是粗体或粗斜体的部分）
        std::regex italic_regex("\\*(?!\\*)(.*?)\\*");
        result = std::regex_replace(result, italic_regex, "<i>$1</i>");
        
        // 5. 转换内联代码：`code` -> <tt>code</tt> (使用<tt>标签而不是<code>)
        std::regex code_regex("`(.*?)`");
        result = std::regex_replace(result, code_regex, "<tt>$1</tt>");
        
        // 6. 转换标题：# 标题 -> <span size="larger" weight="bold">标题</span>
        std::regex h1_regex("^#\\s+(.*?)$", std::regex::multiline);
        result = std::regex_replace(result, h1_regex, "<span size=\"larger\" weight=\"bold\">$1</span>\n");
        
        std::regex h2_regex("^##\\s+(.*?)$", std::regex::multiline);
        result = std::regex_replace(result, h2_regex, "<span size=\"large\" weight=\"bold\">$1</span>\n");
        
        std::regex h3_regex("^###\\s+(.*?)$", std::regex::multiline);
        result = std::regex_replace(result, h3_regex, "<span size=\"medium\" weight=\"bold\">$1</span>\n");
        
        // 7. 转换无序列表：* item -> • item
        std::regex ul_regex("^\\*\\s+(.*?)$", std::regex::multiline);
        result = std::regex_replace(result, ul_regex, "• $1\n");
        
        // 8. 转换有序列表：1. item -> 1. item
        std::regex ol_regex("^\\d+\\.\\s+(.*?)$", std::regex::multiline);
        // 保持原样，只是确保有换行
        result = std::regex_replace(result, ol_regex, "$&\n");
        
        // 9. 转换引用：> text -> <i>text</i>
        std::regex blockquote_regex("^>\\s+(.*?)$", std::regex::multiline);
        result = std::regex_replace(result, blockquote_regex, "<i>$1</i>\n");
        
        // 10. 转换链接：[text](url) -> text (Pango不支持<a>标签，所以只显示文本)
        std::regex link_regex("\\[(.*?)\\]\\((.*?)\\)");
        result = std::regex_replace(result, link_regex, "$1");
        
        // 11. 转换图片：![alt](url) -> [图片: alt] (Pango不支持图片，所以显示替代文本)
        std::regex image_regex("!\\[(.*?)\\]\\((.*?)\\)");
        result = std::regex_replace(result, image_regex, "[图片: $1]");
        
        // 重要修复：Pango标记语言使用 \n 表示换行，而不是 <br/>
        // 不要转换换行符，保持原样
        
        last_processed = result;
        return result;
    }
    
    // 获取上次处理的结果
    std::string get_last_processed() const {
        return last_processed;
    }
};

/**
 * @brief 简单的聊天窗口类
 */
class SimpleChatWindow {
private:
    GtkTextMark* current_response_start_mark;  // 标记当前响应的开始位置
    
public:
    SimpleChatWindow() : current_response_start_mark(nullptr) {
        // 创建主窗口
        window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
        gtk_window_set_title(window, "PMC Chat");
        gtk_window_set_default_size(window, 800, 600);
        g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), nullptr);
        
        // 创建主垂直布局
        GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_container_add(GTK_CONTAINER(window), vbox);
        
        // 创建滚动窗口和文本视图（用于显示聊天记录）
        GtkWidget* scrolled_window = gtk_scrolled_window_new(nullptr, nullptr);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
        
        text_view = GTK_TEXT_VIEW(gtk_text_view_new());
        gtk_text_view_set_editable(text_view, FALSE);
        gtk_text_view_set_cursor_visible(text_view, FALSE);
        gtk_text_view_set_wrap_mode(text_view, GTK_WRAP_WORD_CHAR);
        gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(text_view));
        
        // 设置文本视图的边距
        gtk_text_view_set_left_margin(text_view, 10);
        gtk_text_view_set_right_margin(text_view, 10);
        gtk_text_view_set_top_margin(text_view, 10);
        gtk_text_view_set_bottom_margin(text_view, 10);
        
        // 创建输入区域的水平布局
        GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        
        // 创建输入框
        entry = GTK_ENTRY(gtk_entry_new());
        gtk_entry_set_placeholder_text(entry, "输入消息...");
        gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(entry), TRUE, TRUE, 0);
        
        // 创建发送按钮
        send_button = GTK_BUTTON(gtk_button_new_with_label("发送"));
        gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(send_button), FALSE, FALSE, 0);
        
        // 连接信号
        g_signal_connect(send_button, "clicked", G_CALLBACK(on_send_clicked), this);
        g_signal_connect(entry, "activate", G_CALLBACK(on_entry_activate), this);
        
        // 初始化LLM客户端
        std::string api_key;
        const char* env_key = std::getenv("DEEPSEEK_API_KEY");
        if (env_key && env_key[0] != '\0') {
            api_key = env_key;
        } else {
            // 尝试从文件读取
            std::string key_file = std::string(std::getenv("HOME")) + "/.cert/deepseek.key";
            std::ifstream file(key_file);
            if (file.is_open()) {
                std::getline(file, api_key);
                file.close();
            } else {
                api_key = ""; // 使用空密钥，会在使用时出错
            }
        }
        
        llm_client = std::make_unique<pmc::llm::LLMClient>(api_key);
        markdown_converter = std::make_unique<BufferedMarkdownConverter>();
    }
    
    // 使用带缓冲区的Markdown转换器进行转换
    std::string markdown_to_pango(const std::string& markdown) {
        // 将新块添加到缓冲区
        markdown_converter->append_chunk(markdown);
        
        // 处理整个缓冲区
        std::string result = markdown_converter->process_buffer();
        
        return result;
    }
    
    void show() {
        gtk_widget_show_all(GTK_WIDGET(window));
    }
    
    void append_message(const std::string& message) {
        GtkTextBuffer* buffer = gtk_text_view_get_buffer(text_view);
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(buffer, &end);
        
        // 用户消息不需要Markdown转换，直接显示
        gtk_text_buffer_insert(buffer, &end, message.c_str(), -1);
        gtk_text_buffer_insert(buffer, &end, "\n", -1);
        
        // 滚动到底部
        GtkAdjustment* adj = gtk_scrolled_window_get_vadjustment(
            GTK_SCROLLED_WINDOW(gtk_widget_get_parent(GTK_WIDGET(text_view))));
        gtk_adjustment_set_value(adj, gtk_adjustment_get_upper(adj));
    }
    
    void append_stream_chunk(const std::string& chunk) {
        GtkTextBuffer* buffer = gtk_text_view_get_buffer(text_view);
        
        // 如果这是响应的第一个块，创建一个标记来跟踪开始位置
        if (current_response_start_mark == nullptr) {
            GtkTextIter end;
            gtk_text_buffer_get_end_iter(buffer, &end);
            
            // 添加助手前缀
            gtk_text_buffer_insert(buffer, &end, "Assistant: ", -1);
            
            // 在助手前缀后创建标记
            gtk_text_buffer_get_end_iter(buffer, &end);
            current_response_start_mark = gtk_text_buffer_create_mark(buffer, "current_response_start", &end, TRUE);
        }
        
        // 获取当前响应的开始位置
        GtkTextIter start_iter;
        gtk_text_buffer_get_iter_at_mark(buffer, &start_iter, current_response_start_mark);
        
        // 获取缓冲区的结束位置
        GtkTextIter end_iter;
        gtk_text_buffer_get_end_iter(buffer, &end_iter);
        
        // 删除当前响应区域（从标记位置到缓冲区末尾）
        gtk_text_buffer_delete(buffer, &start_iter, &end_iter);
        
        // 在处理后的位置插入新的内容
        gtk_text_buffer_get_iter_at_mark(buffer, &start_iter, current_response_start_mark);
        
        // 关键修复：插入整个累积的缓冲区内容，而不仅仅是当前的chunk
        // 首先，将新chunk添加到缓冲区
        markdown_converter->append_chunk(chunk);
        
        // 然后处理整个缓冲区
        std::string processed_buffer = markdown_converter->process_buffer();
        
        // 插入处理后的整个缓冲区内容
        if (!processed_buffer.empty()) {
            gtk_text_buffer_insert_markup(buffer, &start_iter, processed_buffer.c_str(), -1);
        }
        
        // 滚动到底部
        GtkAdjustment* adj = gtk_scrolled_window_get_vadjustment(
            GTK_SCROLLED_WINDOW(gtk_widget_get_parent(GTK_WIDGET(text_view))));
        gtk_adjustment_set_value(adj, gtk_adjustment_get_upper(adj));
        
        // 累积当前响应
        current_response += chunk;
    }
    
    void finish_response() {
        // 将完整的响应添加到聊天历史
        if (!current_response.empty()) {
            chat_history.push_back({"assistant", current_response});
            current_response.clear();
        }
        
        // 重置Markdown转换器
        markdown_converter->reset();
        
        // 清除当前响应标记
        if (current_response_start_mark != nullptr) {
            GtkTextBuffer* buffer = gtk_text_view_get_buffer(text_view);
            gtk_text_buffer_delete_mark(buffer, current_response_start_mark);
            current_response_start_mark = nullptr;
        }
        
        // 添加换行符
        GtkTextBuffer* buffer = gtk_text_view_get_buffer(text_view);
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(buffer, &end);
        gtk_text_buffer_insert(buffer, &end, "\n", -1);
    }
    
    std::string get_input_text() {
        const char* text = gtk_entry_get_text(entry);
        return text ? std::string(text) : "";
    }
    
    void clear_input() {
        gtk_entry_set_text(entry, "");
    }
    
    void send_to_llm(const std::string& user_message) {
        // 将用户消息添加到聊天历史
        chat_history.push_back({"user", user_message});
        
        // 创建流式回调函数
        auto stream_callback = [this](const std::string& chunk) {
            // 关键修复：简化完成标记检查
            // 只检查明确的完成标记，避免误判
            if (chunk == "[[PMC_STREAM_COMPLETE]]" || chunk == "[流式传输完成]]") {
                std::cout << "DEBUG: Detected completion marker" << std::endl;
                // 流式传输完成，在主线程中调用finish_response
                g_idle_add([](gpointer data) -> gboolean {
                    auto* self = static_cast<SimpleChatWindow*>(data);
                    self->finish_response();
                    return FALSE; // 只执行一次
                }, this);
                return; // 重要：直接返回，不显示这个字符串
            }
            
            // 使用g_idle_add在主线程中更新UI
            g_idle_add([](gpointer data) -> gboolean {
                auto* chunk_data = static_cast<std::pair<SimpleChatWindow*, std::string>*>(data);
                chunk_data->first->append_stream_chunk(chunk_data->second);
                delete chunk_data;
                return FALSE; // 只执行一次
            }, new std::pair<SimpleChatWindow*, std::string>(this, chunk));
        };
        
        // 在新线程中发送流式请求（避免阻塞主线程）
        std::thread([this, stream_callback]() {
            try {
                llm_client->stream_request(chat_history, stream_callback);
            } catch (const std::exception& e) {
                // 错误处理：在主线程中显示错误消息
                g_idle_add([](gpointer data) -> gboolean {
                    auto* error_data = static_cast<std::pair<SimpleChatWindow*, std::string>*>(data);
                    error_data->first->append_message("错误: " + error_data->second);
                    delete error_data;
                    return FALSE; // 只执行一次
                }, new std::pair<SimpleChatWindow*, std::string>(this, e.what()));
            }
        }).detach(); // 分离线程，让它独立运行
    }
    
private:
    static void on_send_clicked(GtkButton* button, gpointer user_data) {
        auto* self = static_cast<SimpleChatWindow*>(user_data);
        std::string text = self->get_input_text();
        if (!text.empty()) {
            // 用户消息不需要Markdown转换，直接显示
            self->append_message("You: " + text);
            self->clear_input();
            self->send_to_llm(text);
        }
    }
    
    static void on_entry_activate(GtkEntry* entry, gpointer user_data) {
        auto* self = static_cast<SimpleChatWindow*>(user_data);
        std::string text = self->get_input_text();
        if (!text.empty()) {
            // 用户消息不需要Markdown转换，直接显示
            self->append_message("You: " + text);
            self->clear_input();
            self->send_to_llm(text);
        }
    }

    GtkWindow* window;
    GtkTextView* text_view;
    GtkEntry* entry;
    GtkButton* send_button;
    
    std::unique_ptr<pmc::llm::LLMClient> llm_client;
    std::unique_ptr<BufferedMarkdownConverter> markdown_converter;
    std::vector<std::pair<std::string, std::string>> chat_history;
    std::string current_response;
};

/**
 * @brief 运行简单的聊天窗口示例
 */
void run_simple_chat_example() {
    // 标准GTK的gtk_init返回void，不是bool
    gtk_init(nullptr, nullptr);
    
    SimpleChatWindow window;
    window.show();
    
    gtk_main();
}

} // namespace pmc::gui::example

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "用法: " << argv[0] << " <command>" << std::endl;
        std::cerr << "命令: chat - 运行聊天窗口" << std::endl;
        return 1;
    }
    
    std::string command = argv[1];
    
    if (command == "chat") {
        pmc::gui::example::run_simple_chat_example();
    } else {
        std::cerr << "未知命令: " << command << std::endl;
        return 1;
    }
    
    return 0;
}