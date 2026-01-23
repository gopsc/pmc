#pragma once
#include <cmath>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <unistd.h>
#include "../Dashun_linux.hpp"

namespace qing{
class Servo{  /* 适用于180度舵机 */
public:

    /* FIXME: min只能比max小 */
    Servo(int channel, float max=180, float min=0, bool rotate=false)
    : channel(channel), max(max), min(min), rotate(rotate) {}

    /* 控制舵机运行至一个角度 */
    void play(float target) {
        target = std::min(max, target);
        target = std::max(min, target);
        PCA9685_setAngle(
		channel, (rotate) ? 180 - target : target
	);
    }

    /* FIXME: 返回最大值 */
    float get() {
        return max;
    }


private:
    int channel; /* 通道编号 */
    float max;   /* 角度上限 */
    float min;   /* 角度下限 */
    bool rotate; /* 是否反向 */
};
}

namespace qing {
class ActParser{  /* 动作脚本解析器 */
public:

    /* 解析标准输入中的动作脚本 */
    void fromStdin() {
        count  = 0;
        empty_all();
        parse(std::cin);
    }

    /* 解析文件输入流中的动作脚本 */
    void fromFile(const std::string& filename) {
        count  = 0;
        std::ifstream file(filename.c_str(), std::ios::in);
        if (!file.is_open())
            throw std::runtime_error("Open file failed");
        empty_all();
        parse(file);
        file.close();
    }

    /* 解析字符串流中的动作脚本 */
    void fromStr(const std::string& script) {
        std::stringstream ss(script);
        count = 0;
	empty_all();
	parse(ss);
    }

    /* 执行刚刚解析出来的机器人和动作 */
    void play() {
        if (bot.empty() || matrix.empty())
            throw std::runtime_error("No action or robot");
        RobotAction(bot, matrix);
    }

    /* 获得机器人的最大值向量， FIXME: 把逻辑写在外面 */
    std::vector<float> get() {
        auto ret  = std::vector<float>();
        for (auto& item: this->bot) {
            ret.push_back(item.get());
        }
        /* 这个数组中包含机器人每个舵机的角度上限 */
        return ret;
    }

    /* 获取当前这一组动作 */
    std::vector<float> now() {
        if (count >= matrix.size())
            throw std::runtime_error("now finished");
        return matrix[count];
    }

    /* 执行下一组动作 */
    void next() {
        if (count >= matrix.size())
            throw std::runtime_error("No action for next");
        auto v = matrix[count++];
        auto vv = std::vector<std::vector<float>> ({v});
        RobotAction(bot, vv);
    }

    /* 输入下一个动作执行 */
    void next(std::vector<float>& act) {
        count++;
        auto vv = std::vector<std::vector<float>> ({act});
        RobotAction(bot, vv);
    }
private:
    char cs[16] = {0}; /* FIXME: 这个栈有越界风险 */
    int top = 0;
    int count = 0;
    std::vector<float> arr;  /* 临时使用 */
    std::vector<std::vector<float>> matrix; /* 解析出的整套动作 */
    std::vector<Servo> bot; /* 机器人抽象 */
    void parse(std::istream& in) {  /* NOTE: 修改为非静态成员函数能够减少行数 FIXME: 改为使用C++风格的文件操作 */
        empty_tmp();
        char c = 0;
        bool end_flag  = true;
        bool comment_flag  = false;
        bool obj_flag = false;
        while(end_flag) { /* 255 for arm */
            end_flag = (bool)in.get(c);
            //std::cout<<c;
            if ((!comment_flag && c >= '0' && c <= '9') || c == '+' || c == '-') {  /* 读取数字 */
                cs[top++] = c;
                std::cout<<c;
            }
            else if (!comment_flag && c == ' ') {  /* 每个角度以空格结尾 */
                cs[top] = '\0';
                submit_angle();
            }
            else if (!comment_flag && (c == '\r' || c == '\n')) {  /* 忽略回车 */
                ;
            }
            else if (!comment_flag && c == '#') {  /* 注释以#开头 */
                comment_flag = true;
            }
            else if (comment_flag && c == '\n') {  /* 注释以换行结尾 */
                comment_flag = false;
            }
            else if (comment_flag && c != '\n') {  /* 忽略注释 */
                ;
            }
            else if (!comment_flag && !obj_flag && top == 0 && c == '!') {  /* 构建模式 */
                obj_flag = true;
            }
            else if (!comment_flag && obj_flag && (c==';' || end_flag)) {  /* 构建机器人 */
                cs[top] = '\0';
                submit_angle();
                submit_robot();
                obj_flag = false;
            }   
            else if (!comment_flag && !obj_flag && (c == ';' || end_flag)) {   /* 每个动作以分号结尾 */
                cs[top] = '\0';
                submit_angle();
                submit_action();
            }
            else {
                throw std::runtime_error("Invalid action: " + std::to_string(int(c)));
            }
        }
    }

    void submit_angle() {
        if (top>0) {
            float angle = (float)atoi(cs);
            arr.push_back(angle);
            std::cout<<angle<<std::endl;
        }
        cs[0] = '\0';
        top = 0;
    }

    void submit_action() {
        if (!arr.empty()) {
            std::cout<<"recv a action: " << arr.size() << std::endl;
            matrix.push_back(arr);
            arr =  std::vector<float>{};
        }
    }

    void submit_robot() {
        if (!arr.empty()) {
            std::cout << "recv a robot: " << arr.size() << std::endl;
            bot = RobotsFactory(arr);
            arr = std::vector<float>{};
        }
    }

    void empty_tmp() {
        cs[0] = '\0';
        top = 0;
        arr = std::vector<float> {};
    }

    void empty_all() {
        cs[0] = '\0';
        top = 0;
        arr = std::vector<float> {};
        matrix = std::vector<std::vector<float>> {};
        bot = std::vector<Servo> {};
    }

    /* 机器人工厂，FIXME: 这里可以做成舵机工厂就可以了， */
    static std::vector<Servo> RobotsFactory(const std::vector<float>& limit) {
        std::vector<Servo> ss;
        for(int i=0; i<limit.size(); ++i)
            ss.push_back(
                Servo(i, std::abs(limit[i]), 0, (limit[i]>=0)?false:true)
            );
        return ss;
    }

    /* 机器人移动，输入一组舵机对象和一组目标角度 */
    static void RobotMove(std::vector<Servo>& ss, std::vector<float>& target) {
        if (ss.size() + 1 != target.size())
            throw std::runtime_error(
                "Action not match this robot: we are "
                + std::to_string(ss.size())
                + " and " + std::to_string(target.size())
            );

	/* FIXME: 随机下标以消除先后时间差 */
        for (int i=0; i<ss.size(); ++i) {
            ss[i].play(target[i]);
        }

	/* 最后一组数据是毫秒延时 */
	usleep(target[target.size()-1] * 1000);
    }
    
    /* 机器人整套动作 */
    static void RobotAction(
        std::vector<Servo>& ss, std::vector<std::vector<float>>& targets)
    {
        for (auto target: targets)
            RobotMove(ss, target);
    }
};
}
