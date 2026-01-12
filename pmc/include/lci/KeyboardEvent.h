#pragma once
#include <iostream>
#include <string>
#include <map>
#include <linux/input.h>
#define MAX 64 /* FIXME: 这个是不是没用了 */
namespace qing {
class KeyboardEvent{
public:
	KeyboardEvent();
	~KeyboardEvent();
	struct input_event get(); /* 获取键盘事件 */
	class NoEvent: std::runtime_error{ /* 键盘无事件异常 */
	public:
		NoEvent(std::string s): std::runtime_error(s){}
	};
private:
	std::map<std::string, int> pool;
	static bool is_event_with_number(const char *filename);
	static void set_noblocking(int d);
	int maxd();
	static void print(struct input_event ev);
};
}
