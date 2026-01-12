#include <cstring>
#include <vector>
#include "lci/CmdArea.h"
#include "lci/utf8.h"
#include "lci/ansi.h"
constexpr static auto H = 2;
namespace qing {

	CmdArea::CmdArea(Drawable *d, int w, int h, int x, int y, int rotate = 0, int fontsize=18): Area(d, w, h, x, y, rotate, fontsize) {
		auto w_std = w - 2 * x;
		auto w_tar = 16 * fontsize;
		area = std::make_unique<InputBox>(this, (w_tar < w_std) ? w_tar :  w_std, H*fontsize, x, fontsize*2, rotate, fontsize);
	}

	void CmdArea::Input(char c) {
        	std::unique_lock<std::mutex> lck(this->lock);
		this->input += c;
	}

	void CmdArea::ClearInput() {
        	std::unique_lock<std::mutex> lck(this->lock);
		this->input = "";
	}

	void CmdArea::update_input() {
        	std::unique_lock<std::mutex> lck(this->lock);
		area->print((char*)this->input.c_str(), 0);
	}

	void CmdArea::clearBox() {
        	std::unique_lock<std::mutex> lck(this->lock);
		auto clr = black;
		auto size = area->get_size();
		area->rectangle_fill(0, 0, size.w-1, size.h, clr);
	}

	void CmdArea::delete_input() {
        	std::unique_lock<std::mutex> lck(this->lock);
		auto l = this->input.length();
		this->input = this->input.substr(0, l - 1);
	}

	std::string CmdArea::get_input_and_clear() {
        	std::unique_lock<std::mutex> lck(this->lock);
		auto cur = this->input;
		this->input = "";
		return cur;
	}

	void CmdArea::print(char *utf8, int d = 0) {

		auto subpos = area->get_pos();
		auto subsize = area->get_size();
		auto color = black;
		auto fontsize = FontSize_get();
		auto half_f = fontsize / 2;

		this->lock.lock();
		rectangle_fill(subpos.w-1, subpos.h-1, subsize.w+2, subsize.h+2, color);

		char *ansi = utf8::to_ansi(utf8);
		char *p = ansi;
		Rectangle l = get_size();
		while (1) {

			//int size = ansi::size_gb18030((unsigned char*)p, 4);
			/**
			 * 这个老的矢量字库应该是 GB2312 的
			 */
			int size = ansi::size_gb2312(*(unsigned char *)p);

			if (*p == '\n') {

				x_f = 0;
				y_f += fontsize;

			} else if (*p == '\0') {

				break;

			} else if (*p == '\t') {

				int n = x_f / half_f;
				int m = 8 - n % 8;
				x_f += m * half_f;

			} else if (*p == '\r') {
				
				;

			} else if (*p == '\033') {
				p++;
				if (*p != '[' && *p != ']') continue;

				if (*p == ']') {
					p++;
					while (*p != '\007') {
						p++;
					}
				}

				else if (*p == '[') {
					p++;
					int top = 0, t2 = 0;
					char buffer[32], buf2[16];
					while((!(*p > 'a' && *p < 'z') && !(*p >'A' && *p < 'Z')) && (*p != '\0')) {
						buffer[top++] = *(p++);
					}
					if (*p == '\0') break;
					buffer[top++] = *p;
					buffer[top] = '\0';
					//printf("%s\n", buffer);
					int i = 0;
					while (i <= top) {
						if (buffer[i] == ';') {
							buf2[t2] = '\0';
							handle_ansi_code(buf2);
							buf2[0] = '\0';
							t2 = 0;
						} else {
							buf2[t2++] = buffer[i];
						}
						i++;
					}
					buf2[t2] = '\0';
					handle_ansi_code(buf2);
				}

			} else { // 正常的字 

				char c;
				if (size == 1) {
					c = *(p + 1);
					*(p + 1) = '\0';
				}
				drawvfont(x_f, y_f, *(unsigned short*)p, clr);
				if (size == 1) {
					*(p + 1) = c;
				}
				x_f += (size == 1) ? fontsize / 2 : fontsize;

			}

			if (x_f >= l.w - fontsize) {

				y_f += fontsize;
				x_f = 0;

			}

			if (y_f >= l.h - (d + 1) * fontsize) {

				int level = (y_f % fontsize == 0) ? fontsize : y_f % fontsize;
				y_f -= level;
				up(0, 0, l.w, l.h - d * fontsize, level);

			}

			p += size;

		}
		free(ansi);
		subpos.w = ((x_f + fontsize + subsize.w) > l.w - fontsize) ? l.w - fontsize - subsize.w : x_f + fontsize;
		subpos.h = y_f + fontsize;
		area->set_pos(subpos);
		this->lock.unlock();
		this->update_input();
		flush(area.get(), true);
	}

	void CmdArea::handle_ansi_code(char *buffer) {
		static std::vector<std::string> v;
		std::string cur = buffer;
		char c = '\0';
		v.push_back(cur);
		if (!cur.empty() && std::isalpha((c = cur.back()))) {
			for ( const auto& elem: v ) {
				
				if (elem == "32m") {
					Color clr = green;
					set_color(clr);
				} else if (elem == "34m") {
					Color clr = blue;
					set_color(clr);
				} else if (elem == "0m" || elem == "00m") {
					Color clr = white;
					set_color(clr);
				} else if (elem == "1m" || elem == "01m") {
					if (v[0] == "40") {
						;
					}

					if (v[1] == "33") {
						//Color clr = yellow;
						//set_color(clr);
					}
				}
				std::cout << elem << " ";

			}
			v.clear();
			std::cout << std::endl;
		}

	}

}
