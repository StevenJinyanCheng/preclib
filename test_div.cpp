#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include "prec.hpp"

using namespace precn_impl;

void check_div(uint64_t asize, uint64_t bsize) {
    std::cout << "Testing Div " << asize << " / " << bsize << "... ";
    
    precn_t a = precn_new(0);
    precn_t b = precn_new(0);
    precn_realloc(a, asize); a->rsiz = asize;
    precn_realloc(b, bsize); b->rsiz = bsize;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(1, 0xFFFFFFFF);
    
    for(size_t i=0; i<asize; ++i) a->n[i] = dis(gen);
    for(size_t i=0; i<bsize; ++i) b->n[i] = dis(gen);
    
    // Ensure b is not 0
    if (b->rsiz > 0 && b->n[b->rsiz-1] == 0) b->n[b->rsiz-1] = 1;
    
    precn_t q = precn_new(0);
    precn_t r = precn_new(0);
    
    auto start = std::chrono::high_resolution_clock::now();
    precn_div(a, b, q, r);
    auto end = std::chrono::high_resolution_clock::now();
    
    // Check: a = b*q + r
    precn_t check = precn_new(0);
    precn_mul_ntt(b, q, check); // assuming mul works
    precn_add(check, r, check);
    
    // Compare
    int cmp = precn_cmp(check, a);
    
    // Also check r < b
    int r_cmp = precn_cmp(r, b);
    
    if (cmp == 0 && r_cmp < 0) {
        std::cout << "PASS. Time: " << std::chrono::duration<double>(end-start).count() << "s" << std::endl;
    } else {
        std::cout << "FAIL." << std::endl;
        if (cmp != 0) std::cout << "  Reconstruction mismatch." << std::endl;
        if (r_cmp >= 0) std::cout << "  Remainder >= Divisor." << std::endl;
    }
    
    precn_free(a); precn_free(b); precn_free(q); precn_free(r); precn_free(check);
}

int main() {
    // Basic balanced tests
    check_div(100, 50);
    check_div(500, 250);
    check_div(1000, 500);

    // Unbalanced: Small Divisor (High Quotient)
    // O(M*N) where N is small -> O(M). Should be fast.
    check_div(2000, 10); 
    check_div(5000, 100);

    // Unbalanced: Large Divisor (Small Quotient)
    // O((M-N)*N) -> O(small * N) -> O(N). Should be fast.
    check_div(2000, 1500);
    check_div(5000, 4800);

    // Bigger Balanced Tests
    // O((M-N)*N) approx O(N^2). 
    // 5000/2500 words is about 5x heavier than 1000/500? No, 25x heavier.
    // 1000 words took 0.0004s. 5000 words should be ~0.01s - 0.1s.
    check_div(5000, 2500);
    
    // Very Big
    check_div(10000, 5000); // Expect ~0.4s - 1.0s
    check_div(20000, 10000); // Expect ~4s

    return 0;
}
