#pragma once
namespace qing{
class Rectangle{ /* 矩形类 */
public:
	int w=0; /* width */
	int h=0; /* height */
	int b=0; /* ? */
	Rectangle(int w=0, int h=0, int b=0) {
		this->w=w;
		this->h=h;
		this->b=b;
	}
};
}
