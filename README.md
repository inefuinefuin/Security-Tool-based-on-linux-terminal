# Security-Tool-based-on-linux-terminal
一個基於 Linux 終端的純離線、全功能安全工具箱。

## 敬告 ： 項目仍處於開發中 存在一些缺陷

## 🛡️ 項目宗旨
**「數據安全，歸於用戶。」** 本工具致力於實現用戶對資料的完全掌控。通過 100% 離線運行，從物理層面隔絕網絡泄露風險，解決用戶對現代軟件「後台傳輸數據」的疑慮，確保每一份加密存儲的資料都僅限於本地可見。

## ✨ 核心功能
- 📂 **文件管理**：直觀的終端文件瀏覽與管理。
- 📝 **Vim/GNU 編輯支持**：直接在工具內調用 Vim/GNU 進行文本編輯。
- 🐚 **簡易 Shell**：內置精簡 Shell 指令，快速處理系統任務。
- 🔐 **加密解密**：支持本地文件的加密存儲，確保敏感資料安全。
- 📦 **壓縮解壓**：高效整理本地資源，節省存儲空間。
- 💻 **TUI 交互**：基於 ncurses 打造的終端用戶界面。
- 🚫 **完全離線**：無網絡權限要求，絕不讀取、不傳輸任何用戶數據。

## 🛠️ 技術規格
- **開發語言**：C++ (C++20 標準)
- **編譯器**：g++-12
- **運行平台**：Linux (Ubuntu, Debian, Arch，wsl 等)
- **界面實現**：TUI (基於 ANSI Escape Codes / 原生終端交互)
- **輸入處理**: 針對多種終端模擬器優化，精確識別 `KEY_BACKSPACE`、`8` (Ctrl+H) 與 `127` (DEL)。
- **字符編碼**：**原生支持 UTF-16**，確保寬字符與多語言環境下的顯示兼容性。

### 依賴項 (Prerequisites)
本項目需要以下開發庫：
- **ncursesw**: 用於 TUI 界面顯示 (支持寬字符)
- **libsodium**: 用於安全加密解密功能
- **fmt**: 用於現代化的字符串格式化

### 安裝指令
bash: sudo apt-get install libncursesw5-dev libsodium-dev libfmt-dev


### 編譯指令
原始文件壓縮包內編譯: g++-12 -std=c++20 TUI.cpp TuiFileManager.hpp Shell.hpp Core.hpp GeneralFileOper.hpp -lncursesw -lfmt -lsodium -o SecuryTool
更新中的文件編譯 未來將採用: g++-12 -std=c++20 Program.cpp TUI.hpp -lfmt -lncursesw -lsodium -o SecurityTool

## 📖 使用指南 (Usage Guide)
### 1. 內置 Shell 模式
- **`man cmd`**：輸入此指令可獲取目前所有加載命令的詳細手冊與使用方法。
### 2. TUI 主界面交互
- **`方向鍵 Up/Down`**：移動虛擬光標進行功能選擇。
- **`Enter`**：確認並運行當前選中的功能。
- **`ESC` / `Q`**：退出當前界面或返回上級選單。
### 3. 文件管理器 (FileManager)
- **`方向鍵 Up/Down`**：在文件列表中切換選中目標。
- **`Enter`**：
  - 選中**文件夾**：進入該目錄。
  - 選中**文件**：對該文件執行操作（調用 Vim 編輯）。
- **`方向鍵 Left`**：返回上一層文件夾。
  - *安全限制：僅能返回至程序啟動時的初始目錄，防止越權訪問。*
- **`F1`**：**新建文件** —— 快速在當前目錄下創建新檔案。
- **`F2`**：**新建文件夾** —— 快速建立新的目錄結構。

## ⚖️ 許可證 (License)
本項目採用 MIT License。
