#define HTTPLIB_COMPILE /* 无SSL需求，优先选这个；有SSL需求替换为 #define HTTPLIB_OPENSSL_SUPPORT */
#include <string>
#include <fstream>
#include <random>
#include <chrono>
#include <mutex>
#include <stdexcept>
#include "HttpTask.hpp"
#include "nn/NNBuilder.hpp"
#include "hd/Robot.hpp"
namespace qing{
static const std::string ADDR = std::string{"127.0.0.1"};  /* 只监听本机 */
static const int PORT = 9203;
using nnl = NeuralNetwork;
class ParserTask: public HttpTask {
public:
	ParserTask();
	ParserTask(const ParserTask&) = delete;
private:
	NNBuilder nn;
	std::mutex mtx;
	std::vector<std::vector<std::vector<float>>> pool;
	std::vector<std::vector<float>> averages;
	static float normalize_pitch(float pitch) {
		return (pitch + 180) / (2 * 180); /* FIXME: 方向反了 */
	}
	static float normalize_roll(float roll) {
		return (roll + 180) / (2 * 180); /* 观测值 */
	}
	static float calc_err(float nor_pitch, float pitch1) {
	
                return (nor_pitch > 0)  /* 计算误差 */
                     ? ((pitch1 > 0) ? nor_pitch - pitch1 : nor_pitch + pitch1 )
                     : ((pitch1 < 0) ? pitch1 - nor_pitch : -(nor_pitch + pitch1));
	}
	/* FIXME: 使用类封装采样数据 */
	static std::vector<std::vector<float>> calc_aves(std::vector<std::vector<std::vector<float>>>& samples) {
		std::vector<std::vector<float>> a_r{}; /* actions of average result FIXME: 长度必须一致 */
		for (const std::vector<std::vector<float>>& onetim: samples) { /* one time */
			auto first_action = onetim[0];
			std::vector<float> grp_r(first_action.size()); /* group of averate result */
			for (const std::vector<float>& action: onetim) {
				for (size_t i_nora = 0; i_nora < action.size(); ++i_nora) { /* item of normalized angle*/
					grp_r[i_nora] += action[i_nora];
				}
			}
			auto length = onetim.size();
			for (auto& i_r: grp_r) { /* item of average result */
				i_r /= length;
			}
			a_r.push_back(grp_r);
		}
		return a_r;
	}

	std::vector<std::vector<float>> run_infer(
		std::string script, float infer_freq, float train_discount, std::vector<std::vector<float>>& averages);
};
}
