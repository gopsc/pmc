#include "lci/ansi.h"
namespace qing{
	/* 这个函数并不完全
	int ansi::size_gb18030(unsigned char *p, size_t maxLen) {
		if (*p < 0x80) {
			return 1;
		} else if ((*p > 0x81 && *p <= 0xFE) &&
				((*(p+1) >= 0x40 && *(p+1) <= 0x7E) || (*(p+1) >= 0x80 && *(p+1) <= 0xFE))) {
			return 2;
		} else if (maxLen >= 4 && (*p == 0x90 || *p == 0x91 || *p == 0xE8 || *p == 0xE9 ||
				(*p >= 0x8E && *p <= 0xEF) || (*p >= 0xF8 && *p <= 0xFB)) &&
				(*(p+1) & 0xC0) == 0x80 && (*(p+2) & 0xC0) == 0x80 && (*(p+3) & 0xC0) == 0x80) {
			return 4; //注意这里是简化判断
		} else {
			throw ansi::WrongLength();
		}
	}
	*/
	int ansi::size_gb2312(unsigned char first_byte) {
		return (first_byte >= 0xA1 && first_byte <= 0xFE) ? 2 : 1;
	}
}
