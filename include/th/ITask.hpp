#pragma once 
namespace qing{
class ITask { /* 单个任务通用接口 */
public:
    virtual void start() = 0;     /* 任务开始 */
    virtual void stop() = 0;      /* 任务结束 */
    virtual bool isRunning() = 0; /* 是否在跑 */
};
}
