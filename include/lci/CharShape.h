#pragma once
#include "lci/Shape.h"
#include "lci/字体绘制器.h"
namespace qing {
class CharShape: public Shape {
public:
	CharShape(int w, int h, int x, int y, wchar_t wc)
	: Shape(w, h, x, y), wc(wc), 绘制(w, h) {}

	virtual void Draw(Drawable &d, Color clr) override{
		int w = getw();
		int h = geth();
		int x = getx();
	        int y = gety();
		绘制.drawvfont(&d, x, y, wc, clr);
	}

private:
	wchar_t wc;
	字体绘制器 绘制;
};
}
