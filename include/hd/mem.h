#ifndef HD_MEM_H
#define HD_MEM_H
#include <iostream>
#include <sys/sysinfo.h>

struct MemoryInfo {
	long totalMemory;  /* 总内存(bytes) */
	long freeMemory;   /* 空闲内存 */
	long usedMemory;   /* 已用内存 */
	double usageRate;  /* 使用率(%) */
};

MemoryInfo getMemoryUsageWithSysinfo();
#endif
