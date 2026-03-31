#pragma once
namespace qing {

    class ThreadInterface {

        public:
    
	    //---------------------------------------------
            //程序睡眠阶段的操作函数。
            virtual void StopEvent() = 0;
            //程序设置阶段的操作函数。
            virtual void WakeEvent() = 0;
            //程序运行阶段的操作函数。
            virtual void LoopEvent() = 0;
            //程序清理阶段的操作函数。
            virtual void ClearEvent() = 0;

            //可以将操作函数作为一种事件，可以从外界传入写好的事件函数，
            //那么就需要将操作函数设定为静态函数

    };

}
