#include <cmath>
#include "lci/Drawable.h"
namespace qing {

	/* FIXME: 字体宽高分开传入 */
	Drawable::Drawable(int w, int h, int x, int y, int rotate, int fontsize)  /* 构造可绘制对象 */
       	 : w(w), h(h), x(x), y(y), rotate(rotate), font_w(fontsize), font_h(fontsize) {
		zn = zzhinumber;  /* 初始化字库 */
		vennum = ttunumber;
	}


	Rectangle Drawable::get_size() {  /* 获取表示尺寸的矩形 */
		if (rotate == 0 || rotate == 2) {
			return Rectangle(w, h);
		} else if (rotate == 1 || rotate == 3) {
			return Rectangle(h, w);
		}
		throw Drawable::InvalidRotateMode();
	}

	Rectangle Drawable::get_pos() {  /* 获取表示位置的矩形 */
		return Rectangle(x, y);
	}

	void Drawable::set_pos(Rectangle rect) {  /* 设置表示位置的矩形 */
		x = rect.w;
		y= rect.h;
	}

	Color Drawable::get_color(int x, int y) {  /* 获取颜色 */

		if (rotate == 0)
		{
			return _get(x, y);
		}
		
		else if (rotate == 1)
		{
			return _get(y, h - x - 1);
		}
		
		else if (rotate == 2)
		{
			return _get(w - x - 1, h - y - 1);
		}
		
		else if (rotate == 3)
		{
			return _get(w - y - 1 , x);
		}

		throw Drawable::InvalidRotateMode();
	}

	/* 设置颜色 */
	void Drawable::set_color(Color &color) {
		this->clr = color;
	}

	/* 设置字体大小 */
	void Drawable::FontSize(int fontsize) {
		font_w = fontsize;
		font_h = fontsize;
	}
	
	/* 获取字体大小 */
	int Drawable::FontSize_get() {
		return font_w;
	}

	/* 获取字体颜色 */
	Color &Drawable::getFontsColor() {
		return this->clr;
	}


	void Drawable::point(int x, int y, Color &clr) {  /* 绘制点 */

		if (rotate == 0)
		{
			_p(x, y, clr);
		}
		
		else if (rotate == 1)
		{
			_p(y, h - x - 1, clr);
		}
		
		else if (rotate == 2)
		{
			_p(w - x - 1, h - y - 1, clr);
		}
		
		else if (rotate == 3) {
			_p(w - y - 1 , x, clr);
		}

	}

	void Drawable::line(int x1, int y1, int x2, int y2, Color &clr)  /* 绘制线 */
	{
		double t, dt;
		double i, j;
		i = x1;
		j = y1;
		if (std::fabs(x2 - x1) > std::fabs(y2 - y1))
			dt = 1 / (std::fabs(x2 - x1) * 2.0);
		else
			dt = 1 / (std::fabs(y2 - y1) * 2.0);
		for (t = 0; t <= 1.0; t += dt) {
			point((int)(i+0.5), (int)(j+0.5), clr);
			i += dt * (x2 - x1);
			j += dt * (y2 - y1);
		}
	}

	void Drawable::rectangle(int x, int y, int w, int h, Color &clr)  /* 绘制矩形 */
	{
		line(x, y, x+w, y, clr);
		line(x+w, y, x+w, y+h, clr);
		line(x+w, y+h, x, y+h, clr);
		line(x, y+h, x, y, clr);
	}

	void Drawable::rectangle_fill(int x, int y, int w, int h, Color &clr)  /* 填充矩形 */
	{
		for (int i = 0; i < h; ++i)
		{
			line(x, y+i, x+w, y+i, clr);
		}
	}

	void Drawable::up(int x, int y, int w, int h, int level)  /* 画面上移 */
	{
		switch (rotate)
		{
			case 0: {
				_up(x, y, w, h, level);
				break;
			}
			case 1: {
				_left(y, x, h, w, level);
				break;
			}
			case 2: {
				_down(this->w - x - w, this->h - y - h, w, h, level);
				break;
			}
			case 3: {
				_right(this->w - y - h, x, h, w, level);
				break;
			}
		}
	}

	void Drawable::down(int x, int y, int w, int h, int level)  /* 画面下移 */
	{
		switch (rotate)
		{
			case 0: {
				_down(x, y, w, h, level);
				break;
			}
			case 1: {
				_right(y, x, h, w, level);
				break;
			}
			case 2: {
				_up(this->w - x - w, this->h - y - h, w, h, level);
				break;
			}
			case 3: {
				_left(this->w - y - h, x, h, w, level);
				break;
			}
		}
	}

	void Drawable::left(int x, int y, int w, int h, int level)  /* 画面左移 */
	{
		switch (rotate)
		{
			case 0: {
				_left(x, y, w, h, level);
				break;
			}
			case 1: {
				_down(y, x, h, w, level);
				break;
			}
			case 2: {
				_right(this->w - x - w, this-> h - y - h, w, h, level);
				break;
			}
			case 3: {
				_up(this->w - y - h, x, h, w, level);
				break;
			}
		}
	}

	void Drawable::right(int x, int y, int w, int h, int level) /* 画面右移 */
	{
		switch (rotate)
		{
			case 0: {
				_right(x, y, w, h, level);
				break;
			}
			case 1: {
				_up(y, x, h, w, level);
				break;
			}
			case 2: {
				_left(this->w - x - w, this->h - y - h, w, h, level);
				break;
			}
			case 3: {
				_down(this->w - y - h, x, h, w, level);
				break;
			}
		}
	}

	/* FIXME: 单独放在一个文件里 */
	void Drawable::test() {  /* 绘制测试图案 */

		Rectangle size = get_size();
		Color base;
		int k1 = -1;

		for (int y=0; y < size.h; ++y) {
			Color cur = base;
			int k2 = -1;
			
			for (int x=0; x < size.w; x++) {

				if      (cur.r == 0)   k2 = 1;
				else if (cur.r == 255) k2 = -1;

				cur.r += k2;
				cur.g += k2;
				cur.b += k2;

				point(x, y, cur);

			}

			if      (base.r == 0)   k1 = 1;
			else if (base.r == 255) k1 = -1;

			base.r += k1;
			base.g += k1;
			base.b += k1;
		}
	}

	/* 移动测试 */
	void Drawable::test1()
	{
		Rectangle size = get_size();
		up(0, 0, size.w, size.h, 10);
		down(0, 0, size.w, size.h, 5);
		left(0, 0, size.w, size.h, 10);
		right(0, 0, size.w, size.h, 5);
	}

	unsigned char *Drawable::drawvect(int x, int y, unsigned char *vect, int width, int high, Color &clr)
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
				line(ii1, jj1, ii2, jj2, clr);
				if (isend || isallend)
					break;
			}
			i += 2; //指向下一条曲线
			j += 2;
		} while (!isallend);
		return vect + i;
	}

	void Drawable::drawvfont(int x, int y, unsigned short z, Color &clr) {
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
		drawvect(x, y, vec + 2 * i, font_w, font_h, clr);
	}


}
