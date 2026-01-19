#include <iostream>
#include <vector>
#include <random>
#include <cassert>
#include "../prec.hpp"

using namespace precn_impl;

// Helper to compute a % mod
uint64_t precn_mod_u32(precn_t a, uint32_t mod) {
    if (a == NULL || a->rsiz == 0) return 0;
    
    // Manual modulo loop because precn_div_u32 requires writing quotient
    uint64_t rem = 0;
    for (int64_t i = (int64_t)a->rsiz - 1; i >= 0; --i) {
        uint64_t cur = a->n[i] + (rem << 32);
        rem = cur % mod;
    }
    return rem;
}

int main() {
    std::cout << "Starting Large NTT Karatsuba Test..." << std::endl;

    // Size enough to trigger limits
    // Threshold is 4194304. We want as + bs > 4194304.
    // Let's use 2.2M words each. Total 4.4M.
    size_t size = 2200000; 
    
    std::cout << "Allocating vectors of size " << size << "..." << std::endl;
    precn_t a = precn_new(0);
    precn_t b = precn_new(0);
    precn_realloc(a, size);
    precn_realloc(b, size);
    a->rsiz = size;
    b->rsiz = size;

    std::mt19937 rng(12345);
    std::uniform_int_distribution<uint32_t> dist;

    // Fill with random data
    // Use simple pattern for speed filling? 
    // Random is better to avoid sparse optimizations if any (though prec doesn't seem to have valid sparse opts for arrays).
    std::cout << "Filling data..." << std::endl;
    for(size_t i=0; i<size; ++i) {
        a->n[i] = dist(rng);
        b->n[i] = dist(rng);
    }
    // Ensure non-zero MSB
    a->n[size-1] |= 1;
    b->n[size-1] |= 1;

    // Primes for verification
    uint32_t mods[] = {1000000007, 1000000009};
    uint64_t a_mods[2];
    uint64_t b_mods[2];
    
    std::cout << "Computing inputs modulo..." << std::endl;
    for(int k=0; k<2; ++k) {
        a_mods[k] = precn_mod_u32(a, mods[k]);
        b_mods[k] = precn_mod_u32(b, mods[k]);
    }

    precn_t res = precn_new(0);

    std::cout << "Running multiplication (this triggers recursive Karatsuba)..." << std::endl;
    int ret = precn_mul(a, b, res);
    
    if (ret != 0) {
        std::cerr << "Multiplication failed with error code: " << ret << std::endl;
        return 1;
    }
    
    std::cout << "Computation done. Result size: " << res->rsiz << std::endl;
    
    // Verify
    std::cout << "Verifying result..." << std::endl;
    bool ok = true;
    for(int k=0; k<2; ++k) {
        uint64_t expected = (a_mods[k] * b_mods[k]) % mods[k];
        uint64_t actual = precn_mod_u32(res, mods[k]);
        
        if (expected != actual) {
            std::cerr << "Mismatch for mod " << mods[k] << ": expected " << expected << ", got " << actual << std::endl;
            ok = false;
        } else {
             std::cout << "Mod " << mods[k] << " passed." << std::endl;
        }
    }

    precn_free(a);
    precn_free(b);
    precn_free(res);

    if (ok) {
        std::cout << "SUCCESS! Large multiplication Verified." << std::endl;
        return 0;
    } else {
        std::cout << "FAILURE!" << std::endl;
        return 1;
    }
}
