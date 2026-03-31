/*
 *  该文件是i2c用户界面
 *
 * NOTE:  以C++的方式处理异常可能会更好
 * FIXME: 包含询问方法以确定i2c设备的状态
 */

#include <cstring>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <iostream>
#include <pthread.h>
#include "cn/中文化.hpp"

静态的 整数 文件描述符 = -1;
static pthread_mutex_t mutex;  /* FIXME: 这个锁我们只提供接口给外部使用？ */

逻辑型 I2C_检查() {
    返回 !(文件描述符 < 0);
}

空类型 I2C_初始化(常量 字符* 设备路径 = "/dev/i2c-1") {  /* FIXME: 动态指定设备文件 */
    如果 ((文件描述符 = open(设备路径, O_RDWR)) < 0)
        抛出异常 std::runtime_error( /* FIXME: 使用专用的异常类 */
            "Failed to open I2C bus: " + std::string(strerror(errno))
        );
}

空类型 I2C_清理() {
    close(文件描述符);
}

空类型 I2C_置地址(常量 uint8_t 地址) {
    如果 (ioctl(文件描述符, I2C_SLAVE, 地址) < 0)
        抛出异常 std::runtime_error(
            "Failed to set I2C address: " + std::string(strerror(errno))
        );
}

空类型 I2C_写寄存器(无号8位整 寄存器, 无号8位整 值) {  /* 写寄存器 */
    无号8位整 缓冲区[] = {寄存器, 值};
    如果 (write(文件描述符, 缓冲区, 2) != 2)
        抛出异常 std::runtime_error(
            "I2C write error: " + std::string(strerror(errno))
        );
}

/* 读寄存器 */
无号8位整 I2C_读寄存器(无号8位整 寄存器) {

    无号8位整 值;

    如果 (write(文件描述符, &寄存器, 1) != 1)
        抛出异常 std::runtime_error(
            "I2C read error: " + std::string(strerror(errno))
        );

    如果 (read(文件描述符, &值, 1)!= 1)
        抛出异常 std::runtime_error(
            "I2C read error: " + std::string(strerror(errno))
        );

    返回 值;

}


/* 获取锁 */
空类型 I2C_获取锁() {
    pthread_mutex_lock(&mutex); /* 加锁 */
}

/* 释放锁 */
空类型 I2C_释放锁() {
    pthread_mutex_unlock(&mutex);   /* 解锁 */
}
