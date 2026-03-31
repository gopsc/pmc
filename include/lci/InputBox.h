#pragma once
#include "Area.h"
namespace qing {
class InputBox: public Area { /* 输入框 */
public:
	InputBox(Drawable *d, int w, int h, int x, int y, int rotate, int fontsize);
	virtual void print(char *utf8, int line);
};
}
