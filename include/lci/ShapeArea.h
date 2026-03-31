#pragma once
/*
 * 形状区域
 */
#include "lci/Area.h"
#include "lci/Shape.h"
#include "vector"
#include "memory"
namespace qing {
class ShapeArea: public Area {
public:
	ShapeArea(Drawable *d, int w, int h, int x, int y, int rotate, int fontsize)
	: Area(d, w, h, x, y, rotate, fontsize){}
	size_t size() const { return list.size(); }
	void add(const std::shared_ptr<Shape>& item) {
		list.push_back(item);
	}
	Shape& operator[](const size_t idx) {
		return *(list[idx]);
	}
	const Shape& operator[](const size_t idx) const {
		return *(list[idx]);
	}
	std::vector<std::shared_ptr<Shape>>::iterator begin() { return list.begin(); }
	std::vector<std::shared_ptr<Shape>>::iterator end()   { return list.end();   }

	std::vector<std::shared_ptr<Shape>>::const_iterator begin() const { return list.begin(); }
	std::vector<std::shared_ptr<Shape>>::const_iterator end()   const { return list.end();   }
private:
	std::vector<std::shared_ptr<Shape>> list;
};
}
