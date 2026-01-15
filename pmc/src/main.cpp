/*
 *  并发机器（进程托管）
 */
#include <filesystem>
#include <string> 
#include <vector>
#include <csignal> /* 注册信号以优雅退出 */
#include <functional>
#include <args.hxx> /* 处理命令行参数的包 */
#include <nlohmann/json.hpp>  /* 进行json的处理 */
#include "Config.hpp" /* 读取配置 */
//#include "Serial.hpp" /* 与串口设备进行联络（暂时不用）*/ 
#include "Http.hpp"
#include "Logger.hpp"
#include "Thread.hpp" /* 可控线程类 */
//#include "ProcessList.hpp"
//#include "ThreadList.hpp"
#include "Cv_wait.hpp" /* 通过条件变量进行等待 */
#include "Modules.hpp" /* 管理子系统模块 */
#include "PmcTask.hpp"
#include "LciTask.hpp"
#include "ParserServerTask.hpp"

/* lci 临时 */
#include "lci/p_th.h"

/* 中文模式 Chinese Mode */
#include "cn/中文化.hpp"
/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/


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
/* 入口函数 */
int main(int argc, char** argv) {

    /*------------------------------*/
    /* 设置信号行为*/
    set_signal_action();

    /*------------------------------*/
    /* 解析命令行参数 */
    
    args::ArgumentParser parser("这是一个脚本托管服务。", "请使用专门的管理程序进行操作。");
    args::HelpFlag help(parser, "HELP", "显示帮助信息", {'h', "help"});
    args::Flag rsaGen(parser, "RSA GEN", "生成RSA密钥", {'g', "rsa"});
    args::Flag pmcFlag(parser, "run-Pmc-Task", "启动pmc服务", {"pmc"});
    args::Flag lciFlag(parser, "run-Lci-Tast", "启动lci服务", {"lci"});
    args::Flag mybotFlag(parser, "mybot-parser-Task", "启动mybot模块", {"mybot"});
    /***************************/
    args::ValueFlag<std::string> sys(parser, "SUBSYSTEM", "输入自启动脚本的地址", {'s', "sys"});
    args::ValueFlag<std::string> key(parser, "KEY_PATH", "输入公钥的地址", {'k', "key"});
    args::ValueFlag<std::string> addr(parser, "ADDRESS", "输入pmc监听地址", {'a', "addr"});
    args::ValueFlag<int> port(parser, "PORT", "输入pmc监听端口", {'p', "port"});
    /***************************/
    args::ValueFlag<int> rotate(parser, "ROTATE", "输入旋转模式（0~3）", {'r', "rotate"}, 0);
    args::ValueFlag<int> fontsize(parser, "FONTSIZE", "输入字号大小", {'f', "fontsize"}, 18);
    args::ValueFlag<std::string> exec(parser, "EXEC_CMD", "输入运行的命令", {'e', "exec"});


    try { parser.ParseCLI(argc, argv); } /* 解析 */
    
    catch (const args::Help&) /* 显示帮助 */
    {
        std::cout << parser;
        return 0;
    }

    /* 解析失败 */
    catch (const args::ParseError& e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }
    
    /* 验证失败？ */
    catch (const args::ValidationError& e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }






    /*------------------------------*/
    /* 初始化日志模块 */
    qing::Logger::getInstance().init(LogLevel::DEBUG, true);


    try
    {
        I2C_init();
        PCA9685_init();
        MPU6050_init();
    }
    catch (std::runtime_error& exp)
    {
	std::cout << exp.what() << std::endl;
    }

    try{
	/* 初始化相机 */    
	Camera_init(640, 480);
	Camera_check();
	Camera_setVideoFormat();
	Camera_reqBuf();
	Camera_setup();
	Camera_run();
    }
    catch (std::runtime_error& exp)
    {
        std::cout << exp.what() << std::endl;
    }

    /* 初始化OpenSSL */
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();


    /* 任务池 */
    auto taskPool = std::vector<std::shared_ptr<qing::ITask>>{};

    /*----------------------------------------*/
    /* LINUX CHINESE INTERF */
    if (lciFlag) {
    
        if (!(rotate && fontsize && exec)) {
            throw std::runtime_error("您进入了lci模块，请输入以下选项 --rotate， --fontsize， --exec");
        }

	/* 在运行到多线程之前，进行fork() */
	int masterfd; /* 控制台文件描述符 */
	if ((masterfd = posix_openpt(O_RDWR)) == -1) { /* 打开一个新的虚拟控制台 */
		throw std::runtime_error("posix_openpt");
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


	/*--------------------------------------------------*/
	/* 创建两个管道：一个用于发送命令，一个用于接收输出 */
	int command_pipe[2]; /* [0] 是读端，[1] 是写端 */
	if (pipe(command_pipe) == -1)
		throw std::runtime_error(
			"pipe:" + std::string(strerror(errno))
		);
	/*--------------------------------------------------*/
	/* 创建成功 */

	
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
		close(command_pipe[0]); // 关闭读端



		auto lciTask = std::make_shared<qing::LciTask>(
			args::get(exec),
			masterfd, command_pipe[1],
			args::get(rotate),
			args::get(fontsize)
		);
		lciTask->start();
	        taskPool.push_back(lciTask);
	

		/* 等待中间子进程终止 */
		waitpid(pid, nullptr, 0);

		/* 关闭管道 */
		close(command_pipe[1]);
		close(masterfd);

	}
    
    }


    /*----------------------------------------*/
    /* 运行 生成分支 */
    else if (rsaGen)
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

    else if (mybotFlag) {
    	auto task = std::make_shared<qing::ParserServerTask>();
	task->start();
	taskPool.push_back(task);
    }
    /*----------------------------------------*/
    /* 运行配置分支 */
    else if (pmcFlag) {
        if (!(addr && port && sys && key))  {
            throw std::runtime_error("请输入下列参数： --addr, --port, --sys, --key");
        }

        /*-------------------*/
        /* 创建PMC */
        auto pmcTask = std::make_shared<PmcTask>(
            args::get(sys),
            args::get(key),
            args::get(addr),
            args::get(port)
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


    /* 清理 OpenSSL 资源 */
    EVP_cleanup();
    ERR_free_strings();
    return 0;

}
