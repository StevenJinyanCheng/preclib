clang++ -std=c++17 -municode -o minicalc.exe gui_calc_cn.cpp -luser32 -lgdi32 -lcomctl32 -march=native -O3 -ffast-math -funroll-loops
if %errorlevel% neq 0 (
    echo "编译MiniCalc失败"
    pause
    exit /b %errorlevel%
)
echo "MiniCalc安装成功"
pause