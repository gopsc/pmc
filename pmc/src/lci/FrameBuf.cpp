#include <iostream>
#include <cstring>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "lci/FrameBuf.h"
#include "lci/Area.h"
namespace qing {

	/* 这个构造函数太长，应该还可以再提炼一下 */
	FrameBuf::FrameBuf(const char *path, int rotate, int fontsize)
	  : Drawable(0, 0, 0, 0, rotate, fontsize) {

		//int open_dev(const char *path)
		{
			if (d != -1) {
				throw std::runtime_error("Device already opening.");
			}

			if ((d = open(path, O_RDWR)) == -1) {
				throw std::runtime_error("Error openning framebuffer device");
			}
		}

		//void init_info()
		{
			if (ioctl(d, FBIOGET_VSCREENINFO, &var_info) < 0) {
				close(d);
				d=-1;
				throw std::runtime_error("Error reading screen variable information");
			}

			if (ioctl(d, FBIOGET_FSCREENINFO, &fix_info) < 0) {
				close(d);
				d=-1;
				throw std::runtime_error("Error reading fixed information");
			}
		}

		std::cout << "res:\t\t" << var_info.xres << "," << var_info.yres << std::endl;
		std::cout << "res_virtual:\t" << var_info.xres_virtual << "," << var_info.yres_virtual << std::endl;
		std::cout << "bpp:\t\t" << var_info.bits_per_pixel << std::endl;

		w = var_info.xres_virtual;
		h = var_info.yres_virtual;


		/* 设置双缓冲区 */
		//var_info.xres_virtual *= 2;
		//var_info.yres_virtual *= 2;


		/* 
		 * 不知道用不用虚拟尺寸，目前是用，在虚拟机上的效果不清楚。我在虚拟机上见过虚拟比实际小的。
		 *
		 * 而我的机器人上则是虚拟尺寸比物理尺寸大。
		 */
		//long v_size = (long)var_info.xres * var_info.yres * var_info.bits_per_pixel / 8;
		long v_size = (long)var_info.xres_virtual * var_info.yres_virtual * var_info.bits_per_pixel / 8;

		long logic_size = (long)fix_info.smem_len;
		if (v_size == logic_size) {
			// 如果实长和虚长是相等的，那么就证明这个缓冲区无过量
			x_over = 0;
		} else if (v_size < logic_size) {
			// 如果实长大于虚长，那么就证明这个缓冲区存在过量
			int delta = logic_size - v_size;
			//double delta_x = (double)delta / var_info.xres;
			double delta_y = (double)delta / var_info.yres;

			// 是否存在小数差距
			if (delta_y - (int)delta_y != 0) {
				close(d);
				d=-1;
				throw std::runtime_error("y_over");
			}

			// 计算横轴过量
			x_over = delta_y * 8 / var_info.bits_per_pixel;

		}

		// 计算行跳量
		line_jump = (var_info.xres_virtual + x_over) * var_info.bits_per_pixel / 8;


		//
		buf = (char*)mmap(buf, fix_info.smem_len, PROT_WRITE, MAP_SHARED, d, fix_info.smem_start);
		if (buf == MAP_FAILED) {
			close(d);
			d=-1;
			throw std::runtime_error("Failed mapping of the frame buffer to memory");
		}

		clear();
	}

	FrameBuf::~FrameBuf() {  /* 析构函数 */
		clear();
		munmap(buf, fix_info.smem_len);
		close(d);
	}

	/* 获取像素比特 */
	int FrameBuf::get_bpp() { return var_info.bits_per_pixel; }

	/* 32位像素比特的绘制点 */
	void FrameBuf::p32(__u32 ux, __u32 uy, Color &color)
	{
		if (ux < 0 || ux >= var_info.xres_virtual 
		  || uy < 0 || uy >= var_info.yres_virtual)
			return;
		char *p = buf + (uy * line_jump + ux * 4);
		*p++ = color.b;
		*p++ = color.g;
		*p++ = color.r;
	}

	/* 16位像素比特的绘制点 */
	void FrameBuf::p16(__u32 ux, __u32 uy, Color &color) {
		if (ux < 0 || ux >= var_info.xres_virtual
		  || uy < 0 || uy >= var_info.yres_virtual)
			return;
		char *p = buf + (uy * line_jump + ux * 2);
		uint16_t pixel_value = 0;
		pixel_value |= (color.r >> 3) << 11; //5
		pixel_value |= (color.g >> 2) << 5;  //6
		pixel_value |= (color.b >> 3);       //5
		*reinterpret_cast<uint16_t*>(p) = pixel_value;
	}

	/* 获取16像素比特时的像素颜色 */
	Color FrameBuf::get16(__u32 ux, __u32 uy) {
		char *p = buf + uy * line_jump + ux * get_bpp() / 8;
		unsigned char r = (*(short*)p >> 11) & 0x1F;
		unsigned char g = (*(short*)p >> 5)  & 0x3F;
		unsigned char b = (*(short*)p)       & 0x1F;
		return Color((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2));
	}

	/* 获取32位像素比特时的像素颜色 */
	Color FrameBuf::get32(__u32 ux, __u32 uy) {
		char *p = buf + uy * line_jump + ux * get_bpp() / 8;
		unsigned char r = *(p++);
		unsigned char g = *(p++);
		unsigned char b = *(p++);
		return Color(r, g, b);
	}

	/* 覆盖获取点方法 */
	Color FrameBuf::_get(int x, int y) {
		switch (get_bpp()) {
			case 16:
				return get16((__u32)x, (__u32)y);
			case 32:
				return get32((__u32)x, (__u32)y);
		}
		throw FrameBuf::UnknowBPP();
	}

	/* 覆盖绘制点方法 */
	void FrameBuf::_p(int x, int y, Color &clr) {

		int bpp = get_bpp();
		if (bpp == 16) {
			p16((__u32)x, (__u32)y, clr);
		} else if (bpp == 32) {
			p32((__u32)x, (__u32)y, clr);
		}

	}

	/* 覆盖上移方法 */
	void FrameBuf::_up(int x, int y, int w, int h, int level){
		int l = get_bpp() / 8;
		int oneline = w * l;
		char *s = buf + y * line_jump + x * l;
		char *d = s + level * line_jump;
		for (int _ = 0; _ < h - level; ++_) {
			memcpy(s, d, oneline);
			s += line_jump;
			d += line_jump;
		}
		for (int _ = 0; _ < level; ++_) {
			memset(s, 0, oneline);
			s += line_jump;
		}
	}

	/* 覆盖下移方法 */
	void FrameBuf::_down(int x, int y, int w, int h, int level) {
		int l = get_bpp() / 8;
		int oneline = w * l;
		long d = level * line_jump;
		char *start = buf + y * line_jump + x * l;
		char *src = start + h * line_jump;
		char *dist = src - d;

		do {

			src -= line_jump;
			dist -= line_jump;
			memcpy(src, dist, oneline);

		} while(dist != start);

		do {

			src -= line_jump;
			memset(src, 0, oneline);

		} while (src != start);

	}

	/* 覆盖左移方法 */
	void FrameBuf::_left(int x, int y, int w, int h, int level) {

		int l = get_bpp() / 8;
		int d = level * l;
		int length = (w - level) * l;
		char *start = buf + y * line_jump + x * l;
		char *src = start + h * line_jump;
		do {
			src -= line_jump;
			memcpy(src, src + d, length);
			memset(src + length, 0, d);
		} while (src != start);
		
	}

	/* 覆盖右移方法 */
	/*!*/
	void FrameBuf::_right(int x, int y, int w, int h, int level) {
		int l = get_bpp() / 8;
		int d = level * l;
		int length = (w - level) * l;
		char *start = buf + y * line_jump + x * l;
		char *p = start + h * line_jump;
		char *tmp = new char[length];
		do {
			p -= line_jump;
			char *src = p + d;
			memcpy(tmp, p, length);
			memcpy(src, tmp, length);
			memset(p, 0, d);
		} while (p != start);
		delete[] tmp;
	}

	/* 清理方法 */
	void FrameBuf::clear() {
		memset(buf, 0, fix_info.smem_len);
	}

	/* 将一个别的可绘制区域刷新到这个上面，也许可以绘制标头 */
	void FrameBuf::flush(Drawable *n, bool border) {
		Color &clr = n->getFontsColor();
		Rectangle size = n->get_size();
		Rectangle pos = n->get_pos();

		if (border) {
			rectangle(pos.w-1, pos.h-1, size.w+2-1, size.h+2-1, clr);
			rectangle(pos.w-2, pos.h-2, size.w+4-1, size.h+4-1, clr);
			rectangle_fill(pos.w-2, pos.h-14-2+1, size.w+4-1, 14, clr);
		}

		Area *p = dynamic_cast<Area*>(n);
		if (p != nullptr) return;

		// too slow 太慢
		for (int x = 0; x < size.w; ++x) {
			for (int y = 0; y < size.h; ++y) {
				Color color = n->get_color(x, y);
				point(pos.w + x, pos.h + y, color);
			}
		}
	}

}
