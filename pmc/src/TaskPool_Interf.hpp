/*
 * 问：什么是任务？
 *
 * 答：任务（Task）是计算机科学和操作系统中一个多层次的核心概念，其定义随上下文而变化，
 *
 * 但核心本质是：一个独立的工作单元，需要被系统调度和执行。
 *
 */
#pragma once
// 任务池接口
//
// 任务池接口将被线程池、进程池所实现
// 它们的共同点是可以加入进程，可以关闭进程
//
// 优化建议，去掉这个抽象层
class TaskPool_Interf {
public:
    // 关闭所有的任务
    virtual void CloseAll() = 0;
    // 清除已经停止的任务
    virtual void check_and_clear() = 0;
};
