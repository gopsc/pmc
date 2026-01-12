
#include <unistd.h>
#include "lci/CommonThread.h"
namespace qing {

	void CommonThread::thread_main() {
           
		while (chk() != SSHUT) {
			/*进入回字形循环，如果不是进入关闭态
			内层循环会一直进行。*/         
                    
			if (chk() == SSTOP){
				/*内层循环，静止状态*/
				StopEvent();
			}

			if (chk() == SSTART) {
				/*由静止状态退出时，如果进入开始状态
				则进入单次的设置操作*/
				WakeEvent();
			}

			if (chk() == SRUNNING) {
				/*由设置阶段退出时，如果是运行状态，则发出提示*/
			}

			while (chk() == SRUNNING) {
				//进入运行态
				LoopEvent();
			}

			/*循环末尾，进行一次清理
			由于之前可能是由静止态直接切换到关闭工作
			在清理时需要检查成员变量是否是被设置过的*/
			ClearEvent();
		}
            
	}//入口
	 
    CommonThread::CommonThread(std::string name) {
        /*构造函数，完成数据域的设置。*/
        this->label = name;
        //创建线程的函数
        this->thread = new std::thread(&CommonThread::thread_main, this);
    }
    
     //清除线程
     //使线程自主退出的函数必须放到外面或者子类叶子上，
     //否则会导致关闭时子类已经被释放，进而导致调用不了
    CommonThread::~CommonThread(){
        if (this->thread){
            delete this->thread;//删除线程类
        }

    } //销毁线程
    
        
    //-----------------------------------------------------------
    //访问器函数

    void CommonThread::SetLabel(std::string name) {
        //对线程名字的设置
        this->label = name;
    }
    
    std::string& CommonThread::GetLabel() {
        //对线程名字的获取
        return this->label;
    }

    //-----------------------------------------------------------
    //等待线程启动完毕
    //线程启动失败返回失败。线程启动成功或者现程结束返回成功。
    bool CommonThread::WaitStart(unsigned long n) {
        for(unsigned long i=0; i<n; i++){
            enum Stat stat = this->chk();
            if		(stat == SSTOP)				return false;
            else if	(stat == SRUNNING || stat == SSHUT)	return true;
            else if     (stat == SSTART)                        usleep(1);
        }//按次循环
        return false;
    }//等线程开始

    //等线程关闭
    void CommonThread::WaitClose() {
        thread->join();
    }
    //-----------------------------------------------------------

    void CommonThread::StopEvent(){
        this->suspend();
    }
    void CommonThread::WakeEvent(){
        this->run();
    }
    void CommonThread::LoopEvent(){
        usleep(10000);
    }
    void CommonThread::ClearEvent() {
    }
}
