#pragma once
#include "Compress.hpp"
#include "GeneralFileOper.hpp"
#include "TuiFileManager.hpp"
#include <sodium.h>

#include <filesystem>
#include <fstream>
#include <stack>
#include <string>
// #include <iostream>

enum class Crypto_Type {Enc,Dec};
// ===== Core Crypto Classes =====
// File stream admin for handling file I/O
// File encryption class
// File decryption class
class Crypto_FileStream_Admin{
public:
    std::ifstream source_ifs;
    std::ofstream target_ofs;
    void init_ifstream(std::filesystem::path inFile){source_ifs.open(inFile,std::ios::binary);}
    void init_ofstream(std::filesystem::path outFile){target_ofs.open(outFile,std::ios::binary);}
    void close_ifstream(){source_ifs.close();}
    void close_ofstream(){target_ofs.close();}
    ~Crypto_FileStream_Admin(){
        if(source_ifs.is_open()) close_ifstream();
        if(target_ofs.is_open()) close_ofstream();
    }
};
class Crypto_File_Encrypt{
    Crypto_FileStream_Admin fstream_admin;
    unsigned char salt[crypto_pwhash_SALTBYTES];
    unsigned char encrypt_key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
public:
    Crypto_File_Encrypt() {
        if(sodium_init()<0)
            throw std::runtime_error("libsodium init failed");
    }

    void EncryptKey(const std::string password){
        randombytes_buf(salt, sizeof(salt));
        crypto_pwhash(encrypt_key,crypto_secretstream_xchacha20poly1305_KEYBYTES,password.c_str(),password.size(),salt,
            crypto_pwhash_OPSLIMIT_MODERATE,crypto_pwhash_MEMLIMIT_MODERATE, crypto_pwhash_ALG_DEFAULT);
    }

    bool EncryptFile(const std::filesystem::path source_file,const std::filesystem::path target_file){
        fstream_admin.init_ifstream(source_file); fstream_admin.init_ofstream(target_file);
        if(!fstream_admin.source_ifs.is_open()||!fstream_admin.target_ofs.is_open())
            return false;

        crypto_secretstream_xchacha20poly1305_state encrypt_state;
        unsigned char encrypt_header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
        crypto_secretstream_xchacha20poly1305_init_push(&encrypt_state,encrypt_header,encrypt_key);
        fstream_admin.target_ofs.write(reinterpret_cast<char*>(encrypt_header),sizeof(encrypt_header));
        fstream_admin.target_ofs.write(reinterpret_cast<char*>(salt), sizeof(salt));

        const std::size_t BLOCK_SIZE = 4096;
        unsigned char source_buf[BLOCK_SIZE];
        unsigned char target_buf[BLOCK_SIZE+crypto_secretstream_xchacha20poly1305_ABYTES];

        while(!fstream_admin.source_ifs.eof()){
            fstream_admin.source_ifs.read(reinterpret_cast<char*>(source_buf),BLOCK_SIZE);
            std::size_t readbytes = fstream_admin.source_ifs.gcount();
            unsigned char tag = fstream_admin.source_ifs.eof() ? crypto_secretstream_xchacha20poly1305_TAG_FINAL : 0;
            unsigned long long out_len;
            crypto_secretstream_xchacha20poly1305_push(&encrypt_state,target_buf,&out_len,source_buf,readbytes,nullptr,0,tag);
            fstream_admin.target_ofs.write(reinterpret_cast<char*>(target_buf),out_len);
        }
        sodium_memzero(encrypt_key,sizeof(encrypt_key));
        return true;
    }
};
class Crypto_File_Decrypt{
    Crypto_FileStream_Admin fstream_admin;

    crypto_secretstream_xchacha20poly1305_state decrypt_state;
    unsigned char decrypt_header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];

    void DecryptKey(const std::string& password, const unsigned char* salt, unsigned char* decrypt_key){
        crypto_pwhash(decrypt_key,crypto_secretstream_xchacha20poly1305_KEYBYTES,password.c_str(),password.size(),salt,
            crypto_pwhash_OPSLIMIT_MODERATE,crypto_pwhash_MEMLIMIT_MODERATE, crypto_pwhash_ALG_DEFAULT);
    }
public:
    Crypto_File_Decrypt() {
        if(sodium_init()<0)
            throw std::runtime_error("libsodium init failed");
    }

    bool DecryptFile(const std::filesystem::path source_file,const std::filesystem::path target_file, const std::string password){
        fstream_admin.init_ifstream(source_file); fstream_admin.init_ofstream(target_file);
        if(!fstream_admin.source_ifs.is_open()||!fstream_admin.target_ofs.is_open())
            return false;

        fstream_admin.source_ifs.read(reinterpret_cast<char*>(decrypt_header),sizeof(decrypt_header));

        unsigned char salt[crypto_pwhash_SALTBYTES];
        fstream_admin.source_ifs.read(reinterpret_cast<char*>(salt),sizeof(salt));

        unsigned char decrypt_key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
        DecryptKey(password, salt, decrypt_key);


        if(crypto_secretstream_xchacha20poly1305_init_pull(&decrypt_state,decrypt_header,decrypt_key)!=0)
            return false;

        const std::size_t BLOCK_SIZE = 4096;
        unsigned char source_buf[BLOCK_SIZE+crypto_secretstream_xchacha20poly1305_ABYTES];
        unsigned char target_buf[BLOCK_SIZE];
        unsigned long long out_len;
        unsigned char tag;


        while(!fstream_admin.source_ifs.eof()){
            fstream_admin.source_ifs.read(reinterpret_cast<char*>(source_buf),sizeof(source_buf));
            std::size_t read_bytes = fstream_admin.source_ifs.gcount();
            if(read_bytes==0) break;

            if(crypto_secretstream_xchacha20poly1305_pull(&decrypt_state,target_buf,&out_len,&tag,source_buf,read_bytes,nullptr,0)!=0)
                return false;


            fstream_admin.target_ofs.write(reinterpret_cast<char*>(target_buf),out_len);
        }
        sodium_memzero(decrypt_key,sizeof(decrypt_key));
        return true;
    }
};

namespace Crypto_Impl{
    // ===== Crypto File Creation =====
    // Create target encrypted/decrypted file with appropriate extension
    inline std::filesystem::path File_Crypto_SingleFile_Create(Crypto_Type crypto_type,const std::filesystem::path target_folder,const std::filesystem::path source_file){
        std::string target_file_name;
        switch(crypto_type){
            case Crypto_Type::Enc: target_file_name = source_file.filename().string()+".vlt"; break;
            case Crypto_Type::Dec: target_file_name = source_file.filename().stem().string(); break;
            default: break;
        }
        std::filesystem::path target_file(target_folder/target_file_name);
        File_SingleFile_Create(target_file);
        return target_file;
    }

    // ===== Crypto File Cleanup =====
    // Cleanup result structure after crypto operation
    struct Crypto_SFBoolState_Info_Clean{
        bool SFBoolState;
        std::filesystem::path source_file;
        std::filesystem::path target_file;
    };
    inline void Crypto_SFStateBoolState_Func_Clean(const Crypto_SFBoolState_Info_Clean SFBoolState_Info){
        if(SFBoolState_Info.SFBoolState) {
            std::filesystem::remove(SFBoolState_Info.source_file);
            File_EmptyFolder_CleanParent_Recursive(SFBoolState_Info.source_file);
        }
        else{
            std::filesystem::remove(SFBoolState_Info.target_file);
            File_EmptyFolder_CleanParent_Recursive(SFBoolState_Info.target_file);
        }
    }

    // ===== Core Crypto Operations =====
    // Crypto operation parameters
    // Encrypt or decrypt single file

    // Decrypt, edit, then re-encrypt file

    // Recursively encrypt/decrypt folder and all files

    // Decrypt folder, edit contents, then re-encrypt folder recursively

    // Compress/decompress and encrypt/decrypt in single operation

    // Decompress, edit, then compress and re-encrypt
    struct Crypto_Operate_Info{
        std::filesystem::path target_folder;
        std::filesystem::path source_file;
        std::string password;
        bool imply_SFBoolState;
    };
    inline bool Crypto_Core_File(Crypto_Type crypto_type,const Crypto_Operate_Info object_info,std::filesystem::path* target_path_output){
        File_StoreFolder_Create(object_info.target_folder);
        std::filesystem::path target_file;
        bool SFBoolState;
        switch(crypto_type){
            case Crypto_Type::Enc:{
                target_file = File_Crypto_SingleFile_Create(crypto_type,object_info.target_folder,object_info.source_file);
                Crypto_File_Encrypt cfe;
                cfe.EncryptKey(object_info.password);
                SFBoolState = cfe.EncryptFile(object_info.source_file,target_file);
                break;
            }
            case Crypto_Type::Dec:{
                target_file = File_Crypto_SingleFile_Create(crypto_type,object_info.target_folder,object_info.source_file);
                Crypto_File_Decrypt cfd;
                SFBoolState = cfd.DecryptFile(object_info.source_file,target_file,object_info.password);
                break;
            }
            default: break;
        }
        if(object_info.imply_SFBoolState){ Crypto_SFStateBoolState_Func_Clean(Crypto_SFBoolState_Info_Clean{SFBoolState,object_info.source_file,target_file});}
        if(target_path_output!=nullptr) *target_path_output = target_file;
        return SFBoolState;
    }

    inline bool Crypto_File_Temporary(const Crypto_Operate_Info object_info){
        std::filesystem::path temporary_path;
        bool SFBoolState_1 = Crypto_Core_File(Crypto_Type::Dec,object_info,&temporary_path);
        if(!SFBoolState_1) return false;

        Terminal_MountCurrentTUI_LaunchNewTUI(File_Vim_TextCompiler,temporary_path);

        bool SFBoolState_2 = Crypto_Core_File(Crypto_Type::Enc,Crypto_Operate_Info{EncStore,temporary_path,object_info.password,true},nullptr);
        return SFBoolState_2;
    }

    inline bool Crypto_Folder_File_Recursive(Crypto_Type crypto_type,const Crypto_Operate_Info object_info,std::filesystem::path* target_path_output) {
        if(!std::filesystem::is_directory(object_info.source_file)) {return false;}
        File_StoreFolder_Create(object_info.target_folder);
        std::stack<std::filesystem::path> Folder_cache;
        Folder_cache.push(object_info.source_file);
        std::filesystem::path target_base_folder = object_info.target_folder/object_info.source_file.filename();
        while(!Folder_cache.empty()) {
            std::filesystem::path folder_current = Folder_cache.top(); Folder_cache.pop();
            for(const auto& entry: std::filesystem::directory_iterator(folder_current)){
                auto [target_position, ec] = File_SourcePath_To_TargetPath_Convert_Weekly(target_base_folder,object_info.source_file,entry.path());
                if(ec) goto SFBoolState_Failed;
                if(std::filesystem::is_directory(entry.path())){
                    File_StoreFolder_Create(target_position);
                    Folder_cache.push(entry.path());
                }
                else{
                    if(crypto_type==Crypto_Type::Enc) {
                        bool SFBoolState = Crypto_Core_File(crypto_type,Crypto_Operate_Info{target_position.parent_path(),entry.path(),object_info.password,false},nullptr);
                        if(!SFBoolState) goto SFBoolState_Failed;
                    }
                    else if(crypto_type==Crypto_Type::Dec) {
                        bool SFBoolState = Crypto_Core_File(crypto_type,Crypto_Operate_Info{target_position.parent_path(),entry.path(),object_info.password,false},nullptr);
                        if(!SFBoolState) goto SFBoolState_Failed;
                    }
                }
            }
        }
        File_TargetFolder_Clean(object_info.source_file);
        if(target_path_output!=nullptr) *target_path_output = target_base_folder;
        return true;

        SFBoolState_Failed:
        File_TargetFolder_Clean(target_base_folder);
        return false;
    }

    inline bool Crypto_Folder_Temporary_Recursive(const Crypto_Operate_Info object_info){
        std::filesystem::path temporary_path;
        bool SFBoolState_1 = Crypto_Folder_File_Recursive(Crypto_Type::Dec,object_info,&temporary_path);
        if(!SFBoolState_1) return false;

        Terminal_MountCurrentTUI_LaunchNewTUI(Terminal_FileManager_Interface,temporary_path);

        bool SFBoolState_2 = Crypto_Folder_File_Recursive(Crypto_Type::Enc,Crypto_Operate_Info{EncStore,temporary_path,object_info.password,true},nullptr);
        return SFBoolState_2;
    }

    inline bool Crypto_Folder_File_Single(std::string tar_type,Crypto_Type crypto_type,Crypto_Operate_Info object_info,std::filesystem::path* target_path_output){
        bool SFBoolState_1, SFBoolState_2;
        if(crypto_type==Crypto_Type::Enc && tar_type=="Compress"){
            SFBoolState_2 = Compress_Interface("Compress",object_info.source_file);

            object_info.source_file = CompressStore/(object_info.source_file.filename().string()+".tar.gz");
            SFBoolState_1 = Crypto_Core_File(crypto_type,object_info,nullptr);
            
            if(target_path_output!=nullptr) *target_path_output = object_info.target_folder/(object_info.source_file.filename().string()+".vlt");
        }else if(crypto_type==Crypto_Type::Dec && tar_type=="Decompress"){
            std::filesystem::path temporary_path;
            SFBoolState_1 = Crypto_Core_File(crypto_type,object_info,&temporary_path);
            
            SFBoolState_2 = Compress_Interface("Decompress",temporary_path);
            
            if(target_path_output!=nullptr) *target_path_output = DecompressStore/temporary_path.filename().stem().stem();
        }

        if(!SFBoolState_1||!SFBoolState_2) return false;
        return true;
    }

    inline bool Crypto_Folder_Temporary_Single(const Crypto_Operate_Info object_info){
        std::filesystem::path temporary_path;
        bool SFBoolState_1 = Crypto_Folder_File_Single("Decompress",Crypto_Type::Dec,object_info,&temporary_path);
        if(!SFBoolState_1) return false;

        if(std::filesystem::is_regular_file(temporary_path))
            Terminal_MountCurrentTUI_LaunchNewTUI(File_Vim_TextCompiler,temporary_path);
        else
            Terminal_MountCurrentTUI_LaunchNewTUI(Terminal_FileManager_Interface,temporary_path);

        bool SFBoolState_2 = Crypto_Folder_File_Single("Compress",Crypto_Type::Enc,Crypto_Operate_Info{EncStore,temporary_path,object_info.password,true},nullptr);
        return SFBoolState_2;
    }
    
};


// ===== Main Crypto Interface =====
// Public interface for all crypto operations
// Modes: Enc, Dec, Tmp (edit), TarEnc (compress+encrypt), TarDec, TarTmp
inline bool Crypto_TUI_Interface(const std::string crypto_type,const std::filesystem::path source_file,const std::string password) {
    auto Crypto_File_Formate_Select = [source_file](const Crypto_Type crypto_type,const Crypto_Impl::Crypto_Operate_Info& object_info) {
        if (std::filesystem::is_directory(source_file))
            return Crypto_Impl::Crypto_Folder_File_Recursive(crypto_type,object_info,nullptr);
        return Crypto_Impl::Crypto_Core_File(crypto_type,object_info,nullptr);
    };
    if (crypto_type=="Enc") {
        Crypto_Impl::Crypto_Operate_Info object_info{EncStore,source_file,password,true};
        return Crypto_File_Formate_Select(Crypto_Type::Enc,object_info);
    }else if (crypto_type=="Dec") {
        Crypto_Impl::Crypto_Operate_Info object_info{DecStore,source_file,password,true};
        return Crypto_File_Formate_Select(Crypto_Type::Dec,object_info);
    }else if (crypto_type=="Tmp") {
        Crypto_Impl::Crypto_Operate_Info object_info{DecStore,source_file,password,true};
        if (std::filesystem::is_regular_file(source_file)) 
            return Crypto_Impl::Crypto_File_Temporary(object_info);
        else 
            return Crypto_Impl::Crypto_Folder_Temporary_Recursive(object_info);
    }else if(crypto_type=="TarEnc") {
        Crypto_Impl::Crypto_Operate_Info object_info{EncStore,source_file,password,true};
        return Crypto_Impl::Crypto_Folder_File_Single("Compress",Crypto_Type::Enc,object_info,nullptr);
    }else if (crypto_type=="TarDec") {
        Crypto_Impl::Crypto_Operate_Info object_info{DecStore,source_file,password,true};
        return Crypto_Impl::Crypto_Folder_File_Single("Decompress",Crypto_Type::Dec,object_info,nullptr);
    }else if( crypto_type=="TarTmp") {
        Crypto_Impl::Crypto_Operate_Info object_info{DecStore,source_file,password,true};
        return Crypto_Impl::Crypto_Folder_Temporary_Single(object_info);
    }
    return false;
}

// inline bool Crypto_Shell_Interface(const std::string crypto_type,const std::filesystem::path source_file,const std::string password) {
//     return true;
// }


// ===== Test =====
// int main(){
//     std::string path; std::getline(std::cin,path);
//     std::filesystem::path p = path;
//     Crypto_TUI_Interface("Enc",p,"testpassword");
//     Crypto_TUI_Interface("Dec",p,"testpassword");
//     return 0;
// }
