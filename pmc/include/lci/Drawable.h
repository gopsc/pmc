/*
 * 这个是个抽象类，子类实现抽象的内部接口后
 *
 * 可以调用它已经写好的绘制方法。
 */
#pragma once
#include "Rectangle.h"
#include "Color.h"
#include <iostream>

/* 用于文字的绘制 */
extern unsigned char ttu[];
extern int ttunumber;
extern unsigned char zzhi[];
extern int zzhinumber;

namespace qing {
class Drawable {
public:
	enum class Rotate: char {	/* 表示旋转方向 */
		R_0 = 0,
		R_90 = 1,
		R_180 = 2,
		R_270 = 3
	};

	class InvalidRotateMode: std::runtime_error {	/* 非法的旋转模式 */
	public:
		InvalidRotateMode(): std::runtime_error("Invalid rotate mode.") {}
	};

	/*====== attr 属性区（访问器） ======*/
	Drawable(int w, int h, int x, int y, int rotate, int fontsize);
	virtual Rectangle get_size();
	virtual Rectangle get_pos();
	virtual void set_pos(Rectangle rect);
	virtual Color get_color(int x, int y);
	virtual void set_color(Color &color);
	void FontSize(int fontsize);
	int FontSize_get();
	virtual Color& getFontsColor();
	/*------ basic 基本 ------
	 * 这些类需要由子类分别进行实现，但是子类不直接调用 */
	virtual Color _get(int x, int y) = 0; /* 获取像素点像素 */
	virtual void _p(int x, int y, Color &clr) = 0; /* 绘制像素点像素 */
	virtual void _up(int x, int y, int w, int h, int level) = 0; /* FIXME: 不使用屏幕移动函数 */
	virtual void _down(int x, int y, int w, int h, int level) = 0;
	virtual void _left(int x, int y, int w, int h, int level) = 0;
	virtual void _right(int x, int y, int w, int h, int level) = 0;
	//virtual void _clear(int x, int y, int w, int h) = 0;
	/*====== drawer 绘制 ======
	 * 提供给子类和外部的绘制方法 */
	virtual void point(int x, int y, Color &clr);
	virtual void line(int x1, int y1, int x2, int y2, Color &clr);
	virtual void rectangle(int x, int y, int w, int h, Color &clr);
	virtual void rectangle_fill(int x, int y, int w, int h, Color &clr);
	virtual void up(int x, int y, int w, int h, int level);   /* FIXME: 不使用移动函数 */
	virtual void down(int x, int y, int w, int h, int level); /* FIXME: 这些函数麻烦且消耗资源，不如重新绘制 */
	virtual void left(int x, int y, int w, int h, int level);
	virtual void right(int x, int y, int w, int h, int level);
	virtual void flush(Drawable *n, bool border) = 0;
	/*------ printer 打印 ------
	 * 打印文字
	 *
	 * 只实现了打印文字的基本方法 */
	void drawvfont(int x, int y, unsigned short z, Color &clr);
	/*------ test 测试 ------
	 * 绘制测试图案 */
	virtual void test(); /* 颜色测试 */
	virtual void test1(); /* FIXME: 不使用移动测试 */

protected:
	/*------ window ------
	 * 窗口属性 */
	int w, h, x, y, rotate=0;
	/*------ fonts lib ------
	 * 文字库 */
	int font_w, font_h;
	unsigned short *chss = (unsigned short *)zzhi;
	unsigned char *vec = ttu;
	int zn;
	int vennum;
	/* 绘制文字 */
	unsigned char *drawvect(int x, int y, unsigned char *vect, int width, int high, Color &clr);
	/*------ printer ------*/
	Color clr = green;
	//Color clr = white;
};
}
