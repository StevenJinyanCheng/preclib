@echo off
taskkill /im gui_calc_cn.exe /f >nul 2>&1
del gui_calc_cn.exe >nul 2>&1
clang++ -std=c++17 -municode -o gui_calc_cn.exe gui_calc_cn.cpp -luser32 -lgdi32 -lcomctl32
