#ifndef PMC_GUI_GTK_HPP
#define PMC_GUI_GTK_HPP

/**
 * @file GTK.hpp
 * @brief GTK库入口封装
 * 
 * 提供GTK库的统一入口和简化接口，便于在pmc项目中使用GTK进行GUI开发。
 */

#include <gtk/gtk.h>
#include <string>
#include <functional>
#include <memory>
#include <vector>

namespace pmc {
namespace gui {

/**
 * @class GTKApplication
 * @brief GTK应用程序封装类
 * 
 * 封装GTK应用程序的初始化和运行逻辑，提供简化的API。
 */
class GTKApplication {
public:
    /**
     * @brief 构造函数
     * @param app_id 应用程序ID
     * @param app_name 应用程序名称
     */
    GTKApplication(const std::string& app_id, const std::string& app_name);
    
    /**
     * @brief 析构函数
     */
    ~GTKApplication();
    
    /**
     * @brief 初始化GTK应用程序
     * @param argc 命令行参数数量
     * @param argv 命令行参数数组
     * @return 初始化是否成功
     */
    bool initialize(int argc, char** argv);
    
    /**
     * @brief 运行应用程序
     * @return 应用程序退出码
     */
    int run();
    
    /**
     * @brief 获取GTK应用程序指针
     * @return GtkApplication指针
     */
    GtkApplication* get_app() const { return app_; }
    
    /**
     * @brief 设置应用程序图标
     * @param icon_path 图标文件路径
     * @return 设置是否成功
     */
    bool set_icon(const std::string& icon_path);
    
    /**
     * @brief 设置窗口默认大小
     * @param width 窗口宽度
     * @param height 窗口高度
     */
    void set_default_window_size(int width, int height);
    
private:
    std::string app_id_;
    std::string app_name_;
    GtkApplication* app_;
    int default_width_;
    int default_height_;
};

/**
 * @class GTKWindow
 * @brief GTK窗口封装类
 * 
 * 封装GTK窗口的创建和管理，提供简化的窗口操作接口。
 */
class GTKWindow {
public:
    /**
     * @brief 构造函数
     * @param title 窗口标题
     * @param width 窗口宽度
     * @param height 窗口高度
     */
    GTKWindow(const std::string& title, int width = 800, int height = 600);
    
    /**
     * @brief 析构函数
     */
    ~GTKWindow();
    
    /**
     * @brief 显示窗口
     */
    void show();
    
    /**
     * @brief 隐藏窗口
     */
    void hide();
    
    /**
     * @brief 设置窗口标题
     * @param title 新标题
     */
    void set_title(const std::string& title);
    
    /**
     * @brief 获取窗口标题
     * @return 窗口标题
     */
    std::string get_title() const;
    
    /**
     * @brief 设置窗口大小
     * @param width 宽度
     * @param height 高度
     */
    void set_size(int width, int height);
    
    /**
     * @brief 获取窗口宽度
     * @return 窗口宽度
     */
    int get_width() const;
    
    /**
     * @brief 获取窗口高度
     * @return 窗口高度
     */
    int get_height() const;
    
    /**
     * @brief 获取GTK窗口指针
     * @return GtkWindow指针
     */
    GtkWindow* get_window() const { return window_; }
    
    /**
     * @brief 设置关闭回调函数
     * @param callback 关闭回调函数
     */
    void set_close_callback(std::function<void()> callback);
    
    /**
     * @brief 添加子控件
     * @param widget 要添加的控件
     */
    void add_widget(GtkWidget* widget);
    
    /**
     * @brief 设置窗口位置居中
     */
    void center_on_screen();
    
private:
    GtkWindow* window_;
    GtkWidget* container_;
    std::string title_;
    int width_;
    int height_;
    std::function<void()> close_callback_;
};

/**
 * @class GTKButton
 * @brief GTK按钮封装类
 * 
 * 封装GTK按钮的创建和事件处理。
 */
class GTKButton {
public:
    /**
     * @brief 构造函数
     * @param label 按钮标签
     */
    GTKButton(const std::string& label);
    
    /**
     * @brief 析构函数
     */
    ~GTKButton();
    
    /**
     * @brief 设置点击回调函数
     * @param callback 点击回调函数
     */
    void set_click_callback(std::function<void()> callback);
    
    /**
     * @brief 获取GTK按钮指针
     * @return GtkButton指针
     */
    GtkButton* get_button() const { return button_; }
    
    /**
     * @brief 设置按钮标签
     * @param label 新标签
     */
    void set_label(const std::string& label);
    
    /**
     * @brief 获取按钮标签
     * @return 按钮标签
     */
    std::string get_label() const;
    
    /**
     * @brief 启用按钮
     */
    void enable();
    
    /**
     * @brief 禁用按钮
     */
    void disable();
    
    /**
     * @brief 设置按钮工具提示
     * @param tooltip 工具提示文本
     */
    void set_tooltip(const std::string& tooltip);
    
private:
    GtkButton* button_;
    std::string label_;
    std::function<void()> click_callback_;
};

/**
 * @class GTKLabel
 * @brief GTK标签封装类
 * 
 * 封装GTK标签的创建和文本管理。
 */
class GTKLabel {
public:
    /**
     * @brief 构造函数
     * @param text 标签文本
     */
    GTKLabel(const std::string& text = "");
    
    /**
     * @brief 析构函数
     */
    ~GTKLabel();
    
    /**
     * @brief 设置标签文本
     * @param text 新文本
     */
    void set_text(const std::string& text);
    
    /**
     * @brief 获取标签文本
     * @return 标签文本
     */
    std::string get_text() const;
    
    /**
     * @brief 获取GTK标签指针
     * @return GtkLabel指针
     */
    GtkLabel* get_label() const { return label_; }
    
    /**
     * @brief 设置文本对齐方式
     * @param xalign 水平对齐 (0.0=左对齐, 0.5=居中, 1.0=右对齐)
     * @param yalign 垂直对齐 (0.0=顶部, 0.5=居中, 1.0=底部)
     */
    void set_alignment(float xalign = 0.0f, float yalign = 0.5f);
    
    /**
     * @brief 设置文本换行
     * @param wrap 是否换行
     */
    void set_wrap(bool wrap);
    
    /**
     * @brief 设置选择模式
     * @param selectable 是否可选择
     */
    void set_selectable(bool selectable);
    
private:
    GtkLabel* label_;
    std::string text_;
};

/**
 * @class GTKEntry
 * @brief GTK输入框封装类
 * 
 * 封装GTK输入框的创建和文本管理。
 */
class GTKEntry {
public:
    /**
     * @brief 构造函数
     * @param placeholder 占位符文本
     */
    GTKEntry(const std::string& placeholder = "");
    
    /**
     * @brief 析构函数
     */
    ~GTKEntry();
    
    /**
     * @brief 获取输入文本
     * @return 输入文本
     */
    std::string get_text() const;
    
    /**
     * @brief 设置输入文本
     * @param text 新文本
     */
    void set_text(const std::string& text);
    
    /**
     * @brief 清空输入框
     */
    void clear();
    
    /**
     * @brief 获取GTK输入框指针
     * @return GtkEntry指针
     */
    GtkEntry* get_entry() const { return entry_; }
    
    /**
     * @brief 设置占位符文本
     * @param placeholder 占位符文本
     */
    void set_placeholder(const std::string& placeholder);
    
    /**
     * @brief 设置输入变化回调函数
     * @param callback 输入变化回调函数
     */
    void set_changed_callback(std::function<void()> callback);
    
    /**
     * @brief 设置激活回调函数（回车键）
     * @param callback 激活回调函数
     */
    void set_activate_callback(std::function<void()> callback);
    
    /**
     * @brief 启用输入框
     */
    void enable();
    
    /**
     * @brief 禁用输入框
     */
    void disable();
    
private:
    GtkEntry* entry_;
    std::string placeholder_;
    std::function<void()> changed_callback_;
    std::function<void()> activate_callback_;
};

/**
 * @class GTKTextView
 * @brief GTK文本视图封装类
 * 
 * 封装GTK文本视图的创建和文本管理，支持多行文本编辑。
 */
class GTKTextView {
public:
    /**
     * @brief 构造函数
     */
    GTKTextView();
    
    /**
     * @brief 析构函数
     */
    ~GTKTextView();
    
    /**
     * @brief 获取文本内容
     * @return 文本内容
     */
    std::string get_text() const;
    
    /**
     * @brief 设置文本内容
     * @param text 新文本
     */
    void set_text(const std::string& text);
    
    /**
     * @brief 追加文本
     * @param text 要追加的文本
     */
    void append_text(const std::string& text);
    
    /**
     * @brief 清空文本
     */
    void clear();
    
    /**
     * @brief 获取GTK文本视图指针
     * @return GtkTextView指针
     */
    GtkTextView* get_text_view() const { return text_view_; }
    
    /**
     * @brief 获取文本缓冲区
     * @return GtkTextBuffer指针
     */
    GtkTextBuffer* get_buffer() const { return buffer_; }
    
    /**
     * @brief 设置只读模式
     * @param read_only 是否只读
     */
    void set_read_only(bool read_only);
    
    /**
     * @brief 设置文本变化回调函数
     * @param callback 文本变化回调函数
     */
    void set_changed_callback(std::function<void()> callback);
    
    /**
     * @brief 滚动到底部
     */
    void scroll_to_bottom();
    
private:
    GtkTextView* text_view_;
    GtkTextBuffer* buffer_;
    std::function<void()> changed_callback_;
};

/**
 * @class GTKBox
 * @brief GTK盒子布局容器封装类
 * 
 * 封装GTK盒子布局容器的创建和子控件管理。
 */
class GTKBox {
public:
    /**
     * @brief 构造函数
     * @param orientation 布局方向 (GTK_ORIENTATION_HORIZONTAL 或 GTK_ORIENTATION_VERTICAL)
     * @param spacing 子控件间距
     */
    GTKBox(GtkOrientation orientation = GTK_ORIENTATION_VERTICAL, int spacing = 5);
    
    /**
     * @brief 析构函数
     */
    ~GTKBox();
    
    /**
     * @brief 添加子控件
     * @param widget 要添加的控件
     * @param expand 是否扩展
     * @param fill 是否填充
     * @param padding 内边距
     */
    void add_widget(GtkWidget* widget, bool expand = false, bool fill = false, int padding = 0);
    
    /**
     * @brief 获取GTK盒子指针
     * @return GtkBox指针
     */
    GtkBox* get_box() const { return box_; }
    
    /**
     * @brief 设置间距
     * @param spacing 新间距
     */
    void set_spacing(int spacing);
    
    /**
     * @brief 设置对齐方式
     * @param align 对齐方式
     */
    void set_alignment(GtkAlign align);
    
    /**
     * @brief 设置均匀分配
     * @param homogeneous 是否均匀分配空间
     */
    void set_homogeneous(bool homogeneous);
    
private:
    GtkBox* box_;
    GtkOrientation orientation_;
    int spacing_;
};

/**
 * @brief 初始化GTK库
 * @param argc 命令行参数数量指针
 * @param argv 命令行参数数组指针
 * @return 初始化是否成功
 */
bool gtk_init(int* argc, char*** argv);

/**
 * @brief 运行GTK主循环
 */
void gtk_main();

/**
 * @brief 退出GTK主循环
 */
void gtk_main_quit();

/**
 * @brief 显示错误对话框
 * @param parent 父窗口
 * @param title 对话框标题
 * @param message 错误消息
 */
void show_error_dialog(GtkWindow* parent, const std::string& title, const std::string& message);

/**
 * @brief 显示信息对话框
 * @param parent 父窗口
 * @param title 对话框标题
 * @param message 信息消息
 */
void show_info_dialog(GtkWindow* parent, const std::string& title, const std::string& message);

/**
 * @brief 显示确认对话框
 * @param parent 父窗口
 * @param title 对话框标题
 * @param message 确认消息
 * @return 用户是否确认
 */
bool show_confirm_dialog(GtkWindow* parent, const std::string& title, const std::string& message);

} // namespace gui
} // namespace pmc

#endif // PMC_GUI_GTK_HPP