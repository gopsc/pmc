#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iconv.h>
#include "lci/utf8.h"
namespace qing {
	int utf8::size(const unsigned char c) {
		if ((c >> 7) == 0){
			// 0xxxxxxx
			return 1;
		} else if ((c >> 5) == 0b110) {
			// 110xxxxx 10xxxxxx
			return 2;
		} else if ((c >> 4) == 0b1110) {
			// 1110xxxx 10xxxxxx 10xxxxxx
			return 3;
		} else if ((c >> 3) == 0b11110) {
			// 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
			return 4;
		} else {
			return 0;
		}
	}
	char *utf8::to_ansi(char *utf8String) {
		//@20231120:楠舍科技刘仁
		//@20240528,qing:输出字符串的长度不确定
		iconv_t ic;
		ic = iconv_open("GB18030", "UTF-8");
		if (ic == (iconv_t)-1){
			perror("iconv_open");
			return NULL;
		}
		//设置输出字符串的大小为2倍输入
		size_t InputLength = strlen(utf8String);
		size_t OutputLength = InputLength * 2;

		char *ansiString = (char*)malloc(OutputLength);
		if (ansiString == NULL) {
			perror("malloc");
			iconv_close(ic);
			return NULL;
		}

		char *InputBuffer = (char*)utf8String;
		char *OutputBuffer = ansiString;

		size_t result = iconv(ic, &InputBuffer, &InputLength, &OutputBuffer, &OutputLength);
		if (result == (size_t)-1) {
			perror("iconv");
			free(ansiString);
			iconv_close(ic);
			return NULL;
		}

		iconv_close(ic);

		*OutputBuffer = '\0';
		return ansiString;
	}
}
