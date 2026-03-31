/*

 ___col___
/         \
+---------+ \
| + + + + | | row - 第一维度
| + + + + | |
+---------+ /

*/

#pragma once
#include <cmath>
#include <vector>
#include <iostream>
#include <stdexcept>
#include "cn/中文化.hpp"
命名空间 qing {
模板 <类型名 T>	
/* 矩阵类 - 二维向量 (FIXME: 1.增加函数广播方法。2.增加+=操作符等)。3. 增加初始化方法*/
类 Matrx {  
公开的:
    Matrx();
    Matrx(常量 长型 row, 常量 长型 col);  /* 该构造函数用于从头开始构造 */
    Matrx(常量 长型 row, 常量 长型 col, 常量 std::vector<T>& src);  /* 该构造函数用于通过已有的vector进行构造 */
    const T& get(long row, long col) const;   /* 访问器 - 访问矩阵元素 */
    T& witch(long row, long col);  /* 访问器 - 访问矩阵元素 */
    const std::vector<T>& get_data() const;  /* 访问器 - 用于获取原始数据 */
    const long get_col() const;   /* 获取列数 */
    const long get_row() const;   /* 获取行数 */
    bool has_same_shape(const Matrx<T>& other);  /* 两个矩阵是否具有相同形状 */
    bool has_dot_shape(const Matrx<T>& other);   /* 具有矩阵乘法的形状（标准：A的列数与B的行数相等） */
    void assert_same_shape(const Matrx<T>& other); /* 断言两个矩阵具有相同形状 */
    void assert_dot_shape(const Matrx<T>& other);  /* 断言两个矩阵具有点积前置形状 */
    Matrx<T> operator+(const Matrx<T>& other);  /* 加法操作符重定义 - 矩阵加法 */
    Matrx<T> operator+(T& item);  /* 矩阵加法广播 */
    Matrx<T> operator-(const Matrx<T>& other);  /* 操作符重载 - 矩阵减法 */
    Matrx<T> operator-(T& item);  /* 操作符重载 - 矩阵减法广播 */
    const Matrx<T> operator*(const Matrx<T>& other);  /* 操作符重载 - 矩阵乘法 */ 
    Matrx<T> operator*(T& item);  /* 操作符重载 - 矩阵乘法广播 */
    Matrx<T> operator/(T& item);  /* 操作符重载 - 矩阵除法广播 */

    /* 储存矩阵 */
    void save(std::ostream& out) {
        out << row << " " << col;  /* 储存形状 */
        for (const auto& item: arr) { /* 储存数据 */
            out << " ";
            out << item;
        }
    }

    /* FIXME: 不写load()方法的原因是使用工厂模式较为方便 */
    
    /* FIXME: 增加矩阵转置操作 */

    /* FIXME: 添加广播以使得矩阵与向量相乘 */

    /* FIXME: 增加元素对应乘积 */

private:
    long row, col;
    std::vector<T> arr;  /* 存储矩阵的张量 */
};
}
