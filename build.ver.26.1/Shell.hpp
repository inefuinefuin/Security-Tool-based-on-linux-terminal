#pragma once
//WIN32 wait test
#ifdef _WIN32
    #include <format>
    #include <windows.h>
    namespace fmt_lib = std;
#else
    #include <fmt/core.h>
    #include <fmt/xchar.h>
    #include <unistd.h>
    namespace fmt_lib = fmt;
#endif


#include "GeneralFileOper.hpp"
#include "typelib.hpp"

#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>
#include <functional>
#include <cstdlib>



// ===== Feather Shell Core =====
// cd(do) ls(do) pwd(do) mkdir(do) rm(do) rmdir(do) cp(do) mv(do) touch(do) nano(do) vim(do) man(do) exit(do)
// echo kid_shell
// do // enc dec temp compressenc decompressdec tartemp

// ===== Error Handling =====
enum class ShellStatus{
    StatusNULL,
    ExitCode,
    Success,
    InvalidCommand,
    WrongArguments,
    PermissionDenied,
    PathExists,
    InvalidPath,
    FileNotFound,
    ParaParseNULL,
    FileCreateFailed,
    FileOpenFailed,
    UnknownError,
};
struct StatusMessage {
    ShellStatus status;
    std::string message;
    std::string hint;
};
const StatusMessage StatusMessage_Map[]={
    {ShellStatus::ExitCode,"",""},
    {ShellStatus::StatusNULL,"",""},
    {ShellStatus::Success,"",""},
    {ShellStatus::ParaParseNULL,"Parameter parsing failed.","Check the command syntax and try again."},
    {ShellStatus::InvalidCommand,"Invalid command.","Type 'man' to see available commands."},
    {ShellStatus::WrongArguments,"Wrong number of arguments.","Check the command usage with 'man <command>'."},
    {ShellStatus::PermissionDenied,"Permission denied.","Check your access rights for the specified path."},
    {ShellStatus::PathExists,"Path already exists.","Choose a different name or remove the existing file/folder."},
    {ShellStatus::InvalidPath,"Invalid path.","Ensure the path is correct and try again."},
    {ShellStatus::FileNotFound,"File or folder not found.","Verify the path and try again."},
    {ShellStatus::UnknownError,"An unknown error occurred.","Please try again or contact support."},
    {ShellStatus::FileCreateFailed,"Failed to create file or folder.","Check permissions and available disk space."},
    {ShellStatus::FileOpenFailed,"Failed to open file.","Ensure the file exists and you have read permissions."},
};

using Result = Type::Result<ShellStatus,void>;

class MessageHandler{
    static void show(const std::string err_msg, const std::string hint_msg){
        std::cout<<fmt_lib::format("\033[31mError: {}\033[0m\n\033[36mHint: {}\033[0m\n",err_msg,hint_msg);
    }
public:
    static void print_status(const ShellStatus status) {
        if (status == ShellStatus::Success || status == ShellStatus::StatusNULL || status == ShellStatus::ExitCode) return;
        for (const auto& entry : StatusMessage_Map) {
            if (status == entry.status) {
                show(entry.message, entry.hint);
                break;
            }
        }
    }
    static void print_suggestion(const std::string suggest_msg){
        std::cout<<fmt_lib::format("\033[96mSuggest: {}\033[0m\n",suggest_msg);
    }
    static void print_normal_message(const std::string msg){
        std::cout<<fmt_lib::format("{}\n",msg);
    }
    static void print_path_message(const std::filesystem::path& p) {
        std::wcout<<fmt_lib::format(L"{}\n",p.wstring());
    }
};


class ShellCommand {
    virtual Result core(std::filesystem::path& pwd,const std::vector<std::string>& args) = 0;
    virtual bool validate_args(const std::filesystem::path& pwd, const std::vector<std::string>& args) = 0;
public:
    virtual ~ShellCommand() = default;
    virtual Result execute(std::filesystem::path& pwd,const std::vector<std::string>& args) final {
        if(!validate_args(pwd, args)) return Result(ShellStatus::WrongArguments);
        return core(pwd,args);
    }
    virtual std::string help_message() const = 0;
};

class PwdCommand: public ShellCommand{
    Result core(std::filesystem::path& pwd,const std::vector<std::string>& args) override {
        MessageHandler::print_path_message(pwd);
        return Result(ShellStatus::Success);
    }
    bool validate_args(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {
        if(!args.empty()) return false;
        return true;
    }
public:
    std::string help_message() const override {
        return std::string("pwd - Print current working directory\nUsage: pwd\nDescription: Display the absolute path of the current directory.");
    }
};
class ExitCommand: public ShellCommand {
    Result core(std::filesystem::path& pwd,const std::vector<std::string>& args) override {
        MessageHandler::print_normal_message("log out");
        return Result(ShellStatus::ExitCode);
    }
    bool validate_args(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {
        if(!args.empty()) return false;
        return true;
    }
public:
    std::string help_message() const override {
        return std::string("exit - Exit the shell\nUsage: exit\nDescription: Close the current shell session and log out.");
    }
};
class TouchCommand: public ShellCommand {
    std::filesystem::path path_cache;

    Result core(std::filesystem::path& pwd,const std::vector<std::string>& args) override {
        if(std::filesystem::exists(path_cache)) return Result(ShellStatus::PathExists);
        File_SingleFile_Create(path_cache);
        if(!File_Create_SFBoolStatus(path_cache)) return Result(ShellStatus::FileCreateFailed);
        return Result(ShellStatus::Success);
    }
    bool validate_args(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {
        if(args.size()!=1) return false;
        if(auto p=std::filesystem::path(args[0]); !p.empty()){
            if(auto [path,ec] = File_RelativePath_To_AbsPath_Weakly(pwd,p);
                ec||(std::filesystem::exists(path)&&std::filesystem::is_directory(path))){
                return false;
            }else{
                path_cache = path;
            }
        }
        return true;
    } 
public:
    std::string help_message() const override {
        return std::string("touch - Create a new empty file\nUsage: touch <filename>\nDescription: Create a new empty file at the specified path. Fails if file already exists.");
    }
};

class VimCommand: public ShellCommand {
    std::filesystem::path path_cache;

    Result core(std::filesystem::path& pwd, const std::vector<std::string>& args) override {
        if(std::filesystem::exists(path_cache)){
            File_Vim_TextCompiler(path_cache);
            return Result(ShellStatus::Success);
        }else{
            File_SingleFile_Create(path_cache);
            if(!File_Create_SFBoolStatus(path_cache)) return Result(ShellStatus::FileOpenFailed);
            File_Vim_TextCompiler(path_cache);
            return Result(ShellStatus::Success);
        }
    }
    bool validate_args(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {
        if(args.size()!=1) return false;
        if(auto p=std::filesystem::path(args[0]); !p.empty()){
            if(auto [path,ec]=File_RelativePath_To_AbsPath_Weakly(pwd,p);
                ec||(std::filesystem::exists(path)&&std::filesystem::is_directory(path))){
                return false;
            }else{
                path_cache = path;
            }
        }
        return true;
    }
public:
    std::string help_message() const override {
        return std::string("vim - Open Vim text editor\nUsage: vim <filename>\nDescription: Open the specified file in Vim editor. Creates the file if it doesn't exist.");
    }
};
class NanoCommand: public ShellCommand {
    std::filesystem::path path_cache;

    Result core(std::filesystem::path& pwd,const std::vector<std::string>& args) override {
        if(std::filesystem::exists(path_cache)){
            File_GUN_TextCompiler(path_cache);
            return Result(ShellStatus::Success);
        }else{
            File_SingleFile_Create(path_cache);
            if(!File_Create_SFBoolStatus(path_cache)) return Result(ShellStatus::FileOpenFailed);
            File_GUN_TextCompiler(path_cache);
            return Result(ShellStatus::Success);
        }
    }
    bool validate_args(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {
        if(args.size()!=1) return false;
        if(auto p=std::filesystem::path(args[0]); !p.empty()){
            auto [path,ec]=File_RelativePath_To_AbsPath_Weakly(pwd,p);
            if (ec||(std::filesystem::exists(path)&&std::filesystem::is_directory(path))){
                return false;
            }else {
                path_cache = path;
            }
        }
        return true;
    }
public:
    std::string help_message() const override {
        return std::string("nano - Open GNU nano text editor\nUsage: nano <filename>\nDescription: Open the specified file in GNU nano editor. Creates the file if it doesn't exist.");
    }
};

class CdCommand: public ShellCommand {
    std::filesystem::path path_cache;

    Result core(std::filesystem::path& pwd, const std::vector<std::string>& args) override {
        pwd = path_cache;
        return Result(ShellStatus::Success);
    }

    bool validate_args(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {
        if(args.size()!=1) return false;
        if(auto p=std::filesystem::path(args[0]);!p.empty()){
            if(auto [path,ec] = File_RelativePath_To_AbsPath_Abs(pwd,p);
                ec||!std::filesystem::is_directory(path)){
                return false;
            }else{
                path_cache = path;
            }
        }
        return true;
    }
public:    
    std::string help_message() const override {
        return std::string("cd - Change directory\nUsage: cd <directory>\nDescription: Change the current working directory to the specified path.");
    }
};



// class ListCommand: public ShellCommand {
//     void show_plain(const File_Entry& Entries) const {
//     }
//     Result core(std::filesystem::path& pwd, const std::vector<std::string>& args) override {
//         //wait ...
//     }
//     bool validate_args(const std::vector<std::string>& args) const override {
//         return true;
//     }
// public:
//     std::string help_message() const override {
//         return std::string("...");
//     }
// };

class MkdirCommand: public ShellCommand {
    std::filesystem::path path_cache;

    Result core(std::filesystem::path& pwd, const std::vector<std::string>& args) override {
        File_StoreFolder_Create(path_cache);
        return Result(ShellStatus::Success);
    }

    bool validate_args(const std::filesystem::path& pwd, const std::vector<std::string>& args)  override {
        if(args.size()!=1) return false;
        
        std::filesystem::path p(args[0]);
        if(auto [path,ec] = File_RelativePath_To_AbsPath_Weakly(pwd,p);
            ec||(std::filesystem::exists(path))){
            return false;
        }else{
            path_cache = path;
        }
        
        return true;
    }
public:
    std::string help_message() const override {
        return std::string("mkdir - Create a new directory\nUsage: mkdir <directory>\nDescription: Create a new directory at the specified path. Fails if directory already exists.");
    }
};

class RmdirCommand: public ShellCommand {
    std::filesystem::path path_cache;

    Result core(std::filesystem::path& pwd, const std::vector<std::string>& args) override {
        std::filesystem::remove(path_cache);
        return Result(ShellStatus::Success);
    }

    bool validate_args(const std::filesystem::path& pwd,const std::vector<std::string>& args) override {
        if(args.size()!=1) return false;
        
        std::filesystem::path p(args[0]);
        if(auto [path,ec] = File_RelativePath_To_AbsPath_Weakly(pwd,p);
            ec||!std::filesystem::exists(path)||
            !std::filesystem::is_directory(path)||!File_Folder_CheckEmpty(path)){
            return false;
        }else{
            path_cache = path;
        }

        return true;
    }
public:
    std::string help_message() const override {
        return std::string("rmdir - Remove an empty directory\nUsage: rmdir <directory>\nDescription: Remove an empty directory. Fails if directory is not empty or doesn't exist.");
    }
};

class RemoveCommand: public ShellCommand {
    std::filesystem::path path_cache;

    Result core(std::filesystem::path& pwd, const std::vector<std::string>& args) override {
        std::filesystem::remove_all(path_cache);
        return Result(ShellStatus::Success);
    }

    bool validate_args(const std::filesystem::path& pwd,const std::vector<std::string>& args) override {
        if(args.size()!=1) return false;
        std::filesystem::path p(args[0]);
        if(auto [path,ec] = File_RelativePath_To_AbsPath_Weakly(pwd,p);
            ec||!std::filesystem::exists(path)){
            return false;
        }else{
            path_cache = path;
        }
        return true;
    }
public:    
    std::string help_message() const override {
        return std::string("rm - Remove files or directories\nUsage: rm <path>\nDescription: Remove files or directories recursively. Use with caution as this deletes all contents.");
    }
};

// class CopyCommand: public ShellCommand {
//     Result core(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {
//
//     }
//
//     bool validate_args(const std::vector<std::string>& args) const override {
//
//     }
// public:
//     std::string help_message() const override {
//         return std::string("...");
//     }
// };

// class MoveCommand: public ShellCommand {
//     Result core(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {
//
//     }
//
//     bool validate_args(const std::vector<std::string>& args) const override {
//
//     }
// public:
//     std::string help_message() const override {
//         return std::string("...");
//     }
// };

// class EncCommand: public ShellCommand {
//     Result core(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {
//
//     }
//
//     bool validate_args(const std::vector<std::string>& args) const override {
//
//     }
//     std::string help_message() const override {
//         return std::string("...");
//     }
// };

// class DecCommand: public ShellCommand {
//     Result core(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {
//
//     }
//
//     bool validate_args(const std::vector<std::string>& args) const override {
//
//     }
// public:
//     std::string help_message() const override {
//         return std::string("...");
//     }
// };

// class TempCommand: public ShellCommand {
//     Result core(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {
//
//     }
//
//     bool validate_args(const std::vector<std::string>& args) const override {
//
//     }
// public:
//     std::string help_message() const override {
//         return std::string("...");
//     }
// };

// class TarCEncCommand: public ShellCommand {
//     Result core(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {
//
//     }
//
//     bool validate_args(const std::vector<std::string>& args) const override {
//
//     }
// public:
//     std::string help_message() const override {
//         return std::string("...");
//     }
// };

// class DecTarXCommand: public ShellCommand {
//     Result core(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {
//
//     }
//
//     bool validate_args(const std::vector<std::string>& args) const override {
//
//     }
// public:
//     std::string help_message() const override {
//         return std::string("...");
//     }
// };

// class TarTempCommand: public ShellCommand {
//     Result core(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {
//
//     }
//
//     bool validate_args(const std::vector<std::string>& args) const override {
//
//     }
// public:
//     std::string help_message() const override {
//         return std::string("...");
//     }
// };

// class EchoCommand: public ShellCommand {
//     Result core(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {
//
//     }
//
//     bool validate_args(const std::vector<std::string>& args) const override {
//
//     }
// public:
//     std::string help_message() const override {
//         return std::string("...");
//     }
// };

class ManualCommand: public ShellCommand {
    const std::unordered_map<std::string,std::unique_ptr<ShellCommand>>* cmdRegeditPtr;

    Result core(std::filesystem::path& pwd, const std::vector<std::string>& args) override {
        auto it = cmdRegeditPtr->find(args[0]);
        auto cmd_info = it->second->help_message();
        MessageHandler::print_normal_message(cmd_info);
        return Result(ShellStatus::Success);
    }

    bool validate_args(const std::filesystem::path& pwd,const std::vector<std::string>& args) override {
        if(args.size()!=1) return false;
        if(cmdRegeditPtr->find(args[0]) == cmdRegeditPtr->end()) return false;
        return true;
    }
public:
    explicit ManualCommand(const std::unordered_map<std::string,std::unique_ptr<ShellCommand>>* cmdReg)
        : cmdRegeditPtr(cmdReg) {}
    
    std::string help_message() const override {
        return std::string("man - Display manual for commands\nUsage: man <command>\nDescription: Show detailed help information for the specified command.");
    }
};
// ===== Shell Core Class =====



class ShellEnv{
    std::filesystem::path curPath;
    std::unordered_map<std::string,std::unique_ptr<ShellCommand>> cmdRegedit;
public:
    virtual ~ShellEnv() = default;
    explicit ShellEnv(const std::filesystem::path _p): curPath(_p) {
        register_command("pwd", std::make_unique<PwdCommand>());
        register_command("cd", std::make_unique<CdCommand>());
        register_command("touch", std::make_unique<TouchCommand>());
        register_command("vim", std::make_unique<VimCommand>());
        register_command("nano", std::make_unique<NanoCommand>());
        register_command("exit", std::make_unique<ExitCommand>());
        register_command("mkdir", std::make_unique<MkdirCommand>());
        register_command("rmdir", std::make_unique<RmdirCommand>());
        register_command("rm", std::make_unique<RemoveCommand>());
        register_command("man", std::make_unique<ManualCommand>(&cmdRegedit));
    }

    void SystInfo() const {
        //WIN32 wait test
#ifdef _WIN32
        char username[256];
        DWORD size = sizeof(username);
        if (GetUserNameA(username, &size)) {
            std::cout<<fmt_lib::format("\033[032m{}\033[0m@", username);
        }
        char computername[256];
        size = sizeof(computername);
        if (GetComputerNameA(computername, &size)) {
            std::cout<<fmt_lib::format("\033[033m{}\033[0m", computername);
        }
#else
        const char* username = std::getenv("USER");
        if (username) {
            std::cout<<fmt_lib::format("\033[032m{}\033[0m@", username);
        }
        char hostname[256];
        if (gethostname(hostname, sizeof(hostname)) == 0) {
            std::cout<<fmt_lib::format("\033[033m{}\033[0m", hostname);
        }
#endif
        std::cout << " ";
        std::cout.flush();
    }

    void PathInfo() const {
        std::wcout<<fmt_lib::format(L"\033[035m{}\033[0m\033[033m$\033[0m ",curPath.wstring());
        std::cout.flush();
    }

    void register_command(const std::string cmd,std::unique_ptr<ShellCommand> _cmd){
        cmdRegedit[cmd] = std::move(_cmd);
    }

    bool check_cmd(const std::string cmd) const {
        return cmdRegedit.find(cmd) != cmdRegedit.end();
    }

    Result execute(const std::string cmd,const std::vector<std::string>& args) {
        if(!check_cmd(cmd)) return Result(ShellStatus::InvalidCommand);
        auto it = cmdRegedit.find(cmd);
        return it->second->execute(curPath,args);
    }

};


// ===== System Shell Interface =====
inline void System_Shell_Interface(std::filesystem::path target_path) {
    std::string cmd = "bash -c 'bash --rcfile <(echo '\"'\"'PS1=\"\\e[35m\\w\\e[0m\\e[33m$\\e[0m \"'\"'\"') -i'";
    cmd = "cd \"" + target_path.string() + "\" && " + cmd;
    system(cmd.data());
}

// int main(){
//     Terminal_Shell_Interface(std::filesystem::current_path());
// }
