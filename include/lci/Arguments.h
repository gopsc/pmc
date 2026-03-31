#pragma once
#include <iostream>
#include <map>
namespace qing {

/*
 * C++ 的命令行
 */

class Arguments {
public:
	Arguments(int argc, char **argv) {
		for (int i=1; i<argc; ++i) {
			std::string item = argv[i];
			if (item.length() > 1 && item[0] == '-' && item[1] != '-') {
				for (unsigned long j=1; j<item.length(); ++j) {
					if (item.length() == 2 && i < argc-1 && argv[i+1][0] != '-') {
						std::string args = "";
						while (i < argc-1 && argv[i+1][0] != '-') {
							args += (args == "") ? "" : " ";
							args += std::string(argv[++i]);
						}
						mc.insert(std::pair(item[j], args));
						std::cout << item[j] << "=" << args << std::endl;
					} else {
						mc.insert(std::pair(item[j], ""));
						std::cout << item[j] << std::endl;
					}
				}
			} else if (item.length() > 2 && item[0] == '-' && item[1] == '-' && item[2] != '-') {
				std::string arg = get_long_param_key(item);
				size_t pos =arg.find('=');
				if (pos == (long unsigned int)-1) {
					std::cout << arg << std::endl;
					m.insert(std::pair<std::string, std::string>(arg, ""));
				} else {
					std::string k = arg.substr(0, pos);
					std::string v = arg.substr(pos+1, arg.length()-pos-1);
					std::cout << k << "=" << v << std::endl;
					m.insert(std::pair<std::string, std::string>(k, v));
				}

			} else {
				throw WrongArgument(item);
			}

		}
	}

	std::map<std::string, std::string> &get_m() {
		return m;
	}
	std::map<char, std::string> &get_mc() {
		return mc;
	}

	class WrongArgument: std::runtime_error {  /* 错的参数 */
	public:
		WrongArgument(std::string key): std::runtime_error(key) {}
	};

	private:
	std::map<std::string, std::string> m;
	std::map<char, std::string> mc;
			
	std::string get_long_param_key(std::string &s) {
		return s.substr(2, s.length() -2);
	}

};

}
