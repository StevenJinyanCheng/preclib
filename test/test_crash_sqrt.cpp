#include <iostream>
#include <vector>
#include <chrono>
#include "../prec.hpp"

using namespace precn_impl;

int main(int argc, char** argv) {
    size_t size = 10000000; // Start with 10M words (approx 100M decimal digits)
    if (argc > 1) {
        size = std::stoull(argv[1]);
    }
    
    std::cout << "Allocating precn with " << size << " words (" << (size * 4 / 1024.0 / 1024.0) << " MB)..." << std::endl;
    
    precn_t a = precn_new(0);
    precn_realloc(a, size);
    a->rsiz = size;
    
    // Set some bits to make it non-zero and look like a typical large integer shifted
    a->n[size-1] = 0x80000000; // High bit set
    a->n[0] = 1;

    std::cout << "Allocation done. Calling precn_sqrt..." << std::endl;
    
    precn_t res = precn_new(0);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    int ret = precn_sqrt(a, res);
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    
    std::cout << "precn_sqrt returned " << ret << " in " << elapsed.count() << " seconds." << std::endl;
    std::cout << "Result size: " << res->rsiz << std::endl;

    precn_free(a);
    precn_free(res);
    
    return 0;
}
