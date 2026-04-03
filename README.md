# 进程机器（PMC）
这是一个可以创建、管理子进程以及模块的程序。

## pmc是一个什么项目？

这是一个可扩展的模块，可以在文件夹下建立单独的模块，pmc核心模块可以托管运行这些模块。


## 为什么要做一个这样的项目?

2024年的时候，我想做一个树莓派机器人，准备用python来写控制程序，可是当时python的多线程能力较弱，所以想到使用子系统，就自己写了一个进程托管程序


## 项目的文件结构是怎样的？

users用户可以在目录下创建子模块，extra下面还有一些额外的模块。

请留意模块的README文件


## 如何托管外部模块

编辑启动列表程序，常为init.txt


## PMC的安装方式
``` bash   
# 1.安装依赖
sudo apt update
sudo apt install git g++ make
#sudo apt install python3 python3-pip python3-venv
#sudo dnf install python3-virtualenv
sudo apt install libboost-all-dev
#sudo apt install libssl-dev
#sudo apt install cimg-dev
#sudo apt install libcurl4-openssl-dev
#sudo apt install libcurl4-gnutls-dev
#sudo apt install libargs-dev
#sudo apt install libcpp-httplib-dev

2. 创建项目
git clone https://github.com/gopsc/pmc
cd pmc  # 这是pmc模块
make pmc
#sudo make install
```

## 加密通信机制（废弃）
PMC在启动时生成一个AES密钥，然后用一个RSA公钥将其加密发送给访问者。

然后访问者与客户端之间的交流全部用此AES密钥加密


## 如何使用并发机器运行程序
首先应该在模块中创建一个启动脚本，脚本中应该完成工作目录的切换、虚拟环境的激活，以及程序的启动。

## 需要创建启动脚本（废弃）
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


