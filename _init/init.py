#!/usr/bin/env python3
'''

子系统启动脚本
这个脚本的功能是通过PMC调用完成子系统的初始化

'''
from pmc import PmcComm
import os
import csv
import time
script_dir = os.path.dirname(os.path.abspath(__file__))
with open(f"{script_dir}/_init.csv", encoding='utf-8') as f:
    reader = csv.reader(f, delimiter='\n')
    records = [row[0] for row in reader]
def main():
    comm = PmcComm('http://localhost:8012')
    comm.Set_RSA_Pri_Key('private_key.pem')
    comm.Try_Get_AES_Key()
    for item in records:
        if item and not item.startswith('#'):
            comm.try_start(item)
            time.sleep(1)
if __name__ == "__main__":
    main()
