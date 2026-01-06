/* 
 * 任务池接口
 *
 * 任务池接口将被线程池、进程池所实现
 *
 * 优化建议：去掉这个抽象层
 *
 * FIXME: 抽象为可以容纳进程或者线程的一种任务池
 */
#pragma once
class TaskPool_Interf {
public:
    virtual void CloseAll()         = 0; /* 关闭所有的任务 */
    virtual void check_and_clear()  = 0; /* 清除已经停止的任务 */
};
