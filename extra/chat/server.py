#!/usr/bin/env python3
#-------------------------------------------------------------
# 以下区域是我的代码
'''

【项目简介】 网络聊天室 v0.0.2

【加入项目】
注意代码只添加不修改，你可以写个函数，署名
不要修改别人的代码。

【基线规则】
在自己的区域修改代码，谁的代码能跑就加入主线
除此之外只在各自的测试区域内运行

【设计思路】
建议一个项目一个文件先，容易分发

【测试方式】
服务器进程： python chat.py --type=server
客户端进程： python chat.py

--------------------------------
--------------------------------
--------------------------------
【项目名称】 网络聊天室 v 0.0.3 （20250524）
我们会将新版本加入pmc.cops中作为常驻服务

【改进计划】
1.我们专门设置了一个只发送一条消息就关闭的函数以供别的文件调用
2.我们将端口暴露在了公网下，以测试连接情况，发现并修复了许多bug。
3.我们设置欢迎信息只在服务端可见。

【使用方法】
python chat.py [--type=server|client] [--addr=localhost] [--port=8003] [--name=Unknow]

【记得反馈！】
期待您将反馈发送至gitee


####################################
####################################
【名称】AI聊天室
【版本】v0.0.4
前后端分离，并且加入AI聊天功能

##################
##################
FIXME: 加入原生自然语言处理
'''
import argparse
import socket
import select
import threading
from datetime import datetime
from openai import OpenAI
import httpx
import os
from pmc import upload_file

# 获取当前脚本所在目录
basedir = os.path.abspath(os.path.dirname(__file__))

# 生成提示词的绝对路径
dir_path = os.path.join(basedir, '..', 'public', 'prompts')

def get_prompts(filename: str):
    ''' 获取system提示词 '''
    path  = os.path.join(dir_path, filename)
    with open(path, 'r') as f:
        system = f.read()
        return system

# 设置 DeepSeek API URL 和密钥
api_base = "https://api.deepseek.com/v1" 
with open("../users/cert/deepseek.key", "r") as f:
    api_key = f.read().strip()


# 创建自定义的 httpx 客户端，禁用 SSL 验证
custom_http_client = httpx.Client(verify=False)

# 初始化客户端
client = OpenAI(
    base_url=api_base,
    api_key=api_key,
    http_client=custom_http_client # 忽略证书
)

def make_sure_agent(name):
    path = os.path.join(dir_path, '.' + name)
    if not os.path.exists(path):
        os.makedirs(path, exist_ok=True)

'''机器人们的聊天记录与设定'''
MySettings = {}

# 用于交流需要
def datestr():
    '''获取时间字符串（不带毫秒）'''
    now = datetime.now()
    time_str = now.strftime("%Y-%m-%d %H:%M:%S")
    return str(time_str)




def send_to_all_clients(clients: dict, exceptone: socket.socket, msg: str):
    """
    向所有已连接的客户端广播消息，可排除指定客户端不发送。

    Args:
        clients (dict): 客户端套接字对象到地址的映射字典
        exceptone (socket.socket): 要排除的客户端套接字对象(不向其发送消息)
        msg (str): 要广播的消息内容

    遍历所有客户端尝试发送消息(排除exceptone指定的客户端)，
    自动清理断开连接的客户端。
    处理发送过程中可能出现的网络错误，维护有效的客户端列表。
    """
    disconnected = []
    for client in clients:
        if client is not exceptone:
            try:
                client.sendall(msg.encode('utf-8'))
            except (BrokenPipeError, ConnectionResetError):
                disconnected.append(client)
    for client in disconnected:
        client.close()
        del clients[client]

MAX_MEM: int = 20
def chat_with_gpt(agent_name:str, user: str):
    '''与gpt进行聊天'''

    # 添加智能体设定，如果之前没有露面
    settings = MySettings.get(agent_name, None)
    if settings is None:
        settings = [{
                "role": "system",
                "content": get_prompts(f'{agent_name}.txt')
            },
                    {
                "role": "system",
                "content": "你有一个遥控器，遥控器上能够看到别人发来的消息。"
            }]
        MySettings[agent_name] = settings
        
    settings.append({"role": "user", "content": user})

    # 进行聊天
    # 调用GPT
    response = client.chat.completions.create(
        model="deepseek-chat",  # 使用DeepSeek模型
        messages=settings,
        temperature=0.7,
    )
    
    # 处理结果
    content: str = response.choices[0].message.content
    settings.append({"role": "assistant", "content": content})
    
    
    # 更新系统提示词 
    # 调用GPT
    rsp = client.chat.completions.create(
        model="deepseek-chat",  # 使用DeepSeek模型
        messages= settings+ [{"role": "user", "content": '''
# 场景
你正在写新的日记。
# 内容
1.聊聊最近发生了啥有趣的事情吧
2.你有什么感想呢？
3.你现在的情况怎么样？
# 输出
准确、详实的输出
'''}],
        temperature=0.4,
    )

    date = datestr()
    new_prompt: str = date + '\n' + rsp.choices[0].message.content

    # 追加提示词，如果有重复就不添加
    flag = True
    for prompt in settings:
        if prompt['role'] == 'system':
            if prompt['content'] == new_prompt:
                flag = False
                break
    if flag:
        settings.append({"role": 'system', 'content': new_prompt})
        print(new_prompt)

        # 创建智能体日记文件夹
        make_sure_agent(agent_name)

        # 写入日记
        n = date
        n = n.replace(':', '-')
        n = n.replace(' ', '_')
        n = n+ '.txt'
        path = os.path.join(dir_path, '.' + agent_name, n)
        with open(path, 'w') as f:
            f.write(new_prompt)
    
    # 从聊天开始处开始遗忘
    chat_start = 0
    #now_type = settings[chat_start]['role']
    #while now_type == 'system':
    #    chat_start += 1
    #    now_type = settings[chat_start]['role']
       
    #print("this chat start -at idx", chat_start)
    if len(settings) > MAX_MEM:
        del settings[chat_start] # 如果有多条system提示词无，就不能再用这个幻数
        del settings[chat_start]	

    return content


def chat_server(addr: str, port: int):
    """
    服务器主函数，管理客户端连接和消息转发。

    Args:
        addr (str): 绑定IP地址
        port (int): 监听端口号

    创建TCP服务器套接字，使用select处理多客户端连接。
    主要功能：
    1. 接受新连接时记录客户端信息并广播通知(不通知新连接客户端本身)
    2. 接收客户端消息并转发给所有其他客户端
    3. 处理客户端断开事件并通知其他客户端
    4. 支持优雅地处理键盘中断关闭服务器

    实现特点：
    - 使用select实现非阻塞IO多路复用
    - 自动清理断开连接的客户端资源
    - 新客户端加入时不向自己发送欢迎消息
    """
    stop_event = threading.Event()
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as servsock:
            servsock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            servsock.bind((addr, port))
            servsock.listen(10)
            socketlist = [servsock]
            clients = {}
            print(f'the chat server listening at {servsock.getsockname()}')
            while not stop_event.is_set():
                read_sockets, _, exception_sockets = select.select(
                    socketlist, [], socketlist, 0.1
                )
                for notified_socket in exception_sockets:
                    print(f"({datestr()}) Exception closing {clients.get(notified_socket, 'unknown')}")
                    notified_socket.close()
                    if notified_socket in clients:
                        del clients[notified_socket]
                    socketlist.remove(notified_socket)
                for notified_socket in read_sockets:
                    if notified_socket == servsock:
                        client_socket, client_address = servsock.accept()
                        #print(f"({datestr()}) New client from {client_address}")
                        if client_socket.fileno() != -1: #有时会出现无效的文件描述符
                            socketlist.append(client_socket)
                            clients[client_socket] = client_address
                            # 欢迎消息由客户端发送
                    else:
                        try:
                            # 这里可以使用select反复读取
                            # 也可以先设置一次传送的最大量
                            data = notified_socket.recv(8192)
                            allmsg = data.decode('utf-8')
                            if data:
                                send_to_all_clients(clients, notified_socket, f"[{datestr()}]{allmsg}")
                            else:
                                raise ConnectionResetError
                            if data:
                                idx = allmsg.find(": ")
                                msg = allmsg[idx+2:]
                                username = allmsg[:idx]
                                if  msg.startswith("<"):  # 聊天发起
                                    idx = msg.find('>')
                                    agent_name = msg[1:idx]
                                    msg = msg[idx + 1:]
                                    chatgpt = chat_with_gpt(agent_name, f'(这是来自用户{username}的消息){msg}')
                                    send_to_all_clients(clients, notified_socket, f"[{datestr()}]{chatgpt}")
                                elif msg.startswith('{'):  # 遥控发起
                                    idx = msg.find('}')
                                    url = msg[1:idx]
                                    msg = msg[idx + 1:]
                                    print(url)
                                    s = url.split(":")
                                    if (len(s) != 2):
                                        send_to_all_clients(clients, notified_socket, f"[{datestr()}] url格式错误")
                                        continue
                                    else:
                                        ip = s[0]
                                        port = int(s[1])
                                        path = os.path.join(basedir, '..', 'public', msg)
                                        post_res = upload_file(f'{path}', ip, port)
                                        send_to_all_clients(clients, notified_socket, f"[{datestr()}]{post_res}")

                                    # post to url...
                                    #send_to_all_clients(clients, notified_socket, f"[{datestr()}]{post_res}")

                            
                        except ConnectionResetError:
                            # 对方断开了
                            print(f"({datestr()}) Connection closed with {clients[notified_socket]}")
                            notified_socket.close()
                            socketlist.remove(notified_socket)
                            del clients[notified_socket]
                        except UnicodeDecodeError:
                            # 看看对方发了什么东西过来
                            print('-------------', end="")
                            print(data, end="")
                            print('-------------')
                        except OSError:
                            # 对方连接之后在一个不恰当的时机断开了
                            ...
    except KeyboardInterrupt:
        print("\nServer shutting down...")
        stop_event.set()

# 以上区域是我的代码
#--------------------------------------------------------------------------------
#--------------------------------------------------------------------------------
# 以下区域是主线，不要修改！
def main():
    parser = argparse.ArgumentParser(description='聊天室配置。')
    parser.add_argument('--type', type=str, default='client', help='选择聊天室的类型')
    parser.add_argument('--addr', type=str, default='localhost', help='选择要连接的地址')
    parser.add_argument('--port', type=int, default=8003, help='选择要连接的地址')
    parser.add_argument('--name', type=str, default='Unknow', help='你的名字是什么')
    args = parser.parse_args()

    if args.type == 'client': recv_client(args.addr, args.port)
    elif args.type == 'server': chat_server(args.addr, args.port)

if __name__ == "__main__":
    main()


