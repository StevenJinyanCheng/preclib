#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include "../prec_factor.hpp"

using namespace prec_factor;

void print_factors(const factor_result_t& factors) {
    std::cout << "Factors: ";
    for (const auto& f : factors) {
        std::cout << f.prime << "^" << f.count << " ";
    }
    std::cout << std::endl;
}

int main() {
    srand(time(0));
    std::cout << "Testing Factorization..." << std::endl;

    // Test 1: Small composite
    precz n1 = "123456789"; 
    std::cout << "Factoring " << n1 << std::endl;
    factor_result_t res1;
    factorex(n1, res1);
    print_factors(res1);

    // Test 2: Semi-prime (Pollard Rho target)
    // 101 * 103 = 10403
    precz n2 = "10403";
    std::cout << "Factoring " << n2 << std::endl;
    factor_result_t res2;
    factorex(n2, res2);
    print_factors(res2);
    
    // Test 2.5: Explicit ECM Test
    // 449 * 457 = 205193. 
    // Both primes are small enough for B1=2000.
    precz n_ecm = "205193"; 
    std::cout << "[Explicit ECM Test] Factoring " << n_ecm << " using ECM only..." << std::endl;
    precz f_ecm = lenstra_ecm(n_ecm, 2000);
    std::cout << "ECM returned factor: " << f_ecm << std::endl;
    if (f_ecm > 1 && f_ecm < n_ecm) {
        std::cout << "ECM Success!" << std::endl;
    } else {
        std::cout << "ECM Failed (returned " << f_ecm << ")" << std::endl;
    }

    // Test 3: Larger semi-prime
    // 65537 * 65539 (65539 = 65539 is prime? No. 65537 is Fermat prime F4. 65539=something)
    // Let's pick known primes.
    // p = 1000003, q = 1000033
    precz p = "1000003";
    precz q = "1000033";
    precz n3 = p * q;
    std::cout << "Factoring " << n3 << " (" << p << " * " << q << ")" << std::endl;
    if (is_prime(n3)) std::cout << "ERROR: is_prime says composite is prime!" << std::endl;
    else std::cout << "is_prime correctly identifies composite." << std::endl;
    
    // Test 5: Explicit QS Test
    // 5959 = 59 * 101
    precz n_qs = "5959"; 
    std::cout << "[Explicit QS Test] Factoring " << n_qs << " using QS..." << std::endl;
    precz f_qs = qs_factor(n_qs);
    std::cout << "QS returned factor: " << f_qs << std::endl;
    if (f_qs > 1 && f_qs < n_qs) {
        std::cout << "QS Success!" << std::endl;
    } else {
        std::cout << "QS Failed (returned " << f_qs << ")" << std::endl;
    }
    
    factor_result_t res3;
    factorex(n3, res3);
    print_factors(res3);

    // Test 4: Even larger for ECM (hopefully)
    // p = 1234577 (Prime?), q = 7654321(??)
    // Let's generate a number with small factors that ECM can find.
    // But factorex puts Pollard Rho first generally?
    // Let's force ECM by calling it directly if Rho fails? 
    // factorex calls Rho then ECM.
    
    // Test 5: A number that trial division handles
    precz n5 = "3628800"; // 10!
    std::cout << "Factoring " << n5 << " (10!)" << std::endl;
    factor_result_t res5;
    factorex(n5, res5);
    print_factors(res5);
    
    // Test 6: 2^128 - 1
    std::cout << "Factoring 2^128 - 1" << std::endl;
    precz n6 = 1;
    n6 = (n6 << 128) - 1; 
    std::cout << "n = " << n6 << std::endl;
    factor_result_t res6;
    factorex(n6, res6);
    print_factors(res6);

    return 0;
}
