## chat是一个什么项目
该模块是python TCP socket聊天室服务，对每个连接者提供消息转发的服务。

你还可以召唤AI，与它进行对话。

## 如何运行chat服务？

我们在项目中打包了启动脚本```_run.sh```，你只需要安装好依赖的软件，就可以一键创建虚拟环境并且执行它。

```shell
sudo apt update
sudo apt install python3 python3-pip python3-venv
bash _run.sh
```

## 如何连接chat服务？
```shell
python client.py [--addr=localhost] [--port=8003] [--name=Unknow]
```

## 我如何与AI对话

服务器从文件中读取提示词，并且将具有```<chatgpt>```特定前缀的消息转发给AI。

并且具有一定的记忆能力。

## 什么是快捷标签输入

成功连接后输入```chatgpt```将会给接下来的对话添加```<chatgpt>```标签。

在聊天框中输入```where```查看当前的对话标签。

在聊天框中输入```del```以删除当前的前缀。