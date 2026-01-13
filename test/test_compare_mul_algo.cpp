#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include "../prec.hpp"

using namespace precn_impl;

// Helper to create random number of size n words
precn_t create_random_precn(uint64_t n_words) {
    precn_t p = precn_new(0);
    precn_realloc(p, n_words);
    p->rsiz = n_words;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(0, 0xFFFFFFFF);
    
    for(uint64_t i=0; i<n_words; ++i) {
        p->n[i] = dis(gen);
    }
    // Ensure MSB is not 0 to keep size correct
    if (n_words > 0 && p->n[n_words-1] == 0) p->n[n_words-1] = 1;
    
    return p;
}

void run_comparison(uint64_t size) {
    std::cout << "Benchmarking size " << size << " words..." << std::endl;
    
    precn_t a = create_random_precn(size);
    precn_t b = create_random_precn(size);
    
    precn_t res_kara = precn_new(0);
    precn_t res_fft = precn_new(0);
    precn_t res_dispatch = precn_new(0);
    
    int iterations = 1;
    if (size < 1000) iterations = 50;
    else if (size < 5000) iterations = 10;
    else if (size < 20000) iterations = 5;
    else iterations = 2;

    // 1. Pure Karatsuba
    auto start_kara = std::chrono::high_resolution_clock::now();
    for(int i=0; i<iterations; ++i)
        precn_mul_karatsuba(a, b, res_kara); 
    auto end_kara = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time_kara = end_kara - start_kara;
    double avg_kara = time_kara.count() / iterations;

    // 2. NTT (using CRT)
    auto start_ntt = std::chrono::high_resolution_clock::now();
    for(int i=0; i<iterations; ++i)
        precn_mul_ntt(a, b, res_fft); 
    auto end_ntt = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time_ntt = end_ntt - start_ntt;
    double avg_ntt = time_ntt.count() / iterations;

    // 2b. FFT Complex
    precn_t res_fft_c = precn_new(0);
    auto start_fft_c = std::chrono::high_resolution_clock::now();
    for(int i=0; i<iterations; ++i)
        precn_mul_fft_complex(a, b, res_fft_c);
    auto end_fft_c = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time_fft_c = end_fft_c - start_fft_c;
    double avg_fft_c = time_fft_c.count() / iterations;

    // 3. Dispatch (Auto: Karatsuba / Toom-Cook / NTT)
    // g_enable_fft = true; // No longer needed for precn_mul_auto
    auto start_disp = std::chrono::high_resolution_clock::now();
    for(int i=0; i<iterations; ++i)
        precn_mul_auto(a, b, res_dispatch); 
    auto end_disp = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time_disp = end_disp - start_disp;
    double avg_disp = time_disp.count() / iterations;

    // 4. Pure Toom-Cook (Force no NTT - effectively precn_mul_toom_cook now)
    // g_enable_fft = false; // No longer affects precn_mul_toom_cook
    precn_t res_toom = precn_new(0);
    auto start_toom = std::chrono::high_resolution_clock::now();
    for(int i=0; i<iterations; ++i)
        precn_mul_toom_cook(a, b, res_toom);
    auto end_toom = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time_toom = end_toom - start_toom;
    double avg_toom = time_toom.count() / iterations;
    // g_enable_fft = true; // Restore

    // Verification
    bool match_ntt = (precn_cmp(res_kara, res_fft) == 0);
    bool match_fft_c = (precn_cmp(res_kara, res_fft_c) == 0);
    bool match_disp = (precn_cmp(res_kara, res_dispatch) == 0);
    bool match_toom = (precn_cmp(res_kara, res_toom) == 0);

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "  Karatsuba:   " << avg_kara << " s " << std::endl;
    
    std::cout << "  NTT (CRT):   " << avg_ntt << " s ";
    if (match_ntt) std::cout << "[PASS]"; else std::cout << "[FAIL]";
    std::cout << " (Speedup vs Kara: " << avg_kara/avg_ntt << "x)" << std::endl;

    std::cout << "  FFT Complex: " << avg_fft_c << " s ";
    if (match_fft_c) std::cout << "[PASS]"; else std::cout << "[FAIL]";
    std::cout << " (Speedup vs Kara: " << avg_kara/avg_fft_c << "x)" << std::endl;

    std::cout << "  Dispatch:    " << avg_disp << " s ";
    if (match_disp) std::cout << "[PASS]"; else std::cout << "[FAIL]";
    std::cout << " (Speedup vs Kara: " << avg_kara/avg_disp << "x)" << std::endl;

    std::cout << "  Toom-Cook:   " << avg_toom << " s ";
    if (match_toom) std::cout << "[PASS]"; else std::cout << "[FAIL]";
    std::cout << " (Speedup vs Kara: " << avg_kara/avg_toom << "x)" << std::endl;
    
    std::cout << "----------------------------------------" << std::endl;
    
    precn_free(a);
    precn_free(b);
    precn_free(res_kara);
    precn_free(res_fft);
    precn_free(res_fft_c);
    precn_free(res_dispatch);
    precn_free(res_toom);
}

int main() {
    std::cout << "=== Multiplication Algorithm Comparison ===" << std::endl;
    
    // Small sizes (Karatsuba should win)
    run_comparison(100);
    run_comparison(1000);
    
    // Medium sizes (Toom-Cook might win, FFT overhead high)
    run_comparison(4000);
    run_comparison(8000);
    run_comparison(16000);
    
    // Large sizes (FFT should win)
    run_comparison(32000);
    run_comparison(64000);
    run_comparison(128000);
    run_comparison(256000);
    run_comparison(512000);
    run_comparison(1024000);
    return 0;
}
