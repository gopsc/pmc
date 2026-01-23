#include "nn/NeuralNetwork.hpp"
namespace qing{
/* 构造函数 使用基本参数构造 */
NeuralNetwork::NeuralNetwork(
    const long inputno, const long outputno,
    const std::vector<float> &weights,
    const std::vector<float> &bias,
    const float learning_rate,
    const ActivationFunc func_type
): inputno(inputno), outputno(outputno),
    weights(inputno, outputno, weights), bias( 1, outputno, bias),
    learning_rate(learning_rate),
    f_type(func_type) {
        ;/* 进行形状检查 */
}

/* 获取输入层大小 */
long NeuralNetwork::get_inputno() const {
    return this->inputno;
}

/* 获取输出层大小 */
long NeuralNetwork::get_outputno() const {
    return this->outputno;
}

/* 获取权重 */
Matrx<float>& NeuralNetwork::get_weights() {
	return this->weights;
}

/* 获取偏置 */
Matrx<float>& NeuralNetwork::get_bias() {
	return this->bias;
}

/* 正态分布Xavier初始化 */
void NeuralNetwork::xavierNormalInit(std::vector<float> &weights, long fan_in, long fan_out)
{

    /* 高质量的随机数生成器 */
    static std::mt19937 generator(std::random_device{}());

    /* 计算标准差 */
    float sigma = std::sqrt(2.0f / (fan_in + fan_out));

    /* 创建正态分布对象，均值为0，标准差为sigma */
    std::normal_distribution<float> distribution(0.0f, sigma);

    /* 遍历权重矩阵，进行赋值 */
    long k=0;
    for (long i=0; i<fan_in; ++i)
        for (long j=0; j<fan_out; ++j)
            weights[k++] = distribution(generator);

}

/* 均匀分布Xavier初始化函数 */
void NeuralNetwork::xavierUniformInit(std::vector<float> &weights, int fan_in, int fan_out)
{
    /* 使用高质量的随机数生成器 */
    static std::mt19937 generator(std::random_device{}());
        
    /* 计算区间范围：sqrt(2 / (fan_in + fan_out)) */
    float scale = std::sqrt(2.0f / (fan_in + fan_out));
        
    /* 创建均匀分布对象，范围为[-scale, scale] */
    std::uniform_real_distribution<float> distribution(-scale, scale);
        
    /* 遍历权重矩阵，进行赋值 */
    long k=0;
    for (long i=0; i<fan_in; ++i)
        for (long j=0; j<fan_out; ++j)
            weights[k++] = distribution(generator);
}

/* 工厂模式 通过形状构建全连接层 */
NeuralNetwork NeuralNetwork::Create_in_Factory(const long inputno, const long outputno, float learning_rate, ActivationFunc func_type)
{
    std::vector<float> weights(inputno * outputno);
    xavierUniformInit(weights, inputno, outputno);
    std::vector<float> bias(outputno);
    NeuralNetwork layer(inputno, outputno, weights, bias, learning_rate, func_type);
    return layer; /* FIXME: 使用移动语义 */
}


/* 前向反馈 */
std::vector<float> NeuralNetwork::forward(const std::vector<float>& x)
    
{


    /*
     * 矩阵乘法（点积）
     * wi => weights inputs(column)
     * wo => weights outputs(row)
     *
     * NOTE: 通常矩阵乘法是三层循环，这里我们用两层
     */
    //std::vector<float> res(this->outputno);  /* (1, outputs) */
    //for (long i=0; i<this->outputno; ++i) {
    //    long wo = i;
    //    float sum=0.0;
    //    for (long j=0, wi=wo; j<this->inputno; ++j, wi+=this->outputno) {
    //        sum += x[j] * this->weights[wi];
    //    }
    //    res[i]= sum;
    //}
    Matrx<float>mx{1, this->inputno, x};
        
        
    /*
     * 为输入进行备份
     *
     * FIXME: 使用移动语义传入参数，并且储存
     */
    this->prev_inputs = mx;  /* (1, inputs) */


    Matrx<float>r = mx * this->weights;  /* (1, outputs) */

    /* 偏置操作 */
    //for (long i=0; i<this->outputno; ++i) {
    //   res[i]+=this->bias[i];
    //}
    r = r + this->bias;

    /*
     * 为原始计算结果进行备份
     */
    this->prev_raw_inputs = r;

    /*
     * 使用ReLU激活函数
     *
     * FIXME: 建立一个算法类
     */
    f_t f = getf(f_type);

    std::vector<float> vec =r.get_data();
    for (long i=0; i<this->outputno; ++i) {
        float& v = vec[i];
        v = f(v);
    }

    r = Matrx<float>(1, outputno, vec);

    /*
     * 为输出进行备份
     */
    this->prev_outputs = r;


    /* 返回前向反馈的结果。 (1, outputs) */
    return r.get_data();
}


/* 反向传播 */
std::vector<float> NeuralNetwork::backward(const std::vector<float>& errors)  /* errors => (outputs, 1) */

{

    /* 匹配激活导数 */
    f_t fdot = getfdot(f_type);

    std::vector<float> vec = this->prev_raw_inputs.get_data();
    /* 将原始输出转化为导数 */
    for (long i=0; i<this->outputno; ++i) {
        float& v = vec[i];
        v = errors[i] * fdot(v);
    }

    /* 误差乘以导数 */
    //std::vector<float> first(this->outputno);
    //for (long i=0; i<this->outputno; ++i) {
    //    first[i] = errors[i] * this->prev_raw_inputs[i];
    //}
    Matrx<float> first{1, this->outputno, vec};

    /* 更新权重梯度 => (errors * 导数) @ this->inputs => (1, outputs ) @ (inputs, 1) = (outputs, inputs) */
    //this->weights_grad = std::vector<float>(this->outputno * this->inputno);
    //long k = 0;
    //for (long i=0; i<this->inputno; ++i) {
    //    for (long j=0; j<this->outputno; ++j) {
    //        this->weights_grad[k++] = first[j] * this->prev_inputs[i];
    //    }
    //}
    Matrx<float> mx {this->inputno, 1, this->prev_inputs.get_data()};  /* (inputs, 1) */
    this->weights_grad = mx * first;  /* (inputs, 1) @ (1, outputs) => (inputs, outputs) */

    /*
     * 更新偏置梯度
     *
     * NOTE: 在多批次情况下，应该沿着每一样本纵向求平均数
     */
    //this->bias_grad = std::vector<float>(this->outputno);
    //for (long i=0; i<this->outputno; ++i) {
    //    this->bias_grad[i] = first[i];
    //}
    this->bias_grad = first;


    /*
     * 计算上一层的误差责任分担，然后返回
     *
     * this->weights @ first  =>  (outputs, inputs) @ (1, outputs)  =  (inputs, 1)
     */
    //std::vector<float> uplevel_errors(this->inputno);
    //for (long i=0; i<this->inputno; ++i) {
    //    long wii = i * this->outputno;  /* 每一列的起始点 */
    //    float sum = 0.0f;
    //    for (long j=0, wi=wii; j < this->outputno; ++j, ++wi) {
    //        sum += this->weights[wi] * first[j];
    //    }
    //    uplevel_errors[i] = sum;
    //}
    Matrx<float> first1{this->outputno, 1, first.get_data()};
    Matrx<float> uplevel_errs = this->weights * first1;  /* (inputs, outputs) @ (outputs, 1) */
    return uplevel_errs.get_data();

    /* 使用移动语义返回上一层误差 */
    //return std::move(uplevel_errors);
}

void NeuralNetwork::update(float discount) {
    this->weights = this->weights + (this->weights_grad * this->learning_rate * discount);
    this->bias    = this->bias    + (this->bias_grad    * this->learning_rate * discount);
}
}
