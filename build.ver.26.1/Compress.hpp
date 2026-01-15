#pragma once
#include "GeneralFileOper.hpp"
#include <filesystem>
#include <string>
// #include <iostream>

const std::filesystem::path CompressStore = std::filesystem::current_path()/"Admin"/"CompressStore";
const std::filesystem::path DecompressStore = std::filesystem::current_path()/"Admin"/"DecompressStore";

// ===== Core Compression =====
// Compress or decompress files using tar with gzip
// Supports two modes: Compress (create .tar.gz) and Decompress (extract .tar.gz)
enum class CompressMode{Compress,Decompress,};
inline bool Compress(CompressMode tar_type,std::filesystem::path source_path){
    int result = -1;
    switch (tar_type){
    case CompressMode::Compress:{
        File_StoreFolder_Create(CompressStore);
        std::string cmd = "tar -czf "+CompressStore.string()+"/"+source_path.filename().string()+".tar.gz "+source_path.filename().string();
        result = system(cmd.data());
        break;}    
    case CompressMode::Decompress:{
        File_StoreFolder_Create(DecompressStore);
        std::string cmd = "tar -xzf "+source_path.string()+" -C "+DecompressStore.string();
        result = system(cmd.data());
        break;}
    default: break;
    }
    if(result!=0) {
        if(File_Folder_CheckEmpty(CompressStore)) File_TargetFolder_Clean(CompressStore);
        if(File_Folder_CheckEmpty(DecompressStore)) File_TargetFolder_Clean(DecompressStore);
        return false;
    }
    // Cleanup source files after decompression
    // File_TargetGeneral_Clean(source_path);
    // File_EmptyFolder_CleanParent_Recursive(source_path);
    return true;
}


// ===== Compression Interface =====
// User-friendly interface for compression operations (string-based mode selection)
inline bool Compress_Interface(std::string tar_type,std::filesystem::path source_path){
    if (tar_type == "Compress") 
        return Compress(CompressMode::Compress, source_path); 
    else if (tar_type == "Decompress") 
        return Compress(CompressMode::Decompress, source_path);
    return false;
}


// ===== Test Main =====
// int main(){
//     std::string path; std::getline(std::cin,path);
//     std::filesystem::path p = path;
//     Compress_Interface("Compress",p);
//     Compress_Interface("Decompress",p);
//     return 0;
// }