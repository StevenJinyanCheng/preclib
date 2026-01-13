#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include "prec.hpp"

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

void run_benchmark(uint64_t size) {
    std::cout << "Benchmarking size " << size << " words..." << std::endl;
    
    precn_t a = create_random_precn(size);
    precn_t b = create_random_precn(size);
    precn_t res_toom = precn_new(0);
    precn_t res_kara = precn_new(0);
    
    int iterations = 1;
    if (size < 1000) iterations = 100;
    else if (size < 5000) iterations = 10;
    else if(size < 20000) iterations = 5;
    else iterations = 2;
    // Toom-Cook (Dispatch)
    auto start = std::chrono::high_resolution_clock::now();
    for(int i=0; i<iterations; ++i)
        precn_mul_toom_cook(a, b, res_toom); 
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff_toom = end - start;
    
    // Pure Karatsuba
    auto start2 = std::chrono::high_resolution_clock::now();
    for(int i=0; i<iterations; ++i)
        precn_mul_karatsuba(a, b, res_kara); 
    auto end2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff_pure = end2 - start2;
    
    if (precn_cmp(res_toom, res_kara) != 0) {
        std::cout << " [FAIL] Results differ!" << std::endl;
    } else {
        std::cout << " [PASS] Results match." << std::endl;
    }
    
    std::cout << "Toom-Cook (Dispatch): " << diff_toom.count()/iterations << " s/op" << std::endl;
    std::cout << "Pure Karatsuba:       " << diff_pure.count()/iterations << " s/op" << std::endl;
    std::cout << "Speedup (Pure/Toom):  " << diff_pure.count() / diff_toom.count() << "x" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    precn_free(a);
    precn_free(b);
    precn_free(res_toom);
    precn_free(res_kara);
}

int main() {
    std::cout << "Starting benchmark..." << std::endl;
    
    // Warmup
    run_benchmark(100);
    
    // Real benchmarks
    run_benchmark(500);
    for(int i = 500;i < 1000000;i *= 2)
        run_benchmark(i);
    return 0;
}
