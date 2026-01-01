
class DnfsNode:
    @staticmethod
    def crt_from_dict(kwargs: tuple):
        return DnfsNode(kwargs['id'], kwargs['pid'], kwargs['title'], kwargs['content'])
    def __init__(self, id: int, pid: int, title: str, content: str):
        self.id: int = id
        self.pid: int = pid
        self.title: str = title
        self.content: str = content
    #def upd_to(self, dnfs: DnfsComm):
    def upd_to(self, dnfs):
        dnfs.upd(self.id, self.pid, self.title, self.content)

class DnfsTable:
    def __init__(self, data: list) -> None:
        self.arr = [DnfsNode.crt_from_dict(node) for node in data]
    def query_by_title(self, title: str) -> DnfsNode:
        for node in self.arr:
            if node.title == title:
                return node
        raise DnfsTable.NotFound(title)
    def query_by_id(self, id: int):
        for node in self.arr:
            if node.id == id:
                return node
        raise DnfsTable.NotFound(str(id))

    
    class DnfsTabExp(Exception):
        def __init__(self, args):
            super().__init__(args)
    
    class NotFound(DnfsTabExp):
        def __init__(self, args):
            super().__init__(args)

if __name__ == '__main__':
    ...
    '''
    dnfs = DnfsComm('http://qsont.xyz:8006')
    list = dnfs.get_and_trans(0)
    dnfs_tab  =DnfsTable(list)
    print(dnfs_tab.query_by_title("").content)
    '''