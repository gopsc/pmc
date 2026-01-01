import requests
import hashlib
import json

def calculate_sha256(input_string):
    """
    计算输入字符串的SHA-256哈希值
    
    参数:
        input_string (str): 要计算哈希的字符串
    
    返回:
        str: 输入字符串的SHA-256哈希值(十六进制字符串)
    """
    # 创建sha256哈希对象
    sha256_hash = hashlib.sha256()
    
    # 更新哈希对象，需要将字符串编码为字节
    sha256_hash.update(input_string.encode('utf-8'))
    
    # 获取十六进制格式的哈希值
    hex_digest = sha256_hash.hexdigest()
    
    return hex_digest

class DnfsComm:
    '''=====抛出异常表示失败。=====
    Dnfs即树状网络文件系统，该类是用于与dnfs通信的类。
    
    可以获取子节点列表、添加节点、删除节点、更新节点。
    
    计划在以后加入分组获取内容的功能。'''
    
    def __init__(self, url: str) -> None:
        self.url: str = url


    # 获取某个节点下的子节点（0是主节点）
    def get(self, pid: int) -> object:
        rsp = requests.post(self.url + "/get_notes", json={
            "pid": pid
        },)
        if (rsp.status_code == 200):
            return rsp.text
        else:
            raise DnfsComm.GetFailed(rsp.text)
            
    def get_and_trans(self, pid: int) -> list:
        return json.loads(self.get(pid))
    
    # 以pid节点为父节点添加节点
    def add(self, pid: int, title: str, content: str) -> object:
        rsp = requests.post(self.url + "/add_note", json={
            "pid": pid,
            "title": title,
            "content": content
        },)
        if (rsp.status_code == 200 or rsp.status_code == 201):
            return rsp.text
        else:
            raise DnfsComm.AddFailed(rsp.text)

    # 删除一个节点
    def dele(self, id: int) -> object:
        rsp = requests.post(self.url + "/delete_note", json={
            "id": id,
        },)
        if (rsp.status_code == 200):
            return rsp.text
        else:
            raise DnfsComm.DelFailed(rsp.text)

    # 更新id节点
    def upd(self, id: int, pid: int, title: str, content: str) -> object:
        rsp = requests.post(self.url + "/update_note", json={
            "id": id,
            "pid": pid,
            "title": title,
            "content": content
        },)
        if (rsp.status_code == 200):
            return rsp.text
        else:
            raise DnfsComm.UpdFailed(rsp.text)


    class TheExp(Exception):
        def __init__(self, msg: str):
            super().__init__(msg)

    class GetFailed(TheExp):
        def __init__(self, msg: str):
            super().__init__(msg)

    class AddFailed(TheExp):
        def __init__(self, msg: str):
            super().__init__(msg)

    class DelFailed(TheExp):
        def __init__(self, msg: str):
            super().__init__(msg)

    class UpdFailed(TheExp):
        def __init__(self, msg):
            super().__init__(msg)

if __name__ == "__main__":
    comm = DnfsComm("http://101.245.108.250:8006")
    #comm.add(0, "test pkg", "hello")
    #comm.dele(2)
    #comm.upd(id=1, pid=0, title="Users", content=f"""1::admin::{calculate_sha256('baigezi')}::admin
#2::qing::{calculate_sha256('123456')}::normal
#3::test::{calculate_sha256('44112233')}::normal
#4::test2::{calculate_sha256('44112233')}::mormal""")
    #comm.upd(id=2, pid=0, title="Sensors", content='''101::temperature
#102::humidity''')
    comm.upd(id=3, pid=0, title="Protected", content='''''')
    print(comm.get_and_trans(0))
