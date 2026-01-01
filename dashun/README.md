# dahun
大顺脚本是进程托管程序，能够创建子进程并且管理。

## 使用方法

### 核心服务
```
./dashun --daemon --to=test
```

### 启动进程
```
./dashun --to=test --msg='path/to/bat'
```

### 列举进程
```
./dashun --to=test --msg=list
```

### 杀死进程
```
./dashun --to=test --msg=kill <pid>
```

### 清理进程
```
./dashun --to=test --msg=clear
```

## 编译过程

1.首先安装依赖（boost、openuv）

2.使用管理员模式打开Visual Studio，编译程序

3.使用管理员模式打开两个powershell，并且切换工作目录至生成的exe文件

4.在第一个powershell 中输入指令“./dashun --to=test --daemon”启动核心服务

5.在第二个powershell中输入指令“./dashun --to=test --msg=hello,world!”