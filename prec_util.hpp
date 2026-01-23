#ifndef PREC_UTIL_HPP
#define PREC_UTIL_HPP
#include <random>
#include "prec.hpp"
namespace precn_impl {
    precn_t precn_qpow(precn_t base, uint64_t exp) {
        precn_t result = precn_new(1);
        precn_t b = precn_new(0);
        precn_set(b, base);
        while (exp > 0) {
            if (exp & 1) {
                precn_t temp = precn_new(0);
                precn_mul_auto(result, b, temp);
                precn_free(result);
                result = temp;
            }
            precn_t temp = precn_new(0);
            precn_mul_auto(b, b, temp);
            precn_free(b);
            b = temp;
            exp >>= 1;
        }
        precn_free(b);
        return result;
    }
    // Modular Exponentiation: res = base^exp % mod
    // Helper function for Milller-Rabin
    inline void precn_pow_mod(precn_t base, precn_t exp, precn_t mod, precn_t res) {
        precn_set(res, 1);
        if (exp->rsiz == 0) return; // exp=0 -> 1

        precn_t b = precn_new(0);
        precn_t q = precn_new(0); // Dummy quotient
        precn_div(base, mod, q, b); // b = base % mod

        precn_t e_curr = precn_new(0);
        precn_set(e_curr, exp);

        precn_t temp_mul = precn_new(0);
        
        while (e_curr->rsiz > 0 && !(e_curr->rsiz == 1 && e_curr->n[0] == 0)) {
            if (e_curr->n[0] & 1) {
                precn_mul_auto(res, b, temp_mul);
                precn_div(temp_mul, mod, q, res);
            }
            precn_shift_right(e_curr, e_curr, 1);
            
            if (e_curr->rsiz == 0 || (e_curr->rsiz == 1 && e_curr->n[0] == 0)) break; // optimize last square

            precn_mul_auto(b, b, temp_mul);
            precn_div(temp_mul, mod, q, b);
        }

        precn_free(b); precn_free(q); precn_free(e_curr); precn_free(temp_mul);
    }

    // Miller-Rabin Primality Test (Single Witness)
    // Checks if n is a strong pseudoprime to base 'witness'.
    // Returns true if n passes the test (probably prime), false if composite.
    inline bool precn_miller_rabin(precn_t n, uint64_t witness) {
        // Handle small n < 4
        if (n->rsiz == 0) return false;
        if (n->rsiz == 1) {
            if (n->n[0] < 2) return false;
            if (n->n[0] == 2 || n->n[0] == 3) return true;
        }
        if ((n->n[0] & 1) == 0) return false; // Even > 2

        // Write n-1 = 2^s * d
        precn_t nm1 = precn_new(0);
        precn_t one = precn_new(1);
        precn_sub(n, one, nm1);

        uint64_t s = 0;
        // Count trailing zeros of nm1
        for (size_t i = 0; i < nm1->rsiz; ++i) {
            if (nm1->n[i] == 0) {
                s += 32;
            } else {
                uint32_t val = nm1->n[i];
                while ((val & 1) == 0) {
                    val >>= 1;
                    s++;
                }
                break;
            }
        }
        
        precn_t d = precn_new(0); 
        precn_shift_right(d, nm1, s); // d = (n-1) >> s


        // Set base a = witness
        precn_t a = precn_new(0); 
        precn_set(a, witness);

        precn_t x = precn_new(0);
        precn_t q = precn_new(0);
        precn_t temp = precn_new(0);

        bool is_probably_prime = false;

        // a = a % n
        precn_div(a, n, q, a);

        if (a->rsiz == 0) {
            // Witness is 0 mod n. 
            is_probably_prime = false;
        } else {
            // x = a^d % n
            precn_pow_mod(a, d, n, x);

            // if x == 1 or x == n-1
            int cmp_nm1 = precn_cmp(x, nm1);
            if ((x->rsiz == 1 && x->n[0] == 1) || cmp_nm1 == 0) {
                is_probably_prime = true;
            } else {
                for (uint64_t r = 1; r < s; ++r) {
                    // x = x^2 % n
                    precn_mul_auto(x, x, temp);
                    precn_div(temp, n, q, x);
                    
                    if (x->rsiz == 1 && x->n[0] == 1) {
                        // Found non-trivial sqrt(1). Composite.
                        is_probably_prime = false;
                        break;
                    }
                    if (precn_cmp(x, nm1) == 0) {
                        is_probably_prime = true;
                        break;
                    }
                }
            }
        }

        precn_free(nm1); precn_free(one); precn_free(d);
        precn_free(a); precn_free(x); precn_free(q); precn_free(temp);

        return is_probably_prime;
    }
};
#endif /* PREC_UTIL_HPP */