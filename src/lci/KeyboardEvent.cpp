#include <iostream>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "lci/KeyboardEvent.h"
namespace qing {
	/* FIXME: 划分代码块 */
	KeyboardEvent::KeyboardEvent() {
		const char *path = "/dev/input";
		struct dirent *dir;
		DIR *p = opendir(path);
		if(p == NULL) throw std::runtime_error("opendir");
		while ((dir = readdir(p)) != NULL) {
			if (!is_event_with_number(dir->d_name)) continue;
			char full_name[257] = {'\0'};
			snprintf(full_name, sizeof(full_name), "%s/%s", path, dir->d_name);
			struct stat buf;
			if (stat(full_name, &buf) == -1) {
				throw std::runtime_error("stat");
			}

			if(S_ISDIR(buf.st_mode)) continue;

			else if (pool.find(std::string(full_name)) == pool.end()){
				int d = open(full_name, O_RDONLY);
				if (d == -1) {
					;
				} else {
					set_noblocking(d);
					pool.insert(std::make_pair(std::string(full_name), d));
				}
			}
		}
		closedir(p);
	}
	KeyboardEvent::~KeyboardEvent() {
		std::map<std::string, int>::iterator it;
		for (it = pool.begin(); it != pool.end(); ++it) {
			close(it->second);
		}
	}

	/* FIXME: 传入堵塞时间 */
	struct input_event KeyboardEvent::get() {
		fd_set set;
		FD_ZERO(&set);
		for (auto it = pool.begin(); it != pool.end(); ++it) {
			FD_SET(it->second, &set);
		}
		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 60000;
		if (select(this->maxd()+1, &set, NULL, NULL, &timeout) == -1) {
			throw std::runtime_error("select");
		}

		for (auto it = pool.begin(); it != pool.end(); ++it) {
			if (it->second != -1 && FD_ISSET(it->second, &set)) {
				struct input_event ev;
				if (read(it->second, &ev, sizeof(ev)) == sizeof(ev)) {
					return ev;
				}
			}
		}
		throw NoEvent("");
	}
	bool KeyboardEvent::is_event_with_number(const char *filename) {
		size_t len = strlen(filename);
		if (len <= 5) return false;

		if (strncmp(filename, "event", 5) != 0)
			return false;

		for (size_t i=5; i<len; ++i)
			if (!isdigit(filename[i]))
				return false;

		/* 所有条件都满足 */
		return true;
	}

	void KeyboardEvent::set_noblocking(int d) {
		int sig = fcntl(d, F_GETFL, 0);
		if (sig == -1) throw std::runtime_error("fcntl");
		if (fcntl(d, F_SETFL, sig|O_NONBLOCK) == -1)
			throw std::runtime_error("fcntl");
	}

	int KeyboardEvent::maxd() {
		int maxValue = pool.begin()->second;
		std::map<std::string, int>::iterator it;
		for (it = pool.begin(); it != pool.end(); ++it)
			if (it->second > maxValue)
				maxValue = it->second;
		return maxValue;
	}
}
