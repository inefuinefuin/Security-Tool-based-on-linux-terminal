#pragma once

#if __has__include(<format>) && __cplusplus >= 202002L
    #include <format>
    namespace fmt_lib = std;
#else
    #include <fmt/core.h>
    namespace fmt_lib = fmt;
#endif


#include "GeneralFileOper.hpp"

#include <iostream>
// #include <sstream>
#include <map>
#include <vector>
#include <string>
#include <filesystem>
#include <functional>
#include <variant>



// ===== Feather Shell Core =====
// cd(do) ls(do) pwd(do) mkdir(do) rm(do) rmdir(do) cp(do) mv(do) touch(do) nano(do) vim(do) man(do) exit(do)
// echo kid_shell
// do // enc dec temp compressenc decompressdec tartemp

// ===== Error Handling =====
enum class ShellStatus{
    StatusNULL,

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

constexpr StatusMessage StatusMessage_Map[]={
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

// ==== Rust Result ====
template<typename T>
class Result{
    std::variant<T,ShellStatus> data;
public:
    Result(const T& value): data(value) {}
    Result(const ShellStatus& status): data(status) {}

    bool is_ok() const {
        return std::holds_alternative<T>(data);
    }
    bool is_err() const {
        return std::holds_alternative<ShellStatus>(data);
    }

    std::optional<T> ok() const {
        if(is_ok())
            return std::get<T>(data);
        return std::nullopt;
    }
    ShellStatus error() const {
        if(is_err())
            return std::get<ShellStatus>(data);
        return ShellStatus::StatusNULL;
    }

    T unwrap() const {
        if(is_ok())
            return std::get<T>(data);
        throw std::runtime_error("Attempted to unwrap an error Result");
    }
    ShellStatus unwrap_err() const {
        if(is_err())
            return std::get<ShellStatus>(data);
        throw std::runtime_error("Attempted to unwrap an ok Result");
    }

    template<typename U>
    Result<U> map(std::function<U(const T&)> func) const {
        if(is_ok()){
            return Result<U>(func(ok()));
        }else {
            return Result<U>(error());
        }
    }
 };

template<>
class Result<void>{
    ShellStatus data;
public:
    explicit Result(const ShellStatus& status): data(status) {}
    bool is_ok() const {
        return std::get<ShellStatus>(data) == ShellStatus::Success;
    }
    bool is_err() const {
        return !is_ok();
    }
    ShellStatus ok() const {
        if(is_ok()) 
            return ShellStatus::Success;
        return ShellStatus::StatusNULL;
    }
    ShellStatus error() const {
        if(is_err())
            return data;
        return ShellStatus::StatusNULL;
    }
    ShellStatus unwrap() const {
        if(!is_ok())
            throw std::runtime_error("Attempted to unwrap an error Result");
        return ShellStatus::Success;
    }
    ShellStatus unwrap_err() const {
        if(!is_err())
            throw std::runtime_error("Attempted to unwrap_err an ok Result");
        return data;
    }
};


class MessageHandler{
    static void show(const std::string err_msg, const std::string hint_msg){
        std::cout<<fmt_lib::format("\033[31mError: {}\033[0m]\n\033[36mHint: {}\033[0m\n",err_msg,hint_msg);
    }
public:
    static void print_status(const Result result) {
        ShellStatus status = result.unwrap().ok();
        if (status == ShellStatus::Success) return;
        for (const auto& entry : StatusMessage_Map) {
            if (status == entry.status) {
                show(entry.message, entry.hint);
            }
        }
    }
    static void print_suggestion(const std::string suggest_msg){
        std::cout<<fmt_lib::format("\033[96mSuggest: {}\033[0m\n",suggest_msg);
    }
    static void print_normal_message(const std::string msg){
        std::cout<<fmt_lib::format("{}\n",msg);
    }
};


class ShellCommand {
    virtual Result core(const std::filesystem::path& pwd,const std::vector<std::string>& args) = 0;
    virtual bool validate_args(const std::vector<std::string>& args) = 0;
protected:
    struct ParaSets{
        std::vector<char> para;
        std::vector<std::filesystem::path> paths;
    };
    using ParaParseFunc = std::function<void(ParaSets&,char)>;
    virtual Result<ParaSets> para_parse(const std::vector<std::string>& args, ParaParseFunc func=nullptr) const {
        ParaSets set;
        if(args.empty()) return Result<ParaSets>(set);
        for(const auto& arg: args){
            if(args.size()>1 && args[0] =='-'){
                if(!func) continue;            
                for(const auto& c: arg){
                    if(c=='-') { return Result<ParaSets>(ShellStatus::WrongArguments); }
                    func(set,c);
                }
            }else{
                std::filesystem::path p(arg);
                set.paths.push_back(arg);
            }
        }
        return Result<ParaSets>(set);
    }
public:
    virtual ~ShellCommand() = default;
    virtual Result execute(std::filesystem::path& pwd,const std::vector<std::string>& args) final {
        if(!validate_args(args)) return Result(ShellStatus::WrongArguments);
        return core(pwd,args);
    }
    virtual std::string help_message() const = 0;
};

class PwdCommand: public ShellCommand{
    Result core(const std::filesystem::path& pwd,const std::vector<std::string>& args) override {
        MessageHandler::print_normal_message(pwd.wstring());
        return ShellStatus::Success;
    }
    bool validate_args(const std::vector<std::string>& args) const override {
        if(!args.empty()) return false;
        return true;
    }
public:
    std::string help_message() const override {
        return std::string("...");
    }
};
class ExitCommand: public ShellCommand {
    Result core(const std::filesystem::path& pwd,const std::vector<std::string>& args) override {
        MessageHandler::print_normal_message("log out");
        return ShellStatus::Success;
    }
    bool validate_args(const std::vector<std::string>& args) const override {
        if(!args.empty()) return false;
        return true;
    }
public:
    std::string help_message() const override {
        return std::string("...");
    }
};
class TouchCommand: public ShellCommand {
    std::filesystem::path path_cache;

    Result core(const std::filesystem::path& pwd,const std::vector<std::string>& args) override {
        if(std::filesystem::exists(path_cache)) return ShellStatus::PathExists;
        File_SingleFile_Create(path_cache);
        if(!File_Create_SFBoolStatus(path_cache)) return ShellStatus::FileCreateFailed;
        return ShellStatus::Success;
    }
    bool validate_args(const std::vector<std::string>& args) const override {
        if(args.size()!=1) return false;
        if(auto p=std::filesystem::path(args[0]); !p.empty()){
            if(auto [path,ec] = File_RelativePath_To_AbsPath_Weakly(std::filesystem::current_path(),p);
                ec!=0){ 
                return false;
            }else{
                path_cache = path;
            }
        }
        return true;
    } 
public:
    bool help_message() const override {
        return std::string("...");
    }
};
class VimCommand: public ShellCommand {
    std::filesystem::path path_cache;

    Result core(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {
        if(std::filesystem::exists(path_cache)){
            File_Vim_TextCompiler(path_cache);
            return ShellStatus::Success;
        }else{
            File_SingleFile_Create(path_cache);
            if(!File_Create_SFBoolStatus(path_cache)) return ShellStatus::FileOpenFailed;
            File_Vim_TextCompiler(path_cache);
            return ShellStatus::Success;
        }
    }
    bool validate_args(const std::vector<std::string>& args) const override {
        if(args.size()!=1) return false;
        if(auto p=std::filesystem::path(args[0]); !p.empty()){
            if(auto [path,ec]=File_RelativePath_To_AbsPath_Weekly(std::filesystem::current_path(),p);
                ec!=0||(std::filesystem::exists(path)&&std::filesystem::is_directory(path))){
                return false;
            }else{
                path_cache = path;
            }
        }
        return true;
    }
public:
    std::string help_message() const override {
        return std::string("...");
    }
};
class NanoCommand: public ShellCommand {
    std::filesystem::path path_cache;

    Result core(const std::filesystem::path& pwd,const std::vector<std::string>& args) override {
        if(std::filesystem::exists(path_cache)){
            File_Nano_TextCompiler(path_cache);
            return ShellStatus::Success;
        }else{
            File_SingleFile_Create(path_cache);
            if(!File_Create_SFBoolStatus(path_cache)) return ShellStatus::FileOpenFailed;
            File_Nano_TextCompiler(path_cache);
            return ShellStatus::Success;
        }
    }
    bool validate_args(const std::vector<std::string>& args) const override {
        if(args.size()!=1) return false;
        if(auto p=std::filesystem::path(args[0]); !p.empty()){
            if(auto [path,ec]=File_RelativePath_To_AbsPath_Weekly(std::filesystem::current_path(),p);
                ec!=0||(std::filesystem::exists(path)&&std::filesystem::is_directory(path))){
                return false;
            }else{
                path_cache = path;
            }
        }
        return true;
    }
public:
    std::string help_message() const override {
        return std::string("...");
    }
};
class CdCommand: public ShellCommand {
    std::filesystem::path path_cache;

    Result core(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {
        pwd = path_cache;
        return Result(ShellStatus::Success);
    }

    bool validate_args(const std::vector<std::string>& args) const override {
        if(args.size()!=1) return false;
        if(auto p=std::filesystem::path(args[0]);!p.empty()){
            if(auto [path,ec] = File_RelativePath_To_AbsPath_Abs(std::filesystem::current_path(),p);
                ec!=0||!std::filesystem::is_directory(path)){
                return false;
            }else{
                path_cache = path;
            }
        }
        return true;
    }
public:
    std::string help_message() const override {
        return std::string("...");
    }
};

class ListCommand: public ShellCommand {
    void show_plain(const File_Entry& Entries) const {
        for(const auto& entry: Entries){
            if(std::filesystem::is_directory(entry.filepath)){
                std::cout<<fmt_lib::format(L"\033[106m{}\033[0m ",entry.filename);
            }else{
                std::cout<<fmt_lib::format(L"\033[96m{}\033[0m ",entry.filename);
            }
        }
        std::cout<<"\n";
    }
    
    Result core(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {
        // if(args.empty()) {
        //     auto Entries = File_Folder_List_Info(std::filesystem::current_path());
        //     show_plain(Entries);
        //     return Result(ShellStatus::Success)
        // }
        // else{
        //     auto result = para_parse(args);
        //     if(result.is_err()) return Result(result.error());
        //     if(auto [paras,paths] = result.unwrap(); paras.empty()){
        //         if(paths.size()!=1) retrun Result(ShellStatus::WrongArguments);
                
        //         std::filesystem::path try_path(paths[0]);
        //         auto [path,ec] = File_RelativePath_To_AbsPath_Abs(pwd,try_path);
        //         if(ec!=0) return Result(ShellStatus::WrongArguments);
                
        //         auto Entries = File_Folder_List_Info(path);
        //         show_plain(Entries);
        //         return Result(ShellStatus::Success);
        //     }
        // }
    }

    bool validate_args(const std::vector<std::string>& args) const override {
        return true;
    }
public:
    std::string help_message() const override {
        return std::string("...");
    }
};

class MkdirCommand: public ShellCommand {
    Result core(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {

    }

    bool validate_args(const std::vector<std::string>& args) const override {
        
    }
public:
    std::string help_message() const override {
        return std::string("...");
    }
};

class RmdirCommand: public ShellCommand {
    Result core(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {

    }

    bool validate_args(const std::vector<std::string>& args) const override {
        
    }
public:
    std::string help_message() const override {
        return std::string("...");
    }
};

class RemoveCommand: public ShellCommand {
    Result core(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {

    }

    bool validate_args(const std::vector<std::string>& args) const override {
        
    }
public:
    std::string help_message() const override {
        return std::string("...");
    }
};

class CopyCommand: public ShellCommand {
    Result core(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {

    }

    bool validate_args(const std::vector<std::string>& args) const override {
        
    }
public:
    std::string help_message() const override {
        return std::string("...");
    }
};

class MoveCommand: public ShellCommand {
    Result core(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {

    }

    bool validate_args(const std::vector<std::string>& args) const override {
        
    }
public:
    std::string help_message() const override {
        return std::string("...");
    }
};

class EncCommand: public ShellCommand {
    Result core(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {

    }

    bool validate_args(const std::vector<std::string>& args) const override {
        
    }
public:
    std::string help_message() const override {
        return std::string("...");
    }
};

class DecCommand: public ShellCommand {
    Result core(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {

    }

    bool validate_args(const std::vector<std::string>& args) const override {
        
    }
public:
    std::string help_message() const override {
        return std::string("...");
    }
};

class TempCommand: public ShellCommand {
    Result core(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {

    }

    bool validate_args(const std::vector<std::string>& args) const override {
        
    }
public:
    std::string help_message() const override {
        return std::string("...");
    }
};

class TarCEncCommand: public ShellCommand {
    Result core(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {

    }

    bool validate_args(const std::vector<std::string>& args) const override {
        
    }
public:
    std::string help_message() const override {
        return std::string("...");
    }
};

class DecTarXCommand: public ShellCommand {
    Result core(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {

    }

    bool validate_args(const std::vector<std::string>& args) const override {
        
    }
public:
    std::string help_message() const override {
        return std::string("...");
    }
};

class TarTempCommand: public ShellCommand {
    Result core(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {

    }

    bool validate_args(const std::vector<std::string>& args) const override {
        
    }
public:
    std::string help_message() const override {
        return std::string("...");
    }
};

class EchoCommand: public ShellCommand {
    Result core(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {

    }

    bool validate_args(const std::vector<std::string>& args) const override {
        
    }
public:
    std::string help_message() const override {
        return std::string("...");
    }
};

class ManualCommand: public ShellCommand {
    Result core(const std::filesystem::path& pwd, const std::vector<std::string>& args) override {

    }

    bool validate_args(const std::vector<std::string>& args) const override {
        
    }
public:
    std::string help_message() const override {
        return std::string("...");
    }
};
// ===== Shell Core Class =====






















// ===== System Shell Interface =====
inline void System_Shell_Interface(std::filesystem::path target_path) {
     std::string cmd = "bash -c 'bash --rcfile <(echo '\"'\"'PS1=\"\\e[35m\\w\\e[0m\\e[33m$\\e[0m \"'\"'\"') -i'";
    cmd = "cd \"" + target_path.string() + "\" && " + cmd;
    system(cmd.data());
}

// int main(){
//     Terminal_Shell_Interface(std::filesystem::current_path());
// }
