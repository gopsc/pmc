# 进程机器（PMC）
这是一个可以创建、管理子进程以及模块的程序。



## PMC的安装方式
``` bash   
# 1.安装依赖
sudo apt update
sudo apt install git g++ cmake
#sudo apt install openjdk-17-jdk maven
#sudo apt install python3 python3-pip python3-venv
sudo apt install nlohmann-json3-dev
sudo apt install libboost-dev
sudo apt install libssl-dev

2. 创建项目
mkdir bot
sudo mv bot /
cd /bot
git clone https://github.com/gopsc/pmc
cd /pmc
cd _res   #... (安装libargs 和 cpp_httplib)
cd ../pmc  # 这是pmc模块
make
#sudo make install
```

## 加密通信机制
PMC在启动时生成一个AES密钥，然后用一个RSA公钥将其加密发送给访问者。

然后访问者与客户端之间的交流全部用此AES密钥加密

## 需要创建启动脚本
```bash
# _run.sh
# 启动脚本 - 示例
# 需要执行权限

#!/bin/bash

# 切换工作目录
cd "$(dirname "$0")" || exit

# 准备工作（安装依赖）
#bash ./_set.sh

# 准备工作（比如激活虚拟环境）
#source ./.env/bin/activate

# 执行程序前一定要加exec 否则杀不死
exec python main.py

```



## 如何使用并发机器运行程序
首先应该在模块中创建一个启动脚本，脚本中应该完成工作目录的切换、虚拟环境的激活，以及程序的启动。

注意，下面的API使用示例现在需要经过**RSA+AES加密**才能有效

``` bash
curl -X POST http://localhost:8012/api/v1/start?module=<module_name> -d '["your", "params"]'
```

## 如何列举出所有的进程
```bash
curl -X GET http://localhost:8012/api/v1/get_list -d ""
```

## 如何杀死一个进程
```bash
curl -X POST http://localhost:8012/api/v1/kill -d '{"pid": <process_id>}'
```

## 如何清理死亡的进程
```bash
curl -X POST http://localhost:8012/api/v1/clean -d ""
```

## 如何列出所有的模块
```bash
curl -X GET http://localhost:8012/api/v1/list_mods -d ""
```

## 如何创建一个空的模块
```bash
curl -X POST http://localhost:8012/api/v1/crt_mod -d '{"module": "<name>"}'
```

## 如何删除一个已有模块 (**特别小心**)
```bash
curl -X POST http://localhost:8012/api/v1/del_mod -d '{"module": "<name>"}'
```

# [现有模块列表](https://gitee.com/qingsong-gap/dashun/blob/master/pmc/LIST.md)

# 开发方案
### 1.统一的进程间通信机制
### 2.模块启动参数和虚拟端口
### 3.软件复用
### 4.从github/ftp/ipfs拉取模块
### 5.领域特定语言/脚本语言的启动脚本解释器集成（如v8）
### 6.dll加载器（不依赖宿主机环境）
### 7.启动协议