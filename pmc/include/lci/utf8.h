#pragma once
namespace qing {
class utf8{ /*  */
public:
	static int size(const unsigned char c);
	static char *to_ansi(char *s);
};
}
