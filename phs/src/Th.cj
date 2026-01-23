package phs
import std.sync.*;

/*
 * Status 状态
 *
 * FIXME: 在枚举中定义操作函数
 */
enum Stat {
    SHUT | STOP | START | RUNNING
}


/* 
 * Finite Status Machine 有限状态机
 *
 *
 * FIXME(TritiumQin): 1. 柯里化
 *
 * FIXME(qing): 支持自定义状态
 */
open class Fsm {


    /* 状态枚举 */
    private var stat: Stat = STOP

    /* 线程安全 */
    private let mtx: Mutex = Mutex()

    /*
     *  条件变量
     *
     * FIXME: 我不知道这么写意义何在
     */
    private let condition: Condition = synchronized(mtx) {
        mtx.condition()
    }

    /* 辅助条件变量的布尔标志 */
    private var flag: Bool = true


    /* ---------------------------------------------- */

    /*
     * 阻塞等待状态改变，可以传入一个谓词，以确定什么时候终止
     *
     * 在该回调函数中返回true，则等待不会停止
     */
    public func wait(keep!: (Stat) -> Bool = {_ => false}) {
        synchronized(mtx) {
            flag = true
            while(flag || keep(stat)) {
                condition.wait()
            }
        }
    }

    /* ---------------------------------------------- */

    /*
     * 对状态机进行某种操作，输入一个谓词。
     *
     * 会激活堵塞线程
     */
    public func doc(p: (Stat) -> Stat): Stat {
        synchronized(mtx) {
            stat  = p(stat)
            flag=  false // 试图唤醒阻塞的线程
            condition.notifyAll()
            return stat
        }
    }

    /*
     * 对状态机进行某种操作，输入一个谓词。
     *
     * 不会激活堵塞线程
     */
    public func dop(p: (Stat) -> Stat): Stat {
        synchronized(mtx) {
            stat = p(stat)
            return stat
        }
    }


}


/* Controllable Thread 可控制线程
 *
 * FIXME(花间岛):
 *  1. 使用async语法糖
 *  2. 封装iocp
 */
open class Th {

    /* 持有线程类型 */
    private var fut: Option<Future<Int64>> = None

    /* 持有状态机类型 */
    private let fsm: Fsm = Fsm()

    /*----------------------------------------------*/

    /* 持有静止事件回调 */
    private var StopEvent: Option<() -> Unit> = None

    /* 持有唤醒事件回调 */
    private var WakeEvent: Option<() -> Unit> = None

    /* 持有循环事件回调 */
    private var LoopEvent: Option<() -> Unit> = None

    /* 持有清理事件回调 */
    private var ClearEvent: Option<() -> Unit> = None

    /* 持有线程主函数回调 */
    private var Main: Option<() -> Int64> = None // 线程主函数

    /*----------------------------------------------*/

    /* 使用谓词检查线程状态 */
    public func chkp(): Stat {
        return fsm.dop({ stat => return stat })
    }

    /*----------------------------------------------*/

    /* 进入关闭状态 */
    public func shut() {
        fsm.doc({ _ => return SHUT })
    }

    /* 进入静止态 */
    public func stop() {
        fsm.doc({ _ => return STOP })
    }

    /* 进入启动状态 */
    public func start() {
        fsm.doc({ _ => return START })
    }

    /* 进入运行态 */
    public func run() {
        fsm.doc({ _ => return RUNNING })
    }

    /*----------------------------------------------*/

    /* 状态机是否处于静止态 */
    public func isStop() {
        return match( chkp() ) {
            case STOP => true
            case _ => false
    }}

    /* 状态机是否处于关闭态 */
    public func isShut() {
        return match( chkp() ) {
            case SHUT => true
            case _ => false
    }}

    /* 状态机是否处于开始状态 */
    public func isStart() {
        return match( chkp() ) {
            case START => true
            case _ => false
    }}

    /* 状态机是否处于运行态 */
    public func isRunning() {
        return match( chkp() ) {
            case RUNNING => true
            case _ => false
    }}

    /*----------------------------------------------*/

    /* 扮演函数，即设置线程事件 */
    public func Act(

        /* 静止事件 */
        stop!: Option<() -> Unit> = None,

        /* 唤醒事件 */
        wake!: Option<() -> Unit> = None,

        /* 循环事件 */
        loop!: Option<() -> Unit> = None,

        /* 清理事件 */
        clear!:Option<() -> Unit> = None,


        /* 线程主函数 */
        mainloop!: Option<() -> Int64>= None

    )

    {

        /* 只有静止态才能进行设置 */
        if (!isStop()) { return; }

        /* ------------------------------------------------- */

        /* 设置静止事件，确保退出了静止态 */
        StopEvent = stop ?? {=> fsm.wait(keep: {stat =>
                match(stat) {
                    case STOP => true /* 保持等待 */
                    case _ => false
                }
        })}

        /* 唤醒事件，默认直接进入运行态 */
        WakeEvent = wake ?? {=> this.run()}

        /* 循环事件，默认进行延时 */
        LoopEvent = loop ?? {=> sleep(100 * Duration.millisecond)}

        /* 清理事件，默认什么都不做 */
        ClearEvent= clear?? {=> ;}

        /* ------------------------------------------------- */

        /* 线程主函数 */
        Main = mainloop ?? {=>

            /* 回字形循环 外层 */
            while (!isShut()) {
                this.inner()
            }


            return 0
        }

        /* ------------------------------------------------- */

    }


    /* ------------- 回字形循环 内层 -------------- */

    private func inner() {

        /* 静止状态 */
        while(isStop()) { StopEvent.getOrThrow()() }

        /* 唤醒事件 */
        if (isStart()) { WakeEvent.getOrThrow()() }

        /* 运行状态 */
        while(isRunning()) { LoopEvent.getOrThrow()() }

        /* 清理事件 */
        ClearEvent.getOrThrow()()

    }

    /* --------------------------------------------- */

    /*
     * 创建资源，可控线程苏醒
     *
     *   如果在调用中等待线程启动，可能会造成死锁
     *   想了想觉得也没有这个必要
     */
    public func wake() {


        /* 存在微小的冲突可能，所以我不让它进入静止态 */
        this.start()


        /* 创建卵 */
        fut = Some<Future<Int64>>(spawn {
            Main.getOrThrow()()
        })


        /* 等启动
         *
         * 20251020(qing): 如果线程启动太快，该等待可能会导致死锁
         */
        //fsm.wait(keep: {stat => match(stat) {
        //        case START => true
        //        case _ => false
        //}})
    }

    /* 等待线程结束 */
    public func wait() {
        let f = fut.getOrThrow()
        return f.get()
    }

    /* 
     * 关闭线程，然后等待线程结束
     */
    public func shut_and_wait() {
        shut()
        wait()
    }


}
