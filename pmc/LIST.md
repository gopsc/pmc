# 我PMC里跑了哪些服务？
注：该项目的子模块大多属于实验性开发阶段


## TCP8001：协处理器集线器（cops/cops_hub.py）
该模块类似代理，能够访问程序中预存储的URL，并向它们发送POST指令。


## TCP8002：聊天室服务（chat.py）
这是一个TCP通信服务，客户端发送消息，由服务器转发给其他的连接者。

它的客户端分为发送端和接收端。

## TCP8003：ssh协议（sshd.py）
通过ssh协议进行通信，虽然没有运行shell，但能够发送键盘按键信息过去。（遗弃）


## TCP8004：ftp协议（ftpd.py）
能够进行文件传输。


## UDP8004：摄像头UDP图传 (imgs/server.py)
开启一个UDP服务器，向连接者发送我们的摄像头图像。连接者需要发送心跳。


## TCP8005：http集线器（hubs/hubs.py）
开启一个转发器用于从一个端口访问多个服务

## TCP8006：dnfs树状网络文件系统（dnfs/dnfs.py）
能够进行文章的树状存储。

## TCP8007：web终端模拟器（cliw/cliw.py）
这个web命令行能够用来管理PMC系统。


## TCP8008：WEB文件编辑器（guiw/guiw.py）
这个WEB文件编辑器能够对模块进行编辑。

## TCP8009：pmc管理界面（mgmt）
这个模块通过web界面对pmc进行管理。

## TCP8010: 解包升级服务（upd）
首先将文件推送到/dashun_upd，然后通知服务各种细节。由服务将压缩包解包并且执行其中的安装流程。

## TCP8011：pmc界面第二后端（cancerai）
这是个Java的SpringBoot项目，为mgmt提供支持。

## TCP8012：pmc本体
子系统管理界面，api加密通信。（接口详见pmc/README.md）

## TCP8014：duze

## TCP8015: ?

## TCP8016：comm
这是一个社交平台的后端