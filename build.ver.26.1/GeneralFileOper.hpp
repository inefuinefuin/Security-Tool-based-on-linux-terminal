#pragma once
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <optional>

const std::filesystem::path EncStore = std::filesystem::current_path()/"EncStore";
const std::filesystem::path DecStore = std::filesystem::current_path()/"DecStore";
// const std::filesystem::path TempStore = std::filesystem::current_path()/"TempStore";


//Error: Don't deal UTF-16
inline void File_GUN_TextCompiler(const std::filesystem::path target_file) {
    if (!std::filesystem::is_regular_file(target_file)) return;
    std::string cmd = "nano " + target_file.string();
    system(cmd.data());
}
inline void File_Vim_TextCompiler(const std::filesystem::path target_file) {
    if(!std::filesystem::exists(target_file)) return;
    std::string cmd = "vim -n "+target_file.string();
    system(cmd.data());
}

inline void File_SingleFile_Create(const std::filesystem::path target_file){
    if(std::filesystem::exists(target_file)) return;
    std::ofstream ofs(target_file, std::ios::binary);
    ofs.close();
}
inline void File_StoreFolder_Create(const std::filesystem::path target_folder){
    if(std::filesystem::exists(target_folder)) return;
    std::filesystem::create_directories(target_folder);
}

inline bool File_Folder_CheckEmpty(const std::filesystem::path target_folder) {
    return std::filesystem::is_directory(target_folder)&&std::filesystem::directory_iterator(target_folder)==std::filesystem::directory_iterator{};
}
inline void File_Folder_Clean(std::filesystem::path target_folder){
    if (!std::filesystem::is_directory(target_folder)) return;
    std::filesystem::remove_all(target_folder);
}

inline std::filesystem::path File_SourcePath_To_TargetPath_Convert(const std::filesystem::path target_folder,std::filesystem::path base_folder,std::filesystem::path source_path){
    std::filesystem::path relate_path = std::filesystem::relative(source_path,base_folder);
    std::filesystem::path target_path = std::filesystem::weakly_canonical(target_folder/relate_path);
    return target_path;
}

inline void File_General_Clean(const std::filesystem::path target_file) {
    if (!std::filesystem::exists(target_file)) return;
    else if (std::filesystem::is_regular_file(target_file))
        std::filesystem::remove(target_file);
    else std::filesystem::remove_all(target_file);
}

enum class File_Type{File,Folder,};
inline std::filesystem::path File_Non_Repeating_Create(const File_Type file_type,const std::filesystem::path target_folder,const std::wstring filenamee) {
    std::filesystem::path init_path = target_folder/filenamee;
    std::filesystem::path stem = init_path.stem();
    std::filesystem::path ext = init_path.extension();
    std::filesystem::path target_path = init_path;
    for (int i=1; std::filesystem::exists(target_path); i++)
        target_path = target_folder/(stem.wstring()+L"_"+std::to_wstring(i)+ext.wstring());
    switch (file_type) {
        case File_Type::File: File_SingleFile_Create(target_path);break;
        default: File_StoreFolder_Create(target_path);
    }
    return target_path;
}

struct File_Item {
    std::filesystem::path filepath;
    std::wstring filename;
};
inline std::vector<File_Item> File_Folder_List_Info(const std::filesystem::path source_path){
    std::vector<File_Item> Items;
    for(const auto& entry: std::filesystem::directory_iterator(source_path)){
        Items.push_back(File_Item{entry.path(),entry.path().filename().wstring()});
    }
    return Items;
}

inline std::string File_Permission_Gain(const std::filesystem::path source_path) {
    auto File_Perm = [](std::filesystem::perms row_perms) {
        std::string s;
        s+=(row_perms&std::filesystem::perms::owner_read)!=std::filesystem::perms::none ? "r" : "-";
        s+=(row_perms&std::filesystem::perms::owner_write)!=std::filesystem::perms::none ? "w" : "-";
        s+=(row_perms&std::filesystem::perms::owner_exec)!=std::filesystem::perms::none ? "x" : "-";
        s+=(row_perms&std::filesystem::perms::group_read)!=std::filesystem::perms::none ? "r" : "-";
        s+=(row_perms&std::filesystem::perms::group_write)!=std::filesystem::perms::none ? "w" : "-";
        s+=(row_perms&std::filesystem::perms::group_exec)!=std::filesystem::perms::none ? "x" : "-";
        s+=(row_perms&std::filesystem::perms::others_read)!=std::filesystem::perms::none ? "r" : "-";
        s+=(row_perms&std::filesystem::perms::others_write)!=std::filesystem::perms::none ? "w" : "-";
        s+=(row_perms&std::filesystem::perms::others_exec)!=std::filesystem::perms::none ? "x" : "-";
        return s;
    };
    auto File_Type = [](const std::filesystem::path source_file) {
        if (std::filesystem::is_directory(source_file)) return std::string("d");
        return std::string("-");
    };
    return File_Type(source_path)+File_Perm(std::filesystem::status(source_path).permissions());
}

inline bool File_Name_Input_Char_Check(const int ch) {
    auto Input_Has_Control_Char = [&] {
        if (ch<32||ch==127) return true;
        return false;
    }();
    if (Input_Has_Control_Char) return false;
    int delist[] = {'\\','/',':','*','<','>','|','\"','?'};
    for (int i=0; i<sizeof(delist)/sizeof(int); i++)
        if (ch==delist[i])
            return false;
    return true;
}

inline std::wstring String_UTF8_TO_WString(const std::string raw_string){
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.from_bytes(raw_string);
}

inline int WString_Cols_Count(std::wstring wstr){
    int width = 0;
    for(const auto& wc: wstr)
        width+=wcwidth(wc);
    return width;
};


inline std::pair<std::filesystem::path,std::error_code> File_Path_Correct_TryMake(const std::filesystem::path pwd,const std::filesystem::path raw_path) {
    std::filesystem::path target;
    if (raw_path.is_absolute()) target = raw_path;
    else target = pwd/raw_path;
    std::error_code ec;
    target = std::filesystem::weakly_canonical(target,ec);
    return std::pair{target,ec};
}
inline std::optional<std::filesystem::path> File_Path_Correct_Make(const std::wstring source_path){
    try{
        std::filesystem::path initial(source_path);
        std::filesystem::path target = std::filesystem::canonical(initial);
        return target;
    }catch(std::filesystem::filesystem_error& e){
        return std::nullopt;
    }
}
