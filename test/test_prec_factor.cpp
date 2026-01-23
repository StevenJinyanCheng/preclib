#include "../prec.hpp"
#include "../prec_factor.hpp"
#include <iostream>
#include <vector>
#include <string>

void print_factors(const prec_factor::factor_result_t& factors) {
    for (size_t i = 0; i < factors.size(); ++i) {
        std::cout << factors[i].prime;
        if (factors[i].count > 1) std::cout << "^" << factors[i].count;
        if (i < factors.size() - 1) std::cout << " * ";
    }
    std::cout << std::endl;
}

void check_factorization(const std::string& s_num) {
    precz n(s_num.c_str());
    std::cout << "Factoring " << n << "..." << std::endl;
    
    prec_factor::factor_result_t res;
    prec_factor::factorex(n, res);
    
    std::cout << "Factors: ";
    print_factors(res);
    
    precz prod(1);
    for(const auto& f : res) {
        precz p = f.prime;
        for(int k=0; k<f.count; ++k) prod = prod * p;
    }
    
    if (prod == n) {
        std::cout << "Verification SUCCESS." << std::endl;
    } else {
        std::cout << "Verification FAILED: Product is " << prod << std::endl;
    }
    std::cout << "----------------------------------------" << std::endl;
}

int main() {
    // Small number
    check_factorization("120"); // 2^3 * 3 * 5
    
    // Medium number (Pollard Rho range)
    check_factorization("10403"); // 101 * 103
    
    // Larger number 
    check_factorization("18446744073709551617"); // 2^64 + 1 ? No, 2^64-1 is 18446744073709551615
    // 18446744073709551617 is prime? Let's see. 
    // Actually 2^64+1 is composite? 2^64+1 = (2^32-2^16+1)(2^32+2^16+1)? No, those are for 2^32+1.
    // F4 = 2^16+1 = 65537 (prime). F5=2^32+1 (composite). F6=2^64+1 (composite).
    // F6 = 274177 * 67280421310721
    
    // A known semi-prime
    // 314159265358979323 = 317 * ... ?
    check_factorization("314159265358979323");

    // Try a square
    check_factorization("121");

    // Try a prime
    check_factorization("1000000007");

    return 0;
}
