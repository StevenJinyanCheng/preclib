#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <string.h>
#include "../prec.hpp"

using namespace precn_impl;

int main() {
    std::cout << "Testing Divide & Conquer Conversion (Hex/Dec)..." << std::endl;
    
    // Case 1: Convert String to Num
    const char* s_in = "1234567890123456789012345678901234567890";
    std::cout << "Input:  " << s_in << std::endl;
    
    precn_t a = precn_new(0);
    precn_from_str(a, s_in); 
    
    // Case 2: Convert Num to String
    char* s_out = precn_to_str(a);
    std::cout << "Output: " << s_out << std::endl;
    
    if (strcmp(s_in, s_out) == 0) {
        std::cout << "PASS: Round trip successful." << std::endl;
    } else {
        std::cout << "FAIL: Mismatch." << std::endl;
    }
    
    free(s_out);
    
    // Benchmark
    std::string big_s;
    for(int i=0; i<10000; ++i) big_s += "1234567890";
    // 100k digits
    
    auto start = std::chrono::high_resolution_clock::now();
    precn_from_str(a, big_s.c_str());
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Parse 100k digits: " << std::chrono::duration<double>(end-start).count() << "s" << std::endl;
    
    start = std::chrono::high_resolution_clock::now();
    s_out = precn_to_str(a);
    end = std::chrono::high_resolution_clock::now();
    std::cout << "Print 100k digits: " << std::chrono::duration<double>(end-start).count() << "s" << std::endl;
    
    if (big_s == s_out) {
        std::cout << "PASS: Large round trip." << std::endl;
    } else {
        std::cout << "FAIL: Large." << std::endl;
        std::cout << "Expected len: " << big_s.length() << ", Got: " << strlen(s_out) << std::endl;
    }

    precn_free(a); free(s_out);
    return 0;
}
