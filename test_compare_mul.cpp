#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <sstream>
#include "prec.hpp"

using namespace precn_impl;

// Helper to print hex
std::string to_hex(precn_t p) {
  if (p == NULL || p->rsiz == 0) return "0";
  std::ostringstream oss;
  oss << std::hex;
  for (int i = (int)p->rsiz - 1; i >= 0; --i) {
    if (i == (int)p->rsiz - 1)
      oss << p->n[i];
    else
      oss << std::setw(8) << std::setfill('0') << p->n[i];
  }
  return oss.str();
}

// Helper to create random precn_t
precn_t make_random(uint64_t size, std::mt19937_64& rng) {
    precn_t p = __precn_new_size(size, size);
    for(uint64_t i=0; i<size; ++i) {
        p->n[i] = (uint32_t)rng();
    }
    // Ensure high word is not zero to keep size stable for testing logic, 
    // though precn handles it fine.
    if (size > 0 && p->n[size-1] == 0) p->n[size-1] = 1;
    return p;
}

int main() {
    std::mt19937_64 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    int num_tests = 50;
    int failures = 0;

    std::cout << "Running comparison tests between mul_slow and mul_karatsuba..." << std::endl;

    // Test sizes around the cutoff (32) and larger
    std::vector<uint64_t> sizes = {10, 30, 32, 33, 64, 100, 128, 200, 256, 512, 1024, 2048, 4096, 8192};

    for (uint64_t s : sizes) {
        std::cout << "Testing size " << s << " words..." << std::endl;
        for (int i = 0; i < 10; ++i) {
            precn_t a = make_random(s, rng);
            precn_t b = make_random(s, rng);
            
            precn_t res_slow = precn_new(0);
            precn_t res_kara = precn_new(0);
            precn_t res_toom = precn_new(0);

            precn_mul_slow(a, b, res_slow);
            precn_mul_karatsuba(a, b, res_kara);
            precn_mul_toom_cook(a, b, res_toom);

            if (precn_cmp(res_slow, res_kara) != 0) {
                std::cerr << "MISMATCH KARA at size " << s << " test " << i << std::endl;
                std::cerr << "A: " << to_hex(a) << std::endl;
                std::cerr << "B: " << to_hex(b) << std::endl;
                std::cerr << "Slow: " << to_hex(res_slow) << std::endl;
                std::cerr << "Kara: " << to_hex(res_kara) << std::endl;
                failures++;
            }
            if (precn_cmp(res_slow, res_toom) != 0) {
                std::cerr << "MISMATCH TOOM at size " << s << " test " << i << std::endl;
                std::cerr << "A: " << to_hex(a) << std::endl;
                std::cerr << "B: " << to_hex(b) << std::endl;
                std::cerr << "Slow: " << to_hex(res_slow) << std::endl;
                std::cerr << "Toom: " << to_hex(res_toom) << std::endl;
                failures++;
            }

            precn_free(a);
            precn_free(b);
            precn_free(res_slow);
            precn_free(res_kara);
            precn_free(res_toom);
        }
    }

    // Test unbalanced sizes
    std::cout << "Testing unbalanced sizes..." << std::endl;
    for (int i = 0; i < 20; ++i) {
        uint64_t s1 = (rng() % 1000) + 1;
        uint64_t s2 = (rng() % 1000) + 1;
        
        precn_t a = make_random(s1, rng);
        precn_t b = make_random(s2, rng);
        
        precn_t res_slow = precn_new(0);
        precn_t res_kara = precn_new(0);
        precn_t res_toom = precn_new(0);

        precn_mul_slow(a, b, res_slow);
        precn_mul_karatsuba(a, b, res_kara);
        precn_mul_toom_cook(a, b, res_toom);

        if (precn_cmp(res_slow, res_kara) != 0) {
            std::cerr << "MISMATCH KARA at sizes " << s1 << "x" << s2 << std::endl;
            failures++;
        }
        if (precn_cmp(res_slow, res_toom) != 0) {
            std::cerr << "MISMATCH TOOM at sizes " << s1 << "x" << s2 << std::endl;
            failures++;
        }

        precn_free(a);
        precn_free(b);
        precn_free(res_slow);
        precn_free(res_kara);
        precn_free(res_toom);
    }

    if (failures == 0) {
        std::cout << "All comparison tests passed!" << std::endl;
    } else {
        std::cout << failures << " tests failed." << std::endl;
    }

    return failures;
}
