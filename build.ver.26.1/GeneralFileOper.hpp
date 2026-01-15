#pragma once
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <optional>

const std::filesystem::path EncStore = std::filesystem::current_path()/"Admin"/"EncStore";
const std::filesystem::path DecStore = std::filesystem::current_path()/"Admin"/"DecStore";

// ===== Text Editors =====
// Edit file with GNU nano
// Edit file with Vim
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

// ===== File/Folder Creation =====
// Create empty file if not exists
// Create folder recursively if not exists
inline void File_SingleFile_Create(const std::filesystem::path target_file){
    if(std::filesystem::exists(target_file)) return;
    std::ofstream ofs(target_file, std::ios::binary);
    ofs.close();
}
inline void File_StoreFolder_Create(const std::filesystem::path target_folder){
    if(std::filesystem::exists(target_folder)) return;
    std::filesystem::create_directories(target_folder);
}

// ===== File/Folder Cleaning =====
// Check if folder is empty
// Delete folder and all contents
// Delete file or folder (auto-detect type)
// Recursively delete empty parent folders
inline bool File_Folder_CheckEmpty(const std::filesystem::path target_folder) {
    return std::filesystem::is_directory(target_folder)&&std::filesystem::directory_iterator(target_folder)==std::filesystem::directory_iterator{};
}
inline void File_TargetFolder_Clean(std::filesystem::path target_folder){
    if(!std::filesystem::exists(target_folder)) return;
    if (!std::filesystem::is_directory(target_folder)) return;
    std::filesystem::remove_all(target_folder);
}
inline void File_TargetGeneral_Clean(const std::filesystem::path target_file) {
    if (!std::filesystem::exists(target_file)) return;
    else if (std::filesystem::is_regular_file(target_file))
        std::filesystem::remove(target_file);
    else std::filesystem::remove_all(target_file);
}
inline void File_EmptyFolder_CleanParent_Recursive(const std::filesystem::path target_folder){
    if(!std::filesystem::exists(target_folder)) return;
    if(!target_folder.has_parent_path()) return;
    std::filesystem::path middle_path = target_folder.parent_path();
    while(File_Folder_CheckEmpty(middle_path)){
        std::filesystem::remove(middle_path);
        if(!middle_path.has_parent_path()) break;
        middle_path = middle_path.parent_path();
    }
}

// ===== Path Conversion =====
// Convert source path to target path (relative, no existence check)
// Convert to absolute path (verifies existence)
// Convert to absolute path (no existence check)
// Convert input path to absolute path (verifies existence)
inline std::pair<std::filesystem::path,std::error_code> File_SourcePath_To_TargetPath_Convert_Weekly(const std::filesystem::path target_folder,std::filesystem::path base_folder,std::filesystem::path source_path){
    std::filesystem::path relate_path = std::filesystem::relative(source_path,base_folder);
    std::error_code ec;
    std::filesystem::path target_path = std::filesystem::weakly_canonical(target_folder/relate_path,ec);
    return std::pair{target_path,ec};
}
inline std::pair<std::filesystem::path,std::error_code> File_RelativePath_To_AbsPath_Abs(const std::filesystem::path current_path,const std::filesystem::path try_path) {
    std::filesystem::path target;
    if (try_path.is_absolute()) target = try_path;
    else target = current_path/try_path;
    std::error_code ec;
    target = std::filesystem::canonical(target,ec);
    return std::pair{target,ec};
}
inline std::pair<std::filesystem::path,std::error_code> File_RelativePath_To_AbsPath_Weakly(const std::filesystem::path current_path,const std::filesystem::path try_path) {
    std::filesystem::path target;
    if (try_path.is_absolute()) target = try_path;
    else target = current_path/try_path;
    std::error_code ec;
    target = std::filesystem::weakly_canonical(target,ec);
    return std::pair{target,ec};
}
inline std::pair<std::filesystem::path,std::error_code> Input_Path_Check_Abs(const std::wstring source_path){
    std::filesystem::path initial(source_path);
    std::error_code ec;
    std::filesystem::path target = std::filesystem::canonical(initial,ec);
    return std::pair{target,ec};
}


// ===== Non-Repeating Names =====
// File/folder type enumeration
// Create file/folder with auto-increment suffix if name exists
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
        case File_Type::Folder: File_StoreFolder_Create(target_path);break;
        default: break;
    }
    return target_path;
}

// ===== Folder Contents =====
// File entry structure
// List all entries in folder
struct File_Entry {
    std::filesystem::path filepath;
    std::wstring filename;
};
inline std::vector<File_Entry> File_Folder_List_Info(const std::filesystem::path source_path){
    std::vector<File_Entry> Items;
    for(const auto& entry: std::filesystem::directory_iterator(source_path))
        Items.emplace_back(entry.path(),entry.path().filename().wstring());
    return Items;
}

// ===== Permissions =====
// Get permissions in Linux style format (e.g., -rwxrwxrwx)
inline std::string File_Permission_Gain(const std::filesystem::path source_path) {
    auto File_Perm = [&]{
        std::filesystem::perms perms = std::filesystem::status(source_path).permissions();
        std::string s;
        s+=(perms&std::filesystem::perms::owner_read)!=std::filesystem::perms::none ? "r" : "-";
        s+=(perms&std::filesystem::perms::owner_write)!=std::filesystem::perms::none ? "w" : "-";
        s+=(perms&std::filesystem::perms::owner_exec)!=std::filesystem::perms::none ? "x" : "-";
        s+=(perms&std::filesystem::perms::group_read)!=std::filesystem::perms::none ? "r" : "-";
        s+=(perms&std::filesystem::perms::group_write)!=std::filesystem::perms::none ? "w" : "-";
        s+=(perms&std::filesystem::perms::group_exec)!=std::filesystem::perms::none ? "x" : "-";
        s+=(perms&std::filesystem::perms::others_read)!=std::filesystem::perms::none ? "r" : "-";
        s+=(perms&std::filesystem::perms::others_write)!=std::filesystem::perms::none ? "w" : "-";
        s+=(perms&std::filesystem::perms::others_exec)!=std::filesystem::perms::none ? "x" : "-";
        return s;
    }();
    //File Or Folder Type
    auto File_Type = [&]{
        if (std::filesystem::is_directory(source_path)) return std::string("d");
        return std::string("-");
    }();
    return File_Type + File_Perm;
}



// ===== Input Validation =====
// Check filename input for illegal characters
inline bool File_Input_FileName_Char_Check(const int key_code) {
    auto Input_Has_Control_Char = [&] {
        if (key_code<32||key_code==127) return true;
        return false;
    }();
    if (Input_Has_Control_Char) return false;
    int delist[] = {'\\','/',':','*','<','>','|','\"','?'};
    for (int i=0; i<sizeof(delist)/sizeof(int); i++)
        if (key_code==delist[i])
            return false;
    return true;
}

// ===== String Conversion =====
// Convert UTF-8 to wide string
// Count display width of wide string in terminal
inline std::wstring String_UTF8_TO_WString(const std::string raw_str){
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.from_bytes(raw_str);
}
inline int WString_Cols_Count(std::wstring wstr){
    int width = 0;
    for(const auto& wc: wstr)
        width+=wcwidth(wc);
    return width;
};
