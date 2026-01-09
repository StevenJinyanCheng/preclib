#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include "prec.hpp"

using namespace precn_impl;

void print_digits(const int* d, size_t len) {
    if (len > 20) {
        for(size_t i=0; i<10; ++i) std::cout << d[i] << " ";
        std::cout << "... ";
        for(size_t i=len-10; i<len; ++i) std::cout << d[i] << " ";
    } else {
        for(size_t i=0; i<len; ++i) std::cout << d[i] << " ";
    }
    std::cout << std::endl;
}

bool test_base(precn_t original, int base, const char* name) {
    auto t1 = std::chrono::high_resolution_clock::now();
    size_t len = 0;
    int* digits = precn_to_base(original, &len, base);
    auto t2 = std::chrono::high_resolution_clock::now();
    
    std::cout << "Base " << base << " (" << name << "): Got " << len << " digits. "
              << "To: " << std::chrono::duration<double>(t2-t1).count() << "s ";

    // Basic validity check of digits (check random sample if too large?)
    for(size_t i=0; i<len; ++i) {
        if (digits[i] < 0 || digits[i] >= base) {
             std::cout << "\nFAIL: Digit out of range: " << digits[i] << std::endl;
             return false;
        }
    }
    
    precn_t recovered = precn_new(0);
    auto t3 = std::chrono::high_resolution_clock::now();
    precn_from_base(recovered, digits, len, base);
    auto t4 = std::chrono::high_resolution_clock::now();
    
    std::cout << "From: " << std::chrono::duration<double>(t4-t3).count() << "s ";
    
    int cmp = precn_cmp(original, recovered);
    
    if (cmp == 0) {
        std::cout << "[PASS]" << std::endl;
    } else {
        std::cout << "[FAIL]" << std::endl;
        std::cout << "Original size: " << original->rsiz << std::endl;
        std::cout << "Recovered size: " << recovered->rsiz << std::endl;
    }
    
    free(digits);
    precn_free(recovered);
    return cmp == 0;
}

int main() {
    std::cout << "Testing Generic Base Conversion..." << std::endl;
    
    precn_t num = precn_new(0);
    
    // 1. Small Number Test
    // 12345 in base 10
    precn_set(num, 12345);
    // Base 10: [1, 2, 3, 4, 5]
    size_t l;
    int* d = precn_to_base(num, &l, 10);
    std::cout << "12345 in base 10: ";
    print_digits(d, l);
    free(d);
    
    // Base 16: 0x3039 -> [3, 0, 3, 9]
    d = precn_to_base(num, &l, 16);
    std::cout << "12345 in base 16: ";
    print_digits(d, l);
    free(d);
    
    // 2. Large Number Test
    int num_digits = 250000;
    std::string big_s;
    big_s.reserve(num_digits);
    
    std::cout << "Generating " << num_digits << " decimal digits..." << std::endl;
    std::mt19937 rng(1234);
    for(int i=0; i<num_digits; ++i) {
        big_s += std::to_string(rng() % 10);
    }
    
    std::cout << "Parsing decimal string..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    precn_from_str(num, big_s.c_str());
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Parsed in " << std::chrono::duration<double>(end-start).count() << "s" << std::endl;
    
    std::cout << "Testing with ~" << big_s.length() << " decimal digits." << std::endl;
    
    test_base(num, 10, "Decimal");
    test_base(num, 16, "Hex");
    test_base(num, 2, "Binary");
    test_base(num, 60, "Base 60");
    test_base(num, 256, "Base 256");
    
    precn_free(num);
    
    return 0;
}
