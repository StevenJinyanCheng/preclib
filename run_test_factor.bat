@echo off
echo Compiling test_factor.cpp with clang++...
clang++ -O2 -I. test/test_factor.cpp -o test_factor.exe
if %errorlevel% neq 0 (
    echo Compilation failed.
    exit /b 1
)

echo Running test_factor.exe...
.\test_factor.exe
