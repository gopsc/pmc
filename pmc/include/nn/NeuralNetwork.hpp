#pragma once
#include <random>
#include "nn/Matrix.hh"
#include "nn/ILayerFullyConnectted.hpp"

namespace qing {

/* 人工神经网络 - 全连接层 */
class NeuralNetwork: public ILayerFullyConnectted {
public:
    /* 构造函数 使用基本参数构造 */
    NeuralNetwork(
        const long inputno, const long outputno,
        const std::vector<float> &weights,
        const std::vector<float> &bias,
        const float learning_rate,
        const ActivationFunc func_type
    );


    ActivationFunc& get_f() {
        return f_type;
    }

    float& get_lr() {
    	return learning_rate;
    }

    /*
     * 获取输入层大小
     *
     * FIXME: 优化掉这两个访问器，通过直接返回矩阵，获取层的形状
     */
    long get_inputno() const;

    /* 获取输出层大小 */
    long get_outputno() const;


    /* 获取权重、偏置 */
    Matrx<float>& get_weights();

    Matrx<float>& get_bias();

    
    /* 正态分布Xavier初始化 */
    static void xavierNormalInit(std::vector<float> &weights, long fan_in, long fan_out);
    
    /* 均匀分布Xavier初始化函数 */
    static void xavierUniformInit(std::vector<float> &weights, int fan_in, int fan_out);

    /* 工厂模式 通过形状构建全连接层 */
    static NeuralNetwork Create_in_Factory(const long inputno, const long outputno, float learning_rate, ActivationFunc func_type);

    /* 工厂模式 通过输入流读形状构建 */
    static NeuralNetwork Load_in_Factory(std::istream& in) {
        std::string f_name;
        if (!(in >> f_name))
            throw std::runtime_error("read out of lines");
        float learning_rate;
        in >> learning_rate;
        /*------------------*/
        size_t row, col;
        in >> row;  /* FIXME: 如果是负数会怎样 */
        in >> col;
        auto weis = load_vec(in, row * col);
        size_t rowb, colb;
        in >> rowb >> colb; /* FIXME: 如果这里读错了，也要抛出异常 */
        auto bias = load_vec(in, col);
        auto layer  = NeuralNetwork(
            row, col, weis, bias, learning_rate, ILayer::parse_ff_type(f_name));
       /*------------------*/
        return layer;
    }

    static std::vector<float> load_vec(std::istream& in, size_t length) {
        std::vector<float> data;
        for (size_t i = 0; i < length; ++i) {
            float item;
            in >> item;
            data.push_back(item);
        }
	return data;
    }


    /* 前向反馈 */
    std::vector<float> forward(const std::vector<float>& x) override;

    /* 反向传播 */
    std::vector<float> backward(const std::vector<float>& errors) override;

    /*
     * 更新权重
     *
     * 可以输入一个折扣系数
     */
    void update(float discount = 1.0);
    
    /*
     * 神经网络层长大
     *
     * 重新初始化权重矩阵，使用均匀分布初始化
     *
     * num: 增加的神经元数量
     * flag: true为增加输入层，false为增加输出层
     */
    void grow(int num, bool flag) {
        this->inputno = (flag) ? this->inputno + num : this->inputno;
        this->outputno = (flag) ? this->outputno : this->outputno + num;
        auto vec = std::vector<float>(this->inputno * this->outputno);
        xavierUniformInit(vec, this->inputno, this->outputno);
        this->weights = Matrx<float>(this->inputno, this->outputno, vec);
        if (!flag)
            this->bias = Matrx<float>(1, this->outputno);
    }

    /* 输出神经网络到输出流 */
    void save(std::ostream& out) override {
        out << get_f_type(f_type) << " ";
        out << learning_rate << " ";
        weights.save(out);  out << " ";
        bias.save(out);     out << "\n";
    }

    /* 从输入流加载模型 */
    //void load(std::istream& in) override 


private:

    /* 神经网络层规格 */
    long inputno, outputno;

    /* 权重与偏置矩阵 */
    Matrx<float> weights; /* (outputs, inputs) */
    Matrx<float> bias;    /* (1, outputs) */

    /* 学习率 */
    float learning_rate;

    /* 激活函数 */
    ActivationFunc f_type;

    /* 存储在类内部的上一次的输入，输出，和激活前输出 */
    Matrx<float> prev_inputs, prev_raw_inputs, prev_outputs;
    /* 存储本次学习的权重梯度以及偏置梯度 */
    Matrx<float> weights_grad, bias_grad;

};
}
