#pragma once
#include <iostream>
#include <stdexcept>
#include "nn/NeuralNetwork.hpp"
namespace qing {
class NNBuilder {  /* 神经网络构建 */
public:
    void add(NeuralNetwork &layer);

    std::vector<std::pair<long, long>> get_shape() const;

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
    void fork(int index, long num, float rate) {

        /* 检查参数 */
        if (index < 0 || index > nn.size() - 1) { throw std::invalid_argument("wrong index"); }

        /* 获得原来的层以及形状 */
        auto it = nn.begin() + index;
        auto oldi = (*it).get_inputno();
        auto oldo = (*it).get_outputno();

        /* 删除原来的层 */
        nn.erase(it);

        /* 创建新的层 */
        auto f0 = NeuralNetwork::ActivationFunc::Leaky_ReLU;
        auto f1 = (!index < nn.size()) ? NeuralNetwork::ActivationFunc::Leaky_ReLU : NeuralNetwork::ActivationFunc::Sigmoid;
        auto layer0 = NeuralNetwork::Create_in_Factory(oldi, num, rate, f0);
        auto layer1 = NeuralNetwork::Create_in_Factory(num, oldo, rate, f1);

        /* 插入新的层 */
        it = nn.begin() + index;
        nn.insert(it, layer1);

        it = nn.begin() + index;
        nn.insert(it, layer0);
    }

    /*
     * 神经网络生长，增加第layer层，每层增加num个神经元
     *
     * layer: 增加的层数
     * num: 增加的神经元数量
     *
     * FIXME: 神经网络生长时应当冻结其它层（除了最后一层？）（learning_rate == 0）
     */
    void grow(int layer, int num) {
        nn[layer].grow(num, true);
        if (layer >0) {
            nn[layer-1].grow(num, false);
        }
    }

    //std::string serialize() const {
    //    std::string s;
    //    for (long j=0; j<nn.size(); ++j) {
    //        s += nn[j].serialize();
    //    }
    //    return s;
    //}

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
