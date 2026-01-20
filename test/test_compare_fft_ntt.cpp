#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include "../prec.hpp"

// We need to access these direct functions. 
// Assuming they are exposed in prec.hpp based on previous reads.
// extern int precn_mul_fft_complex(precn_t a, precn_t b, precn_t res);
// extern int precn_mul_ntt(precn_t a, precn_t b, precn_t res);

using namespace precn_impl;

void fill_random(precn_t a, size_t size) {
    precn_realloc(a, size);
    a->rsiz = size;
    
    // Simple LCG for speed
    uint64_t state = 123456789 + size;
    for(size_t i=0; i<size; ++i) {
        state = state * 6364136223846793005ULL + 1;
        a->n[i] = (uint32_t)(state >> 32);
    }
    if (size > 0 && a->n[size-1] == 0) a->n[size-1] = 1;
}

void run_test(const std::string& name, size_t size, bool /*unused_res*/ = false) {
    std::cout << "--------------------------------------------------" << std::endl;
    std::cout << "Test: " << name << std::endl;
    std::cout << "Size: " << size << " words (" << (size * 32 / 8 / 1024) << " KB)" << std::endl;
    
    // Strategy is now always safe 8-bit split with Karatsuba recursion for large sizes.
    // Explicit prediction logic removed.

    precn_t a = precn_new(0);
    precn_t b = precn_new(0);
    fill_random(a, size);
    fill_random(b, size);
    
    precn_t res_fft = precn_new(0);
    precn_t res_ntt = precn_new(0);
    
    // 1. FFT strategy (via Karatsuba wrapper to ensure safety)
    // Note: precn_mul_karatsuba_ntt will use FFT at leaves because PRECN_USE_FFT_MUL is defined.
    // It will decompose 1M words until they fit in safe FFT size (256k).
    auto t1 = std::chrono::high_resolution_clock::now();
    precn_mul_karatsuba_ntt(a, b, res_fft);
    auto t2 = std::chrono::high_resolution_clock::now();
    double d_fft = std::chrono::duration<double>(t2 - t1).count();
    
    std::cout << "FFT Time: " << std::fixed << std::setprecision(6) << d_fft << " s" << std::endl;

    // 2. NTT
    auto t3 = std::chrono::high_resolution_clock::now();
    precn_mul_ntt(a, b, res_ntt);
    auto t4 = std::chrono::high_resolution_clock::now();
    double d_ntt = std::chrono::duration<double>(t4 - t3).count();

    std::cout << "NTT Time: " << std::fixed << std::setprecision(6) << d_ntt << " s" << std::endl;
    
    std::cout << "Speedup (NTT/FFT): " << (d_ntt / d_fft) << "x" << std::endl;

    // Verify
    bool ok = true;
    if (res_fft->rsiz != res_ntt->rsiz) {
        std::cout << "FAIL: Size mismatch! FFT=" << res_fft->rsiz << " NTT=" << res_ntt->rsiz << std::endl;
        ok = false;
    } else {
        for(size_t i=0; i<res_fft->rsiz; ++i) {
            if (res_fft->n[i] != res_ntt->n[i]) {
                std::cout << "FAIL: Data mismatch at index " << i << std::endl;
                std::cout << "FFT: " << res_fft->n[i] << " NTT: " << res_ntt->n[i] << std::endl;
                ok = false;
                break;
            }
        }
    }
    
    if (ok) std::cout << "VERIFICATION: PASS" << std::endl;

    precn_free(a);
    precn_free(b);
    precn_free(res_fft);
    precn_free(res_ntt);
}

int main() {
    // Case 1: 50,000 words
    run_test("Medium Size (50k)", 50000, false);
    
    // Case 2: 200,000 words (Direct FFT range, < 262k)
    run_test("Boundary Direct (200k)", 200000, false);

    // Case 3: 300,000 words (Should be Direct 8-bit now)
    run_test("Large Size (300k - Direct)", 300000, false);

    // Case 4: 1,000,000 words (Direct 8-bit)
    run_test("Very Large (1M - Direct)", 1000000, false);

    // Case 5: 2,000,000 words
    run_test("Huge Size (2M - Direct)", 2000000, false);

    // Case 6: 4,000,000 words
    run_test("Massive Size (4M - Direct)", 4000000, false);

    // Case 7: 8,000,000 words
    run_test("Gigantic Size (8M - Direct)", 8000000, false);

    // Case 8: 16,000,000 words
    run_test("Colossal Size (16M - Direct)", 16000000, false);

    // Case 9: 32,000,000 words
    // Memory warning: This will use several GBs of RAM.
    run_test("Astronomical Size (32M - Direct)", 32000000, false);

    return 0;
}
