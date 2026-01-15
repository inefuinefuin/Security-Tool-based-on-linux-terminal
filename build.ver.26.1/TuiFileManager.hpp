#pragma once
#include "GeneralFileOper.hpp"

#include <string>
#include <stack>
#include <optional>
#include <filesystem>
#include <concepts>

#include <fmt/xchar.h>
#include <ncursesw/ncurses.h>
#include <locale.h>

#define ESC 27
#define ENTER 10

#define KEYS 12
#define KINT -1
// ===== Key Set =====
// Valid input key set
// Check if key is in valid key set
constexpr int Input_Key_Set[KEYS] = {
    KEY_UP,KEY_DOWN,KEY_LEFT,
    KEY_F(1),KEY_F(2),
    KEY_BACKSPACE,127,8,KEY_DC,
    ENTER,
    'q',ESC,
};
constexpr bool Input_Key_Check(const int ch) {
    for (int idx=0; idx<KEYS; idx++)
        if (Input_Key_Set[idx]==ch)
            return true;
    return false;
}

// ===== Window Size =====
//Screen size struct and monitor
struct Terminal_Screen_yx_Size {
    int Y; int X;
    bool operator==(const Terminal_Screen_yx_Size & object) const{return Y==object.Y&&X==object.X;}
    bool operator!=(const Terminal_Screen_yx_Size & object) const{return !(*this==object);}
};
inline Terminal_Screen_yx_Size Terminal_yx_Size_Monitor() {
    int y,x; getmaxyx(stdscr,y,x);
    return Terminal_Screen_yx_Size{y,x};
};

// ===== Terminal Utility Functions =====
// Launch new TUI in child process (mount current TUI state)
// Tuiminal Error Ring (beep and flash)
template<typename Fn, typename... Args> requires std::invocable<Fn, Args...>
void Terminal_MountCurrentTUI_LaunchNewTUI(Fn Func_call, Args&&... args){
    def_prog_mode();
    endwin();
    Func_call(std::forward<Args>(args)...);
    reset_prog_mode();
}
void Terminal_Error_Ring() { beep(); flash();}

namespace TUI_KidWin_Components{
    WINDOW* KidWin_Create(const int height, const int width, const int starty, const int startx){
        WINDOW* win = newwin(height,width,starty,startx);
        keypad(win,TRUE);
        start_color(); init_pair(1,COLOR_RED,COLOR_WHITE);
        wbkgd(win,COLOR_PAIR(1)); wattron(win,COLOR_PAIR(1));
        return win;
    }
    void KidWin_Destroy(WINDOW* win){
        delwin(win);
    }

    WINDOW* KidWin_Backup_LastLine(const WINDOW* target_win, const int starty){
        WINDOW* backup = newwin(1,COLS,0,0);
        copywin(target_win,backup,starty,0,0,0,0,COLS-1,0);
        return backup;
    }
    void KidWin_Restore(const WINDOW* backup_win, WINDOW* target_win){
        overwrite(backup_win,target_win);
        delwin(const_cast<WINDOW*>(backup_win));
    }

}
// ===== File Manager TUI =====
// Draw title bar with current path

// Scroll information structure
// Handle scroll events (up/down navigation)
// Draw file entries with permissions

// Input dialog for file/folder name (kid window)
// Create new file or folder
void TUI_Title_Draw(std::filesystem::path source_path) {
    mvaddwstr(0,0,source_path.generic_wstring().data()); refresh();
}

struct Screen_Row_Info {
    int fix; int bp; int offset; int total; int vsb;
};
void Scroll_Event_Response(Screen_Row_Info& scrl, const int ch) {
    int scrl_vsb_len = scrl.offset+1;
    int scrl_cur_pos = scrl.bp+scrl.offset;
    int scrl_len = (scrl.bp-scrl.fix)+scrl_vsb_len;

    if (ch==KINT) return;
    if (ch==KEY_UP) {
        if(scrl_cur_pos>scrl.fix){
            if(scrl.offset == 0){scrl.bp--;}
            else scrl.offset--;
        }else Terminal_Error_Ring();
    }else if (ch==KEY_DOWN) {
        if(scrl_len<scrl.total && scrl_vsb_len<=scrl.vsb){
            if(scrl_vsb_len==scrl.vsb){scrl.bp++;}
            else scrl.offset++;
        }else Terminal_Error_Ring();
    }
}
void File_Entry_Draw(const Screen_Row_Info& scrl,const int for_time, /*vector<path,wstring>*/const auto& Entries) {
    int scrl_cur_y = scrl.fix+scrl.offset;
    int scrl_bp_idx = scrl.bp-scrl.fix;
    int scrl_cur_idx = scrl_bp_idx+scrl.offset;

    for(int idx=scrl_bp_idx; idx<scrl_bp_idx+for_time; idx++){
        int for_offset = idx - scrl_bp_idx;
        std::wstring entry = fmt::format(L"{} {}",String_UTF8_TO_WString(File_Permission_Gain(Entries[idx].filepath)),
            Entries[idx].filename);
        mvaddwstr(scrl.fix+for_offset,0,entry.data());
        if(idx==scrl_cur_idx){mvaddch(scrl_cur_y,WString_Cols_Count(entry),' '|A_STANDOUT);}
    }
}

std::wstring TUI_KidWin_InStr(std::wstring init_name) {
    int win_height = 1, win_pos = LINES - 1;
    WINDOW* win = TUI_KidWin_Components::KidWin_Create(win_height,COLS,win_pos,0);
    WINDOW* backup = TUI_KidWin_Components::KidWin_Backup_LastLine(stdscr,win_pos);

    int tmp_len = init_name.size();
    while(true){
        werase(win);
        std::wstring show_string = L"$Name$"+init_name;
        // int width = std::min{WString_Cols_Count(show_string),COLS-1};
        mvwaddwstr(win,0,0,show_string.data());
        mvwaddch(win,0,WString_Cols_Count(show_string),L' '|A_STANDOUT);
        wrefresh(win);

        wint_t wch;
        int result = get_wch(&wch);
        if(wch==ESC){
            TUI_KidWin_Components::KidWin_Restore(backup, stdscr);
            TUI_KidWin_Components::KidWin_Destroy(win);
            return L"";
        }
        if(wch==ENTER) break;
        if(wch==KEY_BACKSPACE){
            if(tmp_len!=0) {
                tmp_len--;
                init_name.pop_back();
            }
        }
        if(result==OK){
            if(File_Input_FileName_Char_Check(wch)){
                init_name+=static_cast<wchar_t>(wch);
                tmp_len++;
            }
        }
    }
    TUI_KidWin_Components::KidWin_Restore(backup, stdscr);
    TUI_KidWin_Components::KidWin_Destroy(win);
    return init_name;
}
void TUI_FileCreate(const std::filesystem::path& current_path, const int choice) {
    std::wstring raw_name[] = {L"newFile.txt",L"newFolder.txt"};
    std::wstring target_name = raw_name[choice%2];
    target_name = TUI_KidWin_InStr(target_name);

    if(target_name==L"") { Terminal_Error_Ring(); return;}
    
    if(choice==0){
        File_Non_Repeating_Create(File_Type::File,current_path,target_name);
    }else if(choice==1){
        File_Non_Repeating_Create(File_Type::Folder,current_path,target_name);
    }
}
//renamed functions ...


// ===== File Manager TUI Draw =====
class Terminal_File_Manager_Draw {
    Screen_Row_Info scrl{1,1,0,-1,-1};
    std::filesystem::path current_path;
    
    // Draw file list with scroll

    // Navigate into folder or open file
    // Delete selected file or folder
    void File_Info_Draw(const int ch) {
        auto Entries = File_Folder_List_Info(current_path);
        
        scrl.total = Entries.size(); scrl.vsb = LINES-1;
        Scroll_Event_Response(scrl,ch);

        int scrl_total_To_bp_len = scrl.total+(scrl.fix-1)-scrl.bp+1;
        if(scrl_total_To_bp_len<=scrl.vsb) File_Entry_Draw(scrl,scrl_total_To_bp_len,Entries);
        else File_Entry_Draw(scrl,scrl.vsb,Entries);

        refresh();
    }

    void File_Chain_Map() const {
        auto Items = File_Folder_List_Info(current_path);
        auto [p,n] = Items[scrl.bp-scrl.fix+scrl.offset];
        if (std::filesystem::is_directory(p)) {
            folder_chain.push(*this); temp_folder = p;
        }else {
            Terminal_MountCurrentTUI_LaunchNewTUI(File_Vim_TextCompiler,p);
            // Terminal_MountCurrentTUI_LaunchNewTUI(File_GUN_TextCompiler,p);
        }
    }
    void File_Remove() const {
        auto Items = File_Folder_List_Info(current_path);
        auto [p,n] = Items[scrl.bp-scrl.fix+scrl.offset];
        File_TargetGeneral_Clean(p);
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

    // Main drawing function
    void Main_Graphic_Draw(const int ch) {
        clear();
        TUI_Title_Draw(current_path);
        File_Info_Draw(ch);

        if (ch==KEY_F(1)) {
            TUI_FileCreate(current_path,0);
        }else if (ch==KEY_F(2)) {
            TUI_FileCreate(current_path,1);
        }else if (ch==ENTER) {
            File_Chain_Map();
        }else if (ch==8||ch==127||ch==KEY_BACKSPACE||ch==KEY_DC) {
            File_Remove();
        }
    }
};


// ===== File Manager Event Loop =====
// Main event cycle for file manager TUI
inline void Terminal_Screen_Event_Cycle(std::filesystem::path source_path){
    setlocale(LC_ALL,""); initscr(); noecho(); cbreak(); keypad(stdscr,TRUE); curs_set(0); set_escdelay(25);

    Terminal_File_Manager_Draw main_object(source_path);
    
    bool screen_size_state = false;
    while(true){
        Terminal_Screen_yx_Size initial = Terminal_yx_Size_Monitor();
        if (!screen_size_state) main_object.Main_Graphic_Draw(KINT);

        int ch = getch();
        if (auto monitor = Terminal_yx_Size_Monitor(); monitor!=initial) {
            screen_size_state = true; main_object.Main_Graphic_Draw(KINT);
        }

        if (!Input_Key_Check(ch)) continue;
        if(ch==ESC||ch=='q') break;
        
        if (ch==8||ch==127||ch==KEY_BACKSPACE||ch==KEY_DC
            ||ch==KEY_UP||ch==KEY_DOWN||ch==ENTER||ch==KEY_F(1)||ch==KEY_F(2)) {
            main_object.Main_Graphic_Draw(ch);
        }

        if(Terminal_File_Manager_Draw::temp_folder!=std::nullopt && std::filesystem::exists(*(Terminal_File_Manager_Draw::temp_folder)) && ch==ENTER){
            const Terminal_File_Manager_Draw temp_object(*(Terminal_File_Manager_Draw::temp_folder)); main_object = temp_object;
            Terminal_File_Manager_Draw::temp_folder = std::nullopt; screen_size_state = false;
        }
        
        if(ch==KEY_LEFT){
            if(!Terminal_File_Manager_Draw::folder_chain.empty()){
                main_object =  Terminal_File_Manager_Draw::folder_chain.top(); 
                Terminal_File_Manager_Draw::folder_chain.pop(); screen_size_state = false;
            }
        }
    }

    endwin();
}

// ===== File Manager Interface =====
// Launch file manager TUI
inline void Terminal_FileManager_Interface(std::filesystem::path _path){
    Terminal_Screen_Event_Cycle(_path);
}

// ===== TUI File Manager Test =====
// int main(){
//     Terminal_FileManager_Interface(std::filesystem::current_path());
// }