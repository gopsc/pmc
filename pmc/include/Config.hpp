/*
 * 这个类用于读取配置文件，并且返回成nlohmann::json类型。
 *
 * json配置文件：
 * {
 *     "addr": "0.0.0.0",
 *     "port": 8002,
 *     "retry": 10,
 *     "logs dir": "/var/logs/qsont"
 * }
 *
 *
 * qing(20260105):  现在我通常是用libargs来提取参数
 *                  这个文件可能是之前的配置方式
 */
#pragma once
#include <nlohmann/json.hpp> //nlohmann-json3-dev
#include <fstream>
//using json = nlohmann::json;
namespace qing {
class Config {  /* 打开配置文件，失败时抛出异常。 */
private:
    nlohmann::json conf;    /* 用于存储json对象的成员 */
public:
    Config(std::string path) /* 通过配置文件路径构造 */
    try {
        std::ifstream file;
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        file.open(path);
        file >> conf;
    } catch (const std::ios_base::failure& e) {  /* 输入输出流失败异常 */
        throw ConfigOpenErroring(e.what());
    } catch (const nlohmann::json::parse_error& e) {  /* json解析异常 */
        throw ConfigParseErroring(std::string("Parsing Error: ") + e.what());
    }
    
    /* 运算符重载 - 取下标 */
    const nlohmann::json& operator[] (std::string key) { return conf[key]; }

    class Config_Error: public std::exception { /* 配置异常类 */
        private:
            std::string message;
        public:
            explicit Config_Error(const std::string& msg): message(msg) {}
            const char* what() const noexcept override {
                    return message.c_str();
            }
    };

    class ConfigOpenErroring: public Config_Error { /* 打开配置异常类 */
            public:
                ConfigOpenErroring(const std::string& msg): Config_Error(msg) {}
    };

    class ConfigParseErroring: public Config_Error { /*解析配置异常类 */
        public:
            ConfigParseErroring(const std::string& msg): Config_Error(msg) {}
    };
};
}
