#!/usr/bin/env python3
import argparse
import socket

def send_client(msg: str, addr: str, port: int, name: str):
    '''发送消息给聊天室服务器。
    
    需要输入消息、地址、端口、昵称'''
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((addr, port))
            s.sendall(f"{name}: {msg}".encode('utf-8'))
            s.close()
            print('success')
    except ConnectionRefusedError:
        print(f"Could not connect to {addr}:{port}")
    except Exception as e:
        print(f"Unexpected error: {e}")

class ChatComm:
    '''与聊天服务器进行交流的类。'''
    def __init__(self, addr: str, port: int, name: str):
        self.addr  = addr
        self.port  = port
        self. name = name
    def send(self, msg: str):
        send_client(msg, self.addr, self.port, self.name)
def main():
    parser = argparse.ArgumentParser(description='发送到聊天室。')
    parser.add_argument("--msg", type=str, help="要发送的消息")
    parser.add_argument('--addr', type=str, default='localhost', help='选择要连接的地址')
    parser.add_argument('--port', type=int, default=8003, help='选择要连接的地址')
    parser.add_argument('--name', type=str, default='Unknow', help='你的名字是什么')
    args = parser.parse_args()
    comm = ChatComm(args.addr, args.port, args.name)
    comm.send(args.msg)

if __name__ == '__main__':
    main()
