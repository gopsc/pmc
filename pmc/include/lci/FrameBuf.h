#pragma once
#include "Rectangle.h"
#include "Color.h"
#include "Drawable.h"
#include <iostream>
#include <linux/fb.h>
namespace qing{
/* FIXME: 这个类的绘制点方法可能过于复杂 */
class FrameBuf: public Drawable { /* 帧缓冲区类 */
public:
	FrameBuf(const char *path, int rotate, int fontsize);
	~FrameBuf();
	int get_bpp(); /* bits_per_pixel 像素比特 */
	virtual Color _get(int x, int y) override;
	virtual void _p(int x, int y, Color &color) override;
	virtual void _up(int x, int y, int w, int h, int level) override;
	virtual void _down(int x, int y, int w, int h, int level) override;
	virtual void _left(int x, int y, int w, int h, int level) override;
	virtual void _right(int x, int y, int w, int h, int levle) override;
	virtual void flush(Drawable *d, bool border = false) override;
	void clear(); /* FIXME: 也许把这个方法加到抽象父类中？ */

 	class UnknowBPP: std::runtime_error {	/* 未知标准的像素比特 */
	public:
		UnknowBPP(): std::runtime_error("Unknow the number of Bits Per Pixel.") {}
	};

private:
	int d = -1;
	struct fb_var_screeninfo var_info; /* 可变参数 */
	struct fb_fix_screeninfo fix_info; /* 固定参数 */
	int x_over = 0; /* 横轴过量 */
	int line_jump = 0; /* 行跳 */
	char *buf = nullptr; /* 缓冲区指针 */
	
	/* 根据像素比特的不同分情况讨论 */
	Color get16(__u32 x, __u32 y);
	Color get32(__u32 x, __u32 y);
	void p32(__u32 x, __u32 y, Color &color);
	void p16(__u32 x, __u32 y, Color &color);
};
}
