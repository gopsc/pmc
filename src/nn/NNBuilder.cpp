#include "nn/NNBuilder.hpp"
namespace qing {

    void NNBuilder::add(NeuralNetwork &layer) { nn.push_back(layer); }

    std::vector<std::pair<long, long>> NNBuilder::get_shape() const {
        std::vector<std::pair<long, long>> shape;
        for (long j = 0; j < nn.size(); ++j) {
            shape.emplace_back(nn[j].get_inputno(), nn[j].get_outputno());
        }
        return shape;
    }

    void NNBuilder::print_shape() const {
        auto shape = get_shape();
        for (const auto& p : shape) {
            std::cout << p.first << " -> " << p.second << std::endl;
        }
    }

    /* 前向反馈 */
    std::vector<float> NNBuilder::forward(std::vector<float>& r) {
        auto x = r;
        for (long j=0; j< nn.size(); ++j) {
            x = nn[j].forward(x);
        }
	return x;
    }

    /* 反向传播 */
    std::vector<float> NNBuilder::backward(std::vector<float>& errs, float discount) {
        auto e = errs;
        for (long j=nn.size()-1; j>=0;--j) {
            e = nn[j].backward(e);
            nn[j].update(discount);
        }
	return e;
    }

    /* 计算误差 */
    std::vector<float> NNBuilder::cal_err(std::vector<float>& r, std::vector<float>& t) {
        auto res = r;
        for (long j=0; j<r.size(); ++j) {
            res[j] = t[j] - r[j];
        }
	return res;
    }
}
