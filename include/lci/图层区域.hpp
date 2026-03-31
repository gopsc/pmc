/* 图层区域是能够绘制图片的区域类型 */
#define cimg_display 0
#define cimg_use_jpeg
#define cimg_use_png
#include <CImg.h>
#include "cn/中文化.hpp"
#include "lci/Area.h"
使用 命名空间 cimg_library;
命名空间 qing {
类 图层区域: 公开的 Area {  /* 图层区域 */
公开的:
	图层区域(Drawable *d, int w, int h, int x, int y, int rotate, int fontsize, CImg<unsigned char>& img)
	: Area(d, w, h, x, y, rotate, fontsize) {
		/* 储存图片，需要调用图片的构造函数 */
		this->img = img;
	}
	空类型 涂写图像() {
		单精度小数 区域长宽比 = (float)w / h;
		单精度小数 图片长宽比 = (float)img.width() / img.height();
		/* 长宽比越大，代表图片越横向细长
		 * 长宽比越小，代表图片越竖向细长
		 * 长宽比为1时，图片为正方形
		 *
		 * 图片长宽比较大时，取区域长
		 * 图片长宽比较小时，取区域高 */
		逻辑型 长为标准 = 图片长宽比 > 区域长宽比;
		如果(长为标准) {
			auto fixed = scaleByWidth(this->img, this->w);
			CImg<unsigned char> gray = 转灰度图(fixed);
			auto start = (int)(this->h - gray.height()) / 2;
			for (int y = 0; y < gray.height(); ++y) {
				for (int x = 0; x < gray.width(); ++x){
					auto xx = x;
					auto yy = y + start;
					auto clr1 = 颜色转换(gray(x, y), this->clr);
					this->point(xx, yy, clr1);
				}
			}
		}
		否则{ /* 以宽为标准 */
			auto fixed = scaleByHeight(this->img, this->h);
			auto gray = 转灰度图(fixed);
			auto start = (int)(this->w - gray.width()) / 2;
			for (int y = 0; y < gray.height(); ++y) {
				for (int x = 0; x < gray.width(); ++x){
					auto xx = x + start;
					auto yy = y;
					auto clr1 = 颜色转换(gray(x, y), this->clr);
					this->point(xx, yy, clr1);
				}
			}
		}
	}
私有的:
	CImg<unsigned char> img;
	/* 以长为标准缩放 */
	static CImg<unsigned char> scaleByWidth(const CImg<unsigned char>& src, int newWidth) {
		/* 计算缩放比例 */
		float ratio = (float)newWidth / src.width();
		int newHeight = (int)(src.height() * ratio);

		/* 使用_resize方法进行缩放 */
		return src.get_resize(newWidth, newHeight, -100, -100, 5);
	}
	/* 以高为标准缩放 */
	static CImg<unsigned char> scaleByHeight(const CImg<unsigned char>& src, int newHeight) {
		/* 计算缩放比例 */
		float ratio = (float)newHeight / src.height();
		int newWidth = (int)(src.width() * ratio);

		/* 计算_resize方法进行缩放 */
		return src.get_resize(newWidth, newHeight, -100, -100, 5);
	}
	/* 将灰度颜色转换为我们目前画笔的颜色 */
	静态的 Color 颜色转换(无符号 字符 灰度, Color clr) {
		单精度小数 红 = (单精度小数)clr.r / 255;
		单精度小数 绿 = (单精度小数)clr.g / 255;
		单精度小数 蓝 = (单精度小数)clr.b / 255;
		自动的 结果 = Color(红* 灰度*255, 绿*灰度*255, 蓝*灰度*255);
		返回 结果;
	}

	/* 将CImg对象转为灰度图 */
	静态的  CImg<unsigned char> 转灰度图(CImg<unsigned char>& img) {

		if (img.spectrum() == 3) {
			return img.get_normalize(0, 255).get_RGBtoXYZ().get_channel(1);
		}
		else if (img.spectrum() ==4) {
            
			auto rgb_img = t_4_to_3(img);
			return rgb_img.get_RGBtoXYZ().get_channel(1);
		}
		else {
			抛出异常 std::runtime_error("没有对应通道数的算法");
		}
	}

	/* 将4通道图片转为3通道 */
	静态的 CImg<unsigned char> t_4_to_3(CImg<unsigned char>& img) {
		// 创建RGB图像（3通道）
		CImg<unsigned char> rgb_img(img.width(), img.height(), 1, 3);
           
		cimg_forXY(rgb_img, x, y) {
			rgb_img(x, y, 0, 0) = img(x, y, 0, 0);  // R
			rgb_img(x, y, 0, 1) = img(x, y, 0, 1);  // G
			rgb_img(x, y, 0, 2) = img(x, y, 0, 2);  // B
			// 忽略alpha通道 img(x, y, 0, 3)
		}

		return rgb_img;
	}
};
}
