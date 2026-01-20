#include <iostream>
#include <vector>
#include <cmath>
#include "../prec_fft.hpp"

using namespace precn_impl::quick_fft;

bool is_close(double a, double b, double tol = 1e-9) {
    return std::abs(a - b) < tol;
}

int main() {
    std::cout << "Testing prec_fft.hpp implementation..." << std::endl;

    // Test Case: FFT of [1, 2, 3, 4]
    // Expected DFT:
    // k=0: 1+2+3+4 = 10
    // k=1: 1 - 2i - 3 + 4i = -2 + 2i  (Wait, w^1 = -i)
    // k=2: 1 - 2 + 3 - 4 = -2
    // k=3: 1 + 2i - 3 - 4i = -2 - 2i

    size_t n = 4;
    std::vector<uint32_t> in = {1, 2, 3, 4};
    std::vector<double> re(n), im(n);

    // 1. Test CVT
    cvt(in.data(), n, re.data(), im.data());
    
    // 2. Test FFT
    fft(re.data(), im.data(), n, false);

    std::cout << "FFT Results:" << std::endl;
    for(size_t i=0; i<n; ++i) {
        std::cout << "  [" << i << "] " << re[i] << " + " << im[i] << "i" << std::endl;
    }

    if (is_close(re[0], 10.0) && is_close(im[0], 0.0) &&
        is_close(re[1], -2.0) && is_close(im[1], 2.0) &&
        is_close(re[2], -2.0) && is_close(im[2], 0.0) &&
        is_close(re[3], -2.0) && is_close(im[3], -2.0)) {
        std::cout << "[PASS] Forward FFT" << std::endl;
    } else {
        std::cout << "[FAIL] Forward FFT" << std::endl;
    }

    // 3. Test Inverse FFT
    fft(re.data(), im.data(), n, true);
    
    std::cout << "Inverse FFT Results:" << std::endl;
    for(size_t i=0; i<n; ++i) {
         if (std::abs(im[i]) < 1e-10) im[i] = 0.0; // cleanup -0.0
         std::cout << "  [" << i << "] " << re[i] << " + " << im[i] << "i" << std::endl;
    }

    // 4. Test CVT Back
    std::vector<uint32_t> out(n);
    cvt_back(out.data(), n, re.data(), im.data());

    bool ok = true;
    for(size_t i=0; i<n; ++i) {
        if (out[i] != in[i]) ok = false;
    }
    
    if (ok) std::cout << "[PASS] Round-trip (CVT -> FFT -> IFFT -> CVT_BACK)" << std::endl;
    else std::cout << "[FAIL] Round-trip" << std::endl;

    return 0;
}
