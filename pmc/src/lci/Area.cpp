#include <cstdlib>
#include "lci/utf8.h"
#include "lci/ansi.h"
#include "lci/Drawable.h"
#include "lci/Area.h"
namespace qing {

	/* FIXME: 区域类持有可绘制类的引用 */
	Area::Area(Drawable *d, int w, int h, int x, int y, int rotate = 0, int fontsize=18)
		:Drawable(w, h, x, y, rotate, fontsize) {
		this->d = d;
	}

	/* 获取持有可绘制对象的颜色 */
	Color Area::_get(int x, int y) {
		return d->get_color(this->x + x, this->y + y);
	}

	/* 绘制到持有可绘制对象 */
	void Area::_p(int x, int y, Color &clr) {
		d->point(this->x + x, this->y + y, clr);
	}

	/* 持有可绘制对象区域移动 */	
	void Area::_up(int x, int y, int w, int h, int level) {
		d->up(this->x + x, this->y + y, w, h, level);
	}

	void Area::_down(int x, int y, int w, int h, int level) {
		d->down(this->x + x, this->y + y, w, h, level);
	}

	void Area::_left(int x, int y, int w, int h, int level) {
		d->left(this->x + x, this->y + y, w, h, level);
	}

	void Area::_right(int x, int y, int w, int h, int level) {
		d->right(this->x + x, this->y + y, w, h, level);
	}

	/* 将另一个可绘制对象刷新到这里 */
	void Area::flush(Drawable *n, bool border) {
		Rectangle size = n->get_size();
		Rectangle pos = n->get_pos();
		Color &clr = n->getFontsColor();
		if (border) {
			rectangle(pos.w-1, pos.h-1, size.w+1, size.h+1, clr);
		}

		/* FIXME: ? */
		Area *p = dynamic_cast<Area*>(n);
		if (p != nullptr) return;

		// too slow
		// 太慢了
		for (int x = 0; x < size.w; ++x) {
			for (int y = 0; y < size.h; ++y) {
				Color color = n->get_color(x, y);
				point(pos.w + x, pos.h + y, color);
			}
		}
	}

	/* FIXME:  边界检查应该加在父类方法中 不超出边界 */
	/* FIXME: 将border写入成员中，如果有border连同它一起移动 */
	/* FIXME: rotate=1时不能正常移动 */
	void Area::move(int x, int y) {
		if (x >= 0)
			d->right(this->x, this->y, this->w+x, this->h, x);
		else if (x < 0)
			d->left(this->x+x, this->y, this->w-x, this->h, x);
		if (y >= 0)
			d->down(this->x+x, this->y, this->w, this->h+y, y);
		else if (y <0)
			d->up(this->x+x, this->y+y, this->w, this->h-y, y);
		this->x += x;
		this->y += y;
	}
}
