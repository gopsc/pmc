#pragma once
#include <iostream>
#include <cstddef>
namespace qing {
/* ANSI是WINDOWS操作系统默认的编码格式。 */
class ansi{
public:
	static int size_gb18030(unsigned char *c, size_t maxLen);
	static int size_gb2312(unsigned char first_byte);
	class WrongLength: public std::runtime_error {	/* 非法的长度 */
	public:
		WrongLength(): std::runtime_error("") {}
	};
};
}
