# Preclib 使用说明书

**Preclib** 是一个用于任意精度整数运算的 header-only C++ 库。该库支持大整数运算（高达数百万字），并实现了数论变换（NTT）和牛顿-拉夫逊除法等算法。

## 功能特性

*   **Header-only**: 只需包含 `prec.hpp` 即可使用。
*   **算法调度**: 根据输入大小和数据特性自动选择相应的乘法算法。
*   **乘法算法**:
    *   **Schoolbook**: 适用于小输入 $O(N^2)$。
    *   **Karatsuba**: 适用于中等输入 $O(N^{1.585})$。
    *   **Toom-Cook 3**: 适用于中大输入 $O(N^{1.465})$。
    *   **NTT (数论变换)**: 适用于大输入 $O(N \log N)$ (使用 3 模数 CRT)。
    *   **FFT (复数)**: 支持浮点 FFT 实现。
*   **除法**:
    *   **基础除法** (Knuth 算法 D)。
    *   **牛顿-拉夫逊 (Newton-Raphson)**: 针对大整数除法将运算转换为乘法 ($O(M(n))$)。
*   **其他工具**:
    *   模运算支持。
    *   包含基准测试代码 (`test_compare_mul_algo.cpp`, `test_div.cpp`)。
    *   Python 绘图脚本。

## 算法实现说明

库内部根据输入规模采用分层策略选择乘法算法：

| 输入大小 (32位字) | 算法 | 复杂度 |
|-------------------|------|--------|
| < 32 | Schoolbook | $O(N^2)$ |
| 32 - 512 | Karatsuba | $O(N^{1.58})$ |
| 512 - 2000 | Toom-Cook 3 | $O(N^{1.46})$ |
| > 2000 | NTT / FFT | $O(N \log N)$ |

**基准测试数据参考 (100万字):**
*   **NTT 与 Karatsuba 对比**: 在此规模下 NTT 效率通常优于 Karatsuba。
*   **非平衡输入**: 对于长度差异较大的输入，算法调度器会进行相应处理。

## 使用方法

### C++ API

库提供了 `precn` (无符号) 和 `precz` (有符号) 类，封装了内存管理和运算符重载。

#### 无符号整数 (`precn`) 示例

```cpp
#include "prec.hpp"
#include <iostream>

int main() {
    // 初始化
    precn a = "12345678901234567890";
    precn b = 12345;
    
    // 运算 (+, -, *, /, %)
    precn c = a * b;
    precn d = c / 100;
    
    std::cout << "Result: " << c.to_string() << std::endl;
    return 0;
}
```

#### 有符号整数 (`precz`) 示例

```cpp
#include "prec.hpp"

int main() {
    precz pos = "100";
    precz neg = -50;
    
    precz res = pos + neg; // 50
    res = res * -2;        // -100
}
```

### 底层 API (precn_impl)

如需手动管理内存，可使用 `precn_impl` 命名空间下的函数。

```cpp
#include "prec.hpp"

int main() {
    using namespace precn_impl;

    // 创建对象
    precn_t a = precn_new(0); 
    precn_t b = precn_new(0);
    
    // 初始化及赋值...

    // 乘法
    precn_t res = precn_new(0);
    precn_mul_auto(a, b, res); 

    // 除法
    precn_t q = precn_new(0);
    precn_t r = precn_new(0);
    precn_div(a, b, q, r);

    // 释放内存
    precn_free(a); precn_free(b); precn_free(res);
    precn_free(q); precn_free(r);
    
    return 0;
}
```

## 编译与测试

测试代码位于 `test/` 目录下。需要 C++ 编译器（如 clang++ 或 g++）和 Python（用于绘图）。

1.  **编译并运行乘法测试**:
    ```powershell
    clang++ -O3 -o test/test_mul_algo.exe test/test_compare_mul_algo.cpp
    .\test\test_mul_algo.exe
    ```

2.  **生成性能图表**:
    ```powershell
    python plot_benchmark.py
    ```
    (运行后生成 `mul_benchmark_graph.png`)

3.  **编译并运行除法测试**:
    ```powershell
    clang++ -O3 -o test/test_div.exe test/test_div.cpp
    .\test\test_div.exe
    ```

## 文件结构

*   `prec.hpp`: 核心库文件。
*   `prec_util.hpp`: 辅助工具。
*   `uint128.hpp`: 128位整数支持。
*   `test/`: 测试代码目录。