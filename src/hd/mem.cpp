#include "hd/mem.h"

/* 通过SysInfo接口获取内存使用情况 失败会返回NULL */
MemoryInfo getMemoryUsageWithSysinfo() {
	MemoryInfo memInfo = {0};
	struct sysinfo si;

	if (sysinfo(&si) == 0)
	{
		memInfo.totalMemory = si.totalram * si.mem_unit;
		memInfo.freeMemory = si.freeram * si.mem_unit;
		memInfo.usedMemory = memInfo.totalMemory - memInfo.freeMemory;

		if (memInfo.totalMemory > 0)
			memInfo.usageRate = 100.0 * memInfo.usedMemory / memInfo.totalMemory;
	}
	else
		std::cerr << "sysinfo Call Failed" << std::endl;

	return memInfo;
}
