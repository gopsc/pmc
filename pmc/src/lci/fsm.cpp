
#include"lci/fsm.h"
namespace qing{
namespace lci{
    Fsm::Fsm(){
        this->stat = SSTOP;
    }
    
    //------------------------------------------------------
    enum Stat Fsm::chk() {
        std::unique_lock<std::mutex> lck(this->lock);
        return this->stat;
    }//check
    
    void Fsm::suspend(){
        std::unique_lock<std::mutex> lck(this->lock);
        cv.wait(lck);
    }//suspend
    
    //-------------------------------------------------------
    void Fsm::wake() {
        std::unique_lock<std::mutex> lck(this->lock);
        this->stat = SSTART;
        cv.notify_one();
    }//wake
    
    void Fsm::stop() {
        std::unique_lock<std::mutex> lck(this->lock);
        this->stat = SSTOP;
        cv.notify_one();
    }//stop
    
    void Fsm::shut() {
        std::unique_lock<std::mutex> lck(this->lock);
        this->stat = SSHUT;
        cv.notify_one();
    }//close
    
    void Fsm::run() {
	std::unique_lock<std::mutex> lck(this->lock);
        this->stat = SRUNNING;
        cv.notify_one();
    }//run
}//lci	
}//qing
