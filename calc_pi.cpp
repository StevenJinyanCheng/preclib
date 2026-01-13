#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <cmath>
#include "prec.hpp"

using namespace precn_impl;

// wrapper for signed arithmetic
struct SInt {
    precn_t m;
    int sign; // 1 or -1

    SInt() {
        m = precn_new(0);
        sign = 1;
    }
    
    // Create from uint64 with sign
    static SInt from_u64_s(uint64_t val, int s) {
        SInt x;
        // x.m is already allocated 0
        precn_set(x.m, val);
        x.sign = (val == 0) ? 1 : s;
        return x;
    }

    void free_s() {
        precn_free(m);
        m = NULL;
    }
};

void s_mul(SInt& res, const SInt& a, const SInt& b) {
    res.sign = a.sign * b.sign;
    precn_mul_auto(a.m, b.m, res.m);
    if (res.m->rsiz == 0) res.sign = 1;
}

void s_add(SInt& res, const SInt& a, const SInt& b) {
    if (a.sign == b.sign) {
        res.sign = a.sign;
        precn_add(a.m, b.m, res.m);
    } else {
        // Different signs
        int cmp = precn_cmp(a.m, b.m);
        if (cmp > 0) {
            res.sign = a.sign;
            precn_sub(a.m, b.m, res.m);
        } else if (cmp < 0) {
            res.sign = b.sign;
            precn_sub(b.m, a.m, res.m);
        } else {
            precn_set(res.m, 0);
            res.sign = 1;
        }
    }
}

// Chudnovsky Constants
const uint64_t C3_24 = 10939058860032000ULL;
const uint64_t TERM_A = 13591409;
const uint64_t TERM_B = 545140134;

struct BsRes {
    SInt P, Q, T;
    
    // Default constructor calls SInt() which allocates.
    // No need for init().
    
    void clear() { P.free_s(); Q.free_s(); T.free_s(); }
};

// Binary Splitting Recursion
// Computes terms for range [a, b)
// a inclusive, b exclusive
void bs(uint64_t a, uint64_t b, BsRes& res) {
    if (b - a == 1) {
        // Leaf 'a'
        if (a == 0) {
            // P = 1, Q = 1, T = A
            precn_set(res.P.m, 1); res.P.sign = 1;
            precn_set(res.Q.m, 1); res.Q.sign = 1;
            precn_set(res.T.m, TERM_A); res.T.sign = 1;
        } else {
            // P = -(6a-5)(2a-1)(6a-1)
            // Q = C3_24 * a^3
            // T = P * (A + B*a)
            
            // Calc P mag
            precn_t t1 = precn_new(6*a - 5);
            precn_t t2 = precn_new(2*a - 1);
            precn_t t3 = precn_new(6*a - 1);
            precn_t tmp = precn_new(0);
            
            precn_mul_auto(t1, t2, tmp);
            precn_mul_auto(tmp, t3, res.P.m);
            res.P.sign = -1; // always negative factor for k>0
            
            // Calc Q mag
            precn_set(t1, a);
            precn_set(t2, a);
            precn_mul_auto(t1, t2, tmp); // a^2
            precn_mul_auto(tmp, t1, t2); // a^3
            precn_t c = precn_new(C3_24); // Constant doesn't fit in 32 bit, but precn_set handles u64. 
            // Wait, C3_24 is ~10^16. Fits in u64.
            // precn_set<uint64_t> works.
            precn_set(c, C3_24);
            precn_mul_auto(t2, c, res.Q.m);
            res.Q.sign = 1;
            
            // Calc G = A + B*a
            precn_set(t1, TERM_B);
            precn_t ta = precn_new(a);
            precn_mul_auto(t1, ta, tmp); // B*a
            precn_t va = precn_new(TERM_A);
            precn_add(tmp, va, t2); // A + B*a
            
            // T = P * G
            // P is negative. G positive. T negative.
            // s_mul handles signs.
            SInt G; G.m = t2; G.sign = 1;
            // G.m points to t2. s_mul uses G.m.
            s_mul(res.T, res.P, G);
            
            // Cleanup temps (t2 is used in G.m, G is stack but G.m is just pointer t2. 
            // G destructor is NOT called (SInt has no destructor, just free_s method).
            // So we must free t2 manually.
            
            precn_free(t1); precn_free(t3); precn_free(tmp);
            precn_free(c); precn_free(ta); precn_free(va);
            precn_free(t2); // G.m
            G.m = NULL; // Prevent mis-use if we added destructor later
        }
    } else {
        uint64_t m = (a + b) / 2;
        
        BsRes left, right;
        // left, right constructors allocate P,Q,T
        
        bs(a, m, left);
        bs(m, b, right);
        
        // P = P_l * P_r
        s_mul(res.P, left.P, right.P);
        
        // Q = Q_l * Q_r
        s_mul(res.Q, left.Q, right.Q);
        
        // T = Q_r * T_l + P_l * T_r
        SInt t_left_part, t_right_part;
        // Constructors allocate
        
        s_mul(t_left_part, right.Q, left.T);
        s_mul(t_right_part, left.P, right.T);
        
        s_add(res.T, t_left_part, t_right_part);
        
        t_left_part.free_s();
        t_right_part.free_s();
        left.clear();
        right.clear();
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: calc_pi <digits> [file_path]" << std::endl;
        return 1;
    }
    
    int digits = std::atoi(argv[1]);
    std::string out_file_path = "";
    if (argc >= 3) {
        out_file_path = argv[2];
    }
    if (digits < 0) digits = 100;
    
    // N terms
    // 14.18 digits per term
    uint64_t terms = (uint64_t)(digits / 14.1812) + 2;
    
    std::cout << "Calculating Pi to " << digits << " digits (" << terms << " terms)..." << std::endl;
    
    auto t1 = std::chrono::high_resolution_clock::now();
    
    BsRes root;
    bs(0, terms, root);
    
    auto t2 = std::chrono::high_resolution_clock::now();
    std::cout << "BS Phase: " << std::chrono::duration<double>(t2-t1).count() << "s" << std::endl;
    
    // Result S = T / Q
    // Pi = (426880 * sqrt(10005) * Q) / T
    
    // We need fixed point Result * 10^digits.
    // Calc: Num = 426880 * sqrt(10005) * 10^D * Q
    // Denom = T
    
    // 1. Sqrt(10005) * 10^D = Sqrt(10005 * 10^2D)
    precn_t big_10005 = precn_new(10005);
    precn_t sqrt_res = precn_new(0);
    
    // Scale 10005 by 10^2D
    // We construct power string "100...0"
    // Need even number of zeros for integer square root to be exact power of 10 scaled.
    // 2*digits + 10 zeros.
    std::string zeros(2 * digits + 11, '0');
    zeros[0] = '1';
    precn_t scale = precn_new(0);
    precn_from_str(scale, zeros.c_str());
    
    precn_t val_to_sqrt = precn_new(0);
    precn_mul_auto(big_10005, scale, val_to_sqrt);
    
    auto t3 = std::chrono::high_resolution_clock::now();
    precn_sqrt(val_to_sqrt, sqrt_res);
    auto t4 = std::chrono::high_resolution_clock::now();
    std::cout << "Sqrt Phase: " << std::chrono::duration<double>(t4-t3).count() << "s" << std::endl;
    
    // Num = 426880 * sqrt_res * Q
    precn_t k_const = precn_new(426880);
    precn_t num = precn_new(0);
    precn_mul_auto(k_const, sqrt_res, num);
    
    precn_t total_num = precn_new(0);
    precn_mul_auto(num, root.Q.m, total_num);
    
    // Divide by T (T is sum, T.m is magnitude)
    // Pi positive, T should be positive?
    // Check sign of T.
    // If T negative, we take magnitude?
    // Formula gives positive Pi.
    // Let's check root T's sign.
    
    // If T < 0, implies Sum terms net neg?
    // A = 1359... (positive).
    // Terms alternate.
    // Sum is positive.
    
    precn_t pi = precn_new(0);
    auto t5 = std::chrono::high_resolution_clock::now();
    precn_div(total_num, root.T.m, pi, NULL);
    auto t6 = std::chrono::high_resolution_clock::now();
    std::cout << "Div Phase: " << std::chrono::duration<double>(t6-t5).count() << "s" << std::endl;
    
    // Print first 50 digits + last 20
    char* s = precn_to_str(pi);
    std::string res_s(s);
    free(s);
    
    std::cout << "Result length: " << res_s.length() << std::endl;
    if (res_s.length() > 50) {
        std::cout << res_s.substr(0, 1) << "." << res_s.substr(1, 50) << "..." << std::endl;
        std::cout << "Last 10: " << res_s.substr(res_s.length()-10) << std::endl;
    } else {
        std::cout << res_s << std::endl;
    }

    if (!out_file_path.empty()) {
        std::ofstream ofs(out_file_path);
        if (ofs) {
            if (res_s.length() > 0) {
                 ofs << res_s[0] << "." << res_s.substr(1);
            }
        }
    }
    
    precn_free(big_10005); precn_free(sqrt_res); precn_free(scale);
    precn_free(val_to_sqrt); precn_free(k_const); precn_free(num);
    precn_free(total_num); precn_free(pi);
    root.clear();
    
    return 0;
}
