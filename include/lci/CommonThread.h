#pragma once

#include <iostream>
#include <stdexcept>
#include <thread>
#include "fsm.h"
#include "ThreadInterface.h"
/*
@qing：包含一个简易的多线程控制机制。
需要共享内存的变量和函数声明为全局变量（通常需要带锁），
线程中单独使用的变量和函数声明到属于它的命名空间中。
每个线程的主函数包括了对其的流程控制代码。
stop，setup，loop，clean函数中填入每个状态下的执行代码。
其中需要用到的函数，又实现在后边。
*/

namespace qing {
namespace lci{
    //线程的主函数
//-----------------------------------------------------------
/*通常情况下，线程的主函数。*/

    //前向声明脚本类型
    class CommonThread: public lci::Fsm, ThreadInterface {
        //一个持有资源线程的状态机类型        
        public:  
            //构造函数.
	        //没有必要输入打印机、脚本、线程的名字，放到全局变量
            CommonThread(std::string name);
	        //暂时删除复制构造函数
	        CommonThread(CommonThread&) = delete;
            //如果把析构函数放在这里，将会导致纯虚函数错误
            virtual ~CommonThread();

            //对线程名字的设置
            void SetLabel(std::string name);
            //对线程名字的获取
            std::string& GetLabel();
            //-----------------------------------------------------------
            //等待线程启动完毕
            //线程启动失败返回失败。线程启动成功或者现程结束返回成功。
            //
            //这个函数在某些情况下有可能会一直等待，直到循环结束
            bool WaitStart(unsigned long n);
        
            void WaitClose();

            virtual void StopEvent() override;
            virtual void WakeEvent() override;
            virtual void LoopEvent() override;
            virtual void ClearEvent() override;

            void thread_main();
            
            private:
            
                //线程的名字
                std::string label;
                //用来存放该子线程的标识符。
                std::thread *thread = NULL;
            };
}
}
