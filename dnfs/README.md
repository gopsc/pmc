
# DNFS

这个项目叫做**树状网络文件系统（DNFS）**，是个HTTP服务。

它监听本地**8006**端口，访问者可以枚举、添加、删除、更新节点。

# 接口
```bash
# 获取某一层的节点列表
curl -X POST http://localhost:8006/get_notes -d '{"pid": <integer>}'
# 添加一个节点
curl -X POST http://localhost:8006/add_note -d '{"pid": <int>, "title": <str>, "content": <str>}'
# 删除一个节点
curl -X POST http://localhost:8006/delete_note -d '{"id": <int>}'
# 更新一个节点
curl -X POST http://localhost:8006/update_note -d '{"id": <int>, "pid": <int>, "title": <str>, "content": <str>}'
```