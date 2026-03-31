#pragma once
#include <iostream>
#include <stdexcept>
#include "nn/NeuralNetwork.hpp"
namespace qing {
class NNBuilder {  /* 神经网络构建 */
public:
    void add(NeuralNetwork &layer);

    /* 获取变换层的形状
     * FIXME: 这个函数有必要吗  */
    std::vector<std::pair<long, long>> get_shape() const;

    /* 向标准输出打印形状 */
    void print_shape() const;

    /* 向输出流储存模型 */
    void save(std::ostream& out) {
        for (auto& layer: nn) {
            layer.save(out);
        }
    }

    /* 从输入流加载模型 */
    void load(std::istream& in) {
        nn.clear();
        while(true) {
            try{
                auto layer = NeuralNetwork::Load_in_Factory(in);
                add(layer);
            }
	    catch (std::runtime_error& e) {
                break;
            }
        }
    }

    /*
     * 将一层非线性层分裂为两层，相当于插入一层神经元
     */
    //void fork(int index, long num, float rate) {

    /*
     * 神经网络生长，增加第layer层，每层增加num个神经元
     *
     * layer: 增加的层数
     * num: 增加的神经元数量
     *
     * FIXME: 神经网络生长时应当冻结其它层（除了最后一层？）（learning_rate == 0）
     */
    //void grow(int layer, int num) {

    /* 前向反馈 */
    std::vector<float> forward(std::vector<float>& r);

    /* 反向传播 */
    std::vector<float> backward(std::vector<float>& errs, float discount = 1.0);

    /* 计算误差 */
    std::vector<float> cal_err(std::vector<float>& r, std::vector<float>& t);



private:
    std::vector<NeuralNetwork> nn;
};
}
