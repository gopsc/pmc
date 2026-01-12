#include <vector>
#include <stdexcept> /* 标准库异常 */
#include "ITask.hpp"
#include "Thread.hpp"
#include "ProcessTask.hpp"
#include "Crypto/AES_CBC.hpp"
#include "Crypto/MyRSA.hpp"


/*----------------------------------------------*/
/*----------------------------------------------*/
/*
 * 并发器任务，能够监听Http请求，并且根据指令创建、管理子进程
 */
class PmcTask: public qing::ITask {
    public:

        PmcTask(
            const std::string& sys_name, const std::string& key_path,
            const std::string& addr, const int& port
        ): http_addr(addr), http_port(port), name(sys_name)
        {
	    init_key(key_path);
            init_thread();
        };

        /* 容易出问题 */
        PmcTask(const PmcTask&) = delete;

        /* 关闭线程 */
        ~PmcTask() {
            if (th)
                th->WaitClose();
        }

        ////////////////////////////////////////
        ////////////////////////////////////////
        /*
         * 这四个重写的控制方法可以控制线程的状态，分别是停止、启动、运行、关闭
         *
         * 这四个控制方法，调用时如果http正在运行，则会被关闭
         *
         * 如果是在运行态再次运行，这样http就会死亡，会退出运行
         *
         * 会形成死循环， 故在run()中不关闭http服务器
         */


        /* 停止线程 */
        void stop() override {
            if (http) http->stop();
            th->WaitClose();
        }

        /* 唤醒线程 */
        void start() override{
            th->Activate(); /* 对于已经关闭的进程 */
            th->WaitStart();
        }

        /* 线程是否在运行 */
        bool isRunning() override {
            auto stat = th->check();	
            return stat == qing::Fsm::Stat::START 
                || stat == qing::Fsm::Stat::RUNNING;
        }

        ///////////////////////////////////////
        inline void init_key(const std::string& key_path) {

            /* 初始化AES对象 */
            aes = std::make_unique<qing::AES_CBC>();
            //std::cerr << aes->get_key() << std::endl;
            //std::cerr << aes->get_iv() << std::endl;
            //std::string cipher = aes->encrypt(std::string("你好"));
            //std::cerr<< cipher << std::endl;
            //std::cerr << aes->decrypt(cipher) << '\'' << std::endl;
 
            /* 初始化RSA对象 */
            //std::string dir = std::filesystem::canonical("/proc/self/exe").parent_path().string();
            //std::string dir = "."; /* 有时可能情况比较复杂（qemu-user运行） */
            //char sep = std::filesystem::path::preferred_separator;
            //rsa_pub = std::make_unique<qing::MyRSA::Public_Key>(qing::MyRSA::Public_Key(dir + sep + "public_key.pem"));
            rsa_pub = std::make_unique<qing::MyRSA::Public_Key>(qing::MyRSA::Public_Key(key_path));

            /* 初始化crypted_aes_key和crypted_aes_iv */
            crypted_aes_key = rsa_pub->Encrypt(aes->get_key());
            crypted_aes_iv = rsa_pub->Encrypt(aes->get_iv());
            //std::cerr << crypted_aes_key << std::endl;
            //std::cerr << crypted_aes_iv << std::endl;

        }
        ///////////////////////////////////////
        inline void init_thread() {

            /* 优化方案：suspend() 能否正常运行，还需要进行实际的测试 */
            qing::f_t StopEvent = [this](qing::Thread& th) {
                th.suspend();
            };

            /* 唤醒事件（获取资源） */
            qing::f_t WakeEvent = [this](qing::Thread& th) {

                try
                {
                   this->http = std::make_unique<qing::Http> (this->http_addr, this->http_port);  /* 启动http服务器 */
                   this->mods = std::make_unique<qing::Modules> (this->name);         /* 启动子系统 */
                }

                catch (qing::Modules::SubsystemNotExist exp)
                {
                    std::cout << exp.what() << std::endl;
                    th.stop();   /* 线程静止 */
                    return;   /* 结束回调 */
                }

                /*
                 * clear Dead process
                 * 清理死去的进程
                 * 
                 * FIXME: 另一种策略是不做显式的清理视图，而是在每个其他视图中运行一次清理
                 *        （似乎也有可能造成一些灵活性问题）
                 */
                this->http->post("/api/v1/clear", [this](const httplib::Request& req, httplib::Response& res) {
                    try {
                        for (auto it = vecp.begin(); it != vecp.end();) {
                            if (!(*it)->isRunning()) {
                                (*it)->stop();
                                it = vecp.erase(it);
                            }
                            else
                                it++;
                        }
                        res.set_content("OK", "text/plain");
                    }
                    catch (std::exception& e) {
                        res.set_content(e.what(), "text/plain");
                    }
               });

                //-----------------------------------------------
                //-----------------------------------------------
    	        /*
                 * 获取经过RSA加密的AES密钥
                 *
                 * 例子 ： curl -X GET http://localhost:8012/api/v1/get_key
                 */
                this->http->get("/api/v1/get_key", [this] (const httplib::Request& req, httplib::Response& res) {
                    res.status = 200;
                    res.set_content(crypted_aes_key, "text/plain");
                });

                /*
                 * 获取经过RSA加密的AES初始向量
                 *
                 * 例子：curl -X GET http://localhost:8012/api/v1/get_iv
                 */
                this->http->get("/api/v1/get_iv", [this] (const httplib::Request& req, httplib::Response& res) {
                    res.status = 200;
                    res.set_content(crypted_aes_iv, "text/plain");
                });
	    
                /*
                 * list all process you ran
                 *
                 * 列举出所有进程，结果使用AES加密
                 */
                this->http->get("/api/v1/get_list", [this](const httplib::Request& req, httplib::Response& res) {
                    auto list = std::string();
                    for (auto& pt: vecp) {
                        list += (list=="") ? "" : "\n";
                        list += std::to_string(pt->pid());
                        list += " ";
                        list += pt->check();
                        list += "\n";
                    }
                    std::string msg = aes->encrypt(list);
                    res.set_content(msg, "text/plain");
                });

                /*
                 * 启动模块 （消息体应使用AES加密）
                 *
                 * 例子：curl -X POST http://localhost:8009/api/v1/start -d '{"module": <name>}'
                 */
                this->http->post("/api/v1/start", [this](const httplib::Request& req, httplib::Response& res) {

                    try
                    {
                        std::string params = aes->decrypt(req.body); // 解密消息体
                        auto json = nlohmann::json::parse(params); // 对明文进行json解析
                        /*-----------------*/
                        std::string mod = json["module"];
                        std::string path = mods->GetInitp(mod); // 获取启动脚本
                        //this->pl.execute("bash", {cmd}); // 执行
                        std::string cmd = std::string{"bash "};
                        cmd += path;
                        cmd += " 1>/dev/null";
                        auto task = std::make_shared<qing::ProcessTask>(cmd);
                        task->start();
                        vecp.push_back(task);
                        /*-----------------*/
                        LOG_INFO(std::string("PMC-START was called by ") + req.remote_addr);
                        res.status = 200;
                        res.set_content("OK", "text/plain");
                    }

                    catch (std::exception &e) {
                        res.status = 400;
                        res.set_content(e.what(), "text/plain");
                    }

                });

                /*
                 * 杀死一个进程（消息体应该使用AES加密）
                 * 示例：curl -X POST http://localhost:8012/api/v1/kill -d '{"pid": <int>}'
                 */
                this->http->post("/api/v1/kill", [this](const httplib::Request& req, httplib::Response& res) {
                    try
                    {
                        std::string msg = aes->decrypt(req.body);
                        nlohmann::json json = nlohmann::json::parse(msg);
                        /*-------------*/
                        int pid = json["pid"];
                        for (auto it=vecp.begin(); it!=vecp.end(); it++) {
                            if ((*it)->pid() == pid) {
                                (*it)->stop();
                                vecp.erase(it);
                                break;
                            }
                        }
                        /*-------------*/
                        res.status = 200;
                        res.set_content("OK", "text/plain");
                    } 
                    catch (std::exception &e) {
                        res.status = 500;
                        res.set_content(e.what(), "text/plain");
                    }
                });


                /* 
                 * 列举出安装的所有模块(响应体使用AES加密)
                 * 例子：curl -X GET http://localhost:8012/api/v1/list_mods 
                 */
                this->http->get("/api/v1/list_mods", [this](const httplib::Request& req, httplib::Response& res)
                {
                    std::string list = "";
                    for (std::string &name: this->mods->ListAll()) {
                        list += (list == "") ? "" : "\n";
                        list += name;
                    }
                    /*---------------*/
                    std::string encrypted_msg = aes->encrypt(list);
                    res.status = 200;
                    res.set_content(encrypted_msg, "text/plain");
                });
            
                /*
                 * 创建模块的API（消息体应该使用AES加密）
                 *
                 * 例子： curl -X POST http://localhost:8012/api/v1/crt_mod -d '{"module": "hello"}'
                 *
                 * 现在我一般是手动创建模块，未来可以让它变为从github拉取项目的功能
                 */
                this->http->post("/api/v1/crt_mod", [this] (const httplib::Request& req, httplib::Response& res) {
                    try
                    {
                        std::string jmsg = aes->decrypt(req.body);
                        nlohmann::json json = nlohmann::json::parse(jmsg);
                        /*---------------*/
                        std::string module = json["module"];
                        this->mods->CrtMod(module);
                        /*---------------*/
                        LOG_INFO(std::string("PMC.crt_mod was called by ") + req.remote_addr);
                        res.status = 200;
                        res.set_content("OK", "text/plain");
                    }
                
                    catch (qing::Crypto_Basic::CryptoExp& e) {
                        res.status = 400;
                        res.set_content("加密失败", "text/plain");
                    }

                    //catch ( ...json_parser ) {
                    //}

                    catch (qing::Modules::Mods_Exp& e) {
                        res.status = 400;
                        res.set_content("模块创建失败", "text/plain");
                    }

                    catch (std::exception& e) {
                        res.status = 500;
                        res.set_content(e.what(), "text/plain");
                    }
                });
            
                /*
                 * 删除模块的API（消息体应该使用AES加密）
                 *
                 * 例子： curl -X POST http://localhost:8012/api/v1/del_mod -d '{"module": "hello"}'
                 */
                this->http->post("/api/v1/del_mod", [this] (const httplib::Request& req, httplib::Response& res)
                {
                    try
                    {
                        std::string jmsg = aes->decrypt(req.body);
                        nlohmann::json json = nlohmann::json::parse(req.body);
                        /*---------------*/
                        std::string module = json["module"];
                        this->mods->DelMod(module);
                        /*---------------*/
                        LOG_INFO(std::string("PMC.del_mod was called by ") + req.remote_addr);
                        res.status = 200;
                        res.set_content("OK", "text/plain");
                    }
                
                    catch (qing::Crypto_Basic::CryptoExp& e) {
                        res.status = 400;
                        res.set_content("加密失败", "text/plain");
                    }
                
                    catch (qing::Modules::Mods_Exp& e) {
                        res.status = 400;
                        res.set_content(e.what(), "text/plain");
                     }
                
                    catch (std::exception& e) {
                        res.status = 500;
                        res.set_content(e.what(), "text/plain");
                    }
                });

                /* 打印地址 */
                LOG_INFO(this->http->printAddr());

                /* 开始运行，只运行一次 */
                if (th.check()==qing::Fsm::Stat::START) {
                    th.run(); /* 将状态机标记为启动 */
                    /*------------------------*/
                    /* 先从子系统类中获取引导模块的启动脚本位置 然后以此路径创建子进程 */
                    auto cmd = std::string("bash ");
                    cmd += mods->GetInitp("_init");
                    cmd += " 1>/dev/null";
                    auto task = std::make_shared<qing::ProcessTask>(cmd);
                    task->start();
                    vecp.push_back(task);
                    /*------------------------*/
                    http->run(); /* 这个会堵塞线程以进行监听，http服务关闭后会从堵塞状态中恢复，线程也应该停止（弹出状态） */
                }

                /* 如果是处于弹出状态，则暂停线程 */
                if (th.check()==qing::Fsm::Stat::RUNNING) {
                    th.stop();
                }


            };

            /* 循环事件（在该类中废弃）*/
            qing::f_t LoopEvent = [this](qing::Thread& _) {
                //std::this_thread::sleep_for(std::chrono::milliseconds(10L));
                throw std::runtime_error("Event not Support.");
            };

            /* 清理事件 */
            qing::f_t ClearEvent = [this](qing::Thread& _)
            {
                if(this->http)
                {
                    LOG_INFO("Clear Pmc");  /* 日志输出 */
                    this->http.reset();     /* 重置http服务器的指针托管器 */
                    for (auto& task: vecp) {
                        task->stop();
                    }
                }
            };


            this->th = std::make_unique<qing::Thread>(
                StopEvent,
                WakeEvent,
                LoopEvent,
                ClearEvent
            );
        }
    
    private:

	std::unique_ptr<qing::Thread> th; /*持有线程类*/


        std::unique_ptr<qing::Http> http;   /* http服务器对象 */

        std::string http_addr = ""; /* http监听地址 */

        int http_port = 0;      /* http监听端口 */

        /*-----------------------------*/
        /*
         * AES-CBC 加密模块，AES加密的速度快，用于加密通信消息
         */
        std::unique_ptr<qing::AES_CBC> aes; 

        /*
         * RSA非对称加密
         *
         * 用于AES通信密钥的分发
         *
         *
         * 这个密钥我们将会在线程启动后手动从文件系统中加载
         */
        std::unique_ptr<qing::MyRSA::Public_Key> rsa_pub; 

        /*-----------------------------*/

        std::string name = "";  /* 子系统名 */

        std::vector<std::shared_ptr<qing::ProcessTask>> vecp{};

        std::unique_ptr<qing::Modules> mods;  /* 子系统对象 */


        /*
         * AES密钥，用于消息的加密
         *
         * 在线程开始时随机生成
         */
        std::string crypted_aes_key;


        /*
         * AES初始向量，用于消息的加密
         *
         * 在线程开始时设置 
         */
        std::string crypted_aes_iv;

        //-----------------------------------------------------------------------
        /* 下面三个静态函数没有用上 */


        /* 检查消息的参数 */
        //static void check_params(std::string& call, std::string& path) {
        //    if (call == "" || call.size() >= 128  || path == "" || path.size() >= 1024) {
        //            throw std::runtime_error("参数的长度不合适。");
        //        }
        //}
        
        /* 检查消息的参数 */
        //static void check_params(std::string& path) {
        //    if (path == "" || path.size() >= 1024) {
        //            throw std::runtime_error("参数的长度不合适。");
        //        }
        //}

        /* 检查消息的请求头 */
        //static void check_headers(const httplib::Request& req) {
        //        if (!req.has_header("Content-Type") || req.get_header_value("Content-Type") != "application/json") {
        //            throw std::runtime_error("不合法的请求头。");
        //        }
        //}

        /* 检查json数组 */
        //static void check_json_array(nlohmann::json& json) {
        //        if (!json.is_array()) {
        //            throw std::runtime_error("我们需要一个json数组");
        //        }
        //}

};
