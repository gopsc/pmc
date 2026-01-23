
//作者：简单
//修改：qing
/*
 * C++17支持了中文变量名，配合define预编译指令可以将代码中文化。
 * 
 * 在C语言中，变量名只能由字母、数字或是下划线组成。
 * 由于C++语言兼容C语言，最开始的C++是不能够使用多字节变量名的。
 *
 * */
#ifndef __中文模式
#define __中文模式
/*保留字*/
#define 使用 using
#define 命名空间 namespace
#define 取别名 typedef
#define 覆盖 override
#define 取内存大小 sizeof
#define 返回 return
#define 如果 if
#define 否则 else
#define 又或者 else if
#define 真 true
#define 假 false
//#define 且 and
//#define 或 or
#define 空类型 void
#define 常量 const
#define 有符号 signed
#define 无符号 unsigned
#define 文件 FILE
#define 文件结束 EOF
#define 模板 template
#define 类 class
#define 结构体 struct
#define 枚举类型 enum
#define 对象 this
#define 抛出异常 throw
#define 新建 new
//#define 新的 new
#define 删除 delete
#define 跳出 break
#define 继续 continue
#define 判断循环 while
#define 循环判断 do
#define 循环判断尾 while
#define 分支选择 switch
#define 支系 case
#define 缺省 default
#define 计次循环 for
#define 尝试 try
#define 获取异常 catch
#define 捕获异常 catch
#define 退出 exit
#define 公开的 public
#define 受保护的 protected
#define 私有的 private
#define 外部的 extern
#define 静态的 static
#define 虚拟的 virtual

#define 空的 NULL
#define 空指针 nullptr
#define 整数 int
#define 字符 char
#define 短整数 short
#define 长型 long
#define 超长型 uint64_t
#define 单精度小数 float
#define 双精度小数 double
#define 长度单位 size_t
#define 图像单位 graphsize_t
#define 位置单位 position_t
#define 内存单位 memory_t
#define 像素单位 pixel_t
#define 状态 error_t
#define 状态量 status //用于状态机
#define 标识符 identifier_t
#define 设备标识符 device_t
#define 错误码 error_t
#define 布尔 bool
#define 逻辑型 bool
#define 可变类型 auto
#define 适应 auto
#define _文件 FILE
#define _文件结束 EOF
#define _读模式 "r"
#define _写模式 "w"
#define 模板 template
#define 类型名 typename
#define 默认的 default
#define 被删除 delete
#define 入口 main
#define 主函数 main
#define 或 ||
#define 或者 ||
#define 而且 &&
#define 且 &&
#define 等于 ==
#define 大于等于 >=
#define 小于等于 <=
#define 大于 >
#define 小于 <
#define 不报错的 noexcept
#define 不转换的 explicit
#define 纯函数的 consteval
#define 预求值的 constexpr

#define _加法重载 operator+
#define _取下标重载 operator[]
#define _开始 begin
#define _结束 end
//-----------------------------------------------------------
//-----------------------------------------------------------
//基本单元
#ifdef __cplusplus
#include <iostream>

命名空间 中文标准{

类 字符串 {
公开的:
    /* 初始化为空字符串 */
    字符串() {
        对象->s = "";
    }
    /* 以字面量初始化 */
    字符串(常量 字符* 源) {
        对象->s = 源;
    }
    /* 访问器 */
    常量 字符* 取值() {
        返回 s.c_str();
    }
    /*  */
    字符串& _加法重载(常量 字符* 乙方) {
        对象->s += 乙方;
        返回 *对象;
    }
    /* 字符串之间的连接 */
    字符串& _加法重载(字符串& 乙) {
        对象->s += 乙.取值();
        返回 *对象;
    }
私有的:
    std::string s;
};

类 标准输出{
公开的:
    空类型 调试输出(常量 字符* 消息) {
        std::cout << 消息;
    }
    空类型 调试输出(字符串& 信) {
        std::cout<< 信.取值();
    }
};

类 标准输入{
公开的:
    字符串 取字符串(){
        std::string s;
        std::cin >> s;
        返回 字符串(s.c_str());
    }
    整数 取整数() {
        整数 数;
        std::cin >> 数;
        返回 数;
    }
};


}
#endif //__cplusplus
#endif //__中文模式
