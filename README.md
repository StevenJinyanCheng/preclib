# Preclib: High-Performance Arbitrary Precision Arithmetic

**Preclib** is a header-only C++ library for high-speed arbitrary precision integer arithmetic. It focuses on performance for extremely large numbers (up to millions of words) by implementing state-of-the-art algorithms like Number Theoretic Transforms (NTT) and Newton-Raphson division.

## Features

*   **Header-only**: Easy to integrate, just include `prec.hpp`.
*   **Automatic Algorithm Dispatch**: Smartly chooses the fastest algorithm based on input size and balance.
*   **Multiplication Algorithms**:
    *   **Schoolbook**: $O(N^2)$ for small inputs.
    *   **Karatsuba**: $O(N^{1.585})$ for medium inputs.
    *   **Toom-Cook 3**: $O(N^{1.465})$ for medium-large inputs.
    *   **NTT (Number Theoretic Transform)**: $O(N \log N)$ for very large inputs (using 3-moduli CRT).
    *   **FFT (Complex)**: Floating-point FFT support.
*   **Division**:
    *   **Schoolbook Division** (Knuth Algorithm D).
    *   **Newton-Raphson**: Reduces division to multiplication ($O(M(n))$) for massive speedups on large integers.
*   **Utilities**:
    *   Modulo operations.
    *   Benchmarking suite (`test_compare_mul_algo.cpp`, `test_div.cpp`).
    *   Python visualization tools.

## Algorithms & Performance

The library implements a tiered multiplication strategy:

| Input Size (32-bit Words) | Algorithm Selected | Complexity |
|---------------------------|-------------------|------------|
| < 32 | Schoolbook | $O(N^2)$ |
| 32 - 512 | Karatsuba | $O(N^{1.58})$ |
| 512 - 2000 | Toom-Cook 3 | $O(N^{1.46})$ |
| > 2000 | NTT / FFT | $O(N \log N)$ |

**Benchmark Highlights (1M Words):**
*   **NTT vs Karatsuba**: ~30x Speedup
*   **Unbalanced Inputs (100k * 10)**: Optimized dispatch path yields ~60x speedup over blind NTT application.

## Usage

### Modern C++ API (Recommended)

The library provides RAII wrappers for automatic memory management and operator overloading.

#### Unsigned Integers (`precn`)

```cpp
#include "prec.hpp"
#include <iostream>

int main() {
    // Initialize from string or integer
    precn a = "12345678901234567890";
    precn b = 12345;
    
    // Arithmetic operations (+, -, *, /, %)
    precn c = a * b;
    precn d = c / 100;
    
    std::cout << "Result: " << c.to_string() << std::endl;
    // Automatic cleanup
    return 0;
}
```

#### Signed Integers (`precz`)

```cpp
#include "prec.hpp"

int main() {
    precz pos = "100";
    precz neg = -50;
    
    precz res = pos + neg; // 50
    res = res * -2;        // -100
}
```

### Low-Level API

For manual memory control, use `precn_impl` namespace.

```cpp
#include "prec.hpp"

int main() {
    using namespace precn_impl;

    // Create numbers
    precn_t a = precn_new(0); 
    precn_t b = precn_new(0);
    
    // Set values (allocating size)
    // ... initialize a, b ...

    // Multiplication
    precn_t res = precn_new(0);
    precn_mul_auto(a, b, res); // Automatically picks best algo

    // Division
    precn_t q = precn_new(0);
    precn_t r = precn_new(0);
    precn_div(a, b, q, r); // Uses Newton-Raphson for large inputs

    // Cleanup
    precn_free(a); precn_free(b); precn_free(res);
    precn_free(q); precn_free(r);
    
    return 0;
}
```

## Running Benchmarks

Requires `clang++` or `g++` and `python` (for plotting).

1.  **Multiplication Comparison**:
    ```powershell
    clang++ -O3 -o test_mul.exe test_compare_mul_algo.cpp
    .\test_mul.exe
    ```

2.  **Generate Graphs**:
    ```powershell
    python plot_benchmark.py
    ```
    (Generates `mul_benchmark_graph.png`)

3.  **Division Benchmark**:
    ```powershell
    clang++ -O3 -o test_div.exe test_div.cpp
    .\test_div.exe
    ```

## Development

*   `prec.hpp`: Main library file.
*   `precn_mul_ntt`: Implementation of NTT with CRT.
*   `precn_div_newton`: Newton-Raphson inverse iteration.
