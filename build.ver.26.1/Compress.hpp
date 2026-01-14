#pragma once
#include "GeneralFileOper.hpp"
#include <filesystem>
#include <string>
const std::filesystem::path CompressStore = std::filesystem::current_path()/"CompressStore";
const std::filesystem::path DecompressStore = std::filesystem::current_path()/"DecompressStore";
inline void Compress(std::filesystem::path source_path){
    File_StoreFolder_Create(CompressStore);
    std::string cmd = "tar -czf "+CompressStore.string()+"/"+source_path.filename().string()+".tar.gz "+source_path.filename().string();
    system(cmd.data());
    File_General_Clean(source_path);
    if(File_Folder_CheckEmpty(source_path.parent_path()))
        File_Folder_Clean(source_path.parent_path());
}
inline void Decompress(std::filesystem::path source_path){
    File_StoreFolder_Create(DecompressStore);
    std::string cmd = "tar -xzf "+source_path.string()+" -C "+DecompressStore.string();
    system(cmd.data());
    File_General_Clean(source_path);
    if(File_Folder_CheckEmpty(source_path.parent_path()))
        File_Folder_Clean(source_path.parent_path());
}



inline void Compress_Interface(std::string tar_type,std::filesystem::path source_path){
    if(tar_type=="Compress"){
        Compress(source_path);
    }else if(tar_type=="Decompress"){
        Decompress(source_path);
    }
}
