#include "../Shell.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

std::vector<std::string> input_usr_cmd() {
    std::string strs; std::getline(std::cin,strs);
    std::stringstream ss(strs); std::string mid;
    std::vector<std::string> contain;
    while(ss>>mid) contain.push_back(mid);
    return contain;
}

void shell_interface(std::filesystem::path _p){
    ShellEnv shell(_p);
    while(true){
        shell.SystInfo(); shell.PathInfo();
    
        auto usr_contain = input_usr_cmd();
        if(usr_contain.empty()) continue;

        Result result = shell.execute(usr_contain[0],std::vector(usr_contain.begin()+1,usr_contain.end()));
        
        if(!result.is_ok()) { MessageHandler::print_normal_message("\033[031mUnkown Error\033[0m"); continue;}
        ShellStatus sts = result.unwrap();
        if(sts==ShellStatus::ExitCode) break;
        else if(sts==ShellStatus::Success||sts==ShellStatus::StatusNULL) continue;
        else MessageHandler::print_status(sts);

        //clear_cache
        std::cout.flush();
    }
}


int main(){
    shell_interface(std::filesystem::current_path());
}