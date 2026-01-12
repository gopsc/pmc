#pragma once
namespace qing {

/* 32位色彩和16位色彩 */
class Color{
public:
	unsigned char r=0,g=0,b=0;
	Color(unsigned char r=0, unsigned char g=0, unsigned char b=0) {
		this->r=r;
		this->g=g;
		this->b=b;
	}
};

extern const Color black;
extern const Color white;
extern const Color green;
extern const Color blue;

/* FIXME: 完成这个类定义 */
class Color16 {
	public:
};
}
