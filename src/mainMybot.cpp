/*
 *  并发机器（进程托管）
 *  
 *  FIXME: 尽量使用大众的库（boost）
 */
#define HTTPLIB_COMPILE /* 无SSL需求，优先选这个；有SSL需求替换为 #define HTTPLIB_OPENSSL_SUPPORT */

#include <filesystem>
#include <string> 
#include <fstream>
#include <random>
#include <chrono>
#include <mutex>
#include <vector>
#include <stdexcept>
#include <functional>
#include <csignal> /* 注册信号以优雅退出 */
#include <boost/program_options.hpp>
#include "logs/Logger.hpp"
#include "th/Thread.hpp" /* 可控线程类 */
#include "th/Cv_wait.hpp" /* 通过条件变量进行等待 */
#include "th/ITask.hpp"
#include "nn/NNBuilder.hpp"
#include "hd/Robot.hpp"

/* lci 临时 */
#include "lci/p_th.h"

/* 中文模式 Chinese Mode */
#include "cn/中文化.hpp"
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
namespace po = boost::program_options;

/* 版本号 */
static const char *PMC_VERSION = "0.4.x";
/* 条件变量的等待机制让主程序能够等待中断信号的产生 */
qing::Cv_wait cv =  qing::Cv_wait();


/*
 * Http Server Task
 * 超文本传输服务器任务
 */
namespace qing{
class HttpTask: public qing::ITask {
public:
	/* 调用回调函数，设置http服务器 */
	HttpTask(const std::string& addr, const int port, std::function<void(Http&)> f)
	: http_addr(addr), http_port(port), callback(f) {

		init_thread();
	}

	/* 任务不可复制 */
	HttpTask(const HttpTask& ) = delete;

	/* 关闭线程、关闭服务 */
	~HttpTask() {
		if (http) http->stop();
		if (th) th->WaitClose();
	}


	/* 从外部停止任务 */
	void stop() override {
		if (http) http->stop();
		th->WaitClose();
	}

	/* 唤醒线程 */
	void start() override {
		th->Activate(); /* 对于已关闭的线程 */
		th->WaitStart();
	}

	/* 是否在运行 */
	bool isRunning() override {
		auto stat = th->check();
		return stat == Fsm::Stat::START
			|| stat == Fsm::Stat::RUNNING;
	}
private:
	std::unique_ptr<Thread> th;
	std::function<void(Http&)> callback;
	std::unique_ptr<Http> http;
	std::string http_addr;
	int http_port;

	/* 初始化线程 */
	void init_thread();
}; //HttpTask
} // qing


namespace qing{
static const std::string ADDR = std::string{"127.0.0.1"};  /* 只监听本机 */
static const int PORT = 9203;
using nnl = NeuralNetwork;
class ParserTask: public HttpTask { /* 动作解析器 */
public:
	ParserTask();
	ParserTask(const ParserTask&) = delete;

	/* 解析器异常 */
	class ParserException: public std::exception{
	protected:
		std::string message;
	public:
		explicit ParserException(const std::string& msg): message(msg) {}
		const char* what() const noexcept override {
			return message.c_str();
		}
	};

	/* 没有消息体异常 */
	class NoRequestBodyException: public ParserException {
	public:
		NoRequestBodyException(const std::string& msg): ParserException(msg) {}
	};

private:
	NNBuilder nn;
	std::mutex mtx;
	std::vector<std::vector<std::vector<float>>> pool; /* 角度数据 - 样品池 */
	std::vector<std::vector<float>> averages;          /* 角度数据 - 平均值 */
	/* 归一化 */
	static float normalize_pitch(float pitch) {
		return (pitch + 180) / (2 * 180); /* FIXME: 方向反了 */
	}
	/* 归一化 */
	static float normalize_roll(float roll) {
		return (roll + 180) / (2 * 180); /* 观测值 */
	}
	/* 计算误差 */
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

	/* 利用神经网络推理微调角度 */
	std::vector<std::vector<float>> run_infer(
		std::string script, float infer_freq, float train_discount, std::vector<std::vector<float>>& averages);
};
}



/* 捕获中断信号，结束线程并且退出 */
void signalHandler(int signum)
{
    LOG_INFO("收到终止信号，准备停止服务器...");
    cv.WakeCv();    /* 唤醒条件信号即关闭程序 */
}

/*----SIGNAL ACTION----*/
//
/* 对信号的响应行为 */
void set_signal_action() {

    /* 该结构体用于描述信号的处理方式 */
    struct sigaction sa;


    /* 绑定处理函数 */
    sa.sa_handler = signalHandler;
    
    /*
     * 清空并初始化一个信号掩码集  --------------> 用于指定哪些信号在当前或即将执行的信号处理函数期间应该被阻塞（位掩码）
     *                                “在执行这个信号处理函数时，除了当前正在处理的信号外，还希望临时阻塞其他某些信号”
     * 不阻塞其他信号
     * 确保在执行期间没有其他特定的信号会被阻塞
     * 防止处理过程中被其他信号中断
     */
    sigemptyset(&sa.sa_mask);

    /*
     * 用于设置信号处理函数的行为特性。
     * 0 - 默认选项
     * SA_NODEFER - 表示在执行信号处理函数时不允许其他信号被阻塞
     * SA_RESETHAND - 在信号处理函数返回后，将信号的处理方式重置为默认
     * SA_NOCLDSTOP - 对于子进程发送的停止信号，父进程不会停止。
     * SA_NOCLDWAIT - 父进程在等待等待子进程终止时不会因为子进程发送信号而收到通知
     * SA_SIGINFO - 使用扩展的信号信息，信号处理函数接收三个参数而不是一个 
     */
    sa.sa_flags = 0;

    /* 注册信号 */
    sigaction(SIGINT, &sa, nullptr);  /* Ctrl + C */
    sigaction(SIGTERM, &sa, nullptr); /* kill 或 systemctl stop */

}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* 初始化pmc并行机器 */
int pmc_init(po::variables_map&);

/* 任务池 */
auto taskPool = std::vector<std::shared_ptr<qing::ITask>>{};

/*----------------------------------------*/

/* FIXME: 它们没有清理 
 * FIXME: 传入路径、地址 */
void try_start_i2c_and_camera() 
try {
    I2C_初始化();
    PCA9685_初始化();
    MPU6050_初始化();

	/* 初始化相机 */    
	Camera_init(640, 480);
	Camera_check();
	Camera_setVideoFormat();
	Camera_reqBuf();
	Camera_setup();
	//Camera_run();
}
catch (std::runtime_error& exp)
{
	std::cout << exp.what() << std::endl;
}

/* 入口函数 */
int main(int argc, char** argv) {

    /*------------------------------*/
    /* 设置信号行为*/
    set_signal_action();

    /*------------------------------*/
    /* 解析命令行参数 */
    po::options_description desc("pmc subsystem start options");
    desc.add_options()  /* 定义选项 */
	("help,h",    "display help message")
	("version,v", "display version message")
	("mybot",     "run mybot task");

    /* 参数变量映射关系 */
    po::variables_map vm;

    try {  /* 开始解析 */
    	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);
	if (vm.count("help")) { /* 获取帮助页面 */
	    std::cout << desc << std::endl;
	    return 0;
	}
	if (vm.count("version")) { /* 获取版本信息 */
	    std::cout << PMC_VERSION << std::endl;
	    return 0;
	}
    }

    catch (const std::exception& e) {
	std::cerr << "ERROR: " << e.what() << std::endl;
	std::cerr << "using --help to check options message" << std::endl;
	throw e;
    }

    /*----------------------------------------*/
    if (vm.count("mybot")) {
        try_start_i2c_and_camera();  /* 启动硬件模组 FIXME: 没有释放资源 */
	auto task = std::make_shared<qing::ParserTask>();
	task->start();
	taskPool.push_back(task);
    }



    /*
     * 等待
     *
     * 触发器在信号处理那里
     */
    cv.Wait();

    /* 主线程被唤醒 */
    for (auto& task: taskPool) {
        task->stop();
    }
}


namespace qing {

/* 构造函数，主要是需要传入一个http构造回调 */
ParserTask::ParserTask(): HttpTask(ADDR, PORT, [this](Http& http) {


    /* FIXME: 传入模型路径 */
    const char* target = "../public/models/1.txt";
    if (std::filesystem::exists(target)) {
        std::cout << "load model data from file..." << std::endl;
        std::ifstream file;
        file.open(target);
        nn.load(file);
	file.close();
    }
    else { /* FIXME: 单独放在一个函数里 */
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


    /* FIXME:当消息体为空时抛出异常即可 */
    http.post("/api/v1/mybot-sample", [this](const httplib::Request& req, httplib::Response& res) {
        try{
            if (req.body.empty())
	        throw NoRequestBodyException("/api/v1/mybot-sample");
            mtx.lock();
            auto r = run_infer(req.body, 0.0, 0.0, averages);  /* 推理结果无效化 */
            pool.push_back(r);
            averages = calc_aves(pool);
            mtx.unlock();
            res.set_content("OK", "text/plain");
        }
        catch (NoRequestBodyException& exp) {
            res.set_content("no request body", "text/plain");
            res.status =400;
        }
        catch (std::exception& exp) {
            res.set_content(exp.what(), "text/plain");
            res.status =400;
        }

    });

    /*  FIXME: 需要先取得模板数据 */
    http.post("/api/v1/mybot-nn", [this](const httplib::Request& req, httplib::Response& res) {
        try{
            if (req.body.empty())
                throw NoRequestBodyException("/api/v1/mybot-nn");
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
        catch (NoRequestBodyException& exp) {
            res.set_content("no request body", "text/plain");
            res.status =400;
        }
        catch (std::exception& exp) {
            res.set_content(exp.what(), "text/plain");
            res.status =400;
        }
    });

}) {}

/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/* 神经网络推理 */
std::vector<std::vector<float>> ParserTask::run_infer(std::string script, float infer_freq, float train_discount, std::vector<std::vector<float>>& averages) {
    auto parser = ActParser();
    parser.fromStr(script);
    auto bot = parser.get();
    auto res = std::vector<std::vector<float>>{};
    auto all_count = 0;
    while (true) {
        try{
            std::cout << "------------------------------"<< std::endl;
            MPU6050_传感器数据 data = MPU6050_读传感器();
            float pitch, roll;
            MPU6050_计算角度(data, pitch, roll);
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
            MPU6050_传感器数据 data1 = MPU6050_读传感器();
            float pitch1, roll1;  /* 测量执行完毕后的角度 */
            MPU6050_计算角度(data1, pitch1, roll1);
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
        catch (std::runtime_error& exp) { /* FIXME: 该用专用的异常类 */
            std::cout << exp.what() << std::endl;
            break;
        }
    }
    return res;
}
}


namespace qing {

	void HttpTask::init_thread() {

		/**/
		f_t StopEvent = [](Thread& th) {
			th.suspend();
		};

		/* 线程唤醒事件 */
		f_t WakeEvent = [this](Thread& th) {
		
			this->http=std::make_unique<Http>(http_addr, http_port);
			
			this->callback(*http);

			if (th.check() == Fsm::Stat::START) {  /* 只跑一次 */
				th.run(); /* 标记为启动 */
				http->run(); /* 这个会堵塞线程以进行监听 */
			}

			/* HTTP服务器异常关闭 */
			if (th.check() == Fsm::Stat::RUNNING) {
				th.stop();
			}


		};

		f_t LoopEvent = [](Thread& _) {
			throw std::runtime_error("Task  not support this Event");
		};

		f_t ClearEvent = [this](Thread& _) {
			if (this->http)
				this->http.reset(); /*  */
		};

		th = std::make_unique<Thread> (StopEvent, WakeEvent, LoopEvent, ClearEvent);
	}

}
