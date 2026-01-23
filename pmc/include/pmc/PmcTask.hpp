#include <vector>
#include <stdexcept> /* 标准库异常 */
#include "th/ITask.hpp"
#include "th/Thread.hpp"
#include "ProcessTask.hpp"
#include "Crypto/AES_CBC.hpp"
#include "Crypto/MyRSA.hpp"


/*----------------------------------------------*/
/*----------------------------------------------*/
/*
 * 并发器任务，能够监听Http请求，并且根据指令创建、管理子进程
 */
namespace qing {
class PmcTask: public ITask {
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
            return stat == Fsm::Stat::START 
                || stat == Fsm::Stat::RUNNING;
        }

        ///////////////////////////////////////
        inline void init_key(const std::string& key_path) {

            /* 初始化AES对象 */
            aes = std::make_unique<AES_CBC>();
            //std::cerr << aes->get_key() << std::endl;
            //std::cerr << aes->get_iv() << std::endl;
            //std::string cipher = aes->encrypt(std::string("你好"));
            //std::cerr<< cipher << std::endl;
            //std::cerr << aes->decrypt(cipher) << '\'' << std::endl;
 
            /* 初始化RSA对象 */
            //std::string dir = std::filesystem::canonical("/proc/self/exe").parent_path().string();
            //std::string dir = "."; /* 有时可能情况比较复杂（qemu-user运行） */
            //char sep = std::filesystem::path::preferred_separator;
            //rsa_pub = std::make_unique<MyRSA::Public_Key>(MyRSA::Public_Key(dir + sep + "public_key.pem"));
            rsa_pub = std::make_unique<MyRSA::Public_Key>(MyRSA::Public_Key(key_path));

            /* 初始化crypted_aes_key和crypted_aes_iv */
            crypted_aes_key = rsa_pub->Encrypt(aes->get_key());
            crypted_aes_iv = rsa_pub->Encrypt(aes->get_iv());
            //std::cerr << crypted_aes_key << std::endl;
            //std::cerr << crypted_aes_iv << std::endl;

        }
        ///////////////////////////////////////
        void init_thread();
        
    private:

	std::unique_ptr<Thread> th; /*持有线程类*/


        std::unique_ptr<Http> http;   /* http服务器对象 */

        std::string http_addr = ""; /* http监听地址 */

        int http_port = 0;      /* http监听端口 */

        /*-----------------------------*/
        /*
         * AES-CBC 加密模块，AES加密的速度快，用于加密通信消息
         */
        std::unique_ptr<AES_CBC> aes; 

        /*
         * RSA非对称加密
         *
         * 用于AES通信密钥的分发
         *
         *
         * 这个密钥我们将会在线程启动后手动从文件系统中加载
         */
        std::unique_ptr<MyRSA::Public_Key> rsa_pub; 

        /*-----------------------------*/

        std::string name = "";  /* 子系统名 */

        std::vector<std::shared_ptr<ProcessTask>> vecp{};

        std::unique_ptr<Modules> mods;  /* 子系统对象 */


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
}
