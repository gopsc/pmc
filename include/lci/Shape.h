#pragma once
#include "lci/Drawable.h"
#include "lci/Color.h"
namespace qing {
	/* 【形状】
	 * 抽象基类 */
	class Shape {
	public:
		Shape(int w, int h, int x, int y)
		: w(w), h(h), x(x), y(y) {}
		virtual void Draw(Drawable &d, Color clr) = 0;
		int getw() {
			return w;
		}
		void setw(int w) {
			this->w = w;
		}

		int geth() {
			return h;
		}
		void seth(int h) {
			this->h = h;
		}

		int getx() {
			return x;
		}
		void setx(int x) {
			this->x = x;
		}

		int gety() {
			return y;
		}
		void sety(int y) {
			this->y = y;
		}
	private:
		int w, h;
		int x, y;
	};
}
