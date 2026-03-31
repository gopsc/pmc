#ifndef PMC_GUI_HPP
#define PMC_GUI_HPP

/**
 * @file gui.hpp
 * @brief pmc项目GUI模块主入口
 * 
 * 提供GTK GUI模块的统一入口，简化GUI开发。
 */

#include "gui/GTK.hpp"

namespace pmc {
namespace gui {

/**
 * @brief 初始化GUI模块
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return 初始化是否成功
 */
bool init(int argc, char** argv);

/**
 * @brief 运行GUI主循环
 */
void run();

/**
 * @brief 退出GUI主循环
 */
void quit();

/**
 * @brief 创建主窗口
 * @param title 窗口标题
 * @param width 窗口宽度
 * @param height 窗口高度
 * @return 窗口指针
 */
GTKWindow* create_main_window(const std::string& title = "PMC Application", 
                              int width = 800, int height = 600);

/**
 * @brief 创建聊天窗口
 * @param parent 父窗口
 * @return 聊天窗口指针
 */
GTKWindow* create_chat_window(GTKWindow* parent = nullptr);

/**
 * @brief 创建设置窗口
 * @param parent 父窗口
 * @return 设置窗口指针
 */
GTKWindow* create_settings_window(GTKWindow* parent = nullptr);

/**
 * @brief 显示关于对话框
 * @param parent 父窗口
 */
void show_about_dialog(GTKWindow* parent);

/**
 * @brief 显示状态栏
 * @param parent 父窗口
 * @param message 状态消息
 */
void show_status_bar(GTKWindow* parent, const std::string& message);

/**
 * @brief 更新状态栏消息
 * @param message 新状态消息
 */
void update_status_bar(const std::string& message);

/**
 * @brief 显示进度对话框
 * @param parent 父窗口
 * @param title 对话框标题
 * @param message 进度消息
 */
void show_progress_dialog(GTKWindow* parent, const std::string& title, const std::string& message);

/**
 * @brief 更新进度对话框
 * @param progress 进度值 (0.0-1.0)
 * @param message 进度消息
 */
void update_progress_dialog(double progress, const std::string& message);

/**
 * @brief 关闭进度对话框
 */
void close_progress_dialog();

} // namespace gui
} // namespace pmc

#endif // PMC_GUI_HPP