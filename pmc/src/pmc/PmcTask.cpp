#include <stdexcept>
#include "net/Http.hpp"
#include "pmc/Modules.hpp"
#include "pmc/PmcTask.hpp"
#include "logs/Logger.hpp"
namespace qing {

    void PmcTask::init_thread() {

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


}
