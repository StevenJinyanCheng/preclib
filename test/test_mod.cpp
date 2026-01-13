#include <iostream>
#include <vector>
#include <random>
#include <chrono>
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
    if (n_words > 0 && p->n[n_words-1] == 0) p->n[n_words-1] = 1;
    return p;
}

int main() {
    std::cout << "Testing Modulo..." << std::endl;
    
    // Case 1: Simple
    precn_t a = precn_new(100);
    precn_t b = precn_new(30);
    precn_t res = precn_new(0);
    
    precn_mod(a, b, res);
    // 100 % 30 = 10
    if (res->rsiz == 1 && res->n[0] == 10) {
        std::cout << "  100 % 30 = 10 [PASS]" << std::endl;
    } else {
        std::cout << "  100 % 30 = " << (res->rsiz ? res->n[0] : 0) << " [FAIL]" << std::endl;
    }
    precn_free(a); precn_free(b); precn_free(res);

    // Case 2: Large Random
    a = create_random_precn(1000);
    b = create_random_precn(500);
    res = precn_new(0);
    
    // Compute a % b 
    // And also div to verify: a = q*b + r
    precn_t q = precn_new(0);
    precn_t r = precn_new(0);
    
    precn_mod(a, b, res);
    precn_div(a, b, q, r);
    
    if (precn_cmp(res, r) == 0 && precn_cmp(res, b) < 0) {
        std::cout << "  Large Random 1000 % 500 [PASS]" << std::endl;
    } else {
        std::cout << "  Large Random 1000 % 500 [FAIL]" << std::endl;
    }

    precn_free(a); precn_free(b); precn_free(res); precn_free(q); precn_free(r);
    
    return 0;
}
