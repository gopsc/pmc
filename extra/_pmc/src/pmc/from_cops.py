#!/usr/bin/env python3
'''
这是用于访问协处理器http服务的程序
'''
import requests
import os
import argparse
class CopsComm:
    '''与协处理器的通信类'''
    def __init__(self, url: str):
        self.url: str= url
    def http_post(self, module: str):
        response = requests.post(self.url, json={'cmd': module}, headers={'Content-Type': 'application/json'} )
        response.raise_for_status()
        return response.text
if __name__ == "__main__":
    # python3 from_cops.py --url=http://192.168.254.101:8001/api --mod=MPU6050
    parser = argparse.ArgumentParser(description="COPS的http调用示例。")
    parser.add_argument('--url', help='访问的URL')
    parser.add_argument('--mod', help='要输入的指令')
    args = parser.parse_args()
    comm = CopsComm(args.url)
    rsp = comm.http_post(args.mod)
    print(rsp)
 
