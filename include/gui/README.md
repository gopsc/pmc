# PMC GUI 模块 - GTK 封装

## 概述

本模块提供了对GTK库的C++封装，简化在pmc项目中使用GTK进行GUI开发的过程。封装提供了面向对象的接口，隐藏了GTK的C语言细节。

## 文件结构

```
include/gui/
├── GTK.hpp              # 主封装头文件（类声明）
├── GTK_impl.hpp         # 内联实现文件
├── gui.hpp              # GUI模块主入口
├── example.hpp          # 使用示例
└── README.md            # 本文档
```

## 主要类

### 1. GTKApplication
封装GTK应用程序的初始化和运行。

```cpp
pmc::gui::GTKApplication app("com.example.pmc", "PMC Application");
app.initialize(argc, argv);
app.run();
```

### 2. GTKWindow
封装GTK窗口的创建和管理。

```cpp
pmc::gui::GTKWindow window("My Window", 800, 600);
window.set_close_callback([]() {
    std::cout << "Window closing" << std::endl;
    gtk_main_quit();
});
window.show();
```

### 3. GTKButton
封装GTK按钮的创建和事件处理。

```cpp
pmc::gui::GTKButton button("Click Me");
button.set_click_callback([]() {
    std::cout << "Button clicked!" << std::endl;
});
```

### 4. GTKEntry
封装GTK输入框的创建和文本管理。

```cpp
pmc::gui::GTKEntry entry("Enter text here...");
entry.set_activate_callback([]() {
    std::string text = entry.get_text();
    std::cout << "Entered: " << text << std::endl;
});
```

### 5. GTKTextView
封装GTK文本视图的创建和文本管理。

```cpp
pmc::gui::GTKTextView text_view;
text_view.set_read_only(true);
text_view.append_text("Hello, World!\n");
```

### 6. GTKBox
封装GTK盒子布局容器的创建和子控件管理。

```cpp
pmc::gui::GTKBox vbox(GTK_ORIENTATION_VERTICAL, 5);
vbox.add_widget(GTK_WIDGET(button.get_button()));
vbox.add_widget(GTK_WIDGET(entry.get_entry()));
```

## 使用示例

### 简单聊天窗口

```cpp
#include "gui/example.hpp"

int main(int argc, char** argv) {
    pmc::gui::example::run_simple_chat_example(argc, argv);
    return 0;
}
```

### 设置窗口

```cpp
#include "gui/example.hpp"

int main(int argc, char** argv) {
    pmc::gui::example::run_settings_example(argc, argv);
    return 0;
}
```

## 编译要求

### 依赖项
- GTK+ 3.0 或更高版本
- C++11 或更高版本

### Ubuntu/Debian 安装依赖
```bash
sudo apt-get install libgtk-3-dev
```

### Fedora/RHEL 安装依赖
```bash
sudo dnf install gtk3-devel
```

### 编译命令示例
```bash
g++ -std=c++11 -o myapp main.cpp `pkg-config --cflags --libs gtk+-3.0`
```

## 集成到pmc项目

### 1. 在CMakeLists.txt中添加
```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

include_directories(${GTK3_INCLUDE_DIRS})
target_link_libraries(your_target ${GTK3_LIBRARIES})
```

### 2. 使用封装类
```cpp
#include "gui/GTK.hpp"

// 使用pmc::gui命名空间中的类
```

## 设计原则

1. **简化接口**：隐藏GTK的C语言细节，提供C++面向对象接口
2. **资源管理**：使用RAII原则管理GTK对象生命周期
3. **类型安全**：使用C++类型系统减少错误
4. **可扩展性**：易于添加新的封装类
5. **向后兼容**：保持与原生GTK API的互操作性

## 注意事项

1. **线程安全**：GTK不是线程安全的，所有GUI操作必须在主线程进行
2. **内存管理**：GTK使用引用计数，封装类会自动处理引用
3. **信号连接**：使用lambda表达式简化信号处理
4. **错误处理**：提供基本的错误检查和对话框支持

## 扩展指南

要添加新的GTK控件封装：

1. 在`GTK.hpp`中声明类
2. 在`GTK_impl.hpp`中实现内联方法
3. 提供构造函数、析构函数和主要方法
4. 添加信号连接支持
5. 更新示例和文档

## 许可证

本模块遵循pmc项目的整体许可证。

## 贡献

欢迎提交问题和拉取请求改进本模块。