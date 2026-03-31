/*
 * Area是某个形状上的一个区域，它没有自己的帧缓冲区
 *
 *
 */

#pragma once
#include "Drawable.h"
namespace qing {
class Area: public Drawable {
public:
	/* Phase Plotting Method 相图法 */
	Area(Drawable *d, int w, int h, int x, int y, int rotate, int fontsize);
	Color _get(int x, int y) override;	/* 实现接口 */
	void _p(int x, int y, Color &clr) override;
	virtual void flush(Drawable *n, bool border = false) override;
	virtual void move(int x, int y);
protected:
	Drawable *d; /* 持有的绘制主体 */
};
}
