#include <iostream>
#include <vector>
#include <complex>
#include <cmath>
#include <cassert>
#include "../prec.hpp"

using namespace precn_impl;

bool is_close(double a, double b, double tol = 1e-9) {
    return std::abs(a - b) < tol;
}

void test_fft() {
    std::cout << "Testing FFT..." << std::endl;
    
    // Test case: (1 + x)(2 + x) = 2 + 3x + x^2
    // Coeffs: [1, 1] * [2, 1] -> [2, 3, 1]
    
    std::vector<fft::Complex> fa(4), fb(4);
    fa[0] = 1; fa[1] = 1;
    fb[0] = 2; fb[1] = 1;
    
    fft::fft(fa, false);
    fft::fft(fb, false);
    
    std::vector<fft::Complex> fc(4);
    for(int i=0; i<4; ++i) {
        fc[i] = fa[i] * fb[i];
    }
    
    fft::fft(fc, true);
    
    // Expected: 2, 3, 1, 0
    assert(is_close(fc[0].real(), 2.0));
    assert(is_close(fc[1].real(), 3.0));
    assert(is_close(fc[2].real(), 1.0));
    assert(is_close(fc[3].real(), 0.0));
    
    std::cout << " [PASS] Polynomial multiplication (small)" << std::endl;
}

void test_ntt() {
    std::cout << "Testing NTT..." << std::endl;
    
    // Test case: (1 + x)(2 + x) = 2 + 3x + x^2
    // Coeffs: [1, 1] * [2, 1] -> [2, 3, 1]
    // Modulo 998244353
    
    std::vector<long long> na(4, 0), nb(4, 0);
    na[0] = 1; na[1] = 1;
    nb[0] = 2; nb[1] = 1;
    
    ntt::ntt(na, false);
    ntt::ntt(nb, false);
    
    std::vector<long long> nc(4);
    for(int i=0; i<4; ++i) {
        nc[i] = (na[i] * nb[i]) % ntt::mod;
    }
    
    ntt::ntt(nc, true);
    
    // Expected: 2, 3, 1, 0
    assert(nc[0] == 2);
    assert(nc[1] == 3);
    assert(nc[2] == 1);
    assert(nc[3] == 0);
    
    std::cout << " [PASS] Polynomial multiplication (small)" << std::endl;

    // Test case: (1 + 2x + 3x^2 + 4x^3) * (1 + 2x + 3x^2 + 4x^3)
    // Size 8
    // 1, 4, 10, 20, 25, 24, 16
    std::vector<long long> a = {1, 2, 3, 4, 0, 0, 0, 0};
    ntt::ntt(a, false);
    std::vector<long long> c(8);
    for(int i=0; i<8; ++i) c[i] = (a[i] * a[i]) % ntt::mod;
    ntt::ntt(c, true);

    assert(c[0] == 1);
    assert(c[1] == 4);
    assert(c[2] == 10);
    assert(c[3] == 20);
    assert(c[4] == 25);
    assert(c[5] == 24);
    assert(c[6] == 16);
    assert(c[7] == 0);
    std::cout << " [PASS] Polynomial squaring (size 8)" << std::endl;
}

int main() {
    test_fft();
    test_ntt();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
