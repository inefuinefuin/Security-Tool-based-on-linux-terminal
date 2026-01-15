#pragma once
#include "GeneralFileOper.hpp"

#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <string>
#include <filesystem>
#include <functional>

#include <fmt/core.h>

constexpr std::string_view StateCode[] = {
    "Error: Invalid Command",
    "Error: Input wrong args",
    "Error: Invalid path",
    "Error: Create file failed",
    "Error: Create folder failed",
    "Error: Path is existed",
    "Error: Rmdir/rm-directory must be empty folder.If you want delete recursively, please use rm -r/-R",
};
constexpr std::string_view Text[] = {
    "\033[33mWelcome to Linux Grammar Style of Command Line\nYou can run it on powershell to gain cmd interaction just like linux\033[0m",
    "\033[33mif you want to learn all cmd that I'm provided, please input\033[0m \"\033[31mman cmd\033[0m\"",
    "\033[33mLog out\033[0m",
    "all cmd as follow:\n\n"
    "\033[92mman cmd\033[0m: gain all cmd\n\n"
    "\033[92mpwd\033[0m: return current work path\n\n"
    "\033[92mmkfile file_path_name\033[0m: make new file\n\n"
    "\033[92mmkdir directory_path_name\033[0m: you can create new single directory or the tree of directories\n\n"
    "\033[92mrmdir directory_path\033[0m: you can remove existing empty folder\n\n"
    "\033[92mrm [options(-r/-R)] file_path\033[0m: you can remove file or empty directory without options.If you use option, you can remove file_path recursively\n\n"
    "\033[92mcd file_path\033[0m: you can change directory with absolute_file_path or relative_file_path\n\n"
    "\033[92mls [options(-l)]\033[0m: you can list all file under current_work_space.If you choose -l, you will know every permission\n\n"
    "\033[92mcp source_file/source_directory destination_directory\033[0m: you can copy file or directory to other directory\n\n"
    "\033[92mexit\033[0m: exit command line\n",
};
namespace Terminal_IO{
    inline void Terminal_Print(const std::string_view sv) {std::cout<<sv<<"\n";}
    inline std::vector<std::string> Text_split_inStr(const std::string& inStr){
        std::vector<std::string> CmdSet;
        std::stringstream ss(inStr);
        std::string deInStr;
        while (ss>>deInStr) CmdSet.push_back(deInStr);
        return CmdSet;
    }
}

class Shell{
    std::filesystem::path path_work_dir;
    static void Func_man(std::filesystem::path&,std::vector<std::string>,bool*);
    static void Func_pwd(std::filesystem::path&,std::vector<std::string>,bool*);
    static void Func_mkfile(std::filesystem::path&,std::vector<std::string>,bool*);
    static void Func_mkdir(std::filesystem::path&,std::vector<std::string>,bool*);
    static void Func_rmdir(std::filesystem::path&,std::vector<std::string>,bool*);
    static void Func_rm(std::filesystem::path&,std::vector<std::string>,bool*);
    static void Func_cd(std::filesystem::path&,std::vector<std::string>,bool*);
    static void Func_ls(std::filesystem::path&,std::vector<std::string>,bool*);
    static void Func_cp(std::filesystem::path&,std::vector<std::string>,bool*);
    static void Func_exit(std::filesystem::path&,std::vector<std::string>,bool*);
    const inline static std::map<std::string,std::function<void(std::filesystem::path&,std::vector<std::string>,bool*)>> funcs_cmd{
            {"mkdir",Func_mkdir},{"rmdir",Func_rmdir},{"rm",Func_rm},{"cd",Func_cd},{"ls",Func_ls},
            {"cp",Func_cp},{"exit",Func_exit},{"mkfile",Func_mkfile},{"pwd",Func_pwd},{"man",Func_man},
    };
    static bool isInFuncsCmd(const std::string& singleCmd) {
        if (!funcs_cmd.contains(singleCmd)) return false;
        return true;
    }
public:
    explicit Shell(std::filesystem::path init_path):path_work_dir(init_path){}
    void shell() {
        bool state = true;
        Terminal_IO::Terminal_Print(Text[0]);
        Terminal_IO::Terminal_Print(Text[1]);
        while(state){
            std::cout<<fmt::format("\033[35m{}\033[0m\033[33m$\033[0m",path_work_dir.generic_string());

            std::string inStr; std::getline(std::cin,inStr);
            auto CmdSet = Terminal_IO::Text_split_inStr(inStr);

            if (CmdSet.empty()) continue;
            if (!isInFuncsCmd(CmdSet[0])) {Terminal_IO::Terminal_Print(StateCode[0]); continue;}
            funcs_cmd.at(CmdSet[0])(path_work_dir,std::vector(CmdSet.begin()+1,CmdSet.end()),&state);
        }
        Terminal_IO::Terminal_Print(Text[2]);
    }
};

inline void Shell::Func_man(std::filesystem::path&,std::vector<std::string> vec,bool*) {
    if (vec.size()==1 && vec[0]=="cmd") {
        Terminal_IO::Terminal_Print(Text[3]); return;
    }
    Terminal_IO::Terminal_Print(StateCode[1]);
}
inline void Shell::Func_pwd(std::filesystem::path& pwd,std::vector<std::string> vec,bool*) {
    if (!vec.empty()) {Terminal_IO::Terminal_Print(StateCode[1]);return;}
    Terminal_IO::Terminal_Print(pwd.generic_string());
}
inline void Shell::Func_mkfile(std::filesystem::path& pwd,std::vector<std::string> vec,bool*) {
    if (vec.size()!=1){Terminal_IO::Terminal_Print(StateCode[1]);return;}
    std::filesystem::path filepath(vec[0]);
    try {
        auto [target,ec] = File_Path_Correct_TryMake(pwd,filepath);
        if (ec||std::filesystem::exists(target)){
            Terminal_IO::Terminal_Print(StateCode[1]); return;
        }
        File_SingleFile_Create(target);
    }catch (std::filesystem::filesystem_error& e) {
        Terminal_IO::Terminal_Print(StateCode[1]);
        Terminal_IO::Terminal_Print(e.what());
    }
}
inline void Shell::Func_mkdir(std::filesystem::path& pwd,std::vector<std::string> vec,bool*) {
    if (vec.size()!=1) {Terminal_IO::Terminal_Print(StateCode[1]);return;}
    std::filesystem::path dirPath(vec[0]);
    try {
        auto [target,ec] = File_Path_Correct_TryMake(pwd,dirPath);
        if (ec||std::filesystem::exists(target)) {
            Terminal_IO::Terminal_Print(StateCode[5]); return;
        }
        File_StoreFolder_Create(target);
        if (!std::filesystem::exists(target)) {
            Terminal_IO::Terminal_Print(StateCode[4]); return;
        }
    }catch (std::filesystem::filesystem_error& e) {
        Terminal_IO::Terminal_Print(StateCode[1]);
        Terminal_IO::Terminal_Print(e.what());
    }
}
inline void Shell::Func_rmdir(std::filesystem::path& pwd,std::vector<std::string> vec,bool*) {
    if (vec.size()!=1) {Terminal_IO::Terminal_Print(StateCode[1]);return;}
    std::filesystem::path dirPath(vec[0]);
    try {
        auto [target,ec] = File_Path_Correct_TryMake(pwd,dirPath);
        if (ec||!std::filesystem::exists(target)||!std::filesystem::is_directory(target)) {
            Terminal_IO::Terminal_Print(StateCode[1]); return;
        }
        if (!File_Folder_CheckEmpty(target)){Terminal_IO::Terminal_Print(StateCode[6]);return;}
        std::filesystem::remove(target);
    }catch (std::filesystem::filesystem_error& e) {
        Terminal_IO::Terminal_Print(StateCode[1]);
        Terminal_IO::Terminal_Print(e.what());
    }
}
inline void Shell::Func_rm(std::filesystem::path& pwd,std::vector<std::string> vec,bool*) {
    if (vec.empty()) {Terminal_IO::Terminal_Print(StateCode[1]);return;}
    if (vec.size()==1) {
        std::filesystem::path dirPath(vec[0]);
        try {
            auto [target,ec] = File_Path_Correct_TryMake(pwd,dirPath);
            if (ec||!std::filesystem::exists(target)) {
                Terminal_IO::Terminal_Print(StateCode[1]); return;
            }
            if (File_Folder_CheckEmpty(target)||std::filesystem::is_regular_file(target)) {
                std::filesystem::remove(target);
            }else {
                Terminal_IO::Terminal_Print(StateCode[6]);
            }
        }catch (std::filesystem::filesystem_error& e) {
            Terminal_IO::Terminal_Print(StateCode[1]);
            Terminal_IO::Terminal_Print(e.what());
        }
    }
    else if (vec.size()==2&&(vec[0]=="-r"||vec[0]=="-R")) {
        std::filesystem::path dirPath(vec[1]);
        try {
            auto [target,ec] = File_Path_Correct_TryMake(pwd,dirPath);
            if (ec||!std::filesystem::exists(target)) {
                Terminal_IO::Terminal_Print(StateCode[1]); return;
            }
            File_General_Clean(target);
        }catch (std::filesystem::filesystem_error& e) {
            Terminal_IO::Terminal_Print(StateCode[1]);
            Terminal_IO::Terminal_Print(e.what());
        }
    }
    else {
        Terminal_IO::Terminal_Print(StateCode[1]);
    }
}
inline void Shell::Func_cd(std::filesystem::path& pwd,std::vector<std::string> vec,bool*) {
    if (vec.size()!=1) {Terminal_IO::Terminal_Print(StateCode[1]);return;}
    std::filesystem::path dirPath(vec[0]);
    try {
        auto [target,ec] = File_Path_Correct_TryMake(pwd,dirPath);
        if (ec||!std::filesystem::exists(target)||!std::filesystem::is_directory(target)) {
            Terminal_IO::Terminal_Print(StateCode[2]);return;
        }
        pwd = target;
    }catch (std::filesystem::filesystem_error& e) {
        Terminal_IO::Terminal_Print(StateCode[1]);
        Terminal_IO::Terminal_Print(e.what());
    }
}
inline void Shell::Func_ls(std::filesystem::path& pwd,std::vector<std::string> vec,bool*) {
    if (vec.empty()) {
        for (const auto& entry: std::filesystem::directory_iterator(pwd)) {
            if(std::filesystem::is_directory(entry.path())){
                std::cout<<fmt::format("\033[93m{}\033[0m ",entry.path().filename().string());
            }else{
                std::cout<<fmt::format("\033[92m{}\033[0m ",entry.path().filename().string());
            }
        }
        std::cout<<"\n";
    }
    else if (vec.size()==1 && vec[0]=="-l") {
        for (const auto& entry: std::filesystem::directory_iterator(pwd)) {
            if(std::filesystem::is_directory(entry.path())){
                std::cout<<fmt::format("{} \033[93m{}\033[0m\n",File_Permission_Gain(entry.path()),entry.path().filename().string());
            }else{
                std::cout<<fmt::format("{} \033[92m{}\033[0m\n",File_Permission_Gain(entry.path()),entry.path().filename().string());
            }
        }
    }
    else {
        Terminal_IO::Terminal_Print(StateCode[1]);
    }

}
inline void Shell::Func_cp(std::filesystem::path& pwd,std::vector<std::string> vec,bool*) {
    if (vec.size()!=2) {Terminal_IO::Terminal_Print(StateCode[1]); return;}
    std::filesystem::path source(vec[0]);
    std::filesystem::path destination(vec[1]);
    try {
        auto [source_target,source_ec] = File_Path_Correct_TryMake(pwd,source);
        if (source_ec||!std::filesystem::exists(source_target)) {
            Terminal_IO::Terminal_Print(StateCode[1]); return;
        }
        auto [destination_target,destination_ec] = File_Path_Correct_TryMake(pwd,destination);
        if (destination_ec) {
            Terminal_IO::Terminal_Print(StateCode[1]); return;
        }
        if (!std::filesystem::exists(destination_target)) std::filesystem::create_directories(destination_target);

        if (std::filesystem::is_regular_file(source_target)) {
            std::filesystem::copy_file(source_target,destination_target/source_target.filename(),std::filesystem::copy_options::overwrite_existing);
        }else if (std::filesystem::is_directory(source_target)) {
            std::filesystem::copy(source_target,destination_target,std::filesystem::copy_options::recursive|std::filesystem::copy_options::overwrite_existing);
        }else {
            Terminal_IO::Terminal_Print(StateCode[1]);
        }
    }catch (std::filesystem::filesystem_error& e) {
        Terminal_IO::Terminal_Print(StateCode[1]);
        Terminal_IO::Terminal_Print(e.what());
    }
}
inline void Shell::Func_exit(std::filesystem::path&,std::vector<std::string> vec,bool* sts) {
    if (!vec.empty()) Terminal_IO::Terminal_Print(StateCode[1]);
    *sts = false;
}


// ===== Simply Shell Interface =====
inline void Terminal_Shell_Interface(std::filesystem::path target_path) {
    Shell sh(target_path);
    sh.shell();
}

// ===== System Shell Interface =====
inline void System_Shell_Interface(std::filesystem::path target_path) {
     std::string cmd = "bash -c 'bash --rcfile <(echo '\"'\"'PS1=\"\\e[35m\\w\\e[0m\\e[33m$\\e[0m \"'\"'\"') -i'";
    cmd = "cd \"" + target_path.string() + "\" && " + cmd;
    system(cmd.data());
}

// int main(){
//     Terminal_Shell_Interface(std::filesystem::current_path());
// }
