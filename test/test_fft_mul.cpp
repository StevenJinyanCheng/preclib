#include <iostream>
#include <vector>
#include <random>
#include "../prec.hpp"

// Force use of FFT Mul
#define PRECN_USE_FFT_MUL

using namespace precn_impl;

int main() {
    std::cout << "Testing Large Multiplication with AVX2 FFT..." << std::endl;

    // Create two large numbers
    // 5000 words * 32 bits = 160000 bits.
    // Result size ~ 10000 words.
    // This is large enough to be interesting for FFT vs Karatsuba.
    size_t size = 5000; 
    
    precn_t a = precn_new(0);
    precn_t b = precn_new(0);
    precn_realloc(a, size);
    precn_realloc(b, size);
    a->rsiz = size;
    b->rsiz = size;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(0, 0xFFFFFFFF);

    for(size_t i=0; i<size; ++i) a->n[i] = dis(gen);
    for(size_t i=0; i<size; ++i) b->n[i] = dis(gen);

    // Make sure MSB != 0
    a->n[size-1] |= 1;
    b->n[size-1] |= 1;

    precn_t res_fft = precn_new(0);
    
    std::cout << "Running FFT Multiplication..." << std::endl;
    // Direct call to verify the new implementation
    precn_mul_fft_complex(a, b, res_fft);

    std::cout << "Running Karatsuba (Reference)..." << std::endl;
    precn_t res_kara = precn_new(0);
    precn_mul_karatsuba(a, b, res_kara);

    // Compare
    bool ok = true;
    if (res_fft->rsiz != res_kara->rsiz) {
        std::cout << "Size Mismatch! FFT=" << res_fft->rsiz << ", Kara=" << res_kara->rsiz << std::endl;
        ok = false;
    } else {
        for(size_t i=0; i<res_fft->rsiz; ++i) {
            if (res_fft->n[i] != res_kara->n[i]) {
                std::cout << "Mismatch at index " << i << ": FFT=" << res_fft->n[i] << ", Kara=" << res_kara->n[i] << std::endl;
                ok = false;
                break;
            }
        }
    }

    if(ok) std::cout << "[PASS] FFT Multiplication matches Karatsuba." << std::endl;
    else std::cout << "[FAIL] FFT Multiplication incorrect." << std::endl;

    precn_free(a);
    precn_free(b);
    precn_free(res_fft);
    precn_free(res_kara);

    return 0;
}
