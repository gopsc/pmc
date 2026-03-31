/*
 * 用于在可绘制对象上绘制中文文字
 *
 * 绘制时将对象作为参数传入
 */

#include "cn/中文化.hpp"
#include "lci/Drawable.h"
/* 用于文字的绘制，只要不动就线程安全 */
extern unsigned char ttu[];
extern int ttunumber;
extern unsigned char zzhi[];
extern int zzhinumber;

命名空间 qing {
类 字体绘制器 {
私有的:
	/*------ fonts lib ------
	 * 文字库 */
	int font_w, font_h;
	unsigned short *chss = (unsigned short *)zzhi;
	unsigned char *vec = ttu;
	int zn = zzhinumber;
	int vennum = ttunumber;

	unsigned char *drawvect(Drawable* d, int x, int y, unsigned char *vect, int width, int high, Color &clr)
	// x, y是左上角坐标，vect指向矢量图数据，width和high分别代表要绘制的图的宽和高(好像宽需要=高)
	{
		if (!vect) return nullptr;
		int i = 0, j = 1; // 指向第一对数字
		unsigned char ix, iy; // 从数组取出的第一对坐标放在这
		double x1, y1;//, x2, y2; // 绘制直线的实数的坐标
		if (high == 0) high = width; // 调节高宽
		int ii1, jj1, ii2, jj2; // 最后算出的绘制直线的坐标
		int isend = 0; //是否到达曲线尾
		int isallend = 0; // 是否结束整个图像
		do {
			ix = vect[i];
			iy = vect[j];
			isend = ix & 1; // 奇数代表曲线尾
			isallend = iy & 1; // 奇数代表整个图像结束
			if (isallend) {
				i += 2;
				break; // 整个图像结束返回
			}
			if (isend) { // 如果这里曲线结束什么也不画进到下一条曲线
				i += 2;
				j += 2;
				continue;
			}
			ix /= 2;
			iy /= 2;
			x1 = ix;
			y1 = iy;
			x1 = width * x1 / 128.0;
			y1 = high * y1 / 128.0;
			ii1 = (int)(x1 + x);
			jj1 = (int)(y1 + y);
			int k;
			for (k = 0; k < 100000; k++) {
				//x2 = x1;
				//y2 = y1;
				ii2 = ii1;
				jj2 = jj1;
				i += 2;
				j += 2;
				ix = vect[i];
				iy = vect[j];
				isend = ix & 1; //奇数代表曲线尾
				isallend = iy & 1; //奇数代表整个图像结束
				ix /= 2;
				iy /= 2;
				x1 = ix;
				y1 = iy;
				x1 = width * x1 / 128.0;
				y1 = high * y1 / 128.0;
				ii1 = (int)(x1 + x);
				jj1 = (int)(y1 + y);
				d->line(ii1, jj1, ii2, jj2, clr);
				if (isend || isallend)
					break;
			}
			i += 2; //指向下一条曲线
			j += 2;
		} while (!isallend);
		return vect + i;
	}

公开的:
    字体绘制器(const int font_w, const int font_h)
	: font_w(font_w), font_h(font_h) {}
	
	/* 绘制文字 */
	void drawvfont(Drawable* d, int x, int y, unsigned short z, Color &clr) {
		int i, j, k, ii;
		j = -1;
		for (i = 0; i < zn; i++) {
			if (chss[i] == z) {
				j = i;
				break;
			}
		}
		if (j < 0) return;
		k = 0;
		for (i = 0; i < vennum; i++) {
			if (k == j)
				break;
			for (ii =i; ii <vennum; ii++) {
				if (vec[2*ii + 1] & 1) {
					k++;
					i = ii;
					break;
				}
			}
		}
		if (k < j) return;
		drawvect(d, x, y, vec + 2 * i, font_w, font_h, clr);
	}

};
}
