#pragma once
#include "Area.h"
#include "InputBox.h"
#include <memory>
#include <mutex>
namespace qing {
/*
 * CmdArea（命令区域）是一种特殊的Area区域
 *
 * 基层绘制方法我们已经在Area中实现了
 */
class CmdArea: public Area {
public:
	CmdArea(Drawable *d, int w, int h, int x, int y, int rotate, int fontsize);
	void Input(char c);
	void ClearInput();
	virtual void print(char *utf8, int d);
	void update_input();
	void delete_input();
	void clearBox();
	std::string get_input_and_clear();
private:
	int x_f=0, y_f=0;
	std::unique_ptr<InputBox> area;
   	std::mutex lock;
	std::string input = "";
	void handle_ansi_code(char *buffer);
};
}
