#include <filesystem>
#include "net/Http.hpp"
#include "pmc/ParserTask.hpp"
namespace qing {
ParserTask::ParserTask(): HttpTask(ADDR, PORT, [this](Http& http) {
    const char* target = "../public/models/1.txt";
    if (std::filesystem::exists(target)) {
        std::cout << "load model data from file..." << std::endl;
        std::ifstream file;
        file.open(target);
        nn.load(file);
	file.close();
    }
    else {
        std::cout << "create model data rand..." << std::endl;

        auto layer1 = nnl::Create_in_Factory(18, 36, 0.01, nnl::ActivationFunc::Leaky_ReLU);
        auto layer2 = nnl::Create_in_Factory(36, 36, 0.01, nnl::ActivationFunc::Leaky_ReLU);
        auto layer3 = nnl::Create_in_Factory(36, 36, 0.01, nnl::ActivationFunc::Leaky_ReLU);
        auto layer4 = nnl::Create_in_Factory(36, 36, 0.01, nnl::ActivationFunc::Leaky_ReLU);
        auto layer5 = nnl::Create_in_Factory(36, 36, 0.01, nnl::ActivationFunc::Leaky_ReLU);
        auto layer6 = nnl::Create_in_Factory(36, 16, 0.01, nnl::ActivationFunc::Sigmoid);

        nn.add(layer1);
        nn.add(layer2);
        nn.add(layer3);
        nn.add(layer4);
        nn.add(layer5);
        nn.add(layer6);

        /* 储存模型 */
        std::ofstream file;
        file.open("../public/models/1.txt");
        if (!file) {
            std::cout << "Open Model failed" << std::endl;
        }
        else {
            nn.save(file);
            file.close();
        }
    }

    nn.print_shape();

    http.post("/api/v1/mybot-sample", [this](const httplib::Request& req, httplib::Response& res) {
        if (!req.body.empty()) {
            try{
                mtx.lock();
                //just_play(req.body);
                auto r = run_infer(req.body, 0.0, 0.0, averages);  /* 推理结果无效化 */
                pool.push_back(r);
                averages = calc_aves(pool);
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
            try{
                mtx.lock();
                auto r =run_infer(req.body, 0.1, 10.0, averages);
                for (auto& v: r) {  /* 输出运行结果 */
                    std::cout << "(";
                    for (auto& i: v) {
                        std::cout << i;
                        std::cout << ",";
                    }
                    std::cout << ")\t";
                    std::cout << std::endl;
                }
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

}) {}

std::vector<std::vector<float>> ParserTask::run_infer(std::string script, float infer_freq, float train_discount, std::vector<std::vector<float>>& averages) {
    auto parser = ActParser();
    parser.fromStr(script);
    auto bot = parser.get();
    auto res = std::vector<std::vector<float>>{};
    auto all_count = 0;
    while (true) {
        try{
            std::cout << "------------------------------"<< std::endl;
            MPU6050_SensorData data = MPU6050_readSensorData();
            float pitch, roll;
            MPU6050_calAngle(data, pitch, roll);
            auto nor_pitch = normalize_pitch(pitch);
            auto nor_roll  = normalize_roll(roll);
            auto act = parser.now();  /* 获取一个动作 */
            auto x = std::vector<float>(); /* 创建输入 */
            x.push_back(nor_pitch);
            x.push_back(nor_roll);
            auto i{0};
            for (; i < act.size()-1; ++i) { /* 跳过了延时数值 */
                float a;
                if (bot[i] == 0) { a = 0; }  /* 空舵机位 */
                else { a = (float)act[i] / bot[i]; }
                x.push_back(a);
            }
            auto x1 = x; 
            auto o = nn.forward(x1); /* 前向反馈 */
            auto j{2};
            auto new_act = std::vector<float> ();
            unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
            std::mt19937 generator(seed);
            std::uniform_real_distribution<float> distribution(-0.5, 0.5);  // 1-100 均匀分布
            for (; j <x.size(); ++j) {
                auto a = x[j] + o[j-2] * 0.7 * infer_freq;  /* 模型微调 */
                float b= distribution(generator) * 0.3 * infer_freq; /* 随机因子 */
                new_act.push_back((int)((a+b) * bot[j-2]));  /* 10% */
                /* FIXME: item{x} = a+b */
            }
            new_act.push_back(act[j-2]);
            parser.next(new_act); /* 执行一个动作 */
            MPU6050_SensorData data1 = MPU6050_readSensorData();
            float pitch1, roll1;  /* 测量执行完毕后的角度 */
            MPU6050_calAngle(data1, pitch1, roll1);
            auto nor_pitch1 = normalize_pitch(pitch1);
            auto nor_roll1  = normalize_roll(roll1);
            auto err0 = calc_err(nor_pitch, nor_pitch1);
            auto err1 = calc_err(nor_roll, nor_roll1);
            res.push_back({err0, err1}); /* 推到结果集 */
            auto reward = err0 *train_discount + err1 * train_discount; /* 加权求和 */
            nn.backward(o, reward); /* 反向传播 */
            all_count++;

            /* 储存模型 */
            std::ofstream file;
            file.open("../public/models/1.txt");
            if (!file) {
                std::cout << "Open Model failed" << std::endl;
            }
            else {
                nn.save(file);
                file.close();
            }

        }
        catch (std::runtime_error& exp) {
            std::cout << exp.what() << std::endl;
            break;
        }
    }
    return res;
}
}
