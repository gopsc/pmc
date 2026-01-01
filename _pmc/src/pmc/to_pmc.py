#!/usr/bin/env python3

'''

这个代码是与PMC服务通信的工具

即PMC子系统调用

通过PMC服务，你可以创建和管理进程

===============================

20250701: 版本：0.0.4
使得这个模块可以访问任何地址的pmc服务

20250702：版本 0.0.5
使得这个模块在创建之初就保存一个url

20250709: 版本 0.1.0
使得模块在通信过程中应用RSA和AES加密手段

'''
from .rsa import Rsa_Pri_Key
from .aes import MyAES
import requests
import os
import json
import sys


class PmcComm:
    '''并发机器通信'''

    def __init__(self, url: str):
        self.url: str = url

    # 新库
    ############################
    @staticmethod
    def post_to(url, payload="", retry=10):
        count = 0
        while count < retry:
            count += 1
            try:
                response = requests.post(url, data=payload)
                response.raise_for_status()
                return response.text
            except requests.exceptions.HTTPError:
                ...
        raise Exception("连接失败")
        
        
    @staticmethod
    def get_from(url, retry=10):
        count = 0
        while count < retry:
            count += 1
            try:
                response = requests.get(url)
                response.raise_for_status()
                return response.text
            except requests.exceptions.HTTPError:
                ...
        raise Exception("连接失败")
        
    def get_key(self) -> str:
        return PmcComm.get_from(f"{self.url}/api/v1/get_key")
    
    def get_iv(self) -> str:
        return PmcComm.get_from(f"{self.url}/api/v1/get_iv")
        
    def get_list(self,) -> str:
        ls = PmcComm.get_from(f"{self.url}/api/v1/get_list")
        return self.aes.decrypt(ls).strip() # 第二天早上就出现了空格

    def start(self, module: str) -> str:
        msg = json.dumps({'module': module})
        msg = self.aes.encrypt(msg)
        return PmcComm.post_to(
            f'{self.url}/api/v1/start',
            payload=msg
        )

    def try_start(self, module: str) -> str:
        try:
            self.start(module)
        except Exception as e:
            print(e, file=sys.stderr)
       
    def kill(self, pid: int) -> str:
        msg = json.dumps({'pid': pid})
        msg = self.aes.encrypt(msg)
        return PmcComm.post_to(
            f'{self.url}/api/v1/kill',
            payload=msg
        )

    def clear(self, ) -> str:
        return PmcComm.post_to(f'{self.url}/api/v1/clear')

    def list_mods(self,) -> str:
        ls = PmcComm.get_from(f"{self.url}/api/v1/list_mods")
        return self.aes.decrypt(ls)
    
    
    def Set_RSA_Pri_Key(self, fullpath: str) -> None:
        self.rsa_pri: Rsa_Pro_Key = Rsa_Pri_Key(fullpath)

    def Set_AES_Key(self, key: str, iv: str) -> None:
        self.aes = MyAES(key.decode(), iv.decode())
    
        
    def Try_Get_AES_Key(self, retry: int=10) -> None:
        '''试图获取AES密钥，测试中失败率比较高'''
        key_data = self.get_key()
        key = self.rsa_pri.decrypt(key_data)
        iv_data = self.get_iv()
        iv = self.rsa_pri.decrypt(iv_data)
        self.Set_AES_Key(key, iv)

    def Test():
        # 测试对象
        #url = "http://101.245.108.250:8012"
        url = "http://localhost:8012"

        # 创建交流对象
        comm = PmcComm(url)
        comm.Set_RSA_Pri_Key("private_key.pem")
        comm.Try_Get_AES_Key()
        #msg = comm.start("mgmt")
        #msg = comm.kill(6488)
        #msg =comm.clear()
        msg = comm.get_list()
        print(msg)

