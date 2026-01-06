/*
 * 进程列表
 *
 * FIXME: 增加单条字符串命令的输入方式，逐渐淘汰列表模式
 */

#pragma once
#include "Logger.hpp"
#include "TaskPool_Interf.hpp"
#include <list> // 列表类用于容纳子进程对象
#include <memory> // 内存管理类
#include <iostream>
#include <thread>
#include <chrono>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <boost/process.hpp> // libboost-dev
#include <nlohmann/json.hpp>
namespace qing {

    /*
     * 进程列表类
     *
     * 这个类封装了一个容纳ProcessList::SubProcess的列表
     * 
     * 以及基本的对进程操作。
     *
     */
    class ProcessList: public TaskPool_Interf {
    public:

        /* 检查移除所有已终止进程 */
        virtual void check_and_clear() override {
            std::lock_guard<std::mutex> lock(mtx);// 上锁
            for (auto it = vec.begin(); it != vec.end(); ) {
                if (!(*it).p.running()) {
                    it = vec.erase(it);
                }
                else {
                    ++it;
                }
            }
        }

        /* 清理进程组 */
        virtual void CloseAll() override {
            std::lock_guard<std::mutex> lock(mtx);
            for (auto& item: vec) {
                if (item.p.running()) item.p.terminate();
            }
            /* 清空所有元素 */
            vec.clear();
        }

        /*
         * 利用子进程执行一个命令，并且添加到进程列表
         *
         * 这个函数接受一个std::string格式的文件路径，以及std::vector<std::string>类型的参数列表
         *
         *
         * 这两个参数会被转换为空格分隔的一条命令。
         *
         * FIXME: 拆分成两个函数
         */
        void execute(std::string path, std::vector<std::string> arr) {
            std::string cmd = ProcessList::merge_cmdl(path, arr);
            LOG_INFO(cmd);
            {
                std::lock_guard<std::mutex> lock(mtx);
                boost::process::child c(cmd);
                vec.push_back({std::move(c), cmd});
            }
        }

        /* 序列化进程列表 */
        std::string print() {
            std::string ret {""};
            std::lock_guard<std::mutex> lock(mtx);
            for (auto& item: vec) {
                ret += (ret=="") ? "" : "\n";
                ret += std::to_string(item.p.id());
                ret += "\t";
                ret += (item.p.running()) ? "Alive" : "Dead";
                ret += "\t";
                ret += item.cmd;
            }
            /* 返回组合后的结果 */
            return ret;
        }

        /* 杀死一个进程 */
        void kill(int pid) {
            std::lock_guard<std::mutex> lock(mtx);
            for (auto it=vec.begin(); it != vec.end(); it++) /* 能不能直接遍历 */
                if ((*it).p.id() == pid) {
                    (*it).p.terminate();
                    vec.erase(it);
                    return;
                }
        }

        /* 从 nlohmann::json 读取参数 */
        static std::vector<std::string> ReadParams(nlohmann::json& json) {
            std::vector<std::string> arr;
            arr.reserve(json.size()); /* 预分配空间 */
            for (const auto& element: json) {
                if (!element.is_string())
                    throw std::invalid_argument("参数必须以字符串传入。");
                arr.push_back(element.get<std::string>());
            }
            /* 返回经过组合的字符串 */
            return arr;
        }

    private:

        struct SubProcess { /* 此结构体代表一个子进程 */
            boost::process::child p;
            std::string cmd;
        };

        std::list<ProcessList::SubProcess> vec {};  /* 容纳子进程的链表 */
 
        std::mutex mtx{};   /* 用于操作链表的锁 */

        /*
         * 将一个启动脚本地址，和一个std::vector<std::string>>组成的参数列表组装成一条完整的启动命令
         */
        static std::string merge_cmdl(std::string path, std::vector<std::string> arr) {
            std::string ret = path;
            for (const auto& item: arr) {
                ret += " ";
                ret += item;
            }
            /* 返回组合之后的字符串 */
            return ret;
        }
    };
}
