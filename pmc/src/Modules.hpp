#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <sys/stat.h>
namespace qing {
    // 子系统类
    //
    // 优化意见：检索模块内的启动器配置以自动启动
    // 可能会让开发过程麻烦一点
    class Modules {
        public:
        // 构造函数
        // 需要传入子系统的路径
        //
        // 验证是否存在
        Modules(std::string& mods) {
            this->home = mods;
            if (!is_exist(this->home))
                throw SubsystemNotExist(mods);
        }
        
        // 获取子系统下某个模块的启动脚本路径
        // 附带检查 检查是必须的，因为注入的风险
        std::string GetInitp(std::string name) {
            std::string ret = get_mod_init_path(name);
            if (is_exist(ret)) {
                return ret;
            } else {
                throw InitScriptNotExist(ret);
            }
        }
        
        // 列出子系统下的所有模块
        // 优化建议：做抽象
        std::vector<std::string> ListAll() {
            std::vector<std::string> folders;
            try {
                for (const auto& entry: std::filesystem::directory_iterator(this->home)) {
                    if (entry.is_regular_file()) {
                        continue;
                    } else if (entry.is_directory()) {
                        folders.push_back(entry.path().filename().string());
                    }
                }
            }
            catch(const std::filesystem::filesystem_error& e) {
                ListModulesFailed(this->home, e.what());
            }
            return folders;
        }

        // 在该子系统下创建一个初始模块
        void CrtMod(std::string &name) {
            crt_mod_dir(name);
            init_mod_dir(name);
            init_mod_man(name);
            grd_priv(name);
        }
        
        // 删除子系统下一个模块
        void DelMod(std::string &name) {
            std::string fullpath = get_mod_path(name);
            deleteDirectory(fullpath);
        }

        // 超类 模块错误
        class Mods_Exp: public std::runtime_error {
            public:
            explicit Mods_Exp(const std::string& msg): std::runtime_error(msg) {
                ;
            }
        };
        
        // 子系统不存在
        class SubsystemNotExist: public Mods_Exp {
            public:
            explicit SubsystemNotExist(const std::string &name): Mods_Exp("Target subsystem " + name + " not exists.") {
                ;
            }
        };
        
        // 启动脚本不存在
        class InitScriptNotExist: public Mods_Exp {
            public:
            explicit InitScriptNotExist(const std::string &name): Mods_Exp("Target init script " + name + " not exists") {
                ;
            }
        };
        
        // 列举模块失败
        class ListModulesFailed: public  Mods_Exp {
            public:
            explicit ListModulesFailed(const std::string &name, const std::string &reson): Mods_Exp("Error when read subsystem <" + name + "> , because " + reson) {
                ;
            }
        };
        
        // 要创建的路径已经存在
        class AlreadyExist: public Mods_Exp {
            public:
            explicit AlreadyExist(const std::string &name): Mods_Exp(name) {
                ;
            }
        };
        
        // 创建失败
        class CreateFailed: public Mods_Exp {
            public:
            explicit CreateFailed(const std::string &name): Mods_Exp(name) {
                ;
            }
        };
        
        // 提升权限失败
        class GrandPrivilegesFailed: public Mods_Exp {
            public:
            explicit GrandPrivilegesFailed(const std::string &name): Mods_Exp(name) {
                ;
            }
        };
        
        // 删除失败
        class DeleteFailed: public Mods_Exp {
            public:
            explicit DeleteFailed(const std::string &name): Mods_Exp(name) {
                ;
            }
        };
        
        private:

        // 子系统的模块目录
        std::string home;

        // 创建模块目录
        void crt_mod_dir(std::string name) {
            std::string fullpath = get_mod_path(name);
            if (is_exist(fullpath)) {
                throw AlreadyExist(name);
            } else {
                std::filesystem::create_directory(fullpath);
            }
        }

        // 初始化模块目录（创建启动脚本）
        void init_mod_dir(std::string name) {
            std::string fullpath = get_mod_init_path(name);
            if (is_exist(fullpath)) {
                throw AlreadyExist(name);
            } else {
                std::ofstream outFile = std::ofstream(fullpath);
                if (outFile) {
                    outFile << "#!/bin/bash\n";
                    outFile << "cd \"$(dirname \"$0\")\" || exit\n\n";
                    outFile << "exec echo hello!";
                } else {
                    throw CreateFailed(name);
                }
            }
        }

        // 初始化一个模块的手册（README.md）
        void init_mod_man(std::string name) {
            std::string fullpath = get_mod_man_path(name);
            if(is_exist(fullpath)) {
                throw AlreadyExist(fullpath);
            } else {
                std::ofstream outFile = std::ofstream(fullpath);
                if (outFile) {
                    outFile << "# ";
                    outFile << name;
                    outFile << "\n";
                } else {
                    throw CreateFailed(name);
                }
            }
        }

        // 获取模块的路径
        std::string get_mod_path(std::string &name) {
            return this->home + get_sep() + name;
        }

        // 获取模块的启动脚本路径
        std::string get_mod_init_path(std::string &name) {
            return get_mod_path(name) + get_sep()+ "_run.sh";
        }

        // 获取模块的手册路径
        std::string get_mod_man_path(std::string &name) {
            return get_mod_path(name) + get_sep()+ "README.md";
        }
        
        // 赋予一个文件755权限
        void grd_priv(std::string &name) {
            std::string fullpath = get_mod_init_path(name);
            if (chmod(fullpath.c_str(), 0755) == -1) {
                throw GrandPrivilegesFailed("path");
            }
        }
        
        // 获取当前可执行程序所在目录
        static std::string get_curr() {
            return std::filesystem::canonical("/proc/self/exe").parent_path().string();
        }
        
        // 文件是否存在
        static bool is_exist(std::string &path) {
            return std::filesystem::exists(path);
        }
        
        // 获取文件系统分隔符号
        static char get_sep() {
            return std::filesystem::path::preferred_separator;
        }
        
        // 删除一整个文件夹
        static void deleteDirectory(const std::filesystem::path& path) {
            try { // 递归
                std::filesystem::remove_all(path) > 0;
            } catch (const std::filesystem::filesystem_error& e) {
                throw DeleteFailed(e.what());
            }
        }

    };
}
