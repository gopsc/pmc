
## 为什么要做一个这样的项目

当时想做一个树莓派机器人，是准备用python来写控制程序，可是当时python的多线程能力较弱，

所以想到使用子系统，可是我没有VPN，无法安装ROS，所以就自己写了一个进程托管程序

## 使用方式

### 启动列表
```bash
# 创建启动列表，每一行是一个进程
touch init.csv
```

### 创建进程
```bash
curl -X POST http://localhost:5200/api/v2/create -d '["ls"]
```

### 进程列表
```bash
curl http://localhost:5200/api/v2/list
```
