#ifndef PREC_UTIL_HPP
#define PREC_UTIL_HPP
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
    bool precn_miller_rabin(precn_t n, uint64_t witness) {
        
    }
};
#endif /* PREC_UTIL_HPP */