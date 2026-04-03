/* ===============================================
 *  通用子系统、进程机器、进程托管服务
 *
 *
 *  【主函数文件】
 *
 *  直接子系统调用，或者启动一个pmc子系统。
 * ===============================================
 */
#include <filesystem>
#include <iostream>
#include <string> 
#include <fstream>
#include <sstream>
#include <thread>
#include <memory>
#include <vector>
#include <cstddef>
#include <csignal> /* 注册信号以优雅退出 */
#include <functional>
#include <unistd.h>
#include <sys/types.h>
#include <boost/json.hpp>
#include <boost/process.hpp> // libboost-dev
#include <boost/program_options.hpp> /* 解析命令行参数 */
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/property_tree/ptree.hpp>
#include "boost/property_tree/json_parser.hpp"
#include "net/HttpServer.hpp"
#include "logs/Logger.hpp"
#include "th/Thread.hpp" /* 可控线程类 */
#include "th/Cv_wait.hpp" /* 通过条件变量进行等待 */
#include "th/ITask.hpp"
#include "tt/Mq.h"
#include "tt/Pipe.h"
//#include "Tttask.h" /* Transmit */
//#include "pmc/Modules.hpp" /* 管理子系统模块 */
//#include "PmcTask.hpp"


/* lci 临时 */
//#include "lci/p_th.h"

/* 中文模式 Chinese Mode */
//#include "cn/中文化.hpp"


static const char *PMC_VERSION = "0.6.x";  /* 版本号 */
static const size_t MSGLEN = 2048;
static const size_t MSGCNT = 100;

using namespace qing;
namespace po = boost::program_options;


namespace qing {
class ProcessTask: public ITask {
public:
    ProcessTask(std::string cmd): cmd(cmd) {}
    bool isRunning() override {
        return p && p->running();
    }
    void start() override { /* 有必要抛出异常吗 */
        if (!isRunning())
            this->p = std::make_unique<boost::process::child>(cmd);
    }
    void stop() override {
        if (isRunning()) {
            p->terminate();
        }
        p.reset();
    }
    std::string check()  {
        return this->cmd;
    }
    pid_t pid() {
        return p->id();
    }
private:
    std::unique_ptr<boost::process::child> p;
    std::string cmd;
};
}


namespace qing {
/*
 * TRANSMIT TASK
 *
 * FIXME: 输入一个通信对象
 */
class Tttask: public ITask {
public:
Tttask(const size_t msglen, size_t msgcnt, const sf_t callback) {
	this->callback = std::make_unique<sf_t>(callback);
	this->msglen = msglen;
	this->msgcnt = msgcnt;
	init_thread();
}
void start() override {
	if (!this->isRunning()){
		th->Activate();
		th->WaitStart();
	}
}
void stop() override {
	th->WaitClose();
}
bool isRunning() override {
	auto stat = th->check();
	return !(stat == Fsm::Stat::STOP
		|| stat == Fsm::Stat::SHUT);
}
private:
std::unique_ptr<Thread> th;
std::unique_ptr<sf_t> callback;
std::unique_ptr<Mq> mq;
size_t msglen=0, msgcnt=0;
std::string name;
std::string get_pid_str() {
	return std::to_string(getpid());
}
void init_thread() {
	this->th = std::make_unique<Thread> (
		
		/* stop callback */
		[](Thread& th) -> void {
			th.suspend();
		},

		/*start callback */
		[this](Thread& th) -> void {
			name = get_pid_str();
			mq = std::make_unique<Mq> (name, msglen, msgcnt, Mq::CREATOR);
			th.run();
		},

		/* loop callback */
		[this](Thread& th) -> void {
			
			
			mq->recv(*callback,
				
				[]() { /* FIXME: 作为参数传入延时 */
					usleep(10000);
			});

		},

		/* clean callback */
		[this] (Thread& th) -> void {
			mq.reset();
		}

	);
}
};
}


/*-------------------------------------------------------------------------*/
Cv_wait cv =  Cv_wait();  /* 条件变量的等待机制让主程序能够等待中断信号的产生 */

auto taskPool = std::vector<std::shared_ptr<ITask>>{};  /* 底层任务池 - 也就是跑控制器监听线程的任务池 */



namespace qing {
/*
 * process pool 进程池
 *
 * 这个池子只放子进程、目前已经集成取下标操作和删除操作
 */
class PPool {
public:

	/* CREATE PROCESS - 创建进程
	 *
	 * 需要输入启动命令 */
	void crtp(const std::string& cmd) try {
		auto pt = std::make_shared<ProcessTask>(cmd);
		pt->start();
		pool.push_back(pt);
	}

	catch (boost::process::process_error &exp) {
		std::cerr << "Failed to execute this command: " << cmd << std::endl;
	}

	/* CLEAR - 清理
	 * 删除所有已经停下的进程 */
	void clr() {
		auto it = --pool.end();
		for (; it != pool.begin(); --it)
			if (!(*it)-> isRunning()) {
				auto tmp= it;
				it++;
				pool.erase(tmp);
			}

		/* 如果end() == begin()，此步将会试图删除哨兵节点 */
		if (pool.size() > 0 && !(*it)->isRunning())
			pool.erase(it);
	}


	/* 获取进程池的大小 */
	size_t size(){
		return pool.size();
	}

	/* 删除一个进程，需要输入进程的下标
	 * FIXME: 似乎不太自然 */
	void kill(int idx) {
		auto it = pool.begin();
		for (int i=0; i < idx &&  it != pool.end(); ++i, ++it);
		if (it != pool.end()) {
			(*it)->stop();
			pool.erase(it);
		}
	}

	/* 取下标运算符重载 - 取元素对象 */
	ProcessTask& operator[](const size_t idx) {
		auto it = pool.begin();
		for (int i = 0; i < idx; ++it, ++i)
			if (it == pool.end())
				throw std::out_of_range("PPool::operator[]");
		return **it;
	}

	/* FIXME: begin()、end() 
	 * ...... */

private:

	/* 进程任务的容器 */
	std::list<std::shared_ptr<ProcessTask>> pool{};
};
}

/* 用于存放子进程的进程池 */
PPool ppool;


/*-------------------------------------------------------------------------*/
namespace qing {
namespace pmc_mtd {

auto list() -> std::string {
    boost::json::array processes;
    
    for (int i = 0; i < ppool.size(); ++i) {
        processes.push_back({
            {"index", i},
            {"pid", ppool[i].pid()},
            {"status", ppool[i].check()}
        });
    }
    
    return boost::json::serialize(processes);
}

auto kill(const int pid) -> bool {
	for (int i=0; i<ppool.size(); ++i)
		if ( ppool[i].pid() == pid) {
			ppool.kill(i);
			return true;
		}
	return false;
}

auto exec(const std::string& cmd) -> bool try {
	ppool.clr();
	ppool.crtp(cmd);
	return true;
}

catch(std::exception& exp) {
	return false;
}

}
}




boost::property_tree::ptree parseJsonToPtree(const std::string& jsonStr) {
    boost::property_tree::ptree pt;
    try {
        std::stringstream ss(jsonStr);
        boost::property_tree::read_json(ss, pt);
    } catch (const boost::property_tree::json_parser_error& e) {
        // 解析失败处理
        std::cerr << "JSON parse error: " << e.what() << std::endl;
    }
    return pt;
}

namespace qing {
/**
 * @brief 进程托管服务类
 *
 * 提供进程的列表、执行和终止功能
 */
class ProcessManagerService {
public:
    /**
     * @brief 构造函数
     * @param server HTTP服务器引用
     */
    explicit ProcessManagerService(pmc::net::HttpServer& server);

    /**
     * @brief 注册所有进程管理相关的路由
     */
    void registerRoutes();

private:
    pmc::net::HttpServer& server_;

    /**
     * @brief 解析请求体中的JSON数据
     * @param body 请求体字符串
     * @return JSON解析后的属性树
     */
    boost::property_tree::ptree parseJsonBody(const std::string& body);

    /**
     * @brief 构建JSON响应
     * @param success 是否成功
     * @param message 消息
     * @param data 附加数据
     * @return JSON字符串
     */
    std::string buildJsonResponse(bool success, const std::string& message,
                                   const boost::property_tree::ptree& data = boost::property_tree::ptree());

    /**
     * @brief 从请求中提取target参数
     * @param params 查询参数
     * @return target值（空字符串表示本机）
     */
    std::string getTarget(const std::unordered_map<std::string, std::string>& params);

    /**
     * @brief 从请求中提取exec参数
     * @param params 查询参数
     * @return 要执行的指令
     */
    std::string getExecCommand(const std::unordered_map<std::string, std::string>& params);

    /**
     * @brief 从请求中提取kill参数
     * @param params 查询参数
     * @return 目标进程号
     */
    int getPid(const std::unordered_map<std::string, std::string>& params);

    // POST方法处理器
    http::response<http::string_body> handleList(
        const http::request<http::string_body>& req,
        const std::unordered_map<std::string, std::string>& params);

    http::response<http::string_body> handleExec(
        const http::request<http::string_body>& req,
        const std::unordered_map<std::string, std::string>& params);

    http::response<http::string_body> handleKill(
        const http::request<http::string_body>& req,
        const std::unordered_map<std::string, std::string>& params);

    // 内部实现函数（留空）
    std::string listProcesses(const std::string& target);
    std::string execCommand(const std::string& command, const std::string& target);
    bool killProcess(int pid, const std::string& target);
};


ProcessManagerService::ProcessManagerService(pmc::net::HttpServer& server)
    : server_(server) {
}

void ProcessManagerService::registerRoutes() {
    // 注册POST路由
    server_.post("/list", [this](const auto& req, const auto& params) {
        return this->handleList(req, params);
    });

    server_.post("/exec", [this](const auto& req, const auto& params) {
        return this->handleExec(req, params);
    });

    server_.post("/kill", [this](const auto& req, const auto& params) {
        return this->handleKill(req, params);
    });
}

boost::property_tree::ptree ProcessManagerService::parseJsonBody(const std::string& body) {
    boost::property_tree::ptree pt;
    try {
        std::stringstream ss(body);
        boost::property_tree::read_json(ss, pt);
    } catch (...) {
        // JSON解析失败，返回空树
    }
    return pt;
}

std::string ProcessManagerService::buildJsonResponse(bool success, const std::string& message,
                                                      const boost::property_tree::ptree& data) {
    boost::property_tree::ptree response;
    response.put("success", success);
    response.put("message", message);
    if (!data.empty()) {
        response.add_child("data", data);
    }

    std::stringstream ss;
    boost::property_tree::write_json(ss, response);
    return ss.str();
}

std::string ProcessManagerService::getTarget(const std::unordered_map<std::string, std::string>& params) {
    auto it = params.find("target");
    if (it != params.end()) {
        return it->second;
    }
    return "";  // 空字符串表示本机
}

std::string ProcessManagerService::getExecCommand(const std::unordered_map<std::string, std::string>& params) {
    auto it = params.find("command");
    if (it != params.end()) {
        return it->second;
    }
    return "";
}

int ProcessManagerService::getPid(const std::unordered_map<std::string, std::string>& params) {
    auto it = params.find("pid");
    if (it != params.end()) {
        return std::stoi(it->second);
    }
    return -1;
}

http::response<http::string_body> ProcessManagerService::handleList(
    const http::request<http::string_body>& req,
    const std::unordered_map<std::string, std::string>& params) {

    http::response<http::string_body> res;
    res.version(req.version());
    res.set(http::field::content_type, "application/json");

    try {
        std::string target = getTarget(params);

        // TODO: 实现进程列表逻辑
        std::string result = listProcesses(target);

        boost::property_tree::ptree data = parseJsonToPtree(result);
        // TODO: 将result解析为data

        res.result(http::status::ok);
        res.body() = buildJsonResponse(true, "Process list retrieved successfully", data);
    } catch (const std::exception& e) {
        res.result(http::status::internal_server_error);
        res.body() = buildJsonResponse(false, std::string("Error: ") + e.what());
    }

    res.prepare_payload();
    return res;
}

http::response<http::string_body> ProcessManagerService::handleExec(
    const http::request<http::string_body>& req,
    const std::unordered_map<std::string, std::string>& params) {

    http::response<http::string_body> res;
    res.version(req.version());
    res.set(http::field::content_type, "application/json");

    try {
        std::string target = getTarget(params);
        std::string command = getExecCommand(params);

        if (command.empty()) {
            // 尝试从POST body中获取command
            auto body_json = parseJsonBody(req.body());
            command = body_json.get<std::string>("command", "");
        }

        if (command.empty()) {
            res.result(http::status::bad_request);
            res.body() = buildJsonResponse(false, "Missing required parameter: command");
            res.prepare_payload();
            return res;
        }

        // TODO: 实现命令执行逻辑
        std::string result = execCommand(command, target);

        boost::property_tree::ptree data;
        data.put("command", command);
        data.put("output", result);

        res.result(http::status::ok);
        res.body() = buildJsonResponse(true, "Command executed successfully", data);
    } catch (const std::exception& e) {
        res.result(http::status::internal_server_error);
        res.body() = buildJsonResponse(false, std::string("Error: ") + e.what());
    }

    res.prepare_payload();
    return res;
}

http::response<http::string_body> ProcessManagerService::handleKill(
    const http::request<http::string_body>& req,
    const std::unordered_map<std::string, std::string>& params) {

    http::response<http::string_body> res;
    res.version(req.version());
    res.set(http::field::content_type, "application/json");

    try {
        std::string target = getTarget(params);
        int pid = getPid(params);

        if (pid <= 0 && !req.body().empty()) {
            // 尝试从POST body中获取pid
            auto body_json = parseJsonBody(req.body());
            pid = body_json.get<int>("pid", -1);
        }

        if (pid <= 0) {
            res.result(http::status::bad_request);
            res.body() = buildJsonResponse(false, "Missing or invalid parameter: pid");
            res.prepare_payload();
            return res;
        }

        // TODO: 实现进程终止逻辑
        bool success = killProcess(pid, target);

        if (success) {
            boost::property_tree::ptree data;
            data.put("pid", pid);
            res.result(http::status::ok);
            res.body() = buildJsonResponse(true, "Process terminated successfully", data);
        } else {
            res.result(http::status::internal_server_error);
            res.body() = buildJsonResponse(false, "Failed to terminate process");
        }
    } catch (const std::exception& e) {
        res.result(http::status::internal_server_error);
        res.body() = buildJsonResponse(false, std::string("Error: ") + e.what());
    }

    res.prepare_payload();
    return res;
}

// 以下是实现占位符（留空）
std::string ProcessManagerService::listProcesses(const std::string& target) {
    // TODO: 实现进程列表逻辑

    return pmc_mtd::list();
}

std::string ProcessManagerService::execCommand(const std::string& command, const std::string& target) {
    // TODO: 实现命令执行逻辑
    // 如果target为空，在本机执行
    // 否则在目标服务器执行
    return pmc_mtd::exec(command) ? "OK" :"ERR";
}

bool ProcessManagerService::killProcess(int pid, const std::string& target) {
    // TODO: 实现进程终止逻辑
    
    return pmc_mtd::kill(pid);
}
}


std::unique_ptr<pmc::net::HttpServer> server;
/*-------------------------------------------------------------------------*/


/* 捕获中断信号，结束线程并且退出 */
void signalHandler(int signum)
{
    LOG_INFO("收到终止信号，准备停止服务器...");
    cv.WakeCv();    /* 唤醒条件信号即关闭程序 */
    if (server) server->stop();
}

/*----SIGNAL ACTION----*/
//
/* 对信号的响应行为
 * NOTE: 我应该系统地学习一下这个库 */
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
int pmc_init(po::variables_map&);  /* 初始化pmc并行机器 */
void parse_self_init_list(std::string&); /* 解析自启动列表文件 */
void parse_data_from_mq(std::string&); /*  */
int send_start_and_recv_pipe(po::variables_map&); /* 发送启动命令并且接收回复 */
int send_list_and_recv_pipe(po::variables_map&); /* 发送启动命令并且接收回复 */
int send_kill_and_recv_pipe(po::variables_map&); /* 发送删除命令并且接收回复 */


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
	/******** 子系统调用 *********/
	("send,s", po::value<std::string>(), "send a message to pmc sys")
	("exec",   po::value<std::string>(), "message body")
	("list", "list process all run")
	("kill",   po::value<int>(),         "kill a process")
	/********* 子系统启动 ********/
	("init,i", po::value<std::string>(), "self-init list")
	("serve",  po::value<int>(),         "Run as http server");

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
    /* 初始化OpenSSL */
	//OpenSSL_add_all_algorithms();
	//ERR_load_crypto_strings();
	

	if ( vm.count("send") && vm.count("exec")) {
		return send_start_and_recv_pipe(vm);
	}

	else if ( vm.count("send") && vm.count("list")){
		return send_list_and_recv_pipe(vm);
	}

	else if (vm.count("send") && vm.count("kill")) {
		return send_kill_and_recv_pipe(vm);
	}

	/* 初始化日志模块 */
	Logger::getInstance().init(LogLevel::DEBUG, true);

	
	if (!vm.count("serve"))
	/* pmc初始化函数 */
		pmc_init(vm);

    /* 清理 OpenSSL 资源 */
	//EVP_cleanup();
	//ERR_free_strings();

    /**********************************************/

	else if (vm.count("serve")) {
		int port = vm["serve"].as<int>();
		server = std::make_unique<pmc::net::HttpServer>(port, 4);
		ProcessManagerService pmService(*server);
		pmService.registerRoutes();
		server->start();

		std::cout << "Process Manager Service running on port " << port << std::endl;
		std::cout << "Available endpoints:" << std::endl;
		std::cout << "  POST /list?target=optional" << std::endl;
		std::cout << "  POST /exec?command=ls&target=optional" << std::endl;
		std::cout << "  POST /kill?pid=1234&target=optional" << std::endl;

		server->run();
	}
	
	
	/*
	 * 等待
	 *
	 * 触发器在信号处理那里
	 */
	else cv.Wait();

	/* 主线程被唤醒 */
	for (auto& task: taskPool) {
		task->stop();
	}
}

/*-----------------------------------------------------------------------------*/
/* 向pmc的消息队列发送启动命令并且从管道接收回复 */
int send_start_and_recv_pipe(po::variables_map& vm)
try {
	auto name_pipe = std::to_string(getpid());
	auto name_mq = vm["send"].as<std::string>();
	auto mq = Mq(name_mq, 2048, 100, Mq::USER); /* 放在这里避免管道文件的残留 */
	auto exec = vm["exec"].as<std::string> ();

	boost::json::object obj;
	obj["pipe"] = name_pipe;
	obj["type"] = "exec";
	obj["exec"] = exec;
	auto jsonstr = boost::json::serialize(obj);


	auto pipe = Pipe(name_pipe, Pipe::CREATOR); /* FIXME: 创建管道文件到/tmp下面 */
	if (!pipe.openForRead(true)) {
		std::cerr << "Failed to open pipe" << std::endl;
		return 1;
	}

	mq.send(jsonstr);
	if (!pipe.waitForRead(1, 0)) {
		std::cerr << "Pipe Read Timeout" << std::endl;
		return 1;
	}

	std::string res = "";
	if (pipe.readPipe(&res) < 0) {
		std::cerr << "Failed to read pipe" << std::endl;
		return 1;
	}

	std::cout << res << std::endl;
	return 0;
}

//catch (boost::interprocess::interprocess_exception &exp)  {
//	std::cerr << "Target Pmc Is Not Found" << std::endl;
//	return 1;
//}

catch (std::exception &exp) {
    std::cerr << "异常类型: " << typeid(exp).name() << std::endl;
    std::cerr << "异常信息: " << exp.what() << std::endl;
    return 1;
}


/* 向目标子系统发送列举命令并收集结果 */
int send_list_and_recv_pipe(po::variables_map& vm)
try {

	auto name_pipe = std::to_string(getpid());
	auto name_mq = vm["send"].as<std::string> ();
	auto mq = Mq(name_mq, 2048, 100, Mq::USER);

	boost::json::object obj;
	obj["pipe"] = name_pipe;
	obj["type"] = "list";
	auto jsonstr = boost::json::serialize(obj);
	auto pipe = Pipe(name_pipe, Pipe::CREATOR);

	if (!pipe.openForRead(true)) {
		std::cerr << "Failed to open pipe" << std::endl;
		return 1;
	}


	mq.send(jsonstr);
	if (!pipe.waitForRead(1, 0)) {
		std::cerr << "Pipe Read Timeout" << std::endl;
		return 1;
	}

	std::string res = "";
	if (pipe.readPipe(&res) < 0) {
		std::cerr << "Failed to read pipe" << std::endl;
		return 1;
	}
	std::cout << res << std::endl;
	return 0;

}

catch (boost::interprocess::interprocess_exception &exp)  {
	std::cerr << "Target Pmc Is Not Found" << std::endl;
	return 1;
}

catch (std::exception &exp) {
    std::cerr << "异常类型: " << typeid(exp).name() << std::endl;
    std::cerr << "异常信息: " << exp.what() << std::endl;
    return 1;
}

/* 删除操作调用 */
int send_kill_and_recv_pipe(po::variables_map &vm)
try{

	auto name_pipe = std::to_string(getpid());
	auto name_mq = vm["send"].as<std::string> ();
	auto mq = Mq(name_mq, 2048, 100, Mq::USER);
	auto kill = vm["kill"].as<int>();

	boost::json::object obj;
	obj["pipe"] = name_pipe;
	obj["type"] = "kill";
	obj["kill"] = kill;
	auto jsonstr = boost::json::serialize(obj);

	auto pipe = Pipe(name_pipe, Pipe::CREATOR);
	if (!pipe.openForRead(true)) {
		std::cerr << "Failed to open pipe" << std::endl;
		return 1;
	}

	mq.send(jsonstr);
	if (!pipe.waitForRead(1, 0)) {
		std::cerr << "Pipe Read Timeout" << std::endl;
		return 1;
	}

	std::string res = "";
	if (pipe.readPipe(&res) < 0) {
		std::cerr << "Failed to read pipe" << std::endl;
		return 1;
	}
	else  {
		std::cout << res << std::endl;
		return 0;
	}

}

/* 没有打开目标子系统的消息队列时，会触发这个异常
 * 但是实际上这里捕获的是进程间通信通用异常 */
catch (boost::interprocess::interprocess_exception &exp)  {
	std::cerr << "Target Pmc Is Not Found" << std::endl;
	return 1;
}

/* 这些函数抛出异常后可能导致管道文件残留 */
catch (std::exception &exp) {
    std::cerr << "异常类型: " << typeid(exp).name() << std::endl;
    std::cerr << "异常信息: " << exp.what() << std::endl;
    return 1;
}


/*-----------------------------------------------------------------------------*/
/* 初始化子系统 */
int pmc_init(po::variables_map& vm) {
	if (vm.count("init")){
		std::string path = vm["init"].as<std::string>();
		parse_self_init_list(path);
	}



	/*-------------------*/
	auto ttt = std::make_shared<Tttask> (MSGLEN, MSGCNT,
		[](const char *data) { /* 处理收到的数据 */
			auto json_str = std::string(data);
			parse_data_from_mq(json_str);
		});
	ttt->start();
	taskPool.push_back(ttt);
	return 0;
}

/* 解析自启动列表、添加自启动项 */
void parse_self_init_list(std::string &path) 
try {

	std::ifstream file(path);
	if (!file.is_open()) {
		std::cerr << "Failed to open self-init list" << std::endl;
		return;
	}

	std::string row = "";
	char ch;
	while (file.get(ch)) {

		if (ch == '\n') { 
	
			auto pos = row.find("#");
			if (pos != -1)
				row = row.substr(0, pos);
			if (row != "") {
				ppool.clr();
				ppool.crtp(row);
				row = "";
			}
		}

		else {
			row += ch;
		}
	
	}

}

catch (std::exception &exp) {
    std::cerr << "异常类型: " << typeid(exp).name() << std::endl;
    std::cerr << "异常信息: " << exp.what() << std::endl;
}


/* 解析从消息队列中接收到的数据 */
void parse_data_from_mq(std::string& json_str)
try {
	boost::json::value jv = boost::json::parse(json_str);
	std::string namepipe = jv.at("pipe").as_string().c_str(); 
	std::string typ = jv.at("type").as_string().c_str();
	auto pipe = Pipe(namepipe, Pipe::USER);
	pipe.openForWrite();


	/* 列举出所有子进程，包含序号、pid、执行命令。
	 *
	 * {
	 * 	"pipe": "<int>",
	 * 	"type": "list",
	 * }
	 *
	 */
	if (typ == "list") {
		auto msg = pmc_mtd::list();
		pipe.writePipe(msg);
		return;
	}

	/* 删除一个子进程，需要提供pid
	 *
	 * {
	 *	"pipe": "<integer>",
	 *	"type": "kill",
	 *	"kill": <integer>
	 * }
	 *
	 */
	else if (typ == "kill") {
		int trg = jv.at("kill").as_int64();
		auto flag = pmc_mtd::kill(trg);
		std::string msg = flag ? "OK" : "Nope";
		pipe.writePipe(msg);
		return;
	}

	/* 启动一个子进程
	 *
	 * {
	 *	"pipe": "<integer>",
	 *	"type": "exec",
	 *	"exec": "<commandline>"
	 * }
	 *
	 */
	else if (typ == "exec"){
		std::string cmd = jv.at("exec").as_string().c_str();
		std::string msg = (pmc_mtd::exec(cmd)) ? "OK" : "ERR";
		pipe.writePipe(msg);
		return;
	}
}

catch (std::exception &exp) {
    std::cerr << "异常类型: " << typeid(exp).name() << std::endl;
    std::cerr << "异常信息: " << exp.what() << std::endl;
}
