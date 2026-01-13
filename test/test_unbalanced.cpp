#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include "../prec.hpp"

using namespace precn_impl;

precn_t create_random_precn(uint64_t n_words) {
    precn_t p = precn_new(0);
    precn_realloc(p, n_words);
    p->rsiz = n_words;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(0, 0xFFFFFFFF);
    for(uint64_t i=0; i<n_words; ++i) p->n[i] = dis(gen);
    if (n_words > 0 && p->n[n_words-1] == 0) p->n[n_words-1] = 1;
    return p;
}

int main() {
    uint64_t large_size = 100000;
    uint64_t small_size = 10;
    
    std::cout << "Testing Unbalanced Multiplication: " << large_size << " x " << small_size << " words" << std::endl;
    
    precn_t a = create_random_precn(large_size);
    precn_t b = create_random_precn(small_size);
    precn_t res_old = precn_new(0);
    precn_t res_auto = precn_new(0);
    
    // Benchmark NTT (Forced)
    auto start_ntt = std::chrono::high_resolution_clock::now();
    for(int i=0; i<50; ++i) precn_mul_ntt(a, b, res_old);
    auto end_ntt = std::chrono::high_resolution_clock::now();
    
    // Benchmark Auto (Should pick Slow)
    auto start_auto = std::chrono::high_resolution_clock::now();
    for(int i=0; i<50; ++i) precn_mul_auto(a, b, res_auto);
    auto end_auto = std::chrono::high_resolution_clock::now();
    
    // Validate
    if (precn_cmp(res_old, res_auto) != 0) {
        std::cout << "MISMATCH ERROR!" << std::endl;
        return 1;
    }
    
    std::cout << "NTT Time:  " << std::chrono::duration<double>(end_ntt - start_ntt).count() / 50 << "s" << std::endl;
    std::cout << "Auto Time: " << std::chrono::duration<double>(end_auto - start_auto).count() / 50 << "s" << std::endl;
    
    double speedup = std::chrono::duration<double>(end_ntt - start_ntt).count() / std::chrono::duration<double>(end_auto - start_auto).count();
    std::cout << "Speedup: " << speedup << "x" << std::endl;

    precn_free(a); precn_free(b); precn_free(res_old); precn_free(res_auto);
    return 0;
}
