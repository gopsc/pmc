#define HTTPLIB_COMPILE /* 无SSL需求，优先选这个；有SSL需求替换为 #define HTTPLIB_OPENSSL_SUPPORT */
#include <string>
#include <mutex>
#include <stdexcept>
#include "HttpServerTask.hpp"
#include "Dashun.h"
#include "Dashun_linux.hpp"
namespace qing{
static const char* ADDR= "0.0.0.0";
static constexpr int PORT = 9203;
static NNBuilder nn;
using nnl = NeuralNetwork;
class ParserServerTask: public HttpServerTask {
public:
	ParserServerTask(): HttpServerTask(ADDR, PORT, [this](Http& http) {

		auto layer1 = nnl::Create_in_Factory(18, 36, 0.01, nnl::ActivationFunc::Leaky_ReLU);
		auto layer2 = nnl::Create_in_Factory(36, 36, 0.01, nnl::ActivationFunc::Leaky_ReLU);
		auto layer3 = nnl::Create_in_Factory(36, 36, 0.01, nnl::ActivationFunc::Leaky_ReLU);
		auto layer4 = nnl::Create_in_Factory(36, 16, 0.01, nnl::ActivationFunc::Sigmoid);

		nn.add(layer1);
		nn.add(layer2);
		nn.add(layer3);
		nn.add(layer4);

		nn.print_shape();

		http.post("/api/v1/mybot", [this](const httplib::Request& req, httplib::Response& res) {
			if (!req.body.empty()) {
			try{
				mtx.lock();
				auto parser = ActParser();
				parser.fromStr(req.body);
				parser.play();
				mtx.unlock();
				res.set_content("OK", "text/plain");
			}
			catch (std::exception& exp) {
				res.set_content(exp.what(), "text/plain");
				res.status =400;
			}
			}
			else {
				res.set_content("Empty request body", "text/plain");
				res.status = 400;
			}
		});

		/*  FIXME: 需要先取得模板数据 */
		http.post("/api/v1/mybot-nn", [this](const httplib::Request& req, httplib::Response& res) {
			if (!req.body.empty()) {
				mtx.lock();
				auto parser = ActParser();
				try{ parser.fromStr(req.body); }
				catch (std::runtime_error& exp) {
					res.set_content(exp.what(), "text/plain");
					res.status  = 400;
				}
				auto bot = parser.get();
				while (true) {
				try{
					std::cout << "------------------------------"<< std::endl;
					MPU6050_SensorData data = MPU6050_readSensorData();
					float pitch, roll;
					MPU6050_calAngle(data, pitch, roll);
					printf("pitch: %.2f\troll: %.2f\n");
					auto act = parser.now();  /* 获取一个动作 */
					auto x = std::vector<float>();
					auto nor_pitch = normalize_pitch(pitch);
					auto nor_roll  = normalize_roll(roll);
					x.push_back(nor_pitch);
					x.push_back(nor_roll);
					auto i{0};
					for (; i < act.size()-1; ++i) {
						float a;
						if (bot[i] == 0) { a = 0; }  /* 空舵机位 */
						else { a = (float)act[i] / bot[i]; }
						x.push_back(a);
						std::cout << act[i] << "-" << bot[i] << ",";
					}
					std::cout << std::endl;
					auto x1 = x;  /* 前向反馈 */
					auto o = nn.forward(x1);
					auto j{2};
					auto new_act = std::vector<float> ();
					for (; j <x.size(); ++j) {
						auto a = x[j] + o[j-2] * 0.1;  /* 10% */
						new_act.push_back((int)(a * bot[j-2]));
						std::cout << a<< ",";
					}
					new_act.push_back(act[j-2]);
					std::cout << act[j-2];
					std::cout << std::endl;
					parser.next(new_act); /* 执行一个动作 */
					MPU6050_SensorData data1 = MPU6050_readSensorData();
					float pitch1, roll1;
					MPU6050_calAngle(data1, pitch1, roll1);
					printf("pitch1: %.2f\troll1: %.2f\n");
					continue;
				}
				catch (std::runtime_error& exp) {
					std::cout << exp.what() << std::endl;
					break;
				}
				}
				mtx.unlock();
				res.set_content("OK", "text/plain");
			}
			else {
				res.set_content("Empty request body", "text/plain");
				res.status = 400;
			}
		});

	}) {};

	ParserServerTask(const ParserServerTask&) = delete;
private:
	std::mutex mtx;
	static float normalize_pitch(float pitch) {
		return (pitch + 3.14) / (2 * 3.14); /* FIXME: 方向反了 */
	}
	static float normalize_roll(float roll) {
		return (roll + 0.28) / (2 * 0.28); /* 观测值 */
	}
};
}
