#pragma once
#include<mutex>
#include<condition_variable>
namespace qing{
namespace lci{
	//用来存储状态的枚举类型
	enum Stat{
	    SSHUT = -1,
	    SSTOP = 0,
	    SSTART = 1,
	    SRUNNING = 2
	};//Status

	class Fsm {//FiniteStateMachine有限状态机
	public:
	    //构造函数
	    Fsm();
	    //检查状态
	    enum Stat chk();
	    //改变状态
	    void wake();
	    void stop();
	    void shut();
	    void run();
	//protected:
	    //等待事件唤醒
	    void suspend();
	private:
	    //锁类型，确保状态机线程安全
	    std::mutex lock;
	    //条件变量，用来执行等待或者唤醒
	    std::condition_variable cv;
	    //存放状态机的状态
	    enum Stat stat;
	};//StateMachine	
}//lci
}//qing
