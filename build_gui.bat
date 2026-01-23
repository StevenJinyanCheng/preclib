@echo off
echo Compiling gui_calc.cpp...
clang++ -O2 -I. gui_calc.cpp -o gui_calc.exe -luser32 -lgdi32 -lcomctl32
if %errorlevel% neq 0 (
    echo Compilation failed.
    exit /b 1
)
echo Compilation successful.
echo Running gui_calc.exe...
start gui_calc.exe
