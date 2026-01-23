/*
 *  并发机器（进程托管）
 *  
 *  FIXME: 尽量使用大众的库（boost）
 */
#include <filesystem>
#include <string> 
#include <vector>
#include <csignal> /* 注册信号以优雅退出 */
#include <functional>
#include <boost/program_options.hpp>
#include "net/Http.hpp"
#include "logs/Logger.hpp"
#include "th/Thread.hpp" /* 可控线程类 */
#include "th/Cv_wait.hpp" /* 通过条件变量进行等待 */
#include "pmc/Modules.hpp" /* 管理子系统模块 */
#include "pmc/PmcTask.hpp"
#include "pmc/LciTask.hpp"
#include "pmc/ParserTask.hpp"

/* lci 临时 */
#include "lci/p_th.h"

/* 中文模式 Chinese Mode */
#include "cn/中文化.hpp"
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
namespace po = boost::program_options;

/* 版本号 */
static const char *PMC_VERSION = "0.3.x";
/* 条件变量的等待机制让主程序能够等待中断信号的产生 */
qing::Cv_wait cv =  qing::Cv_wait();



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
int pmc_init(po::variables_map&);

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
	("pmc",       "run pmc task")
	("lci",       "run lci task")
	("mybot",     "run mybot task")
	/*****************/
	("sys",  po::value<std::string>(), "subsystem root dirent path")
	("key",  po::value<std::string>(), "subsystem public key filepath")
	("addr", po::value<std::string>(), "subsystem listen ip address")
	("port", po::value<int>(), "subsystem listen TCP port")
	/*****************/
	("exec",     po::value<std::string>(), "lci module run commandline")
	("rotate",   po::value<int>()->default_value(0),  "lci module rotate mode(0~3)")
	("fontsize", po::value<int>()->default_value(18), "lci module fontsize option");

    /* 参数变量映射关系 */
    po::variables_map vm;

    try {  /* 开始解析 */
    	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);
	if (vm.count("help")) {
	    std::cout << desc << std::endl;
	    return 0;
	}
	if (vm.count("version")) {
	    std::cout << PMC_VERSION << std::endl;
	    return 0;
	}
    }

    catch (const std::exception& e) {
	std::cerr << "ERROR: " << e.what() << std::endl;
	std::cerr << "using --help to check options message" << std::endl;
	throw e;
    }


    /* pmc初始化函数 */
    return pmc_init(vm);
}


void ensure_keys()  /* 如果公钥私钥不存在，就重新生成一份 */
{
        if (!std::filesystem::exists("./public_key.pem") || !std::filesystem::exists("./private_key.pem"))
	{
            auto rsa_keypair = qing::MyRSA::Generator();
            rsa_keypair.save_pri_key("./private_key.pem");  
            rsa_keypair.save_pub_key("./public_key.pem");

            /* 以下是测试部分 */
            //auto rsa_pri = qing::MyRSA::Private_Key("./private_key.pem");
            //auto rsa_pub = qing::MyRSA::Public_Key("./public_key.pem");

            //std::string hex = rsa_pub.Encrypt("你好");
            //std::cerr << rsa_pri.Decrypt(hex) << '.' << std::endl;	
        }
}

void try_start_i2c_modules() 
try {
        I2C_init();
        PCA9685_init();
        MPU6050_init();
}
catch (std::runtime_error& exp)
{
	std::cout << exp.what() << std::endl;
}

void try_start_camera_thread()
try {
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


/* 初始化PMC并发机器 */
int pmc_init(po::variables_map& vm) {


    /* 初始化日志模块 */
    qing::Logger::getInstance().init(LogLevel::DEBUG, true);


    /* 启动硬件模组 */
    try_start_i2c_modules();
    try_start_camera_thread();


    /* 初始化OpenSSL */
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    int masterfd; /* 控制台文件描述符 */
    /* 在运行到多线程之前，进行fork() */
    if ((masterfd = posix_openpt(O_RDWR)) == -1) { /* 打开一个新的虚拟控制台 */
        throw std::runtime_error("posix_openpt");
    }

    /*--------------------------------------------------*/
    /* 创建两个管道：一个用于发送命令，一个用于接收输出 */
    int command_pipe[2]; /* [0] 是读端，[1] 是写端 */
    if (pipe(command_pipe) == -1)
        throw std::runtime_error(
            "pipe:" + std::string(strerror(errno))
        );
    /* 创建成功 */
    /*--------------------------------------------------*/

    /* 任务池 */
    auto taskPool = std::vector<std::shared_ptr<qing::ITask>>{};

    /*----------------------------------------*/
    /* LINUX CHINESE INTERF */
    if (vm.count("lci")) {

        /* FIXME: 这个分支与别的分支不能共存，因为主线程需要等待子进程终止 */
    
        if (!(vm.count("rotate") && vm.count("fontsize") && vm.count("exec"))) {
            throw std::runtime_error("您进入了lci模块，请输入以下选项 --rotate， --fontsize， --exec");
        }


	if (grantpt(masterfd) != 0 || unlockpt(masterfd) != 0) { /* 解锁虚拟控制台？ */
		close(masterfd);
		throw std::runtime_error("grantpt/unlockpt");  /* FIXME: 输出错误字符串 */
	}

	char *slave_name = ptsname(masterfd);  /* 获取虚拟控制台pty的名字 */
	if (slave_name == NULL) {
		close(masterfd);
		throw std::runtime_error("ptsname");
	}

	LOG_INFO(
		std::string("新的虚拟终端运行在 ") + slave_name
	);



	
	/* Fork中间子进程 */
	pid_t pid = fork();

	/* pid为-1通常意味着分叉失败 */
	if (pid == -1)
		throw std::runtime_error(
			std::string("fork") + std::string(strerror(errno))
		);

	/* 如果pid为0表示这是子进程 */
	else if (pid == 0) {
		LOG_INFO("LCI subprocess is running...");
		close(command_pipe[1]); /* 关闭管道写端 */
		int slavefd = open(slave_name, O_RDWR);
		if (slavefd == -1)
			throw std::runtime_error(
				std::string("open slave pty") + std::string(strerror(errno))
			);

		//runIntermediateProcess(command_pipe[0], slavefd);
		int command_pipe_read = command_pipe[0];
		char buffer[1024];
		ssize_t nread;
		while (true) {
			memset(buffer, 0, sizeof(buffer)); /* 清空缓冲区 */
		    nread = read(command_pipe_read, buffer, sizeof(buffer) - 1);	/* 从命令管道读取数据 */
			if (nread <= 0) break;  /* 没有数据可读，退出循环 */

			/* 查找命令的结束位置 */
			/* FIXME: 更换结束符号，这样我们可以运行一整个脚本 */
			char* end_of_cmd = strchr(buffer, '\n');
			if (end_of_cmd != nullptr)
				*end_of_cmd = '\0';
			else
				continue; /* 没找到结束符号，继续读取 */

			/* 检查是否受到结束信号 */
			if (strcmp(buffer, "END") == 0)
				break;

			/* 输入输出重定向 */
			if (dup2(slavefd, STDIN_FILENO) != STDIN_FILENO
			|| dup2(slavefd, STDOUT_FILENO) != STDOUT_FILENO
			|| dup2(slavefd, STDERR_FILENO) != STDERR_FILENO)
				throw std::runtime_error(
					std::string("dup2") // +
				);

			/* ？*/
			if (slavefd > STDERR_FILENO)
				close(slavefd);

			/* 执行命令 */
			execlp("/bin/sh", "/bin/sh", "-c", buffer, nullptr);

			/* 如果能运行到这里表示进程切换异常 */
			throw std::runtime_error(
				std::string("execlp") // + 
			);

		}

		//close(command_pipe[0]);
		//_exit(0);
	}

	/* 返回其他正数表示这是主进程 */
	else {

		LOG_INFO("LCI main thread is running...");



		auto lciTask = std::make_shared<qing::LciTask>(
			vm["exec"].as<std::string>(),
			masterfd, command_pipe[1],
			vm["rotate"].as<int>(),
			vm["fontsize"].as<int>()
		);
		lciTask->start();
	        taskPool.push_back(lciTask);
	

		/* 不等待中间子进程终止 */
		//waitpid(pid, nullptr, 0);

	}
    
    }


    /*----------------------------------------*/
    else if (vm.count("mybot")) {
    	auto task = std::make_shared<qing::ParserTask>();
	task->start();
	taskPool.push_back(task);
    }
    /*----------------------------------------*/
    /* 运行配置分支 */
    else if (vm.count("pmc")) {

        if (!(vm.count("addr") && vm.count("port") && vm.count("sys") && vm.count("key")))  {
            throw std::runtime_error("请输入下列参数： --addr, --port, --sys, --key");
        }

	ensure_keys();

        /*-------------------*/
        /* 创建PMC */
        auto pmcTask = std::make_shared<qing::PmcTask>(
            vm["sys"].as<std::string>(),
            vm["key"].as<std::string>(),
            vm["addr"].as<std::string>(),
            vm["port"].as<int>()
        );
        pmcTask->start();
	taskPool.push_back(pmcTask);
        /*-------------------*/

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

    /* 将管道移动到分支外面 */
    close(command_pipe[0]); // 关闭读端
    close(command_pipe[1]);

    close(masterfd);  /* 关闭虚拟终端 */

    /* 清理 OpenSSL 资源 */
    EVP_cleanup();
    ERR_free_strings();
    return 0;

}
