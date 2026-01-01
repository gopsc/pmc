/*
 * 版本：0.0.1 （已经废弃）
 * 协处理器通信服务（COPS）
 * 作者：qing。邮箱：1776177662@qq.com
 *
 *
 * 该程序运行于Linux系统，它维护一个串口设备文件，并且开启HTTP服务，对其进行调用
 * 需要COPS专用的下位机，程序才可以正常运行。
 *
 * COPS 通信方法：上位机发送一个数据，由下位机接收并且响应。如此反复。
 *
 *
 * 在某些情况下，我们可能还需要反转通信流程，创造能够从单片机接收刺激的服务。
 *
 * 在某些情况下，可能需要扫描所有的串口设备，以确定哪个是COPS。
 *
 *
 * 使用方法：
 *
 * ./copsd {--input {input}} {--print}
 *
 *
 * ---------------------------------------------------------------------------
 * ---------------------------------------------------------------------------
 *
 *  版本：0.0.2
 *  PMC 并发管理器
 *  作者：qing，邮箱：1776177662@qq.com
 *
 *  该程序能够创建进程并且执行指令。通过开启的HTTP服务进行线程的执行
 *
 *  并且能够管理创建的进程
 *
 *
 *  使用方法：
 *
 *  ./copsd -i config.json -t init.sh
 * 
 * ---------------------------------------------------------------------------
 * ---------------------------------------------------------------------------
 * 
 * 版本：0.0.3（20250525）
 * 名称：PMC 并发管理器
 * 
 * 该程序的核心线程模块中已经去除了cops.server的内容，还增加了杀死子进程的调用
 *
 * ---------------------------------------------------------------------------
 * ---------------------------------------------------------------------------
 * 
 * 版本：0.0.4（20250614）
 * 名称：PMC 并发管理器
 * 
 * 也许尝试使用管道 + 进程锁实现子系统调用 （未实现）
 *
 *
 * --------------------
 * --------------------
 *
 * 版本：0.0.5（20250629）
 * 名称：大顺脚本
 *
 * 让子系统能够自行重启 （未实现）
 *
 *
 * 版本： 0.0.6（20250708）
 * 名称：Dashun
 *
 * 让子系统具备加密特性，只有拥有私钥的人才能控制
 */

#include "Config.hpp" // 读取并且封装配置文件
#include "Serial.hpp" // 与串口设备进行联络（暂时不用）
#include "Http.hpp" // http服务器类
#include "Logger.hpp" // 日志类
#include "Thread.hpp" // 线程类
#include "ProcessList.hpp" // 进程列表类
#include "ThreadList.hpp" // 线程列表类
#include "Cv_wait.hpp" // 通过条件变量进行等待的类
#include "Modules.hpp" // 管理子系统模块的
#include "Crypto/AES_CBC.hpp" // AES_CBC加密方式
#include "Crypto/MyRSA.hpp" // RSA加密方式
#include <stdexcept> // 标准库异常
#include <filesystem> // 文件系统操作
#include <string> 
#include <vector>
#include <csignal> // 注册信号以优雅退出
#include <args.hxx> // libargs-dev
                    // 处理命令行参数的包
#include <nlohmann/json.hpp>    // nlohmann-json3-dev
                                // 进行json的处理
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
//全局变量


// 该列表采用的是自己编写的qing::Thread作为容纳元素，qing::Thread是一个可控线程，可以对线程的运行进行控制
//
// 使用了线程列表后，每个线程都能在项目关闭时显式退出
qing::ThreadList thlist{}; 


// 条件变量的等待机制让主程序能够等待中断信号的产生
qing::Cv_wait cv =  qing::Cv_wait();


// AES-CBC 加密模块，用于加密通信消息
//
// AES加密的速度快
std::unique_ptr<qing::AES_CBC> aes; 


// RSA非对称加密
//
// 用于AES通信密钥的分发
//
//
// 这个密钥我们将会在启动后手动从文件系统中加载
std::unique_ptr<qing::MyRSA::Public_Key> rsa_pub; 


// AES密钥，用于消息的加密
//
// 在程序开始时随机生成
std::string crypted_aes_key;


// AES初始向量，用于消息的加密
//
// 在程序开始时设置 
std::string crypted_aes_iv;


//----------------------------------------------
//----------------------------------------------
// 并发器线程，能够监听Http请求，并且根据指令创建、管理子进程
//
// 它继承自我手搓的qing::Thread可控制线程类，能够设置线程运行期间的事件函数，实现线程自主退出
//
//
class PmcTh: public qing::Thread {
    public:

        // 优化建议：将set函数的内容放置到这里
        PmcTh(std::string& addr, int& port, std::string& sys_name) {
            this->http_addr = addr;
            this->http_port = port;
            this->name = sys_name;
        };

        // 因为子进程被复制容易出问题，这里不允许复制
        PmcTh(const PmcTh&) = delete;

        ////////////////////////////////////////
        ////////////////////////////////////////
        // 这四个重写的控制方法可以控制线程的状态，分别是停止、启动、运行、关闭
        //
        //这四个控制方法，调用时如果http正在运行，则会被关闭
        //如果是在运行态再次运行，这样http就会死亡，会退出运行
        // 会形成死循环， 故在run()中不关闭http服务器
        //


        /* 停止线程 */
        void stop() override {
            if (http) http->stop();
            qing::Thread::stop();
        }

        /* 唤醒线程 */
        void wake() override {
            if (http) http->stop();
            qing::Thread::wake();
        }

        /* 进入运行态 */
        void run() override {
            //if (http) http->stop();
            qing::Thread::run();
	    }

        /* 结束线程 */
        void shut() override {
            if (http) http->stop();
            qing::Thread::shut();
        }

        ///////////////////////////////////////
        ///////////////////////////////////////
        // 四大事件函数，管理线程的行为

        // 静止事件
        //
        // 优化方案：suspend() 能否正常运行，还需要进行实际的测试
        virtual void StopEvent() override {
            suspend();
        }

        // 唤醒事件（获取资源）
        virtual void WakeEvent() override {
                


            // 这个try乍看之下没有用，但是目前也可以用来做调试
            //
            try {
                http = std::make_unique<qing::Http> (http_addr, http_port);
                mods = std::make_unique<qing::Modules> (this->name);
            } catch (qing::Modules::SubsystemNotExist exp) {
                //std::cout << exp.what() << std::endl;
                throw exp;
            }

            /* clear Dead process */
            http->post("/api/v1/clear", [this](const httplib::Request& req, httplib::Response& res) {
                try {
                    pl.check_and_clear();
                    res.set_content("OK", "text/plain");
                } catch (std::exception& e) {
                    res.set_content(e.what(), "text/plain");
                }
            });

            //-----------------------------------------------
            //-----------------------------------------------
    	    /*
        	 * 获取经过RSA加密的AES密钥的API
             * 例子 ： curl -X GET http://localhost:8012/api/v1/get_key
             */
            http->get("/api/v1/get_key", [] (const httplib::Request& req, httplib::Response& res) {
                res.status = 200;
                res.set_content(crypted_aes_key, "text/plain");
            });

            /*
             * 获取经过RSA加密的AES初始向量的API
             * 例子：curl -X GET http://localhost:8012/api/v1/get_iv
             */
            http->get("/api/v1/get_iv", [] (const httplib::Request& req, httplib::Response& res) {
                res.status = 200;
                res.set_content(crypted_aes_iv, "text/plain");
            });
	    
            /* list all process you ran
            * 列举出所有进程，使用AES加密
            */
            http->get("/api/v1/get_list", [this](const httplib::Request& req, httplib::Response& res) {
                std::string list= pl.print();
                std::string msg = aes->encrypt(list);
                res.set_content(msg, "text/plain");
            });

            /* 启动模块 使用AES加密
             * 例子：curl -X POST http://localhost:8009/api/v1/start -d '{"module": <name>}'
             */
            http->post("/api/v1/start", [this](const httplib::Request& req, httplib::Response& res) {
                try {
                    std::string params = aes->decrypt(req.body); // 解密消息体
                    auto json = nlohmann::json::parse(params); // 对明文进行json解析
                    std::string mod = json["module"];
                    std::string cmd = mods->GetInitp(mod); // 获取启动脚本
                    pl.execute(cmd, std::vector<std::string>()); // 执行
                    LOG_INFO(std::string("PMC-START was called by ") + req.remote_addr);
                    res.status = 200;
                    res.set_content("OK", "text/plain");
                } catch (std::exception &e) {
                    res.status = 400;
                    res.set_content(e.what(), "text/plain");
                }
            });

            /*
             * 杀死一个进程，使用AES加密参数
             * 示例：curl -X POST http://localhost:8012/api/v1/kill -d '{"pid": <int>}'
             */
            http->post("/api/v1/kill", [this](const httplib::Request& req, httplib::Response& res) {
                try {
                    std::string msg = aes->decrypt(req.body);
                    nlohmann::json json = nlohmann::json::parse(msg);
                    int pid = json["pid"];
                    pl.kill(pid);
                    res.status = 200;
                    res.set_content("OK", "text/plain");
                } catch (std::exception &e) {
                    res.status = 500;
                    res.set_content(e.what(), "text/plain");
                }
            });


            /* 
             * 列举出安装的所有模块，使用AES加密
             * 例子：curl -X GET http://localhost:8012/api/v1/list_mods 
             */
            http->get("/api/v1/list_mods", [this](const httplib::Request& req, httplib::Response& res) {
                std::string list = "";
                for (std::string &name: mods->ListAll()) {
                    list += (list == "") ? "" : "\n";
                    list += name;
                }
                std::string encrypted_msg = aes->encrypt(list);
                res.status = 200;
                res.set_content(encrypted_msg, "text/plain");
            });
            
            /* 创建模块的API（现在消息体需要加密） 例子：
              curl -X POST http://localhost:8012/api/v1/crt_mod -d '{"module": "hello"}'
            */
            http->post("/api/v1/crt_mod", [this] (const httplib::Request& req, httplib::Response& res) {
                LOG_INFO(std::string("PMC.crt_mod was called by ") + req.remote_addr);
                std::string module;
                try {
                    std::string jmsg = aes->decrypt(req.body);
                    nlohmann::json json = nlohmann::json::parse(jmsg);
                    module = json["module"];
                    mods->CrtMod(module);
                    res.status = 200;
                    res.set_content("OK", "text/plain");
                } catch (qing::Crypto_Basic::CryptoExp& e) {
                    res.status = 400;
                    res.set_content("加密失败", "text/plain");
                //} catch ( ...json_parser ) {
                } catch (qing::Modules::Mods_Exp& e) {
                    res.status = 400;
                    res.set_content("模块创建失败", "text/plain");
                } catch (std::exception& e) {
                    res.status = 500;
                    res.set_content(e.what(), "text/plain");
                }
            });
            
            /* 创建模块的API（现在需要加密消息体） 例子：
              curl -X POST http://localhost:8012/api/v1/del_mod -d '{"module": "hello"}'
            */
            http->post("/api/v1/del_mod", [this] (const httplib::Request& req, httplib::Response& res) {
                LOG_INFO(std::string("PMC.del_mod was called by ") + req.remote_addr);
                std::string module;
                try {
                    std::string jmsg = aes->decrypt(req.body);
                    nlohmann::json json = nlohmann::json::parse(req.body);
                    module = json["module"];
                    mods->DelMod(module);
                    res.status = 200;
                    res.set_content("OK", "text/plain");
                } catch (qing::Crypto_Basic::CryptoExp& e) {
                    res.status = 400;
                    res.set_content("加密失败", "text/plain");
                } catch (qing::Modules::Mods_Exp& e) {
                    res.status = 400;
                    res.set_content(e.what(), "text/plain");
                } catch (std::exception& e) {
                    res.status = 500;
                    res.set_content(e.what(), "text/plain");
                }
            });


            /* 打印地址 */
            LOG_INFO(http->printAddr());

            // 开始运行，只运行一次
            if (check()==qing::Fsm::Stat::START) {
                run();
                
                //------------------------'
                // 先从子系统类中获取引导模块的启动脚本位置
                // 然后以此路径创建子进程
                pl.execute(mods->GetInitp("_init"), std::vector<std::string>{});
                http->run();
                //------------------------

            }

            // 如果是处于弹出状态，则暂停线程
            if (check()==qing::Fsm::Stat::RUNNING) {
                stop();
            }

        }

        // 循环事件（在该类中废弃）
        virtual void LoopEvent() override {
            ;
        }

        // 清理事件
        virtual void ClearEvent() override {

            if(http) {

                LOG_INFO("Clear Pmc");

                // 重置http服务器的指针托管器
                http.reset();

                // 关闭所有的进程
                pl.CloseAll();

            }
            
        }
    
    private:

        // http监听地址
        std::string http_addr = "";

        // http监听端口
        int http_port = 0;  

        // 子系统名 
        std::string name = ""; 

        // 进程列表对象
        qing::ProcessList pl{};

        // http服务器对象
        std::unique_ptr<qing::Http> http; 

        // 子系统对象
        std::unique_ptr<qing::Modules> mods;

        //-----------------------------------------------------------------------
        // 下面三个静态参数似乎没有用上
        //
        // 检查消息的参数
        //static void check_params(std::string& call, std::string& path) {
        //    if (call == "" || call.size() >= 128  || path == "" || path.size() >= 1024) {
        //            throw std::runtime_error("参数的长度不合适。");
        //        }
        //}
        
        // 检查消息的参数
        //static void check_params(std::string& path) {
        //    if (path == "" || path.size() >= 1024) {
        //            throw std::runtime_error("参数的长度不合适。");
        //        }
        //}

        // 检查消息的请求头
        //static void check_headers(const httplib::Request& req) {
        //        if (!req.has_header("Content-Type") || req.get_header_value("Content-Type") != "application/json") {
        //            throw std::runtime_error("不合法的请求头。");
        //        }
        //}

        // 检查json数组
        //static void check_json_array(nlohmann::json& json) {
        //        if (!json.is_array()) {
        //            throw std::runtime_error("我们需要一个json数组");
        //        }
        //}

};


// 捕获中断信号，结束线程并且退出
void signalHandler(int signum) {

    LOG_INFO("收到终止信号，准备停止服务器...");

    // 唤醒条件信号即关闭程序
    cv.WakeCv();

}

//--SIGNAL ACTION--
// 对信号的响应行为
void set_signal_action() {

    // 该结构体用于描述信号的处理方式
    struct sigaction sa;


    //绑定处理函数
    sa.sa_handler = signalHandler;
    
    sigemptyset(&sa.sa_mask);   // 清空并初始化一个信号掩码集  --------------> 用于指定哪些信号在当前或即将执行的信号处理函数期间应该被阻塞（位掩码）
                                //                                “在执行这个信号处理函数时，除了当前正在处理的信号外，还希望临时阻塞其他某些信号”
                                // 不阻塞其他信号
                                // 确保在执行期间没有其他特定的信号会被阻塞
                                // 防止处理过程中被其他信号中断

    sa.sa_flags = 0;    // 用于设置信号处理函数的行为特性。
                        // 0 - 默认选项
                        // SA_NODEFER - 表示在执行信号处理函数时不允许其他信号被阻塞
                        // SA_RESETHAND - 在信号处理函数返回后，将信号的处理方式重置为默认
                        // SA_NOCLDSTOP - 对于子进程发送的停止信号，父进程不会停止。
                        // SA_NOCLDWAIT - 父进程在等待等待子进程终止时不会因为子进程发送信号而收到通知
                        // SA_SIGINFO - 使用扩展的信号信息，信号处理函数接收三个参数而不是一个 

    //注册信号
    sigaction(SIGINT, &sa, nullptr); // Ctrl + C
    sigaction(SIGTERM, &sa, nullptr); // kill 或 systemctl stop

}

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
// 入口
int main(int argc, char** argv) {

    //------------------------------
    // 设置信号行为
    set_signal_action();

    //------------------------------
    // 解析命令行参数
    //
    // 优化方案：这里有一个“大工程”，即将所有的配置都做成命令行参数的形式
    //
    //
    // 创建参数解析器
    args::ArgumentParser parser("这是一个脚本托管服务。", "请使用专门的管理程序进行操作。");


    // 显示帮助信息的标志
    args::HelpFlag help(parser, "HELP", "显示帮助信息", {'h', "help"});

    // 生成RSA密钥对的标志
    args::Flag rsaGen(parser, "RSA GEN", "生成RSA密钥", {'g', "rsa"});

    // 输入pmc监听地址
    args::ValueFlag<std::string> addr(parser, "ADDRESS", "输入pmc监听地址", {'a', "addr"});

    // 输入pmc监听端口
    args::ValueFlag<int> port(parser, "PORT", "输入pmc监听端口", {'p', "port"});
    
    // 输入子系统的路径
    args::ValueFlag<std::string> sys(parser, "SUBSYSTEM", "输入自启动脚本的地址", {'s', "sys"});

    // 解析
    try {
        parser.ParseCLI(argc, argv);
    } catch (const args::Help&) {
        std::cout << parser;
        return 0;
    } catch (const args::ParseError& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    } catch (const args::ValidationError& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }


    //------------------------------
    /* 初始化日志模块 */
    qing::Logger::getInstance().init(LogLevel::DEBUG, true);







    //----------------------------------------
    // 运行 生成分支
    if (rsaGen) {
        auto rsa_keypair = qing::MyRSA::Generator();
        rsa_keypair.save_pri_key("./private_key.pem");  
        rsa_keypair.save_pub_key("./public_key.pem");

        // 以下是测试部分
        //auto rsa_pri = qing::MyRSA::Private_Key("./private_key.pem");
        //auto rsa_pub = qing::MyRSA::Public_Key("./public_key.pem");

        //std::string hex = rsa_pub.Encrypt("你好");
        //std::cerr << rsa_pri.Decrypt(hex) << '.' << std::endl;	

        return 0;
    }

    //----------------------------------------
    // 运行配置分支
    else if (addr && port && sys) {

        // 初始化OpenSSL
        OpenSSL_add_all_algorithms();
        ERR_load_crypto_strings();

        std::string addr_str = args::get(addr);
        int port_int = args::get(port);
        std::string sys_path = args::get(sys);

        // 初始化AES对象
        aes = std::make_unique<qing::AES_CBC>();
        //std::cerr << aes->get_key() << std::endl;
        //std::cerr << aes->get_iv() << std::endl;
        //std::string cipher = aes->encrypt(std::string("你好"));
        //std::cerr<< cipher << std::endl;
        //std::cerr << aes->decrypt(cipher) << '\'' << std::endl;

        // 初始化RSA对象
        std::string dir = std::filesystem::canonical("/proc/self/exe").parent_path().string();
        char sep = std::filesystem::path::preferred_separator;
        rsa_pub = std::make_unique<qing::MyRSA::Public_Key>(qing::MyRSA::Public_Key(dir + sep + "public_key.pem"));

        // 初始化crypted_aes_key和crypted_aes_iv
        crypted_aes_key = rsa_pub->Encrypt(aes->get_key());
        crypted_aes_iv = rsa_pub->Encrypt(aes->get_iv());
        //std::cerr << crypted_aes_key << std::endl;
        //std::cerr << crypted_aes_iv << std::endl;

        // 创建PMC
        auto pmc = std::make_shared<PmcTh>(addr_str, port_int, sys_path);

        // 向线程列表中加入pmc
        thlist.AddTh(pmc);

        //启动线程
        //
        // 在类中不是说该方法不安全么
        thlist.StartTh();

        // 等待
        //
        // 唤醒分支在信号处理那里
        cv.Wait();

        // 主线程被唤醒
        //
        // 关闭所有线程，准备退出
        thlist.CloseAll();

        // 清理 OpenSSL 资源
        EVP_cleanup();
        ERR_free_strings();

    }

    return 0;
}
