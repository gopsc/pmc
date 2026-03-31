#include "lci/utf8.h"
#include "lci/ansi.h"
#include "lci/InputBox.h"
namespace qing {
	InputBox::InputBox(Drawable *d, int w, int h, int x, int y, int rotate = 0, int fontsize=18): Area(d, w, h, x, y, rotate, fontsize) {
		this->d = d;
	}

	void InputBox::print(char *utf8, int _line) {

		char *ansi = utf8::to_ansi(utf8);
		char *p = ansi;
		int fontsize = FontSize_get();
		int x_f = 0, y_f = _line * fontsize;
		Rectangle l = get_size();
		while (1) {

			//int size = ansi::size_gb18030((unsigned char*)p, 4);
			
			/* 这个汉字库应该是 GB2312 的 */
			int size = ansi::size_gb2312(*(unsigned char *)p);

			if (*p == '\0') {

				break;

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

				break;

			}

			p += size;

		}
		free(ansi);
		int y2 = y_f + fontsize-2;
		line(x_f + 2, y2, x_f + fontsize - 4, y2, clr);
	}
}
