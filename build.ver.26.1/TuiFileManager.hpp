#pragma once
#include "GeneralFileOper.hpp"
#include <string>
#include <stack>
#include <optional>

#include <filesystem>
#include <fmt/xchar.h>

#include <ncursesw/ncurses.h>
#include <locale.h>

#include <concepts>


#define ESC 27
#define ENTER 10

#define KEYS 12
#define KINT -1

constexpr int Input_Key_Set[KEYS] = {
    KEY_UP,KEY_DOWN,KEY_LEFT,
    KEY_F(1),KEY_F(2),
    KEY_BACKSPACE,127,8,KEY_DC,
    ENTER,
    'q',ESC,
};
constexpr bool Input_Key_Check(int ch) {
    for (int idx=0; idx<KEYS; idx++)
        if (Input_Key_Set[idx]==ch)
            return true;
    return false;
}

struct Terminal_Screen_yx_Size {
    int Y; int X;
    bool operator==(const Terminal_Screen_yx_Size & object) const{return Y==object.Y&&X==object.X;}
    bool operator!=(const Terminal_Screen_yx_Size & object) const{return !(*this==object);}
};
inline Terminal_Screen_yx_Size Terminal_yx_Size_Monitor() {
    int y,x; getmaxyx(stdscr,y,x);
    return Terminal_Screen_yx_Size{y,x};
};

template<typename Fn, typename... Args> requires std::invocable<Fn, Args...>
void Terminal_Template_Current_MountTemporary_NewTUI_Launch(Fn Func_call, Args&&... args){
    def_prog_mode();
    endwin();
    Func_call(std::forward<Args>(args)...);
    reset_prog_mode();
}
void Terminal_Error_Ring() {
    beep();
    flash();
}

class Terminal_File_Manager_Draw {
    struct Screen_Row_Info {
        int fix; int bp; int offset; int total; int vsb;
    };
    mutable Screen_Row_Info scrl{1,1,0,0,0};
    std::filesystem::path current_path;

    void Title_Draw(std::filesystem::path source_path) const {
        mvaddwstr(0,0,source_path.generic_wstring().data());
        refresh();
    }
    void File_List_Info_Draw(const int ch) const {
        auto file_list_info = File_Folder_List_Info(current_path);
        scrl.total = file_list_info.size(); scrl.vsb = LINES-1;

        [&] {
            int scrl_vsb_len = scrl.offset+1;
            int scrl_cur_pos = scrl.bp+scrl.offset;
            int scrl_len = (scrl.bp-scrl.fix)+scrl_vsb_len;

            if (ch==-1) return;
            if (ch==KEY_UP) {
                if(scrl_cur_pos>scrl.fix){
                    if(scrl.offset == 0){scrl.bp--;}
                    else scrl.offset--;
                }else{
                    Terminal_Error_Ring();
                }
            }else if (ch==KEY_DOWN) {
                if(scrl_len<scrl.total && scrl_vsb_len<=scrl.vsb){
                    if(scrl_vsb_len==scrl.vsb){scrl.bp++;}
                    else scrl.offset++;
                }else{
                    Terminal_Error_Ring();
                }
            }
        }();

        int scrl_cur_y = scrl.fix+scrl.offset;
        int scrl_total_To_bp_len = scrl.total+(scrl.fix-1)-scrl.bp+1;
        int scrl_bp_idx = scrl.bp-scrl.fix;
        int scrl_cur_idx = scrl_bp_idx+scrl.offset;

        auto Terminal_Print_Args_Choose = [&](int for_time){
            for(int idx=scrl_bp_idx; idx<scrl_bp_idx+for_time; idx++){
                int for_offset = idx - scrl_bp_idx;
                std::wstring entry = fmt::format(L"{} {}",String_UTF8_TO_WString(File_Permission_Gain(file_list_info[idx].filepath)),
                    file_list_info[idx].filename);
                mvaddwstr(scrl.fix+for_offset,0,entry.data());
                if(idx==scrl_cur_idx){mvaddch(scrl_cur_y,WString_Cols_Count(entry),' '|A_STANDOUT);}
            }
        };
        if(scrl_total_To_bp_len<=scrl.vsb) Terminal_Print_Args_Choose(scrl_total_To_bp_len);
        else Terminal_Print_Args_Choose(scrl.vsb);
    }

    void File_Chain_Map() const {
        auto Items = File_Folder_List_Info(current_path);
        auto [p,n] = Items[scrl.bp-scrl.fix+scrl.offset];
        if (std::filesystem::is_directory(p)) {
            folder_chain.push(*this); temp_folder = p;
        }else {
            // Terminal_Template_Current_MountTemporary_NewTUI_Launch(File_GUN_TextCompiler,p);
            Terminal_Template_Current_MountTemporary_NewTUI_Launch(File_Vim_TextCompiler,p);
        }
    }

    void File_Remove() const {
        auto Items = File_Folder_List_Info(current_path);
        auto [p,n] = Items[scrl.bp-scrl.fix+scrl.offset];
        File_General_Clean(p);
    }

    std::wstring Terminal_Child_Win_MakeName(std::wstring raw_name) const {
        int child_win_height = 1, child_win_pos = LINES - child_win_height;
        WINDOW* win = newwin(child_win_height,COLS,child_win_pos,0);
        keypad(win,TRUE);
        WINDOW* backup = newwin(1,COLS,0,0);
        copywin(stdscr,backup,child_win_pos,0,0,0,0,COLS-1,0);

        start_color();
        init_pair(1,COLOR_RED,COLOR_WHITE);
        wbkgd(win,COLOR_PAIR(1)); wattron(win,COLOR_PAIR(1));

        int tmp_len = raw_name.size();

        // move(LINES-1,0); clrtoeol(); refresh();
        while(true){
            werase(win);
            std::wstring show_string = L"$Name$"+raw_name;
            // int width = std::min{WString_Cols_Count(show_string),COLS-1};
            mvwaddwstr(win,0,0,show_string.data());
            mvwaddch(win,0,WString_Cols_Count(show_string),L' '|A_STANDOUT);
            wrefresh(win);

            wint_t wch;
            int result = get_wch(&wch);
            if(wch==KEY_BACKSPACE){
                if(tmp_len!=0) {
                    tmp_len--;
                    raw_name.pop_back();
                }
            }

            if(wch==ENTER) break;
            if(wch==ESC) goto ESC_LABEL;
            if(result==OK){
                if(File_Name_Input_Char_Check(wch)){
                    raw_name+=static_cast<wchar_t>(wch);
                    tmp_len++;
                }
            }
        }

        overwrite(backup,stdscr);
        delwin(win);
        return raw_name;

        ESC_LABEL:
        overwrite(backup,stdscr);
        delwin(win);
        return L"";
    }
    void Terminal_File_Create(int choice) const {
        std::wstring raw_name[] = {L"newFile",L"newFolder"};
        std::wstring target_name = raw_name[choice%2];
        target_name = Terminal_Child_Win_MakeName(target_name);
        if(target_name==L"") return;
        switch(choice){
            case 0:{
                File_Non_Repeating_Create(File_Type::File,current_path,target_name);
                break;
            }
            default:{
                File_Non_Repeating_Create(File_Type::Folder,current_path,target_name);
            }
        }
    }
public:
    inline static std::stack<Terminal_File_Manager_Draw> folder_chain;
    inline static std::optional<std::filesystem::path> temp_folder = std::nullopt;
    explicit Terminal_File_Manager_Draw(std::filesystem::path file_path): current_path(file_path){}
    Terminal_File_Manager_Draw operator=(const Terminal_File_Manager_Draw & object) {
        if (this==&object) return *this;
        this->current_path = object.current_path;
        this->scrl = object.scrl;
        return *this;
    }
    void Main_Graphic_Draw(const int ch) {
        clear();
        Title_Draw(current_path);
        File_List_Info_Draw(ch);
        refresh();

        if (ch==KEY_F(1)) {
            Terminal_File_Create(0);
            refresh();
        }else if (ch==KEY_F(2)) {
            Terminal_File_Create(1);
            refresh();
        }else if (ch==ENTER) {
            File_Chain_Map();
        }else if (ch==8||ch==127||ch==KEY_BACKSPACE||ch==KEY_DC) {
            File_Remove();
        }
    }
};

inline void Input_Key_Event_Response(Terminal_File_Manager_Draw& object,int ch, bool* main_event_cycle) {
    if (ch=='q'||ch==ESC) { *main_event_cycle = false; return;}
    if (ch==8||ch==127||ch==KEY_BACKSPACE||ch==KEY_DC||ch==KEY_UP||ch==KEY_DOWN||ch==ENTER||ch==KEY_F(1)||ch==KEY_F(2)) {
        object.Main_Graphic_Draw(ch);
    }
}

inline void Terminal_Screen_Event_Cycle(std::filesystem::path source_path){
    //unicode
    setlocale(LC_ALL,"");
    initscr();
    noecho(); cbreak(); keypad(stdscr,TRUE); curs_set(0); set_escdelay(25);

    bool main_event_cycle = true, screen_size_state = false;
    Terminal_File_Manager_Draw main_object(source_path);
    while(main_event_cycle){
        Terminal_Screen_yx_Size initial = Terminal_yx_Size_Monitor();
        if (!screen_size_state) {
            main_object.Main_Graphic_Draw(KINT);
        }

        int ch = getch();
        if (Terminal_Screen_yx_Size monitor = Terminal_yx_Size_Monitor(); monitor!=initial) {
            screen_size_state = true; main_object.Main_Graphic_Draw(KINT);
        }

        if (!Input_Key_Check(ch)) continue;
        Input_Key_Event_Response(main_object,ch,&main_event_cycle);

        if(Terminal_File_Manager_Draw::temp_folder!=std::nullopt && ch==ENTER && std::filesystem::exists(*(Terminal_File_Manager_Draw::temp_folder))){
            const Terminal_File_Manager_Draw temp_object(*(Terminal_File_Manager_Draw::temp_folder)); main_object = temp_object;
            Terminal_File_Manager_Draw::temp_folder = std::nullopt; screen_size_state = false;
        }
        if(ch==KEY_LEFT){
            if(!Terminal_File_Manager_Draw::folder_chain.empty()){
                main_object =  Terminal_File_Manager_Draw::folder_chain.top(); Terminal_File_Manager_Draw::folder_chain.pop();
                screen_size_state = false;
            }
        }
    }

    endwin();
}


inline void Terminal_FileManager_Interface(std::filesystem::path _path){
    Terminal_Screen_Event_Cycle(_path);
}

// int main(){
//     Terminal_FileManager_Interface(std::filesystem::current_path());
// }
