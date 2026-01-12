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
    args::ValueFlag<std::string> sys(parser, "SUBSYSTEM", "输入自启动脚本的地址", {'s', "sys"});
    args::ValueFlag<std::string> key(parser, "KEY_PATH", "输入公钥的地址", {'k', "key"});
    args::ValueFlag<std::string> addr(parser, "ADDRESS", "输入pmc监听地址", {'a', "addr"});
    args::ValueFlag<int> port(parser, "PORT", "输入pmc监听端口", {'p', "port"});

    try {   /* 解析 */
        parser.ParseCLI(argc, argv);
    }
    
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

    /* 初始化OpenSSL */
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();


    /*----------------------------------------*/
    /* 运行 生成分支 */
    if (rsaGen)
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

    /*----------------------------------------*/
    /* 运行配置分支 */
    else if (addr && port && sys && key)
    {
        /*-------------------*/
        /* 创建PMC */
        auto pmcTask = std::make_shared<PmcTask>(
            args::get(sys),
            args::get(key),
            args::get(addr),
            args::get(port)
        );
        pmcTask->start();
        /*-------------------*/

        /* 
         * 等待
         *
         * 触发器在信号处理那里
         */
        cv.Wait();

        /* 主线程被唤醒 */
        pmcTask->stop();

    }

    /* 清理 OpenSSL 资源 */
    EVP_cleanup();
    ERR_free_strings();
    return 0;
}
