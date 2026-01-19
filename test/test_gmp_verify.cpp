#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <iomanip>
#include <sstream>
#include "../prec.hpp"

using namespace precn_impl;

// Helper to write precn_t to file in hex
void write_hex(const std::string& filename, precn_t p) {
    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
        std::cerr << "Failed to open " << filename << std::endl;
        exit(1);
    }
    
    if (p == NULL || p->rsiz == 0) {
        outfile << "0";
        return;
    }

    outfile << std::hex;
    for (int64_t i = (int64_t)p->rsiz - 1; i >= 0; --i) {
        if (i == (int64_t)p->rsiz - 1)
             outfile << p->n[i];
        else
             outfile << std::setw(8) << std::setfill('0') << p->n[i];
    }
    outfile.close();
}

int main() {
    std::cout << "Generating data for GMP verification..." << std::endl;

    // Size to trigger recursive Karatsuba (> 4194304 total input)
    // 2.2M words each = 4.4M total words
    size_t size = 2200000; 

    precn_t a = precn_new(0);
    precn_t b = precn_new(0);
    precn_realloc(a, size);
    precn_realloc(b, size);
    a->rsiz = size;
    b->rsiz = size;

    std::mt19937 rng(42);
    std::uniform_int_distribution<uint32_t> dist;

    std::cout << "Filling vectors..." << std::endl;
    for(size_t i=0; i<size; ++i) {
        a->n[i] = dist(rng);
        b->n[i] = dist(rng);
    }
    // Ensure logical size is correct by setting MSB non-zero if it happened to be 0
    if (a->n[size-1] == 0) a->n[size-1] = 1;
    if (b->n[size-1] == 0) b->n[size-1] = 1;

    precn_t res = precn_new(0);

    std::cout << "Multiplying..." << std::endl;
    int ret = precn_mul(a, b, res);
    if(ret != 0) {
        std::cerr << "Multiplication failed: " << ret << std::endl;
        return 1;
    }

    std::cout << "Writing A.txt..." << std::endl;
    write_hex("test/A.txt", a);
    std::cout << "Writing B.txt..." << std::endl;
    write_hex("test/B.txt", b);
    std::cout << "Writing Res.txt..." << std::endl;
    write_hex("test/Res.txt", res);

    precn_free(a);
    precn_free(b);
    precn_free(res);
    
    std::cout << "Done. Validating with Python script next." << std::endl;
    return 0;
}
