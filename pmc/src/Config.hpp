/*
 * 这个类用于读取配置文件，并且返回成nlohmann::json类型。
 *
 * 
 * 
 *
 * 配置文件：
 * {
 *     "addr": "0.0.0.0",
 *     "port": 8002,
 *     "retry": 10,
 *     "logs dir": "/var/logs/qsont"
 * }
 *
 */
#pragma once
#include <nlohmann/json.hpp> //nlohmann-json3-dev
#include <fstream>
//using json = nlohmann::json;
namespace qing {
    // 打开配置文件，失败时抛出异常。
    class Config {
        private:
            // 用于存储json对象的成员
            nlohmann::json conf;
        public:
            // 通过配置文件路径构造
            Config(std::string path) try {
                std::ifstream file;
                file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
                file.open(path);
                file >> conf;
            // 输入输出流失败异常
            } catch (const std::ios_base::failure& e) {
                throw ConfigOpenErroring(e.what());
            // json解析异常
            } catch (const nlohmann::json::parse_error& e) {
                throw ConfigParseErroring(std::string("Parsing Error: ") + e.what());
            }

            // 运算符重载 - 取下标
            const nlohmann::json& operator[] (std::string key) {
                return conf[key];
            }

            // ？？这个是用来干啥的？ 前期使用的静态接口
            //
            // 优化建议：删除
            static nlohmann::json OpenConfig(std::string path) try {
                std::ifstream file;
                file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
                file.open(path);
                nlohmann::json conf;
                file >> conf;
                return conf;
            } catch (const std::ios_base::failure& e) {
                throw ConfigOpenErroring(e.what());
            } catch (const nlohmann::json::parse_error& e) {
                throw ConfigParseErroring(std::string("Parsing Error: ") + e.what());
            }

            // 配置异常类
            class Config_Error: public std::exception {
                private:
                    std::string message;
                public:
                    explicit Config_Error(const std::string& msg): message(msg) {}
                    const char* what() const noexcept override {
                         return message.c_str();
                   }
            };

            // 打开配置异常类
            class ConfigOpenErroring: public Config_Error {
                 public:
                     ConfigOpenErroring(const std::string& msg): Config_Error(msg) {}
            };

            // 解析配置异常类
            class ConfigParseErroring: public Config_Error {
                public:
                    ConfigParseErroring(const std::string& msg): Config_Error(msg) {}
            };
    };
}
