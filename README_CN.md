# Preclib: 高性能任意精度算术库

**Preclib** 是一个用于高速任意精度整数运算的 header-only C++ 库。它专注于极大数（高达数百万字）的性能，实现了数论变换（NTT）和牛顿-拉夫逊除法等最先进的算法。

## 特性

*   **Header-only**: 易于集成，只需包含 `prec.hpp`。
*   **自动算法调度**: 根据输入大小和平衡性智能选择最快的算法。
*   **乘法算法**:
    *   **教科书式 (Schoolbook)**: 适用于小输入 $O(N^2)$。
    *   **Karatsuba**: 适用于中等输入 $O(N^{1.585})$。
    *   **Toom-Cook 3**: 适用于中大输入 $O(N^{1.465})$。
    *   **NTT (数论变换)**: 适用于非常大的输入 $O(N \log N)$ (使用 3 模数 CRT)。
    *   **FFT (复数)**: 支持浮点 FFT。
*   **除法**:
    *   **教科书式除法** (Knuth 算法 D)。
    *   **牛顿-拉夫逊 (Newton-Raphson)**: 将除法简化为乘法 ($O(M(n))$)，在大整数上实现大幅加速。
*   **工具**:
    *   模运算。
    *   基准测试套件 (`test_compare_mul_algo.cpp`, `test_div.cpp`)。
    *   Python 可视化工具。

## 算法与性能

该库实施了分层乘法策略：

| 输入大小 (32位字) | 选定的算法 | 复杂度 |
|---------------------------|-------------------|------------|
| < 32 | Schoolbook | $O(N^2)$ |
| 32 - 512 | Karatsuba | $O(N^{1.58})$ |
| 512 - 2000 | Toom-Cook 3 | $O(N^{1.46})$ |
| > 2000 | NTT / FFT | $O(N \log N)$ |

**基准测试亮点 (100万字):**
*   **NTT vs Karatsuba**: ~30倍加速
*   **不平衡输入 (100k * 10)**: 优化的调度路径比盲目应用 NTT 快 ~60倍。

## 使用方法

### 现代 C++ API (推荐)

该库提供 RAII 包装器，用于自动内存管理和运算符重载。

#### 无符号整数 (`precn`)

```cpp
#include "prec.hpp"
#include <iostream>

int main() {
    // 从字符串或整数初始化
    precn a = "12345678901234567890";
    precn b = 12345;
    
    // 算术运算 (+, -, *, /, %)
    precn c = a * b;
    precn d = c / 100;
    
    std::cout << "Result: " << c.to_string() << std::endl;
    // 自动清理
    return 0;
}
```

#### 有符号整数 (`precz`)

```cpp
#include "prec.hpp"

int main() {
    precz pos = "100";
    precz neg = -50;
    
    precz res = pos + neg; // 50
    res = res * -2;        // -100
}
```

### 底层 API

对于手动内存控制，请使用 `precn_impl` 命名空间。

```cpp
#include "prec.hpp"

int main() {
    using namespace precn_impl;

    // 创建数字
    precn_t a = precn_new(0); 
    precn_t b = precn_new(0);
    
    // 设置值 (分配大小)
    // ... 初始化 a, b ...

    // 乘法
    precn_t res = precn_new(0);
    precn_mul_auto(a, b, res); // 自动选择最佳算法

    // 除法
    precn_t q = precn_new(0);
    precn_t r = precn_new(0);
    precn_div(a, b, q, r); // 对大输入使用牛顿-拉夫逊法

    // 清理
    precn_free(a); precn_free(b); precn_free(res);
    precn_free(q); precn_free(r);
    
    return 0;
}
```

## 运行基准测试

需要 `clang++` 或 `g++` 和 `python` (用于绘图)。

1.  **乘法比较**:
    ```powershell
    clang++ -O3 -o test/test_mul_algo.exe test/test_compare_mul_algo.cpp
    .\test\test_mul_algo.exe
    ```

2.  **生成图表**:
    ```powershell
    python plot_benchmark.py
    ```
    (生成 `mul_benchmark_graph.png`)

3.  **除法基准测试**:
    ```powershell
    clang++ -O3 -o test/test_div.exe test/test_div.cpp
    .\test\test_div.exe
    ```

## 开发

*   `prec.hpp`: 主库文件。
*   `precn_mul_ntt`: 使用 CRT 实现的 NTT。
*   `precn_div_newton`: 牛顿-拉夫逊逆迭代。
