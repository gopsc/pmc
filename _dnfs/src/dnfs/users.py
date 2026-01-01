
'''我们还需要在这里做一些限制 '''

class Users:
    '''一个单独的用户'''
    @staticmethod
    def crt_user(msg: str):
        '''工厂模式：通过一行shadow创建用户'''
        cols = msg.split("::")
        id: int = int(cols[0])
        name: str = cols[1]
        pass_hash: str = cols[2]
        privileges: str = cols[3]
        return Users(id, name, pass_hash, privileges)
    def __init__(self, id: int, name: str, pass_hash: str, privileges: str):
        '''通过参数构造用户'''
        self.id: int = id
        self.name: str = name
        self.pass_hash: str = pass_hash
        self.privileges: str = privileges
    def __str__(self):
        '''将用户转换为一行shadow'''
        return f'{self.id}::{self.name}::{self.pass_hash}::{self.privileges}'
    def get_id(self) -> int:
        return self.id
    def get_name(self) -> str:
        return self.name
    def get_pass_hash(self) -> str:
        return self.pass_hash
    def get_privileges(self) -> str:
        return self.privileges

class UsersTable:
    MAX: int=128
    '''保存一组用户的用户表'''
    def __init__(self, message: str):
        '''输入一个shadow，读出每一行用户'''
        if message == '': return
        rows = message.split("\n")
        self.arr= [Users.crt_user(row) for row in rows]
    def __str__(self):
        '''将用户表转换为shadow'''
        return '\n'.join([str(user) for user in self.arr])
    def user_exists(self, name: str, pass_hash: str) -> tuple:
        '''用户是否存在，是的话返回一个四元组，否则返回None。'''
        for user in self.arr:
            if user.get_name() == name and user.get_pass_hash() == pass_hash:
                return user.get_id(), user.get_name(), user.get_pass_hash(), user.get_privileges()
        return None
    def change_password(self, id: int, new: str):
        '''修改一个用户的密码，找不到就。'''
        for user in self.arr:
            if user.get_id() == id:
                user.pass_hash = new
                return
        raise UsersTable.NotFound()
    def add_user(self, name: str, pass_hash: str, privileges: str):
        '''新增一个用户，自动生成id'''
        id: int = 0
        while id < UsersTable.MAX: # 最多128个
            id += 1
            hello = True
            for user in self.arr:
                if id == user.get_id():
                    hello = False
            if hello: break
        if id >= UsersTable.MAX:
            raise UsersTable.UserOver()
        self.arr.append(Users(id, name, pass_hash, privileges))
    def del_user(self, id: int) -> bool:
        '''根据id删除一个用户。成功返回真，失败返回假。'''
        for user in self.arr:
            if user.get_id() == id:
                self.arr.remove(user)
                return True
        return False
    
    class NotFound(Exception):
        def __init__(self):
            super().__init__('输入用户的id没被找到')
    
    class UserOver(Exception):
        def __init__(self):
            super().__init__(f'超过最大用户数量：{UsersTable.MAX}')
    
if __name__ == "__main__":
    msg = """1::admin::123456::admin
2::qing::654321::normal
3::test::123::normal
4::test1::321::normal"""
    
    table = UsersTable(msg)
    print(table.user_exists('qing', '654321'))
    print(table.user_exists('qing', '123456'))
    table.change_password(1, "new")
    table.add_user(name='added', pass_hash='thepass', privileges="normal")
    table.del_user(2)
    print(str(table))
