#pragma once
#include <cmath>
#include <iostream>
/* FIXME: 设置为每层可调节的 */
/* FIXME: 现在这个“接口”并未提供抽象函数 */
constexpr float LEAKY_RELU_ALPHA = 0.01;
using f_t = float(*)(float); /* FIXME: 在qing::Thread附近已经定义了f_t标识符 */

/* 神经网络层接口 */
class ILayer {
public:

    /* 激活函数枚举类 */
    enum class ActivationFunc {
        None, ReLU, Leaky_ReLU, Sigmoid, Tanh
    };

    /* 获取类型名 */
    static const char* get_f_type(ActivationFunc& af);

    /* 通过类型名获取实体 */
    static ActivationFunc parse_ff_type(const std::string& name);


    /* 向输出流存储模型 */
    virtual void save(std::ostream& out) = 0;

    /* 从输入流加载模型 */
    //virtual void load(std::istream& in) = 0;

protected:

    /* 获取函数对象 - 获取激活函数 */
    static f_t getf(ActivationFunc type);

    /* 获取函数对象 - 求激活函数导数 */
    static f_t getfdot(ActivationFunc type);

    /* ReLU 激活函数 */
    static float relu(float x);

    /* ReLU 导数 */
    static float relu_derivative(float x);

    /*
     * leaky ReLU 激活函数
     *
     * NOTE: alpha太小或者选择使用ReLU函数的情况下，会导致梯度消失 
     * NOTE: Leaky ReLU 不会使梯度完全消失，但也会使它变得很小
     */
    static float leaky_relu(float x);
    /* leaky ReLU 导数 */
    static float leaky_relu_derivative(float x);

    /* Sigmoid 激活函数 */
    static float sigmoid(float x);
    /* Sigmoid 导数 */
    static float sigmoid_derivative(float x);

    /* Tanh 激活函数 */
    static float tanh(float x);
    /* Tanh 导数 */
    static float tanh_derivative(float x);
};
