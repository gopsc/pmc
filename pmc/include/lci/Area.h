#pragma once
#include "Drawable.h"
namespace qing {
class Area: public Drawable {
/*
 * Phase Plotting Method 相图法
 *
 * FIXME: 改变区域在宿主对象上的位置
 */
public:
	Area(Drawable *d, int w, int h, int x, int y, int rotate, int fontsize);
	Color _get(int x, int y) override;	/* 实现这些基层抽象方法 */
	void _p(int x, int y, Color &clr) override;
	void _up(int x, int y, int w, int h, int level) override;
	void _down(int x, int y, int w, int h, int level) override;
	void _left(int x, int y, int w, int h, int level) override;
	void _right(int x, int y, int w, int h, int level) override;
	virtual void flush(Drawable *n, bool border = false) override;
	virtual void move(int x, int y);
protected:
	Drawable *d;
};
}
