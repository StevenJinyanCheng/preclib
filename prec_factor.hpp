// Just for fun.
#ifndef PREC_FACTOR_HPP
#define PREC_FACTOR_HPP
#include "prec_util.hpp"
#include <vector>
#include <iostream>
#include <algorithm>

namespace prec_factor {
    struct factor_result_one {
        precz prime;
        int count;
    };
    typedef std::vector<factor_result_one> factor_result_t;

    inline precz gcd(precz a, precz b) {
        // Binary GCD Algorithm (Stein's Algorithm)
        // Magnitudes only, result is positive
        if (a.sign == -1) a = -a;
        if (b.sign == -1) b = -b;

        if (a.is_zero()) return b;
        if (b.is_zero()) return a;

        int shift = 0;

        // 1. Find common power of 2
        while (a.is_even() && b.is_even()) {
            a >>= 1;
            b >>= 1;
            shift++;
        }

        // 2. Make a odd
        while (a.is_even()) {
            a >>= 1;
        }

        // 3. Loop
        while (!b.is_zero()) {
            while (b.is_even()) {
                b >>= 1;
            }

            if (a > b) {
                // swap a and b
                precz t = a; a = b; b = t;
            }

            b -= a;
        }

        if (shift == 0) return a;
        return a << shift;
    }
    
    // Wrapper for Miller Rabin
    inline bool is_prime(const precz& n, int k=10) {
        if (n <= 1) return false;
        if (n == 2 || n == 3) return true;
        if (n.is_even()) return false;
        
        // Random witnesses
        // Deterministic sets for uint64 exist but for huge numbers random is fine.
        static const uint64_t bases[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};
        for(uint64_t w : bases) {
             if (precz(w) >= n) break;
             if (!precn_impl::precn_miller_rabin(n.p.p, w)) return false;
        }
        
        // If n is very large, try some random bases too
        for (int i=0; i < k; ++i) {
             uint64_t w = 40 + (rand() % 1000);
             if (!precn_impl::precn_miller_rabin(n.p.p, w)) return false;
        }

        return true;
    }

    // Trial Division for small primes
    // Updates n by dividing out factors. Appends to factors list.
    inline void trial_division(precz& n, factor_result_t& factors) {
        // A small list of primes
        static const int small_primes[] = {
            2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101,
            103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251
        };

        for (int p_val : small_primes) {
            precz p(p_val);
            if (n < p) break;
            if (n == p) {
                 factors.push_back({p, 1});
                 n = 1;
                 break;
            }
            int count = 0;
            while(true) {
                // To avoid expensive %, check divisibility or just div
                // precz provides %
                precz rem = n % p;
                if (!rem.is_zero()) break;
                
                n = n / p;
                count++;
            }
            if (count > 0) {
                factors.push_back({p, count});
            }
            if (n == 1) break;
        }
    }

    // Pollard's Rho Algorithm
    // Returns a non-trivial factor or 1 if failed.
    inline precz pollard_rho(const precz& n) {
        if (n == 1) return 1;
        if (n.is_even()) return 2;
        
        precz x = precz(rand()) % (n - 2) + 2;
        precz y = x;
        precz c = precz(rand()) % (n - 1) + 1;
        precz d = 1;
        
        int steps = 0;
        int cycle_limit = 20000; // Limit to prevent infinite loop
        
        // Brent's cycle finding or standard Floyd
        while (d == 1) {
            // x = f(x)
            x = (x*x + c) % n;
            
            // y = f(f(y))
            y = (y*y + c) % n;
            y = (y*y + c) % n;
            
            // d = gcd(|x-y|, n)
            precz diff = x - y;
            if (diff.sign == -1) diff = -diff;
            
            d = gcd(diff, n);
            
            if (d == n) {
                // Failure, try different c
                // Limit restarts to avoid infinite loop
                static int restarts = 0; 
                if (++restarts > 5) return 1;

                x = precz(rand()) % (n - 2) + 2;
                y = x;
                c = precz(rand()) % (n - 1) + 1;
                d = 1;
                // steps = 0; // Do NOT reset steps to guarantee termination
            }
            
            if (++steps > cycle_limit) return 1; // Give up
        }
        return d;
    }
    
    // Helper: Modular Exponentiation for precz
    inline precz pow_mod(precz base, precz exp, precz mod) {
        precz res = 1;
        base = base % mod;
        while (!exp.is_zero()) {
            if (!exp.is_even()) res = (res * base) % mod;
            base = (base * base) % mod;
            exp >>= 1;
        }
        return res;
    }

    // Legendre Symbol (a/p)
    inline int legendre(const precz& a, const precz& p) {
        precz val = a % p;
        if (val.is_zero()) return 0;
        if (p == 2) return 1; 
        precz res = pow_mod(val, (p - 1) >> 1, p);
        if (res == 1) return 1;
        if (res == p - 1) return -1;
        return 0; // Should not happen for prime p unless a%p==0
    }

    // Tonelli-Shanks for sqrt(n) mod p
    inline precz tonelli_shanks(const precz& n, const precz& p) {
        if (legendre(n, p) != 1) return 0; // No root
        if (p == 2) return (n % 2);
        
        precz q = p - 1;
        int s = 0;
        while (q.is_even()) {
            q >>= 1;
            s++;
        }
        
        precz z = 2;
        while (legendre(z, p) != -1) z++;
        
        precz m = s;
        precz c = pow_mod(z, q, p);
        precz t = pow_mod(n, q, p);
        precz r = pow_mod(n, (q + 1) >> 1, p);
        
        while (t != 1) {
            precz tt = t;
            int i = 0;
            while (tt != 1) {
                tt = (tt * tt) % p;
                i++;
                if (i == s) return 0; // Error
            }
            
            int shift_amt = s - i - 1;
            precz b = c;
            // pow_mod(c, 2^(s-i-1), p)
            // b = b^(2^shift_amt)
            for(int k=0; k<shift_amt; ++k) b = (b*b)%p;
            
            m = i;
            c = (b * b) % p;
            t = (t * c) % p;
            r = (r * b) % p;
        }
        return r;
    }

    inline precz integer_sqrt(const precz& n) {
        // Simple bisection or reliance on library.
        // Assuming library lacks direct precz::sqrt, implementing robust integer search
        if(n <= 0) return 0;
        
        // Initial guess x = n is safe but slow. 
        // Newton iteration converges quadratically so it's fine.
        precz x = n;
        precz y = 0;
        
        while(true) {
             if (x.is_zero()) break; 
             y = (x + n/x) / 2; // Using /2 instead of >>1 to be safe
             if (y >= x) return x;
             x = y;
        }
        return x;
    }

    // Helper for positive modulo
    inline precz pos_mod(const precz& a, const precz& n) {
        precz r = a % n;
        if (r < 0) r += n;
        return r;
    }

    // Lenstra Elliptic Curve Factorization (ECM) - Phase 1
    // Using Montgomery curves By^2 = x^3 + Ax^2 + x
    inline precz lenstra_ecm(const precz& n, int B1 = 2000) {
        // Suyama's parameterization
        // We try multiple curves
        for (int curve_try = 0; curve_try < 20; ++curve_try) {
            precz sigma = 6 + curve_try;
            precz u = (sigma * sigma); u -= 5;
            precz v = sigma * 4;
            
            precz diff = v - u; 
            
            precz u3 = pos_mod(pos_mod(u*u, n) * u, n);
            
            // X = u^3, Z = v^3
            precz x = u3;
            precz z = pos_mod(pos_mod(v*v, n) * v, n);
            
            // Calculate a24 = (v-u)^3 * (3u+v) / (16u^3v) mod n
            // We need modular inverse of 16u^3v
            precz term1 = pos_mod(pos_mod(diff*diff, n) * diff, n); // (v-u)^3
            precz term2 = pos_mod(u*3 + v, n);
            precz num = pos_mod(term1 * term2, n);
            
            precz denom = pos_mod(pos_mod(precz(16) * u3, n) * v, n);
            
            // GCD check before inverse
            precz g = gcd(denom, n);
            if (g > 1 && g < n) return g;
            if (g == n) continue; // Fail (denom is 0 mod n)
            
            // Extended Euclid for Inverse
            precz denom_inv = 0;
            {
                 // Local scope for ext euclid
                 precz m = n, a = denom;
                 precz m0 = m, t, q;
                 precz x0 = 0, x1 = 1;
                 
                 // if m == 1 return 0
                 
                 while (a > 1) {
                     if (m.is_zero()) break; // Should not happen
                     q = a / m;
                     t = m;
                     m = a % m; a = t;
                     t = x0;
                     x0 = x1 - q * x0;
                     x1 = t;
                 }
                 if (x1 < 0) x1 += m0;
                 denom_inv = x1;
            }
            
            precz a24 = pos_mod(num * denom_inv, n);
            
            // Multiply P by all prime powers <= B1
            // P_current = P (initially from Suyama)
            
            static const int primes[] = {
                 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101,
                 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199,
                 211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283, 293
            };
            
            for(int p : primes) {
                if (p > B1) break;
                // Calculate max power q = p^e <= B1
                int q = p;
                while (1) {
                    long long next_q = (long long)q * p;
                    if (next_q > B1) break;
                    q = (int)next_q;
                }
                
                // Montgomery Ladder: computes P_new = q * P
                // P is (x:z).
                
                precz x1 = x, z1 = z; // R0 
                precz x2, z2; // R1
                
                // Init R1 = 2 * R0
                {
                    precz sum = pos_mod(x + z, n);
                    precz dif = pos_mod(x - z, n); 
                    precz A = pos_mod(sum*sum, n);
                    precz B = pos_mod(dif*dif, n);
                    precz C = pos_mod(A - B, n);
                    x2 = pos_mod(A * B, n);
                    // z2 = C * (B + a24 * C)
                    z2 = pos_mod(C * pos_mod(B + pos_mod(a24 * C, n), n), n);
                }
                
                // Original P coords for addition formula
                precz x_diff = x, z_diff = z;
                
                // Find MSB of q
                int msb = 0;
                for(int b=30; b>=0; --b) if((q >> b) & 1) { msb=b; break; }
                
                // Iterate from msb-1 down to 0
                 for(int b = msb-1; b >= 0; --b) {
                     int bit = (q >> b) & 1;
                     if (bit == 0) {
                         // R1 = R0 + R1; R0 = 2R0
                         // Add R0, R1 -> new R1
                         {
                            // A = (x1-z1)(x2+z2)
                            precz sub1 = pos_mod(x1 - z1, n);
                            precz add2 = pos_mod(x2 + z2, n);
                            precz A = pos_mod(sub1 * add2, n);
                            
                            // B = (x1+z1)(x2-z2)
                            precz add1 = pos_mod(x1 + z1, n);
                            precz sub2 = pos_mod(x2 - z2, n);
                            precz B = pos_mod(add1 * sub2, n);
                            
                            precz sumAB = pos_mod(A + B, n);
                            precz difAB = pos_mod(A - B, n);
                            
                            x2 = pos_mod(z_diff * pos_mod(sumAB*sumAB, n), n);
                            z2 = pos_mod(x_diff * pos_mod(difAB*difAB, n), n);
                         }
                         // Double R0 -> new R0
                         {
                            precz sum = pos_mod(x1 + z1, n);
                            precz dif = pos_mod(x1 - z1, n);
                            precz A = pos_mod(sum*sum, n);
                            precz B = pos_mod(dif*dif, n);
                            precz C = pos_mod(A - B, n);
                            x1 = pos_mod(A * B, n);
                            z1 = pos_mod(C * pos_mod(B + pos_mod(a24 * C, n), n), n);
                         }
                     } else {
                         // R0 = R0 + R1; R1 = 2R1
                         // Add R0, R1 -> new R0
                         {
                            precz sub1 = pos_mod(x1 - z1, n);
                            precz add2 = pos_mod(x2 + z2, n);
                            precz A = pos_mod(sub1 * add2, n);
                            
                            precz add1 = pos_mod(x1 + z1, n);
                            precz sub2 = pos_mod(x2 - z2, n);
                            precz B = pos_mod(add1 * sub2, n);
                            
                            precz sumAB = pos_mod(A + B, n);
                            precz difAB = pos_mod(A - B, n);
                            
                            x1 = pos_mod(z_diff * pos_mod(sumAB*sumAB, n), n);
                            z1 = pos_mod(x_diff * pos_mod(difAB*difAB, n), n);
                         }
                         // Double R1 -> new R1
                         {
                            precz sum = pos_mod(x2 + z2, n);
                            precz dif = pos_mod(x2 - z2, n);
                            precz A = pos_mod(sum*sum, n);
                            precz B = pos_mod(dif*dif, n);
                            precz C = pos_mod(A - B, n);
                            x2 = pos_mod(A * B, n);
                            z2 = pos_mod(C * pos_mod(B + pos_mod(a24 * C, n), n), n);
                         }
                     }
                 }
                 x = x1; z = z1;
                 
                 // Safety for small numbers: frequent check
                 if ((n.p.p && n.p.p->rsiz <= 4) || (curve_try > 0 && p % 10 == 0)) {
                      precz g_check = gcd(z, n);
                      if (g_check > 1 && g_check < n) return g_check;
                 }
            }
            
            precz g3 = gcd(z, n);
            // std::cout << "[lenstra_ecm] Curve " << curve_try << " finished. GCD=" << g3 << std::endl;
            if (g3 > 1 && g3 < n) return g3;
        }
        return 1;
    }

    
    // Quadratic Sieve Factorization (Simplified MPQS)
    inline precz qs_factor(const precz& n) {
        if(n.is_even()) return 2;
        if(is_prime(n)) return n;

        std::cout << "[QS] Setup for n=" << n << std::endl;
        
        // 1. Setup
        precz root_n = integer_sqrt(n);
        std::cout << "[QS] root=" << root_n << std::endl;

        if (root_n*root_n == n) return root_n; // Perfect square
        
        // Tuning
        int bound_B = 50; // Very small for demo. Increase for production.
        int sieve_interval = 200; 

        // 2. Factor Base
        std::vector<int> factor_base;
        factor_base.push_back(-1); // Special handling needed? Just collecting primes for now.
        factor_base.push_back(2);
        
        // Simple prime gen
        int p = 3;
        while(factor_base.size() < (size_t)bound_B) {
            bool is_p = true;
            for(int j=2; j*j<=p; ++j) if(p%j==0) {is_p=false; break;}
            if(is_p) {
                // Check Quadratic Residue
                int L = legendre(n, p);
                // std::cout << "Legendre(" << n << ", " << p << ") = " << L << std::endl;
                if (L == 1) {
                    factor_base.push_back(p);
                }
            }
            p += 2;
        }
        std::cout << "[QS] Factor base generated. Max prime=" << factor_base.back() << std::endl;

        struct Relation {
            precz x;
            precz val; // Q(x)
            std::vector<int> exponents; // modulo 2
        };
        std::vector<Relation> relations;
        size_t target_rels = factor_base.size() + 5;
        
        // 3. Simple Sieving (finding smooth numbers)
        // Check Q(x) for x = 0, 1, ...
        // Q(x) = (root_n + x)^2 - n
        
        std::cout << "[QS] Starting sieving with FB size " << factor_base.size() << std::endl;
        
        // Using int for sieving range is risky for massive n, but okay for standard QS steps
        for(long long i = 0; relations.size() < target_rels && i < sieve_interval * 100; ++i) {
             precz x = root_n + i;
             precz q = x*x - n;
             
             precz temp = q;
             std::vector<int> exps(factor_base.size(), 0);
             
             // Factor over FB
             // Handle -1
             if (temp < 0) {
                 temp = -temp;
                 exps[0] = 1;
             }
             
             for(size_t j=1; j<factor_base.size(); ++j) {
                 int prime = factor_base[j];
                 while ( (temp % prime).is_zero() ) {
                     temp = temp / prime;
                     exps[j] ^= 1; // Flip bit
                 }
             }
             
             if (temp == 1) {
                 // Smooth!
                 relations.push_back({x, q, exps});
                 // std::cout << "Found relation " << relations.size() << "/" << target_rels << std::endl;
             }
        }
        
        if (relations.size() < target_rels) {
            std::cout << "[QS] Not enough relations found." << std::endl;
            return 1;
        }

        // 4. Gaussian Elimination (GF(2))
        // Matrix M: rows = relations, cols = primes
        // We transpose to solve M^T * v = 0
        int M_rels = (int)relations.size();
        int N_dim = (int)factor_base.size();
        
        // Lambda to check a solution
        // Returns factor if found, or 1 if trivial/failure
        auto try_solve = [&](const std::vector<int>& sol_indices) -> precz {
            if (sol_indices.empty()) return 1;

             std::map<int, int> counts;
             for(int idx : sol_indices) counts[idx]++;
             
             std::vector<int> final_indices;
             for(auto& pair : counts) {
                 if (pair.second % 2 != 0) final_indices.push_back(pair.first);
             }

             precz X = 1;
             std::vector<long long> total_exps(factor_base.size(), 0);

             for(int idx : final_indices) {
                 X = (X * relations[idx].x) % n;
                 
                 precz val = relations[idx].val;
                 if (val < 0) {
                      total_exps[0]++;
                      val = -val;
                 }
                 // Re-factor to get exact exponents
                 for(size_t j=1; j<factor_base.size(); ++j) {
                      int p_val = factor_base[j];
                      while ( (val % p_val).is_zero() ) {
                          val = val / p_val;
                          total_exps[j]++;
                      }
                 }
             }
             
             precz Y = 1;
             for(size_t j=1; j<factor_base.size(); ++j) {
                 long long c = total_exps[j];
                 if (c % 2 != 0) {
                     return 1; // Check failed
                 }
                 precz p_pow = pow_mod(factor_base[j], c / 2, n);
                 Y = (Y * p_pow) % n;
             }

             // std::cout << "[QS] Checking X=" << X << " Y=" << Y << std::endl;
             
             precz g1 = gcd(X + Y, n);
             if (g1 > 1 && g1 < n) return g1;
             
             precz g2 = gcd(X - Y, n);
             if (g2 > 1 && g2 < n) return g2;
             
             return 1;
        };

        struct BasisRow {
            int pivot; // column index
            std::vector<int> bits; // the row
            std::vector<int> history; // indices of relations summed
        };
        std::vector<BasisRow> basis;

        for(int r=0; r<M_rels; ++r) {
            std::vector<int> row = relations[r].exponents;
            std::vector<int> current_hist;
            current_hist.push_back(r);
            
            // Reduce row against basis
            for(const auto& b : basis) {
                if (row[b.pivot]) {
                    // XOR row with basis
                    for(int k=0; k<N_dim; ++k) row[k] ^= b.bits[k];
                    // Append history
                    current_hist.insert(current_hist.end(), b.history.begin(), b.history.end());
                }
            }
            
            // Find pivot
            int pivot = -1;
            for(int k=0; k<N_dim; ++k) {
                if(row[k]) {
                    pivot = k;
                    break;
                }
            }
            
            if (pivot == -1) {
                // Dependency found! Try it.
                // std::cout << "[QS] Dependency found at relation " << r << std::endl;
                precz f = try_solve(current_hist);
                if (f > 1 && f < n) return f;
                // If trivial, continue searching
            } else {
                // Add to basis
                basis.push_back({pivot, row, current_hist});
            }
        }
        
        std::cout << "[QS] Failed to find factors after checking dependencies." << std::endl;
        return 1;
    }

    /*
     * Main factorization driver
     */
    inline void factorex(precz n, factor_result_t& result) {
        if (n <= 1) return;
        
        trial_division(n, result);
        if (n == 1) return;
        
        if (is_prime(n)) {
            result.push_back({n, 1});
            return;
        }
        
        // Use a queue or stack for factors to decompose
        std::vector<precz> composite_queue;
        composite_queue.push_back(n);
        
        while(!composite_queue.empty()) {
            precz curr = composite_queue.back();
            composite_queue.pop_back();
            
            if (curr == 1) continue;
            if (is_prime(curr)) {
                result.push_back({curr, 1});
                continue;
            }
            
            precz factor = pollard_rho(curr);

            if (factor == 1 || factor == curr) {
                // Try ECM
                factor = lenstra_ecm(curr);
            }

            if (factor == 1 || factor == curr) {
                // Try QS
                factor = qs_factor(curr);
            }
            
            if (factor == 1 || factor == curr) {
                // Failed to factor
                result.push_back({curr, 1}); // Treat as prime/unfactored
            } else {
                composite_queue.push_back(factor);
                composite_queue.push_back(curr / factor);
            }
        }
        // Sort or aggregate?
    }

}
#endif /* PREC_FACTOR_HPP */