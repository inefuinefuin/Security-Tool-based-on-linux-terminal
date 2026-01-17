cmake_minimum_required(VERSION 4.0)
project(build_ver_26_1)

set(CMAKE_CXX_STANDARD 23)

include_directories(.)

add_executable(build_ver_26_1
        Compress.hpp
        Core.hpp
        GeneralFileOper.hpp
        Program.cpp
        Shell.hpp
        TUI.hpp
        TuiFileManager.hpp
        typelib.hpp)
