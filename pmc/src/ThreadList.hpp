
#include "Thread.hpp"
#include "TaskPool_Interf.hpp"
#include <list>
#include <memory.h>
namespace qing {
    /* 线程列表类 */
    class ThreadList: public TaskPool_Interf {
    private:
        std::list<std::shared_ptr<qing::Thread>> veth{}; // 核心线程组

    public:
        // 激活并且添加线程
        void AddTh(std::shared_ptr<qing::Thread> th) {
            th->Activate();
            veth.push_back(th);
        }

        // 启动所有线程
        void StartTh() {
            for (auto& item: veth) {
                item->WaitStart();
            }
        }

        // 终止所有线程
        virtual void CloseAll() override {
            for(auto& item: veth) {
                item->WaitClose();
            }
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