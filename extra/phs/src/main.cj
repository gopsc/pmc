package phs

import std.io.*;
import std.fs.*;
import std.collection.*;
import std.sync.*;
import std.process.*;
import stdx.net.http.*;
import stdx.encoding.json.*;

const ADDR = "127.0.0.1"         /* 监听IP地址 */
const PORT = 5200u16           /* 端口 UInt16类型 */


class Ps {
    private let ps   = LinkedList<SubProcess>()
    private let mtx  = Mutex()

    /* 创建进程 */
    public func create(arr: Array<String>) {
        let path = arr[0]
        let args = arr[1..]
        let subp: SubProcess = launch(path, args,
            //stdIn: ProcessRedirect.Discard, /* WINDOWS中不支持输入重定向 */
            //stdOut: ProcessRedirect.Discard, /* 加了这行之后就在控制台看不到输出了 */
            //stdErr: ProcessRedirect.Discard
        )
        this.clear() /* 清理进程 */
        synchronized(mtx) {
            ps.addLast(subp)
            return;
        }
    }

    /* 返回进程列表 */
    public func list() {
        var msg = ""
println('1')
        this.clear() /* 清理进程，有锁 */
println('2')
        synchronized(mtx) {
println('3')
            for (p in ps) {
                if (msg != "") { msg += "\n"; }
                msg += '${p.pid.toString()}\t'
                msg += '${p.command} '
                //for (i in p.arguments) {  /* 加上这个会失败 */
                //    msg += '${i} '
                //}
                /* 启动时间 */
                msg += '\t${p.userTime.toString()}'
            }
println('4')
        }
        return msg;
    }

    /* 杀死已经存在的进程 */
    public func kill(pid: Int64) {
        this.clear() /* 清理 */
        synchronized(mtx) {
            for (p in ps) {
                if (p.pid == pid) {
                    p.terminate(force: true)
                    return
                }
            }
        }
    }

    /*
     * 进程自启动
     *
     * FIXME: 支持注释
     */
    public  func autoRun(csvpath: String) {
        let file = File(csvpath, Read)
        let bytes = readToEnd(file)
        let ctt = String.fromUtf8(bytes)
        let cmds = ctt.split("\n", removeEmpty: true)
        for (cmd in cmds) {
            try{
                let arr = cmd.split(" ", removeEmpty: true)
                this.create(arr)
            } catch (exp: Exception) {
                ;
            }
        }
        file.close()
    }

    /* 清理已经死去的进程，默认不加锁 */
    private func clear() {
        synchronized(mtx) {
            let len = ps.size
            for (i in (len-1)..=0 : -1) {
                try {
                    let p = ps.nodeAt(i).getOrThrow()
println("start...")
                    p.value.wait(timeout: 1 * Duration.millisecond)  /* 进程有没有退出 */
println("end...")
                    ps.remove(p)
                }
                catch (exp: TimeoutException) {                      /* 如果进程没有结束，会运行到这里 */
                     ;                                      
println("skip...")
                }
            }
        }
    }

}


/*
 * 将JSON数组转为字符串的数组
 *
 * Json对象的每个元素只能是字符串，否则会报错
 */
func Jarr_to_arr(arr: JsonArray): Array<String> {
    let len = arr.size() // Json数组的长度
    let ret = Array<String>(len, repeat: "")
    for (i in 0..len) {
        let option: Option<JsonValue> = arr.get(i)
        let jitem:  JsonValue         = option.getOrThrow()
        let jstr:   JsonString        = jitem.asString()
        let str:    String            = jstr.getValue()
        ret[i] = str
    }


    return ret
}

/* cj命令行参数不包含可执行文件名 */
main(_: Array<String>): Int64 {

    /* 进程组 */
    let ps = Ps()
    ps.autoRun("./init.csv")


    /* 创建http服务器 */
    let server: Server = ServerBuilder()
        .addr(ADDR) /* 设置地址 */
        .port(PORT) /* 设置端口 */
        .build()

    /* 创建进程的HTTP方法 */
    server.distributor.register("/api/v2/create", { context =>
        if (context.request.method == "POST") {
            let content = StringReader(context.request.body).readToEnd()    /* 阅读请求体 */ 
            let array = JsonValue.fromStr(content).asArray()                /* 解析json格式的请求体 */
            let net_args: Array<String> = Jarr_to_arr(array)                /* json数组转原生数组 */
            ps.create(net_args)

            /* ---- 准备响应 ---- */
            let Headers = HttpHeaders()
            Headers.add("Content-Type", "text/html; charset=utf-8;")
            context.responseBuilder.setHeaders(Headers)
            context.responseBuilder.body('OK')
        }
    })

    /* 
     * 输出进程列表的HTTP方法
     */
    server.distributor.register("/api/v2/list", { context => 
        if (context.request.method == "GET") {

println('a')
            /* 准备表单字符串 */
            let ret = ps.list()

println('b')
            /**** 准备响应 ****/
            let Headers = HttpHeaders()
            Headers.add("Content-Type", "text/html; charset=utf-8;")
            context.responseBuilder.setHeaders(Headers)
            context.responseBuilder.body(ret)

println('c')
        }
    })

    /* 杀死一个已经存在的进程 */
    server.distributor.register("/api/v2/kill", { context =>
        if (context.request.method == "POST") {
            let content = StringReader(context.request.body).readToEnd()    /* 阅读请求体 */ 
            let target_pid = JsonValue.fromStr(content)
                .asObject()
                .get("pid")
                .getOrThrow()
                .asInt()
                .getValue()
            ps.kill(target_pid)

            /**** 准备响应 ****/
            let Headers = HttpHeaders()
            Headers.add("Content-Type", "text/html; charset=utf-8;")
            context.responseBuilder.setHeaders(Headers)
            context.responseBuilder.body("done")
        }
    })

    /* 服务器开始运行 */
    server.serve()
    return 0

}

