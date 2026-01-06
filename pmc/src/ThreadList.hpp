
#include "Thread.hpp"
#include "TaskPool_Interf.hpp"
#include <list>
#include <memory.h>
namespace qing {
class ThreadList: public TaskPool_Interf { /* 线程列表类 */
private:
    std::list<std::shared_ptr<qing::Thread>> veth{};
public:
    void AddTh(std::shared_ptr<qing::Thread> th) {  /* 激活并且添加线程 */
        th->Activate();
        veth.push_back(th);
    }
    
    void StartTh() {    /* 启动所有线程 */
        for (auto& item: veth)  item->WaitStart();
    }

    virtual void CloseAll() override { /* 终止所有线程 */
        for(auto& item: veth) item->WaitClose();
    }

    /*
    * 检查废弃线程的逻辑
    * 
    * 将废弃线程关闭
    */
    virtual void check_and_clear() override {
        for (auto it = veth.begin(); it != veth.end(); ) {
        qing::Fsm::Stat stat = (*it)->check();
        switch (stat) {
            case qing::Fsm::Stat::SHUT: //一般不会主动进入完全关闭的状态
                (*it)->WaitClose();
                it = veth.erase(it);
                break;
            default:
                ++it;
                break;
            }
        }
    }

};
}