#include <iostream>

int main() {
    std::cout << "Checking SIMD support..." << std::endl;

#ifdef __SSE__
    std::cout << "SSE is enabled" << std::endl;
#else
    std::cout << "SSE is NOT enabled" << std::endl;
#endif

#ifdef __SSE2__
    std::cout << "SSE2 is enabled" << std::endl;
#else
    std::cout << "SSE2 is NOT enabled" << std::endl;
#endif

#ifdef __SSE3__
    std::cout << "SSE3 is enabled" << std::endl;
#else
    std::cout << "SSE3 is NOT enabled" << std::endl;
#endif

#ifdef __SSSE3__
    std::cout << "SSSE3 is enabled" << std::endl;
#else
    std::cout << "SSSE3 is NOT enabled" << std::endl;
#endif

#ifdef __SSE4_1__
    std::cout << "SSE4.1 is enabled" << std::endl;
#else
    std::cout << "SSE4.1 is NOT enabled" << std::endl;
#endif

#ifdef __SSE4_2__
    std::cout << "SSE4.2 is enabled" << std::endl;
#else
    std::cout << "SSE4.2 is NOT enabled" << std::endl;
#endif

#ifdef __AVX__
    std::cout << "AVX is enabled" << std::endl;
#else
    std::cout << "AVX is NOT enabled" << std::endl;
#endif

#ifdef __AVX2__
    std::cout << "AVX2 is enabled" << std::endl;
#else
    std::cout << "AVX2 is NOT enabled" << std::endl;
#endif

#ifdef __AVX512F__
    std::cout << "AVX-512 Foundation (F) is enabled" << std::endl;
#else
    std::cout << "AVX-512 Foundation (F) is NOT enabled" << std::endl;
#endif

#ifdef __AVX512BW__
    std::cout << "AVX-512 Byte/Word (BW) is enabled" << std::endl;
#else
    std::cout << "AVX-512 Byte/Word (BW) is NOT enabled" << std::endl;
#endif

#ifdef __AVX512CD__
    std::cout << "AVX-512 Conflict Detection (CD) is enabled" << std::endl;
#else
    std::cout << "AVX-512 Conflict Detection (CD) is NOT enabled" << std::endl;
#endif

#ifdef __AVX512DQ__
    std::cout << "AVX-512 Doubleword/Quadword (DQ) is enabled" << std::endl;
#else
    std::cout << "AVX-512 Doubleword/Quadword (DQ) is NOT enabled" << std::endl;
#endif

#ifdef __AVX512VL__
    std::cout << "AVX-512 Vector Length (VL) is enabled" << std::endl;
#else
    std::cout << "AVX-512 Vector Length (VL) is NOT enabled" << std::endl;
#endif

    return 0;
}
