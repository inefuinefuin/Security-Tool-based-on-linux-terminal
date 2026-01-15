
#include "GeneralFileOper.hpp"
#include "TuiFileManager.hpp"
#include "Shell.hpp"
#include "Core.hpp"
#include "Compress.hpp"
#include <ncursesw/ncurses.h>

#include <string_view>
#include <string>
#include <filesystem>
#include <optional>
#include <stack>

#define Main_Menu 4
#define Crypto_Menu_Num 6
#define Shell_Menu_Num 2
#define Tar_Menu_Num 2
// ===== Terminal Screen Texts =====
std::string TUI_Title = "Security File Manager Tool";
const std::string Menu[Main_Menu] = {
    "Crypto Tool",
    "File Manager TUI",
    "Shell Tool",
    "Tar Tool",
};
const std::string Crypto_Menu[Crypto_Menu_Num] = {
    "Encrypt",
    "Decrypt",
    "Temporary Decrypt",
    "Compress&Encrypt",
    "Decrypt&Decompress",
    "Temporary Decompress&Decrypt",
};
const std::string Shell_Menu[Shell_Menu_Num] = {
    "Simply SHELL",
    "System Shell",
};
const std::string Tar_Menu[Tar_Menu_Num] = {
    "Tar Compress",
    "Tar Decompress",
};
enum class Menu_Option {Main,Crypto,Shell,Tar,};
const std::string* Menu_Array_Match[] = {Menu,Crypto_Menu,Shell_Menu,Tar_Menu,};
constexpr int Menu_Match(Menu_Option option) {
    switch (option){
        case Menu_Option::Main: return Main_Menu;
        case Menu_Option::Crypto: return Crypto_Menu_Num;
        case Menu_Option::Shell: return Shell_Menu_Num;
        case Menu_Option::Tar: return Tar_Menu_Num;
        default: return 0;
    }
}

std::optional<std::filesystem::path> TUI_KidWin_InPath() {
    std::wstring raw_path=L"";
    int win_height = 1, win_pos = LINES - 1;
    WINDOW* win = TUI_KidWin_Components::KidWin_Create(win_height,COLS,win_pos,0);
    WINDOW* backup = TUI_KidWin_Components::KidWin_Backup_LastLine(stdscr,win_pos);

    int tmp_len = raw_path.size();
    while(true){
        werase(win);
        std::wstring show_string = L"$Path$"+raw_path;
        mvwaddwstr(win,0,0,show_string.data());
        mvwaddch(win,0,WString_Cols_Count(show_string),L' '|A_STANDOUT);
        wrefresh(win);

        wint_t wch;
        int result = get_wch(&wch);
        if(wch==ESC) {
            TUI_KidWin_Components::KidWin_Restore(backup, stdscr);
            TUI_KidWin_Components::KidWin_Destroy(win);
            return std::nullopt;
        }
        if(wch==ENTER) break;
        if(wch==KEY_BACKSPACE){
            if(tmp_len!=0) {
                tmp_len--;
                raw_path.pop_back();
            }
        }
        if(result==OK){
            raw_path+=static_cast<wchar_t>(wch);
            tmp_len++;
        }
    }
    TUI_KidWin_Components::KidWin_Restore(backup, stdscr);
    TUI_KidWin_Components::KidWin_Destroy(win);

    auto [target_path,ec] = Input_Path_Check_Abs(raw_path);
    return ec ? std::nullopt : std::optional<std::filesystem::path>{target_path};
}
std::string TUI_KidWin_inPwd() {
    auto hide_password = [](int for_time){
        std::string show_string = "$Password$";
        for(int i=0;i<for_time;i++)
            show_string+="*";
        return show_string;
    };

    std::string raw_pwd="";
    int win_height = 1, win_pos = LINES - 1;
    WINDOW* win = TUI_KidWin_Components::KidWin_Create(win_height,COLS,win_pos,0);
    WINDOW* backup = TUI_KidWin_Components::KidWin_Backup_LastLine(stdscr,win_pos);
    
    int tmp_len = raw_pwd.size();
    while(true){
        werase(win);
        std::string show_string = hide_password(tmp_len);
        mvwaddstr(win,0,0,show_string.data());
        mvwaddch(win,0,show_string.size(),' '|A_STANDOUT);
        wrefresh(win);

        wint_t wch;
        int result = get_wch(&wch);
        if(wch==ESC){
            TUI_KidWin_Components::KidWin_Restore(backup, stdscr);
            TUI_KidWin_Components::KidWin_Destroy(win);
            return "";
        }
        if(wch==ENTER) break;
        if(wch==KEY_BACKSPACE){
            if(tmp_len!=0) {
                tmp_len--;
                raw_pwd.pop_back();
            }
        }
        if(result==OK){
            raw_pwd+=wch;
            tmp_len++;
        }
    }
    TUI_KidWin_Components::KidWin_Restore(backup, stdscr);
    TUI_KidWin_Components::KidWin_Destroy(win);
    return raw_pwd;
}


void TUI_Title_Draw(const std::string show_str){
    mvaddstr(0,0,show_str.data()); refresh();
}
void TUI_Menu_Draw(const Screen_Row_Info& scrl,const int for_time,const std::string* menu){
    int scrl_cur_y = scrl.fix+scrl.offset;
    int scrl_bp_idx = scrl.bp-scrl.fix;
    int scrl_cur_idx = scrl_bp_idx+scrl.offset;

    for(int idx=scrl_bp_idx; idx<scrl_bp_idx+for_time; idx++){
        int for_offset = idx - scrl_bp_idx;
        std::string entry = menu[idx];
        mvaddstr(scrl.fix+for_offset,0,entry.data());
        if(idx==scrl_cur_idx){mvaddch(scrl_cur_y,entry.size(),' '|A_STANDOUT);}
    }
}
class TUI_Main_Screen_Draw{
    Screen_Row_Info scrl{1,1,0,-1,-1};
    Menu_Option current_menu;

    void Menu_Entry_Draw(const int ch) {
        scrl.total = Menu_Match(current_menu); scrl.vsb = LINES-1;

        Scroll_Event_Response(scrl,ch);

        int scrl_total_To_bp_len = scrl.total+(scrl.fix-1)-scrl.bp+1;
        if(scrl_total_To_bp_len<=scrl.vsb) TUI_Menu_Draw(scrl,scrl_total_To_bp_len,Menu_Array_Match[static_cast<int>(current_menu)]);
        else TUI_Menu_Draw(scrl,scrl.vsb,Menu_Array_Match[static_cast<int>(current_menu)]);

        refresh();
    }

    void Menu_Chain_Map() const {
        int scroll_idx = scrl.bp - scrl.fix + scrl.offset;
        if(current_menu==Menu_Option::Main) {
            Menu_Option selected_option;
            switch(scroll_idx){
                case 0: selected_option = Menu_Option::Crypto; break;
                case 1: Terminal_MountCurrentTUI_LaunchNewTUI(Terminal_FileManager_Interface,std::filesystem::current_path()); return;
                case 2: selected_option = Menu_Option::Shell; break;
                case 3: selected_option = Menu_Option::Tar; break;
                default: return;
            }
            menu_chain.push(*this); temp_menu = selected_option;
        }else if(current_menu==Menu_Option::Crypto) {
            std::string crypto_type;
            switch(scroll_idx){
                case 0: crypto_type = "Enc"; break;
                case 1: crypto_type = "Dec"; break;
                case 2: crypto_type = "Tmp"; break;
                case 3: crypto_type = "TarEnc"; break;
                case 4: crypto_type = "TarDec"; break;
                case 5: crypto_type = "TarTmp"; break;
                default: return;
            }
            auto target_path = TUI_KidWin_InPath();
            auto pwd = TUI_KidWin_inPwd();
            if(target_path!=std::nullopt&&pwd!="")
                Crypto_TUI_Interface(crypto_type,*target_path,pwd);
            else Terminal_Error_Ring();
        }else if(current_menu==Menu_Option::Shell) {
            if(scroll_idx==0) {
                Terminal_MountCurrentTUI_LaunchNewTUI(Terminal_Shell_Interface,std::filesystem::current_path());
            }else if(scroll_idx==1) {
                Terminal_MountCurrentTUI_LaunchNewTUI(System_Shell_Interface,std::filesystem::current_path());
            }else return;
        }else if(current_menu==Menu_Option::Tar) {
            auto target_path = TUI_KidWin_InPath();
            if(target_path==std::nullopt||!std::filesystem::exists(*target_path)) {
                Terminal_Error_Ring(); return;
            }
            if(scroll_idx==0) {
                Compress_Interface("Compress",*target_path);
            }else if(scroll_idx==1) {
                Compress_Interface("Decompress",*target_path);
            }else return;
        }
        return;
    }
public:
    inline static std::stack<TUI_Main_Screen_Draw> menu_chain;
    inline static std::optional<Menu_Option> temp_menu = std::nullopt;
    explicit TUI_Main_Screen_Draw(Menu_Option menu_option): current_menu(menu_option){}
    TUI_Main_Screen_Draw operator=(const TUI_Main_Screen_Draw & object) {
        if (this==&object) return *this;
        this->current_menu = object.current_menu;
        this->scrl = object.scrl;
        return *this;
    }
    void Main_Graphic_Draw(const int ch) {
        clear();
        TUI_Title_Draw(TUI_Title);
        Menu_Entry_Draw(ch);
        
        if(ch==ENTER){
            Menu_Chain_Map();
            refresh();
        }

    }

};


// ===== Terminal Main Event Cycle =====
inline void TUI_Main_Event_Cycle() {
    setlocale(LC_ALL, ""); initscr(); noecho(); cbreak(); keypad(stdscr,TRUE); curs_set(0); set_escdelay(25);

    TUI_Main_Screen_Draw main_object(Menu_Option::Main);
    bool screen_size_state = false;
    while (true) {
        Terminal_Screen_yx_Size initial = Terminal_yx_Size_Monitor();
        if (!screen_size_state) main_object.Main_Graphic_Draw(KINT);

        int ch = getch();
        if (Terminal_Screen_yx_Size monitor = Terminal_yx_Size_Monitor(); monitor!=initial) {
            screen_size_state = true; main_object.Main_Graphic_Draw(KINT);
        }
        if (ch==ESC||ch=='q') break;
        if (ch==ENTER||ch==KEY_UP||ch==KEY_DOWN) main_object.Main_Graphic_Draw(ch);
        if(TUI_Main_Screen_Draw::temp_menu!=std::nullopt && ch==ENTER){
            const TUI_Main_Screen_Draw temp_object(*TUI_Main_Screen_Draw::temp_menu);
            main_object = temp_object;
            TUI_Main_Screen_Draw::temp_menu = std::nullopt; screen_size_state = false;
        }else if(!TUI_Main_Screen_Draw::menu_chain.empty() && ch==KEY_LEFT){
            const TUI_Main_Screen_Draw temp_object(TUI_Main_Screen_Draw::menu_chain.top());
            main_object = temp_object;
            TUI_Main_Screen_Draw::menu_chain.pop(); screen_size_state = false;
        }
    }
    endwin();
}


// ===== Test =====
// int main(){
//     TUI_Main_Event_Cycle();
// }
