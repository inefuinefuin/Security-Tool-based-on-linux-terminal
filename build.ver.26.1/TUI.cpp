
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

#define TXTNUM 7
constexpr std::string_view Terminal_Screen_Text[] = {
    "Security File Manager Tool",
    "Encrypt",
    "Decrypt",
    "Temporary",
    "File Manager TUI",
    "File Manager SHELL",
    "Tar Compress",
    "Tar Decompress",
};

class Terminal_Screen_Draw {
    mutable int vlm_cur_row = 1;
    void Title_Draw() const {
        mvaddstr(0,0,Terminal_Screen_Text[0].data());
    }
    void Entry_Draw(const int ch) const {
        [&] {
            if (ch==-1) return;
            if (ch==KEY_UP) {
                if(vlm_cur_row>1)
                    vlm_cur_row--;
                else Terminal_Error_Ring();
            }else if (ch==KEY_DOWN) {
                if (vlm_cur_row<TXTNUM)
                    vlm_cur_row++;
                else Terminal_Error_Ring();
            }
        }();
        for (int i=1;i<=TXTNUM;i++) {
            mvaddstr(i,0,Terminal_Screen_Text[i].data());
            if (i==vlm_cur_row) mvaddch(vlm_cur_row,Terminal_Screen_Text[i].size(),' '|A_STANDOUT);
        }
    }

    std::optional<std::filesystem::path> Input_Gain_Path() const {
        std::wstring raw_path=L"";
        int child_win_height = 1, child_win_pos = LINES - child_win_height;
        WINDOW* win = newwin(child_win_height,COLS,child_win_pos,0);
        keypad(win,TRUE);
        WINDOW* backup = newwin(1,COLS,0,0);
        copywin(stdscr,backup,child_win_pos,0,0,0,0,COLS-1,0);

        start_color();
        init_pair(1,COLOR_RED,COLOR_WHITE);
        wbkgd(win,COLOR_PAIR(1)); wattron(win,COLOR_PAIR(1));

        int tmp_len = raw_path.size();
        while(true){
            werase(win);
            std::wstring show_string = L"$Path$"+raw_path;
            mvwaddwstr(win,0,0,show_string.data());
            mvwaddch(win,0,WString_Cols_Count(show_string),L' '|A_STANDOUT);
            wrefresh(win);

            wint_t wch;
            int result = get_wch(&wch);
            if(wch==KEY_BACKSPACE){
                if(tmp_len!=0) {
                    tmp_len--;
                    raw_path.pop_back();
                }
            }

            if(wch==ENTER) break;
            if(wch==ESC) goto ESC_LABEL;
            if(result==OK){
                raw_path+=static_cast<wchar_t>(wch);
                tmp_len++;
            }
        }

        overwrite(backup,stdscr);
        delwin(win);
        // return raw_path;
        if(File_Path_Correct_Make(raw_path) == std::nullopt) {return std::nullopt;}
        else {return *File_Path_Correct_Make(raw_path);}

        ESC_LABEL:
        overwrite(backup,stdscr);
        delwin(win);
        return std::nullopt;
    }
    std::string Input_Gain_Password() const {
        std::string raw_pwd="";
        int child_win_height = 1, child_win_pos = LINES - child_win_height;
        WINDOW* win = newwin(child_win_height,COLS,child_win_pos,0);
        keypad(win,TRUE);
        WINDOW* backup = newwin(1,COLS,0,0);
        copywin(stdscr,backup,child_win_pos,0,0,0,0,COLS-1,0);

        start_color();
        init_pair(1,COLOR_RED,COLOR_WHITE);
        wbkgd(win,COLOR_PAIR(1)); wattron(win,COLOR_PAIR(1));

        int tmp_len = raw_pwd.size();
        auto hide_password = [&]{
            std::string show_string = "$Password$";
            for(int i=0;i<tmp_len;i++)
                show_string+="*";
            return show_string;
        };
        while(true){
            werase(win);
            std::string show_string = hide_password();
            mvwaddstr(win,0,0,show_string.data());
            mvwaddch(win,0,show_string.size(),' '|A_STANDOUT);
            wrefresh(win);

            wint_t wch;
            int result = get_wch(&wch);
            if(wch==KEY_BACKSPACE){
                if(tmp_len!=0) {
                    tmp_len--;
                    raw_pwd.pop_back();
                }
            }

            if(wch==ENTER) break;
            if(wch==ESC) goto ESC_LABEL;
            if(result==OK){
                raw_pwd+=wch;
                tmp_len++;
            }
        }

        overwrite(backup,stdscr);
        delwin(win);
        return raw_pwd;

        ESC_LABEL:
        overwrite(backup,stdscr);
        delwin(win);
        return "";
    }

    void Choice_Map() const {
        if(vlm_cur_row==1){
            auto target_path = Input_Gain_Path(); refresh();
            auto pwd = Input_Gain_Password(); refresh();
            if(target_path!=std::nullopt&&pwd!="")
                Crypto_Interface("Enc",*target_path,pwd);
            else Terminal_Error_Ring();
        }else if(vlm_cur_row==2){
            auto target_path = Input_Gain_Path(); refresh();
            auto pwd = Input_Gain_Password(); refresh();
            if(target_path!=std::nullopt&&(*target_path).extension()==".vlt"&&pwd!="")
                Crypto_Interface("Dec",*target_path,pwd);
            else Terminal_Error_Ring();
        }else if(vlm_cur_row==3){
            auto target_path = Input_Gain_Path(); refresh();
            auto pwd = Input_Gain_Password(); refresh();
            if(target_path!=std::nullopt&&pwd!="")
                Crypto_Interface("Tmp",*target_path,pwd);
            else Terminal_Error_Ring();
        }else if(vlm_cur_row==4){
            Terminal_Template_Current_MountTemporary_NewTUI_Launch(Terminal_FileManager_Interface,std::filesystem::current_path());
        }else if(vlm_cur_row==5){
            Terminal_Template_Current_MountTemporary_NewTUI_Launch(Terminal_Shell_Interface,std::filesystem::current_path());
        }else if(vlm_cur_row==6){
            auto target_path = Input_Gain_Path(); refresh();
            if(target_path!=std::nullopt)
                Compress_Interface("Compress",*target_path);
            else Terminal_Error_Ring();
        }else if(vlm_cur_row==7){
            auto target_path = Input_Gain_Path(); refresh();
            if(target_path!=std::nullopt)
                Compress_Interface("Decompress",*target_path);
            else Terminal_Error_Ring();
        }
    }
public:
    void Main_Graphic_Draw(const int ch) const {
        clear();
        Title_Draw();
        Entry_Draw(ch);
        refresh();
        if (ch==ENTER) {
            Choice_Map();
            refresh();
        }
    }
};

inline void Terminal_Screen_Event_Main_Cycle() {
    initscr();
    noecho(); cbreak(); keypad(stdscr,TRUE); curs_set(0); set_escdelay(25);
    bool main_event_cycle = true, screen_size_state = false;
    Terminal_Screen_Draw main_object;
    while (main_event_cycle) {
        Terminal_Screen_yx_Size initial = Terminal_yx_Size_Monitor();
        if (!screen_size_state) {
            main_object.Main_Graphic_Draw(KINT);
        }
        int ch = getch();
        if (Terminal_Screen_yx_Size monitor = Terminal_yx_Size_Monitor(); monitor!=initial) {
            screen_size_state = true; main_object.Main_Graphic_Draw(KINT);
        }
        if (ch==ENTER||ch==KEY_UP||ch==KEY_DOWN) main_object.Main_Graphic_Draw(ch);
        if (ch==ESC||ch=='q') break;
    }
    endwin();
}

int main(){
    Terminal_Screen_Event_Main_Cycle();
}
