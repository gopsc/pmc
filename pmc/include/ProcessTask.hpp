#pragma once
#include <iostream>
#include <memory> // 内存管理类
#include <boost/process.hpp> // libboost-dev
#include "ITask.hpp"
namespace qing {
class ProcessTask: public ITask {
public:
    ProcessTask(std::string cmd): cmd(cmd) {}
    bool isRunning() override {
        return p && p->running();
    }
    void start() override {
        if (!isRunning())
            this->p = std::make_unique<boost::process::child>(cmd);
        else  /* 有必要抛出异常吗 */
            throw std::runtime_error("Already Running");
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
