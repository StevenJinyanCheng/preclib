#ifndef PREC_FFT
#define PREC_FFT
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <type_traits>
#include <string>
#include <vector>
#include <map>
#include <complex>
#include <algorithm>
#include "uint128.hpp"

// Check for AVX2 support
#if defined(__AVX2__) || defined(__ARCH_AVX2__)
#define PRECN_FFT_USE_AVX2
#include <immintrin.h>
#endif

namespace precn_impl {
namespace quick_fft {
    const double PI = 3.14159265358979323846;

    inline void cvt(uint32_t *in, size_t n, double *re, double *im) {
        size_t i = 0;
#ifdef PRECN_FFT_USE_AVX2
        __m256d v_zero = _mm256_setzero_pd();
        for (; i + 4 <= n; i += 4) {
            __m128i v_in_idx = _mm_loadu_si128((const __m128i*)(in + i));
            __m256d v_val = _mm256_cvtepi32_pd(v_in_idx);
            _mm256_storeu_pd(re + i, v_val);
            _mm256_storeu_pd(im + i, v_zero);
        }
#endif
        for (; i < n; ++i) {
            re[i] = (double)(in[i]);
            im[i] = 0.0;
        }
    }

    inline void cvt_back(uint32_t *out, size_t n, double *re, double *im) {
        size_t i = 0;
#ifdef PRECN_FFT_USE_AVX2
        __m256d v_half = _mm256_set1_pd(0.5);
        for (; i + 4 <= n; i += 4) {
            __m256d v_re = _mm256_loadu_pd(re + i);
            v_re = _mm256_add_pd(v_re, v_half);
            // cvttpd_epi32 truncates, so we added 0.5 to round
            __m128i v_out = _mm256_cvttpd_epi32(v_re);
            _mm_storeu_si128((__m128i*)(out + i), v_out);
        }
#endif
        for (; i < n; ++i) {
            out[i] = (uint32_t)(re[i] + 0.5);
        }
    }

    inline void fft(double *re, double *im, size_t n, bool inverse = false) {
        // Bit Reversal
        size_t j = 0;
        for (size_t i = 1; i < n; i++) {
            size_t bit = n >> 1;
            while (j & bit) {
                j ^= bit;
                bit >>= 1;
            }
            j ^= bit;
            if (i < j) {
                std::swap(re[i], re[j]);
                std::swap(im[i], im[j]);
            }
        }

        // Iterative FFT
        for (size_t len = 2; len <= n; len <<= 1) {
            double ang = 2 * PI / len * (inverse ? 1 : -1);
            std::complex<double> wlen(cos(ang), sin(ang));
            double wlen_r = wlen.real();
            double wlen_i = wlen.imag();
            size_t half = len >> 1;

            size_t i = 0;
            // Iterate blocks
            for (i = 0; i < n; i += len) {
                double w_r = 1.0;
                double w_i = 0.0;
                size_t k = 0;

#ifdef PRECN_FFT_USE_AVX2
                if (half >= 4) {
                    double wr[4], wi[4];
                    for(int z=0; z<4; ++z) {
                        double a = ang * z;
                        wr[z] = cos(a);
                        wi[z] = sin(a);
                    }
                    
                    __m256d vw_r = _mm256_loadu_pd(wr);
                    __m256d vw_i = _mm256_loadu_pd(wi);

                    double step_ang = ang * 4;
                    double w4_r = cos(step_ang);
                    double w4_i = sin(step_ang);
                    __m256d v_w4_r = _mm256_set1_pd(w4_r);
                    __m256d v_w4_i = _mm256_set1_pd(w4_i);

                    for (; k + 4 <= half; k += 4) {
                        __m256d v_ur = _mm256_loadu_pd(re + i + k);
                        __m256d v_ui = _mm256_loadu_pd(im + i + k);
                        __m256d v_vr = _mm256_loadu_pd(re + i + k + half);
                        __m256d v_vi = _mm256_loadu_pd(im + i + k + half);

                        __m256d v_tr = _mm256_sub_pd(
                            _mm256_mul_pd(vw_r, v_vr), 
                            _mm256_mul_pd(vw_i, v_vi)
                        );
                        __m256d v_ti = _mm256_add_pd(
                            _mm256_mul_pd(vw_r, v_vi), 
                            _mm256_mul_pd(vw_i, v_vr)
                        );

                        __m256d v_new_ur = _mm256_add_pd(v_ur, v_tr);
                        __m256d v_new_ui = _mm256_add_pd(v_ui, v_ti);
                        __m256d v_new_vr = _mm256_sub_pd(v_ur, v_tr);
                        __m256d v_new_vi = _mm256_sub_pd(v_ui, v_ti);

                        _mm256_storeu_pd(re + i + k, v_new_ur);
                        _mm256_storeu_pd(im + i + k, v_new_ui);
                        _mm256_storeu_pd(re + i + k + half, v_new_vr);
                        _mm256_storeu_pd(im + i + k + half, v_new_vi);

                        __m256d v_next_wr = _mm256_sub_pd(
                            _mm256_mul_pd(vw_r, v_w4_r), 
                            _mm256_mul_pd(vw_i, v_w4_i)
                        );
                        __m256d v_next_wi = _mm256_add_pd(
                            _mm256_mul_pd(vw_r, v_w4_i), 
                            _mm256_mul_pd(vw_i, v_w4_r)
                        );
                        vw_r = v_next_wr;
                        vw_i = v_next_wi;
                    }
                    
                    double a = ang * k;
                    w_r = cos(a);
                    w_i = sin(a);
                }
#endif

                for (; k < half; k++) {
                    double u_r = re[i + k];
                    double u_i = im[i + k];
                    double v_r = re[i + k + half];
                    double v_i = im[i + k + half];
                    
                    double t_r = w_r * v_r - w_i * v_i;
                    double t_i = w_r * v_i + w_i * v_r;
                    
                    re[i + k] = u_r + t_r;
                    im[i + k] = u_i + t_i;
                    re[i + k + half] = u_r - t_r;
                    im[i + k + half] = u_i - t_i;
                    
                    double next_wr = w_r * wlen_r - w_i * wlen_i;
                    double next_wi = w_r * wlen_i + w_i * wlen_r;
                    w_r = next_wr;
                    w_i = next_wi;
                }
            }
        }

        if (inverse) {
#ifdef PRECN_FFT_USE_AVX2
            __m256d v_n_inv = _mm256_set1_pd(1.0 / n);
            size_t i = 0;
            for (; i + 4 <= n; i += 4) {
                __m256d v_re = _mm256_loadu_pd(re + i);
                __m256d v_im = _mm256_loadu_pd(im + i);
                v_re = _mm256_mul_pd(v_re, v_n_inv);
                v_im = _mm256_mul_pd(v_im, v_n_inv);
                _mm256_storeu_pd(re + i, v_re);
                _mm256_storeu_pd(im + i, v_im);
            }
            for (; i < n; ++i) {
                re[i] /= n;
                im[i] /= n;
            }
#else
            for (size_t i = 0; i < n; ++i) {
                re[i] /= n;
                im[i] /= n;
            }
#endif
        }
    }
};
};
#endif
