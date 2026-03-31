#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <unistd.h>

struct CpuData {
	std::string cpu_name;
	long user;
	long nice;
	long system;
	long idle;
	long iowait;
	long irq;
	long softirq;
	long steal;
	long guest;
	long guest_nice;

	long total() const {
		return user + nice + system + idle + iowait + irq + softirq + steal;
	}

	long idle_total() const {
		return idle + iowait;
	}
};

class CpuMonitor {
private:
	std::vector<CpuData> read_cpu_stats() {
		std::vector<CpuData> stats;
		std::ifstream file("/proc/stat");
		std::string line;

		while (std::getline(file, line)) {
			if (line.substr(0, 3) == "cpu") {
				std::istringstream iss(line);
				CpuData data;
				iss >> data.cpu_name >> data.user >> data.nice >> data.system
					>> data.idle >> data.iowait >> data.irq >> data.softirq
					>> data.steal >> data.guest >> data.guest_nice;
				stats.push_back(data);
			}
		}

		return stats;
	}

public:
	double get_cpu_usage() {
		auto stats1 = read_cpu_stats();
		sleep(1);
		auto stats2 = read_cpu_stats();

		if (stats1.empty() || stats2.empty()) {
			return -1.0;
		}

		long total_diff = stats2[0].total() - stats1[0].total();
		long idle_diff = stats2[0].idle_total() - stats1[0].idle_total();

		if (total_diff == 0) {
			return 0.0;
		}

		return 100.0 * (total_diff - idle_diff) / total_diff;
	}

	std::vector<double> get_per_cpu_usage() {
		auto stats1 = read_cpu_stats();
		sleep(1);
		auto stats2 = read_cpu_stats();

		std::vector<double> usages;

		for (size_t i =  1; i < stats1.size() && i < stats2.size(); i++) {
			long total_diff = stats2[i].total() - stats1[1].total();
			long idle_diff = stats2[i].idle_total() - stats1[i].idle_total();

			if (total_diff > 0) {
				usages.push_back(100.0 * (total_diff - idle_diff) / total_diff);
			} else {
				usages.push_back(0.0);
			}
		}

		return usages;
	}
};


