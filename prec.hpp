#ifndef PREC_HPP
#define PREC_HPP
#ifndef __cplusplus
#error "U idiot, this is a C++ header!"
#endif
#ifndef __min
#define __min(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef __max
#define __max(a,b) (((a)>(b))?(a):(b))
#endif
#define PREC_VERSION "1.1"
#define _preclib_log(msg) fprintf(stderr, "[preclib] %s\n", msg)
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
#ifdef PRECLIB_HI
struct __preclib_hi{
  __preclib_hi(){
    printf("Hello from preclib v%s and Steven. Have a nice day!\n", PREC_VERSION);
  }
};
__preclib_hi _hello;
#endif


struct __precn_struct {
  uint64_t asiz; // 分配的块数 (allocated blocks)
  uint64_t rsiz; // 实际使用的块数 (used blocks)
  uint32_t *n;   // little endian words
};


namespace precn_impl {
typedef struct __precn_struct *precn_t;
// Global configuration flags
static bool g_enable_fft = true;

inline precn_t __precn_new_size(uint64_t asiz, uint64_t rsiz) {
  precn_t p = (precn_t)malloc(sizeof(struct __precn_struct));
  if (p == NULL) return NULL;
  p->asiz = asiz;
  p->rsiz = rsiz;
  /* ensure at least 1 element when asiz == 0 to keep calloc well-defined */
  p->n = (uint32_t*)calloc(asiz ? asiz : 1, sizeof(uint32_t));
  if (p->n == NULL) {
    free(p);
    return NULL;
  }
  return p;
}

template<typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
inline precn_t precn_new(T a) {
  uint64_t val;
  if (std::is_signed<T>::value) {
    /* safe conversion for signed types */
    val = (a < 0) ? 0ull : static_cast<uint64_t>(a);
  } else {
    val = static_cast<uint64_t>(a);
  }

  if (val < 0x100000000ull) {
    precn_t p = __precn_new_size(2, 1);
    if (!p) return NULL;
    p->n[0] = (uint32_t)(val & 0xFFFFFFFFULL);
    return p;
  }

  precn_t p = __precn_new_size(2, 2);
  if (!p) return NULL;
  p->n[0] = (uint32_t)(val & 0xFFFFFFFFULL);
  p->n[1] = (uint32_t)((val >> 32) & 0xFFFFFFFFULL);
  return p;
}

inline void precn_realloc(precn_t a, uint64_t nsiz) {
  if (a == NULL) return;
  if (nsiz == 0) return;
  if (a->asiz < nsiz) {
    void *tmp = realloc(a->n, nsiz * sizeof(uint32_t));
    if (tmp == NULL) {
      /* leave original allocation intact on OOM */
      return;
    }
    a->n = (uint32_t*)tmp;
    /* zero newly allocated area */
    if (nsiz > a->asiz) {
      memset(a->n + a->asiz, 0, (nsiz - a->asiz) * sizeof(uint32_t));
    }
    a->asiz = nsiz;
  }
}

inline void precn_free(precn_t a) {
  if (a == NULL) return;
  if (a->n != NULL) free(a->n);
  free(a);
}

inline void precn_set(precn_t a, precn_t b) {
  if (a == NULL || b == NULL) return;
  if (a->asiz < b->rsiz) {
    void *tmp = realloc(a->n, b->rsiz * sizeof(uint32_t));
    if (tmp == NULL) return; /* leave `a` unchanged on OOM */
    a->n = (uint32_t*)tmp;
    a->asiz = b->rsiz;
  }
  memcpy(a->n, b->n, b->rsiz * sizeof(uint32_t));
  a->rsiz = b->rsiz;
}

template<typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
inline void precn_set(precn_t a, T b) {
  if (a == NULL) return;
  uint64_t val;
  if (std::is_signed<T>::value) {
    val = (b < 0) ? 0ull : static_cast<uint64_t>(b);
  } else {
    val = static_cast<uint64_t>(b);
  }

  if (a->asiz < 2) {
    void *tmp = realloc(a->n, 2 * sizeof(uint32_t));
    if (tmp == NULL) return;
    a->n = (uint32_t*)tmp;
    a->asiz = 2;
  }

  a->n[0] = (uint32_t)(val & 0xFFFFFFFFULL);
  a->n[1] = (uint32_t)((val >> 32) & 0xFFFFFFFFULL);
  a->rsiz = (val < 0x100000000ull) ? 1 : 2;
}

/* Multiply `a` in-place by a 32-bit unsigned integer `b`.
 * Returns 0 on success, -1 if `a` is NULL, -2 on allocation failure.
 */
inline int precn_mul_u32(precn_t a, uint32_t b) {
  if (a == NULL) return -1;
  if (b == 0) {
    /* result is zero */
    if (a->n && a->asiz > 0) a->n[0] = 0;
    a->rsiz = 0;
    return 0;
  }
  if (b == 1) return 0;

  uint64_t carry = 0;
  for (uint64_t i = 0; i < a->rsiz; ++i) {
    uint64_t prod = (uint64_t)a->n[i] * (uint64_t)b + carry;
    a->n[i] = (uint32_t)(prod & 0xFFFFFFFFULL);
    carry = prod >> 32;
  }

  if (carry) {
    uint64_t need = a->rsiz + 1;
    if (a->asiz < need) {
      void *tmp = realloc(a->n, need * sizeof(uint32_t));
      if (tmp == NULL) return -2; /* OOM */
      a->n = (uint32_t*)tmp;
      /* zero newly allocated words */
      if (need > a->asiz) memset(a->n + a->asiz, 0, (need - a->asiz) * sizeof(uint32_t));
      a->asiz = need;
    }
    a->n[a->rsiz] = (uint32_t)carry;
    a->rsiz += 1;
  }

  /* trim any high zero words (shouldn't normally be necessary) */
  while (a->rsiz > 0 && a->n[a->rsiz - 1] == 0) a->rsiz--;
  return 0;
}

/* Divide `a` by a 32-bit unsigned integer `b`.
 * Stores quotient in `res` and returns remainder.
 */
inline uint32_t precn_div_u32(precn_t a, uint32_t b, precn_t res) {
  if (b == 0) return 0; // Error
  if (a == NULL) return 0;
  
  precn_realloc(res, a->rsiz);
  if (res->asiz < a->rsiz) return 0; // OOM

  uint64_t rem = 0;
  for (int64_t i = (int64_t)a->rsiz - 1; i >= 0; --i) {
    uint64_t cur = a->n[i] + (rem << 32);
    res->n[i] = (uint32_t)(cur / b);
    rem = cur % b;
  }
  res->rsiz = a->rsiz;
  while (res->rsiz > 0 && res->n[res->rsiz - 1] == 0) res->rsiz--;
  return (uint32_t)rem;
}

/* precn_cmp(a, b): a < b => -1; a == b => 0; a > b => 1 */
inline int precn_cmp(precn_t a, precn_t b) {
  uint64_t arsiz = (a == NULL) ? 0 : a->rsiz;
  uint64_t brsiz = (b == NULL) ? 0 : b->rsiz;
  if (arsiz < brsiz) return -1;
  if (arsiz > brsiz) return 1;
  if (arsiz == 0) return 0;
  for (int64_t i = (int64_t)arsiz - 1; i >= 0; --i) {
    uint32_t aw = (a == NULL) ? 0 : a->n[i];
    uint32_t bw = (b == NULL) ? 0 : b->n[i];
    if (aw < bw) return -1;
    if (aw > bw) return 1;
  }
  return 0;
}

int precn_add(precn_t a, precn_t b, precn_t res){
  uint64_t arsiz = (a == NULL) ? 0 : a->rsiz;
  uint64_t brsiz = (b == NULL) ? 0 : b->rsiz;
  uint64_t maxsiz = (arsiz > brsiz) ? arsiz : brsiz;
  precn_realloc(res, maxsiz + 1);
  if (res->asiz < maxsiz + 1) return -1; // OOM

  uint64_t i;
  uint64_t carry = 0;
  for (i = 0; i < maxsiz; ++i) {
    uint64_t aw = (i < arsiz) ? a->n[i] : 0;
    uint64_t bw = (i < brsiz) ? b->n[i] : 0;
    uint64_t sum = aw + bw + carry;
    res->n[i] = (uint32_t)(sum & 0xFFFFFFFFULL);
    carry = sum >> 32;
  }
  if (carry > 0) {
    res->n[i] = (uint32_t)(carry & 0xFFFFFFFFULL);
    res->rsiz = maxsiz + 1;
  } else {
    res->rsiz = maxsiz;
  }
  return 0;
}

int precn_sub(precn_t a, precn_t b, precn_t res){
  uint64_t arsiz = (a == NULL) ? 0 : a->rsiz;
  uint64_t brsiz = (b == NULL) ? 0 : b->rsiz;
  if (precn_cmp(a, b) < 0) {
    // a < b, result would be negative; not supported
    return -1;
  }
  precn_realloc(res, arsiz);
  if (res->asiz < arsiz) return -1; // OOM

  uint64_t i;
  int64_t borrow = 0;
  for (i = 0; i < arsiz; ++i) {
    int64_t aw = (i < arsiz) ? a->n[i] : 0;
    int64_t bw = (i < brsiz) ? b->n[i] : 0;
    int64_t diff = aw - bw - borrow;
    if (diff < 0) {
      diff += 0x100000000LL;
      borrow = 1;
    } else {
      borrow = 0;
    }
    res->n[i] = (uint32_t)(diff & 0xFFFFFFFFULL);
  }
  // Adjust rsiz
  while (arsiz > 0 && res->n[arsiz - 1] == 0) {
    --arsiz;
  }
  res->rsiz = arsiz;
  return 0;
}
int precn_mul_slow(precn_t a, precn_t b, precn_t res){
  // slow multiplication algorithm (schoolbook)
  uint64_t arsiz = (a == NULL) ? 0 : a->rsiz;
  uint64_t brsiz = (b == NULL) ? 0 : b->rsiz;
  precn_realloc(res, arsiz + brsiz + 1);
  if (res->asiz < arsiz + brsiz + 1) return -1; // OOM
  memset(res->n, 0, res->asiz * sizeof(uint32_t));
  for (uint64_t i = 0; i < arsiz; ++i) {
    uint64_t carry = 0;
    for (uint64_t j = 0; j < brsiz; ++j) {
      uint64_t prod = (uint64_t)a->n[i] * (uint64_t)b->n[j] + (uint64_t)res->n[i + j] + carry;
      res->n[i + j] = (uint32_t)(prod & 0xFFFFFFFFULL);
      carry = prod >> 32;
    }
    if (carry > 0) {
      res->n[i + brsiz] += (uint32_t)carry;
    }
  }
  // Adjust rsiz
  uint64_t rrsiz = arsiz + brsiz;
  while (rrsiz > 0 && res->n[rrsiz - 1] == 0) {
    --rrsiz;
  }
  res->rsiz = rrsiz;
  return 0;
}
inline precn_t precn_slice(precn_t a, uint64_t start, uint64_t len) {
  precn_t p = __precn_new_size(len, len);
  if (p == NULL) return NULL;
  for (uint64_t i = 0; i < len; ++i) {
    if (start + i < a->rsiz) {
      p->n[i] = a->n[start + i];
    } else {
      p->n[i] = 0;
    }
  }
  while (p->rsiz > 0 && p->n[p->rsiz - 1] == 0) {
    p->rsiz--;
  }
  return p;
}

inline int precn_add_shifted(precn_t a, precn_t b, uint64_t shift) {
  if (b == NULL || b->rsiz == 0) return 0;
  uint64_t min_end = shift + b->rsiz;
  precn_realloc(a, min_end + 1);
  if (a->asiz < min_end + 1) return -1;

  if (shift > a->rsiz) {
    memset(a->n + a->rsiz, 0, (shift - a->rsiz) * sizeof(uint32_t));
  }

  uint64_t carry = 0;
  uint64_t i;
  for (i = 0; i < b->rsiz; ++i) {
    uint64_t idx = shift + i;
    uint64_t aw = (idx < a->rsiz) ? a->n[idx] : 0;
    uint64_t bw = b->n[i];
    uint64_t sum = aw + bw + carry;
    a->n[idx] = (uint32_t)(sum & 0xFFFFFFFFULL);
    carry = sum >> 32;
  }
  
  uint64_t idx = shift + b->rsiz;
  while (carry) {
    if (idx >= a->asiz) {
        precn_realloc(a, idx + 2);
        if (a->asiz <= idx) return -1;
    }
    uint64_t aw = (idx < a->rsiz) ? a->n[idx] : 0;
    uint64_t sum = aw + carry;
    a->n[idx] = (uint32_t)(sum & 0xFFFFFFFFULL);
    carry = sum >> 32;
    idx++;
  }
  
  if (idx > a->rsiz) a->rsiz = idx;
  if (min_end > a->rsiz) a->rsiz = min_end;
  
  return 0;
}

inline int precn_mul_karatsuba(precn_t a, precn_t b, precn_t res);
inline int precn_mul_toom_cook(precn_t a, precn_t b, precn_t res);

namespace toom_cook {

struct sprecn {
    precn_t p;
    int sign; // 1 for positive, -1 for negative, 0 for zero
};

inline sprecn sprecn_new(uint64_t val) {
    sprecn s;
    s.p = precn_new(val);
    s.sign = (val == 0) ? 0 : 1;
    return s;
}

inline void sprecn_free(sprecn s) {
    precn_free(s.p);
}

inline void sprecn_set(sprecn& a, sprecn b) {
    precn_set(a.p, b.p);
    a.sign = b.sign;
}

// a = a + b
inline void sprecn_add(sprecn& a, sprecn b) {
    if (b.sign == 0) return;
    if (a.sign == 0) {
        sprecn_set(a, b);
        return;
    }
    if (a.sign == b.sign) {
        precn_add(a.p, b.p, a.p);
    } else {
        int cmp = precn_cmp(a.p, b.p);
        if (cmp > 0) {
            precn_sub(a.p, b.p, a.p);
        } else if (cmp < 0) {
            precn_t tmp = precn_new(0);
            precn_sub(b.p, a.p, tmp);
            precn_set(a.p, tmp);
            precn_free(tmp);
            a.sign = b.sign;
        } else {
            precn_set(a.p, 0);
            a.sign = 0;
        }
    }
}

// a = a - b
inline void sprecn_sub(sprecn& a, sprecn b) {
    if (b.sign == 0) return;
    sprecn b_neg = b;
    b_neg.sign = -b.sign;
    sprecn_add(a, b_neg);
}

// a = a * u32
inline void sprecn_mul_u32(sprecn& a, uint32_t b) {
    if (a.sign == 0) return;
    if (b == 0) {
        precn_set(a.p, 0);
        a.sign = 0;
        return;
    }
    precn_mul_u32(a.p, b);
}

// a = a / u32
inline void sprecn_div_u32(sprecn& a, uint32_t b) {
    if (a.sign == 0 || b == 0) return;
    precn_t res = precn_new(0);
    precn_div_u32(a.p, b, res);
    precn_set(a.p, res);
    precn_free(res);
    if (a.p->rsiz == 0) a.sign = 0;
}

// Toom-3 (3-way) multiplication
// Points: 0, 1, -1, -2, inf
inline void toom3_3(precn_t a, precn_t b, precn_t res) {
    uint64_t n = __max(a->rsiz, b->rsiz);
    uint64_t m = (n + 2) / 3;

    // Evaluation
    // p0 = a0
    // p1 = a0 + a1 + a2
    // p_1 = a0 - a1 + a2
    // p_2 = a0 - 2a1 + 4a2
    // p_inf = a2
    
    sprecn pa0; pa0.p = precn_slice(a, 0, m); pa0.sign = 1;
    sprecn pa1; pa1.p = precn_slice(a, m, m); pa1.sign = 1;
    sprecn pa2; pa2.p = precn_slice(a, 2*m, m); pa2.sign = 1;
    
    sprecn pb0; pb0.p = precn_slice(b, 0, m); pb0.sign = 1;
    sprecn pb1; pb1.p = precn_slice(b, m, m); pb1.sign = 1;
    sprecn pb2; pb2.p = precn_slice(b, 2*m, m); pb2.sign = 1;

    // A(0), B(0)
    sprecn w0_a = pa0; // copy
    sprecn w0_b = pb0;

    // A(1), B(1)
    sprecn w1_a = sprecn_new(0); sprecn_add(w1_a, pa0); sprecn_add(w1_a, pa1); sprecn_add(w1_a, pa2);
    sprecn w1_b = sprecn_new(0); sprecn_add(w1_b, pb0); sprecn_add(w1_b, pb1); sprecn_add(w1_b, pb2);

    // A(-1), B(-1)
    sprecn wm1_a = sprecn_new(0); sprecn_add(wm1_a, pa0); sprecn_sub(wm1_a, pa1); sprecn_add(wm1_a, pa2);
    sprecn wm1_b = sprecn_new(0); sprecn_add(wm1_b, pb0); sprecn_sub(wm1_b, pb1); sprecn_add(wm1_b, pb2);

    // A(-2), B(-2)
    sprecn wm2_a = sprecn_new(0); 
    sprecn t_a1 = sprecn_new(0); sprecn_set(t_a1, pa1); sprecn_mul_u32(t_a1, 2);
    sprecn t_a2 = sprecn_new(0); sprecn_set(t_a2, pa2); sprecn_mul_u32(t_a2, 4);
    sprecn_add(wm2_a, pa0); sprecn_sub(wm2_a, t_a1); sprecn_add(wm2_a, t_a2);
    sprecn_free(t_a1); sprecn_free(t_a2);
    
    sprecn wm2_b = sprecn_new(0);
    sprecn t_b1 = sprecn_new(0); sprecn_set(t_b1, pb1); sprecn_mul_u32(t_b1, 2);
    sprecn t_b2 = sprecn_new(0); sprecn_set(t_b2, pb2); sprecn_mul_u32(t_b2, 4);
    sprecn_add(wm2_b, pb0); sprecn_sub(wm2_b, t_b1); sprecn_add(wm2_b, t_b2);
    sprecn_free(t_b1); sprecn_free(t_b2);

    // A(inf), B(inf)
    sprecn winf_a = pa2;
    sprecn winf_b = pb2;

    // Pointwise multiplication
    sprecn r0 = sprecn_new(0); precn_mul_toom_cook(w0_a.p, w0_b.p, r0.p); r0.sign = w0_a.sign * w0_b.sign;
    sprecn r1 = sprecn_new(0); precn_mul_toom_cook(w1_a.p, w1_b.p, r1.p); r1.sign = w1_a.sign * w1_b.sign;
    sprecn rm1 = sprecn_new(0); precn_mul_toom_cook(wm1_a.p, wm1_b.p, rm1.p); rm1.sign = wm1_a.sign * wm1_b.sign;
    sprecn rm2 = sprecn_new(0); precn_mul_toom_cook(wm2_a.p, wm2_b.p, rm2.p); rm2.sign = wm2_a.sign * wm2_b.sign;
    sprecn rinf = sprecn_new(0); precn_mul_toom_cook(winf_a.p, winf_b.p, rinf.p); rinf.sign = winf_a.sign * winf_b.sign;

    // Interpolation
    // c0 = r0
    // c4 = rinf
    // tmp1 = (r1 + rm1) / 2
    // c2 = tmp1 - c0 - c4
    // tmp2 = (r1 - rm1) / 2
    // R = (c0 + 4c2 + 16c4 - rm2) / 2
    // c3 = (R - tmp2) / 3
    // c1 = tmp2 - c3

    sprecn c0 = r0;
    sprecn c4 = rinf;
    
    sprecn tmp1 = sprecn_new(0); sprecn_add(tmp1, r1); sprecn_add(tmp1, rm1); sprecn_div_u32(tmp1, 2);
    sprecn c2 = sprecn_new(0); sprecn_add(c2, tmp1); sprecn_sub(c2, c0); sprecn_sub(c2, c4);
    
    sprecn tmp2_val = sprecn_new(0); sprecn_add(tmp2_val, r1); sprecn_sub(tmp2_val, rm1); sprecn_div_u32(tmp2_val, 2);
    
    sprecn R = sprecn_new(0);
    sprecn t4c2 = sprecn_new(0); sprecn_set(t4c2, c2); sprecn_mul_u32(t4c2, 4);
    sprecn t16c4 = sprecn_new(0); sprecn_set(t16c4, c4); sprecn_mul_u32(t16c4, 16);
    sprecn_add(R, c0); sprecn_add(R, t4c2); sprecn_add(R, t16c4); sprecn_sub(R, rm2); sprecn_div_u32(R, 2);
    sprecn_free(t4c2); sprecn_free(t16c4);
    
    sprecn c3 = sprecn_new(0); sprecn_add(c3, R); sprecn_sub(c3, tmp2_val); sprecn_div_u32(c3, 3);
    
    sprecn c1 = sprecn_new(0); sprecn_add(c1, tmp2_val); sprecn_sub(c1, c3);

    // Recomposition
    precn_set(res, c0.p);
    precn_add_shifted(res, c1.p, m);
    precn_add_shifted(res, c2.p, 2*m);
    precn_add_shifted(res, c3.p, 3*m);
    precn_add_shifted(res, c4.p, 4*m);

    // Cleanup
    sprecn_free(w1_a); sprecn_free(w1_b);
    sprecn_free(wm1_a); sprecn_free(wm1_b);
    sprecn_free(wm2_a); sprecn_free(wm2_b);
    sprecn_free(r0); sprecn_free(r1); sprecn_free(rm1); sprecn_free(rm2); sprecn_free(rinf);
    sprecn_free(tmp1); sprecn_free(c2); sprecn_free(tmp2_val); sprecn_free(R); sprecn_free(c3); sprecn_free(c1);
    
    // Note: pa0..pa2, pb0..pb2 were shallow copies or new structs wrapping existing pointers?
    // Wait, sprecn_new allocates new precn.
    // But I did sprecn_set(pa0, sprecn{a0, 1}).
    // sprecn{a0, 1} is a temporary struct. sprecn_set copies content.
    // So pa0 owns its memory. a0 owns its memory.
    // I need to free pa0..pa2, pb0..pb2.
    sprecn_free(pa0); sprecn_free(pa1); sprecn_free(pa2);
    sprecn_free(pb0); sprecn_free(pb1); sprecn_free(pb2);
}

// Toom-2.5 (2-3) multiplication
// Points: 0, 1, -1, inf
inline void toom2_3(precn_t a, precn_t b, precn_t res) {
    uint64_t n = b->rsiz; // assume b is larger
    uint64_t m = (n + 2) / 3;

    sprecn pa0; pa0.p = precn_slice(a, 0, m); pa0.sign = 1;
    sprecn pa1; pa1.p = precn_slice(a, m, m); pa1.sign = 1;
    
    sprecn pb0; pb0.p = precn_slice(b, 0, m); pb0.sign = 1;
    sprecn pb1; pb1.p = precn_slice(b, m, m); pb1.sign = 1;
    sprecn pb2; pb2.p = precn_slice(b, 2*m, m); pb2.sign = 1;

    // w0 = A(0)B(0)
    sprecn w0_a = pa0;
    sprecn w0_b = pb0;

    // w1 = A(1)B(1)
    sprecn w1_a = sprecn_new(0); sprecn_add(w1_a, pa0); sprecn_add(w1_a, pa1);
    sprecn w1_b = sprecn_new(0); sprecn_add(w1_b, pb0); sprecn_add(w1_b, pb1); sprecn_add(w1_b, pb2);

    // wm1 = A(-1)B(-1)
    sprecn wm1_a = sprecn_new(0); sprecn_add(wm1_a, pa0); sprecn_sub(wm1_a, pa1);
    sprecn wm1_b = sprecn_new(0); sprecn_add(wm1_b, pb0); sprecn_sub(wm1_b, pb1); sprecn_add(wm1_b, pb2);

    // winf = A(inf)B(inf) = a1 * b2
    sprecn winf_a = pa1;
    sprecn winf_b = pb2;

    sprecn r0 = sprecn_new(0); precn_mul_toom_cook(w0_a.p, w0_b.p, r0.p); r0.sign = w0_a.sign * w0_b.sign;
    sprecn r1 = sprecn_new(0); precn_mul_toom_cook(w1_a.p, w1_b.p, r1.p); r1.sign = w1_a.sign * w1_b.sign;
    sprecn rm1 = sprecn_new(0); precn_mul_toom_cook(wm1_a.p, wm1_b.p, rm1.p); rm1.sign = wm1_a.sign * wm1_b.sign;
    sprecn rinf = sprecn_new(0); precn_mul_toom_cook(winf_a.p, winf_b.p, rinf.p); rinf.sign = winf_a.sign * winf_b.sign;

    // Interpolation
    // c0 = r0
    // c3 = rinf
    // c2 = (r1 + rm1)/2 - c0
    // c1 = (r1 - rm1)/2 - c3

    sprecn c0 = r0;
    sprecn c3 = rinf;
    
    sprecn tmp = sprecn_new(0); sprecn_add(tmp, r1); sprecn_add(tmp, rm1); sprecn_div_u32(tmp, 2);
    sprecn c2 = sprecn_new(0); sprecn_add(c2, tmp); sprecn_sub(c2, c0);
    
    sprecn tmp2 = sprecn_new(0); sprecn_add(tmp2, r1); sprecn_sub(tmp2, rm1); sprecn_div_u32(tmp2, 2);
    sprecn c1 = sprecn_new(0); sprecn_add(c1, tmp2); sprecn_sub(c1, c3);

    precn_set(res, c0.p);
    precn_add_shifted(res, c1.p, m);
    precn_add_shifted(res, c2.p, 2*m);
    precn_add_shifted(res, c3.p, 3*m);

    sprecn_free(w1_a); sprecn_free(w1_b);
    sprecn_free(wm1_a); sprecn_free(wm1_b);
    sprecn_free(r0); sprecn_free(r1); sprecn_free(rm1); sprecn_free(rinf);
    sprecn_free(tmp); sprecn_free(c2); sprecn_free(tmp2); sprecn_free(c1);
    
    sprecn_free(pa0); sprecn_free(pa1);
    sprecn_free(pb0); sprecn_free(pb1); sprecn_free(pb2);
}

// Toom-2.4 (2-4) multiplication
// Points: 0, 1, -1, -2, inf
inline void toom2_4(precn_t a, precn_t b, precn_t res) {
    uint64_t n = b->rsiz;
    uint64_t m = (n + 3) / 4;

    sprecn pa0; pa0.p = precn_slice(a, 0, m); pa0.sign = 1;
    sprecn pa1; pa1.p = precn_slice(a, m, m); pa1.sign = 1;
    
    sprecn pb0; pb0.p = precn_slice(b, 0, m); pb0.sign = 1;
    sprecn pb1; pb1.p = precn_slice(b, m, m); pb1.sign = 1;
    sprecn pb2; pb2.p = precn_slice(b, 2*m, m); pb2.sign = 1;
    sprecn pb3; pb3.p = precn_slice(b, 3*m, m); pb3.sign = 1;

    // Evaluations
    sprecn w0_a = pa0;
    sprecn w0_b = pb0;

    sprecn w1_a = sprecn_new(0); sprecn_add(w1_a, pa0); sprecn_add(w1_a, pa1);
    sprecn w1_b = sprecn_new(0); sprecn_add(w1_b, pb0); sprecn_add(w1_b, pb1); sprecn_add(w1_b, pb2); sprecn_add(w1_b, pb3);

    sprecn wm1_a = sprecn_new(0); sprecn_add(wm1_a, pa0); sprecn_sub(wm1_a, pa1);
    sprecn wm1_b = sprecn_new(0); sprecn_add(wm1_b, pb0); sprecn_sub(wm1_b, pb1); sprecn_add(wm1_b, pb2); sprecn_sub(wm1_b, pb3);

    sprecn wm2_a = sprecn_new(0); 
    sprecn t_a1 = sprecn_new(0); sprecn_set(t_a1, pa1); sprecn_mul_u32(t_a1, 2);
    sprecn_add(wm2_a, pa0); sprecn_sub(wm2_a, t_a1);
    sprecn_free(t_a1);
    
    sprecn wm2_b = sprecn_new(0);
    sprecn t_b1 = sprecn_new(0); sprecn_set(t_b1, pb1); sprecn_mul_u32(t_b1, 2);
    sprecn t_b2 = sprecn_new(0); sprecn_set(t_b2, pb2); sprecn_mul_u32(t_b2, 4);
    sprecn t_b3 = sprecn_new(0); sprecn_set(t_b3, pb3); sprecn_mul_u32(t_b3, 8);
    sprecn_add(wm2_b, pb0); sprecn_sub(wm2_b, t_b1); sprecn_add(wm2_b, t_b2); sprecn_sub(wm2_b, t_b3);
    sprecn_free(t_b1); sprecn_free(t_b2); sprecn_free(t_b3);

    sprecn winf_a = pa1;
    sprecn winf_b = pb3;

    sprecn r0 = sprecn_new(0); precn_mul_toom_cook(w0_a.p, w0_b.p, r0.p); r0.sign = w0_a.sign * w0_b.sign;
    sprecn r1 = sprecn_new(0); precn_mul_toom_cook(w1_a.p, w1_b.p, r1.p); r1.sign = w1_a.sign * w1_b.sign;
    sprecn rm1 = sprecn_new(0); precn_mul_toom_cook(wm1_a.p, wm1_b.p, rm1.p); rm1.sign = wm1_a.sign * wm1_b.sign;
    sprecn rm2 = sprecn_new(0); precn_mul_toom_cook(wm2_a.p, wm2_b.p, rm2.p); rm2.sign = wm2_a.sign * wm2_b.sign;
    sprecn rinf = sprecn_new(0); precn_mul_toom_cook(winf_a.p, winf_b.p, rinf.p); rinf.sign = winf_a.sign * winf_b.sign;

    // Interpolation (Same as Toom-3)
    sprecn c0 = r0;
    sprecn c4 = rinf;
    
    sprecn tmp1 = sprecn_new(0); sprecn_add(tmp1, r1); sprecn_add(tmp1, rm1); sprecn_div_u32(tmp1, 2);
    sprecn c2 = sprecn_new(0); sprecn_add(c2, tmp1); sprecn_sub(c2, c0); sprecn_sub(c2, c4);
    
    sprecn tmp2_val = sprecn_new(0); sprecn_add(tmp2_val, r1); sprecn_sub(tmp2_val, rm1); sprecn_div_u32(tmp2_val, 2);
    
    sprecn R = sprecn_new(0);
    sprecn t4c2 = sprecn_new(0); sprecn_set(t4c2, c2); sprecn_mul_u32(t4c2, 4);
    sprecn t16c4 = sprecn_new(0); sprecn_set(t16c4, c4); sprecn_mul_u32(t16c4, 16);
    sprecn_add(R, c0); sprecn_add(R, t4c2); sprecn_add(R, t16c4); sprecn_sub(R, rm2); sprecn_div_u32(R, 2);
    sprecn_free(t4c2); sprecn_free(t16c4);
    
    sprecn c3 = sprecn_new(0); sprecn_add(c3, R); sprecn_sub(c3, tmp2_val); sprecn_div_u32(c3, 3);
    
    sprecn c1 = sprecn_new(0); sprecn_add(c1, tmp2_val); sprecn_sub(c1, c3);

    precn_set(res, c0.p);
    precn_add_shifted(res, c1.p, m);
    precn_add_shifted(res, c2.p, 2*m);
    precn_add_shifted(res, c3.p, 3*m);
    precn_add_shifted(res, c4.p, 4*m);

    sprecn_free(w1_a); sprecn_free(w1_b);
    sprecn_free(wm1_a); sprecn_free(wm1_b);
    sprecn_free(wm2_a); sprecn_free(wm2_b);
    sprecn_free(r0); sprecn_free(r1); sprecn_free(rm1); sprecn_free(rm2); sprecn_free(rinf);
    sprecn_free(tmp1); sprecn_free(c2); sprecn_free(tmp2_val); sprecn_free(R); sprecn_free(c3); sprecn_free(c1);
    
    sprecn_free(pa0); sprecn_free(pa1);
    sprecn_free(pb0); sprecn_free(pb1); sprecn_free(pb2); sprecn_free(pb3);
}

} // namespace toom_cook

namespace fft {
    using Complex = std::complex<double>;
    const double PI = 3.14159265358979323846;

    inline void fft(std::vector<Complex> & a, bool invert) {
        size_t n = a.size();

        for (size_t i = 1, j = 0; i < n; i++) {
            size_t bit = n >> 1;
            for (; j & bit; bit >>= 1)
                j ^= bit;
            j ^= bit;
            if (i < j)
                std::swap(a[i], a[j]);
        }

        for (size_t len = 2; len <= n; len <<= 1) {
            double ang = 2 * PI / len * (invert ? -1 : 1);
            Complex wlen(cos(ang), sin(ang));
            for (size_t i = 0; i < n; i += len) {
                Complex w(1);
                for (size_t j = 0; j < len / 2; j++) {
                    Complex u = a[i + j], v = a[i + j + len / 2] * w;
                    a[i + j] = u + v;
                    a[i + j + len / 2] = u - v;
                    w *= wlen;
                }
            }
        }

        if (invert) {
            for (Complex & x : a)
                x /= n;
        }
    }
}

namespace ntt {
    struct NTTPrime {
        long long mod;
        long long root;
    };
    
    // 3 primes for CRT
    static const NTTPrime primes[] = {
        {998244353, 3},
        {167772161, 3},
        {469762049, 3}
    };

    inline long long power(long long base, long long exp, long long mod) {
        long long res = 1;
        base %= mod;
        while (exp > 0) {
            if (exp % 2 == 1) res = (res * base) % mod;
            base = (base * base) % mod;
            exp /= 2;
        }
        return res;
    }

    inline long long modInverse(long long n, long long mod) {
        return power(n, mod - 2, mod);
    }

    inline void ntt(std::vector<long long> & a, bool invert, const NTTPrime& p = primes[0]) {
        size_t n = a.size();
        long long mod = p.mod;

        for (size_t i = 1, j = 0; i < n; i++) {

            size_t bit = n >> 1;
            for (; j & bit; bit >>= 1)
                j ^= bit;
            j ^= bit;
            if (i < j)
                std::swap(a[i], a[j]);
        }

        for (size_t len = 2; len <= n; len <<= 1) {
            long long wlen = power(p.root, (mod - 1) / len, mod);
            if (invert)
                wlen = modInverse(wlen, mod);
            
            for (size_t i = 0; i < n; i += len) {
                long long w = 1;
                for (size_t j = 0; j < len / 2; j++) {
                    long long u = a[i + j], v = (a[i + j + len / 2] * w) % mod;
                    a[i + j] = (u + v < mod ? u + v : u + v - mod);
                    a[i + j + len / 2] = (u - v >= 0 ? u - v : u - v + mod);
                    w = (w * wlen) % mod;
                }
            }
        }

        if (invert) {
            long long n_inv = modInverse(n, mod);
            for (long long & x : a)
                x = (x * n_inv) % mod;
        }
    }
}

// NTT multiplication using 3-moduli CRT with precomputed constants
int precn_mul_ntt(precn_t a, precn_t b, precn_t res) {
    uint64_t as = a->rsiz;
    uint64_t bs = b->rsiz;
    
    if (as == 0 || bs == 0) {
        precn_set(res, 0);
        return 0;
    }

    size_t n = 1;
    while (n < as + bs) n <<= 1;
    
    using unsigned_int128 = unsigned __int128;
    
    // M = P1 * P2 * P3 = 0x4114009ed000061800001
    static const unsigned_int128 M = ((unsigned_int128)0x411400 << 64) | 0x9ed000061800001ULL;
    
    // C1 = M1 * inv1 = 0x258ad1c6205353045308eb
    static const unsigned_int128 C1 = ((unsigned_int128)0x258ad1 << 64) | 0xc6205353045308ebULL;
    
    // C2 = M2 * inv2 = 0xb7799b9802e42304316b3
    static const unsigned_int128 C2 = ((unsigned_int128)0xb7799 << 64) | 0xb9802e42304316b3ULL;
    
    // C3 = M3 * inv3 = 0x1011948a4c7e6b2ce9e064
    static const unsigned_int128 C3 = ((unsigned_int128)0x101194 << 64) | 0x8a4c7e6b2ce9e064ULL;
    
    // Approximations for quotient estimation (inv_Pi = (double)(Ci) / M)
    // Actually D_i should be C_i / M.
    static const double D1 = 5.76879912487719304082e-01;
    static const double D2 = 1.76206510208806343964e-01;
    static const double D3 = 2.46913577303474351954e-01;

    std::vector<long long> results[3];
    
    for(int k=0; k<3; ++k) {
        std::vector<long long> fa(n, 0), fb(n, 0);
        long long mod = ntt::primes[k].mod;
        
        for(size_t i=0; i<as; ++i) fa[i] = a->n[i] % mod;
        for(size_t i=0; i<bs; ++i) fb[i] = b->n[i] % mod;
        
        ntt::ntt(fa, false, ntt::primes[k]);
        ntt::ntt(fb, false, ntt::primes[k]);
        
        for(size_t i=0; i<n; ++i) fa[i] = (fa[i] * fb[i]) % mod;
        
        ntt::ntt(fa, true, ntt::primes[k]);
        results[k] = std::move(fa);
    }
    
    uint64_t res_len = as + bs;
    precn_realloc(res, res_len + 5); 
    if (res->asiz < res_len) return -1;
    
    unsigned_int128 carry = 0;
    for(size_t i=0; i<res_len; ++i) {
        long long r1 = (i < n) ? results[0][i] : 0;
        long long r2 = (i < n) ? results[1][i] : 0;
        long long r3 = (i < n) ? results[2][i] : 0;
        
        // x = (r1*C1 + r2*C2 + r3*C3) % M
        
        unsigned_int128 sum = (unsigned_int128)r1 * C1 + (unsigned_int128)r2 * C2 + (unsigned_int128)r3 * C3;
        
        // Estimate quotient q = floor(sum / M)
        // sum / M ~= r1 * (C1/M) + ... ~= r1 * D1 + ...
        double q_approx = r1 * D1 + r2 * D2 + r3 * D3;
        uint64_t q = (uint64_t)q_approx;
        
        unsigned_int128 val = sum - (unsigned_int128)q * M;
        
        // Correction for precision errors in float approximation
        if (val >= M) {
             if (val > ((unsigned_int128)1 << 120)) { // Underflow check (val became very large positive because it was negative)
                 val += M;
             } else {
                 val -= M;
                 if (val >= M) val -= M;
             }
        }

        val += carry;
        res->n[i] = (uint32_t)(val & 0xFFFFFFFF);
        carry = val >> 32;
    }
    
    size_t idx = res_len;
    while(carry) {
        if(idx >= res->asiz) {
             precn_realloc(res, idx + 10);
        }
        res->n[idx] = (uint32_t)(carry & 0xFFFFFFFF);
        carry >>= 32;
        idx++;
    }
    
    res->rsiz = idx;
    while(res->rsiz > 0 && res->n[res->rsiz - 1] == 0) {
        res->rsiz--;
    }
    
    return 0;
}

// FFT multiplication
int precn_mul_fft_complex(precn_t a, precn_t b, precn_t res) {
    uint64_t as = a->rsiz;
    uint64_t bs = b->rsiz;
    
    if (as == 0 || bs == 0) {
        precn_set(res, 0);
        return 0;
    }

    // Convert to 8-bit chunks to ensure precision in double
    // Each 32-bit word becomes 4 chunks
    // Max value in convolution: N * (2^8)^2 = N * 2^16
    // For N=2^20 (1M words), val = 2^36 << 2^53. Safe.
    size_t n = 1;
    while (n < 4 * (as + bs)) n <<= 1;
    
    std::vector<fft::Complex> fa(n), fb(n);
    
    for (size_t i = 0; i < as; ++i) {
        uint32_t val = a->n[i];
        fa[4*i]   = val & 0xFF;
        fa[4*i+1] = (val >> 8) & 0xFF;
        fa[4*i+2] = (val >> 16) & 0xFF;
        fa[4*i+3] = (val >> 24);
    }
    for (size_t i = 0; i < bs; ++i) {
        uint32_t val = b->n[i];
        fb[4*i]   = val & 0xFF;
        fb[4*i+1] = (val >> 8) & 0xFF;
        fb[4*i+2] = (val >> 16) & 0xFF;
        fb[4*i+3] = (val >> 24);
    }
    
    fft::fft(fa, false);
    fft::fft(fb, false);
    
    for (size_t i = 0; i < n; ++i) {
        fa[i] *= fb[i];
    }
    
    fft::fft(fa, true);
    
    // Reconstruct
    uint64_t res_len = as + bs;
    precn_realloc(res, res_len);
    if (res->asiz < res_len) return -1; // OOM
    
    uint64_t carry = 0;
    // We have 4 chunks per word of output roughly
    // But we need to reconstruct 32-bit words from 8-bit stream
    for (size_t i = 0; i < res_len * 4; ++i) {
        uint64_t val = (uint64_t)(fa[i].real() + 0.5) + carry;
        carry = val >> 8;
        uint32_t part = val & 0xFF;
        
        size_t word_idx = i / 4;
        size_t byte_idx = i % 4;
        
        if (byte_idx == 0) res->n[word_idx] = part;
        else res->n[word_idx] |= (part << (8 * byte_idx));
    }
    
    res->rsiz = res_len;
    while (res->rsiz > 0 && res->n[res->rsiz - 1] == 0) {
        res->rsiz--;
    }
    
    return 0;
}

// Karatsuba multiplcation (Pure)
int precn_mul_karatsuba(precn_t a, precn_t b, precn_t res){
  uint64_t as = a->rsiz;
  uint64_t bs = b->rsiz;
  
  if (as < bs) return precn_mul_karatsuba(b, a, res);
  // Now as >= bs

  if (bs < 32) {
    return precn_mul_slow(a, b, res);
  }
  
  uint64_t n = __max(as, bs);
  uint64_t m = (n + 1) / 2;
  
  precn_t a0 = precn_slice(a, 0, m);
  precn_t a1 = precn_slice(a, m, as > m ? as - m : 0);
  precn_t b0 = precn_slice(b, 0, m);
  precn_t b1 = precn_slice(b, m, bs > m ? bs - m : 0);
  
  precn_t z0 = precn_new(0);
  precn_t z2 = precn_new(0);
  precn_t z1 = precn_new(0);
  
  precn_mul_karatsuba(a0, b0, z0);
  precn_mul_karatsuba(a1, b1, z2);
  
  precn_t sum_a = precn_new(0);
  precn_add(a0, a1, sum_a);
  precn_t sum_b = precn_new(0);
  precn_add(b0, b1, sum_b);
  
  precn_t prod_sum = precn_new(0);
  precn_mul_karatsuba(sum_a, sum_b, prod_sum);
  
  precn_sub(prod_sum, z0, z1);
  precn_sub(z1, z2, z1);
  
  precn_set(res, z0);
  precn_add_shifted(res, z1, m);
  precn_add_shifted(res, z2, 2 * m);
  
  precn_free(a0); precn_free(a1);
  precn_free(b0); precn_free(b1);
  precn_free(z0); precn_free(z2); precn_free(z1);
  precn_free(sum_a); precn_free(sum_b);
  precn_free(prod_sum);
  
  return 0;
}

// Toom-Cook multiplication (with dispatch)
int precn_mul_toom_cook(precn_t a, precn_t b, precn_t res) {
    uint64_t as = a->rsiz;
    uint64_t bs = b->rsiz;

    if (as < bs) return precn_mul_toom_cook(b, a, res);
    
    // Dispatch to Toom-Cook for larger sizes
    if (bs > 1000) {

        // Toom-3 (3-3) - Balanced
        if (as * 2 < bs * 3) { // as < 1.5 bs
            toom_cook::toom3_3(a, b, res);
            return 0;
        }
        
        // Toom-2.3 (2-3) - Unbalanced (b=2 parts, a=3 parts)
        if (as < bs * 2) { // as < 2 bs
            toom_cook::toom2_3(b, a, res);
            return 0;
        }
        
        // Toom-2.4 (2-4) - More unbalanced (b=2 parts, a=4 parts)
        if (as < bs * 3) { // as < 3 bs
            toom_cook::toom2_4(b, a, res);
            return 0;
        }
    }

    // Fallback to Karatsuba
    return precn_mul_karatsuba(a, b, res);
}

// ==========================================================
// Division Implementations
// ==========================================================

// Bitwise Left Shift: res = a << bits
int precn_shift_left(precn_t res, precn_t a, uint64_t bits) {
    if (!a || !res) return -1;
    if (a->rsiz == 0) { precn_set(res, 0); return 0; }
    
    uint64_t wshift = bits / 32;
    uint32_t bshift = bits % 32;
    
    precn_t temp = precn_new(0);
    precn_realloc(temp, a->rsiz + wshift + 1);
    
    if (bshift == 0) {
        memcpy(temp->n + wshift, a->n, a->rsiz * sizeof(uint32_t));
        temp->rsiz = a->rsiz + wshift;
    } else {
        uint32_t carry = 0;
        for (uint64_t i = 0; i < a->rsiz; ++i) {
            uint64_t val = ((uint64_t)a->n[i] << bshift) | carry;
            temp->n[i + wshift] = (uint32_t)(val & 0xFFFFFFFF);
            carry = val >> 32;
        }
        if (carry) {
            temp->n[a->rsiz + wshift] = carry;
            temp->rsiz = a->rsiz + wshift + 1;
        } else {
            temp->rsiz = a->rsiz + wshift;
        }
    }
    
    memset(temp->n, 0, wshift * sizeof(uint32_t));
    
    while(temp->rsiz > 0 && temp->n[temp->rsiz-1] == 0) temp->rsiz--;
    
    precn_set(res, temp);
    precn_free(temp);
    return 0;
}

// Bitwise Right Shift: res = a >> bits
int precn_shift_right(precn_t res, precn_t a, uint64_t bits) {
    if (!a || !res) return -1;
    uint64_t wshift = bits / 32;
    uint32_t bshift = bits % 32;
    
    if (wshift >= a->rsiz) {
        precn_set(res, 0);
        return 0;
    }
    
    precn_t temp = precn_new(0);
    precn_realloc(temp, a->rsiz - wshift);
    
    if (bshift == 0) {
        memcpy(temp->n, a->n + wshift, (a->rsiz - wshift) * sizeof(uint32_t));
        temp->rsiz = a->rsiz - wshift;
    } else {
        for (uint64_t i = 0; i < a->rsiz - wshift; ++i) {
            uint64_t val = a->n[i + wshift];
            if (i + wshift + 1 < a->rsiz) {
                val |= ((uint64_t)a->n[i + wshift + 1] << 32);
            }
            temp->n[i] = (uint32_t)((val >> bshift) & 0xFFFFFFFF);
        }
        temp->rsiz = a->rsiz - wshift;
        while(temp->rsiz > 0 && temp->n[temp->rsiz-1] == 0) temp->rsiz--;
    }
    
    precn_set(res, temp);
    precn_free(temp);
    return 0;
}

// Schoolbook Division (Knuth Algorithm D)
int precn_div_slow(precn_t a, precn_t b, precn_t q, precn_t r) {
    if (!a || !b) return -1;
    if (b->rsiz == 0) return -1; 
    
    int cmp = precn_cmp(a, b);
    if (cmp < 0) {
        if (q) precn_set(q, 0);
        if (r) precn_set(r, a);
        return 0;
    }
    if (cmp == 0) {
        if (q) precn_set(q, 1);
        if (r) precn_set(r, 0);
        return 0;
    }
    
    // Normalize - Shift left so b's MSB >= 2^31
    uint32_t msb = b->n[b->rsiz - 1]; // Safe as rsiz > 0 and checked non-zero
    int shift = 0;
    while ((msb & 0x80000000) == 0) {
        msb <<= 1;
        shift++;
    }
    
    precn_t u = precn_new(0);
    precn_t v = precn_new(0);
    precn_shift_left(u, a, shift);
    precn_shift_left(v, b, shift);
    
    // Safety padding for u logic
    // We treat u as having an extra leading zero virtually if needed
    // m = size(u) - size(v)
    // u size roughly a->rsiz
    // v size roughly b->rsiz (same as n_len)
    
    uint64_t n_len = v->rsiz;
    uint64_t m_len = (u->rsiz >= n_len) ? (u->rsiz - n_len) : 0;
    
    // Ensure u buffer has space for u[u->rsiz] which is treated as 0
    precn_realloc(u, u->rsiz + 2);
    u->n[u->rsiz] = 0;
    u->n[u->rsiz+1] = 0;
    
    precn_t q_tmp = precn_new(0);
    precn_realloc(q_tmp, m_len + 2);
    q_tmp->rsiz = m_len + 1;
    memset(q_tmp->n, 0, (m_len + 2) * sizeof(uint32_t));
    
    uint64_t vn1 = v->n[n_len - 1];
    uint64_t vn2 = (n_len >= 2) ? v->n[n_len - 2] : 0;
    
    for (int64_t j = m_len; j >= 0; --j) {
        // qhat estimation
        // u[j+n] ... u[j+n-1]
        // Index mapping: u[j+n_len] is safe (might be the 0 pad)
        uint64_t high = u->n[j + n_len];
        uint64_t low = u->n[j + n_len - 1];
        uint64_t num = (high << 32) | low;
        
        uint64_t qhat = num / vn1;
        uint64_t rhat = num % vn1;
        
        while (1) {
             if (qhat >= 0x100000000ULL || 
                (qhat * vn2 > ((rhat << 32) | ((j + n_len >= 2) ? u->n[j + n_len - 2] : 0)))) {
                 qhat--;
                 rhat += vn1;
                 if (rhat >= 0x100000000ULL) break;
             } else {
                 break;
             }
        }
        
        // Multiply and subtract: u[j..] -= v * qhat
        uint64_t borrow = 0;
        uint64_t carry_mul = 0;
        for(uint64_t i=0; i<n_len; ++i) {
             uint64_t p = (uint64_t)v->n[i] * qhat + carry_mul;
             carry_mul = p >> 32;
             uint32_t val = (uint32_t)(p & 0xFFFFFFFF);
             
             int64_t diff = (int64_t)u->n[j+i] - val - borrow;
             if (diff < 0) {
                 diff += 0x100000000LL;
                 borrow = 1;
             } else {
                 borrow = 0;
             }
             u->n[j+i] = (uint32_t)diff;
        }
        
        int64_t diff = (int64_t)u->n[j+n_len] - carry_mul - borrow;
        u->n[j+n_len] = (uint32_t)(diff < 0 ? diff + 0x100000000LL : diff);
        
        if (diff < 0) {
             qhat--;
             uint64_t carry_add = 0;
             for(uint64_t i=0; i<n_len; ++i) {
                  uint64_t s = (uint64_t)u->n[j+i] + v->n[i] + carry_add;
                  u->n[j+i] = (uint32_t)(s & 0xFFFFFFFF);
                  carry_add = s >> 32;
             }
             u->n[j+n_len] += (uint32_t)carry_add;
        }
        
        q_tmp->n[j] = (uint32_t)qhat;
    }
    
    if (q) {
        precn_set(q, q_tmp);
        while(q->rsiz > 0 && q->n[q->rsiz - 1] == 0) q->rsiz--;
    }
    
    if (r) {
        u->rsiz = n_len;
        while(u->rsiz > 0 && u->n[u->rsiz - 1] == 0) u->rsiz--;
        precn_shift_right(r, u, shift);
    }
    
    precn_free(u); precn_free(v); precn_free(q_tmp);
    return 0;
}

// Automatic Multiplication Dispatch
int precn_mul_auto(precn_t a, precn_t b, precn_t res) {
    uint64_t n = a->rsiz;
    uint64_t m = b->rsiz;
    if (n == 0 || m == 0) {
        precn_set(res, 0);
        return 0;
    }
    
    // Ensure n >= m
    if (n < m) {
        uint64_t t = n; n = m; m = t;
    }

    // Cost Model Estimation (time in seconds)
    // Constants derived from benchmarks on the target machine
    
    // 1. Schoolbook: O(N*M)
    // Constant approx 1.5e-9
    double t_slow = 1.5e-9 * (double)n * (double)m;

    // 2. Polynomial (Karatsuba/Toom-Cook): O(N * M^0.6) approx for unbalanced
    // Balanced Toom-3 is N^1.46. Unbalanced decomposes to chunks.
    // Constant approx 2.5e-8
    double t_poly = 2.5e-8 * (double)n * pow((double)m, 0.6);

    // 3. NTT: O((N+M) * log(N+M))
    // Constant approx 4.5e-8
    double size_sum = (double)(n + m);
    double t_ntt = 4.5e-8 * size_sum * log2(size_sum);

    // Decision
    if (t_slow <= t_poly && t_slow <= t_ntt) {
        return precn_mul_slow(a, b, res);
    }
    
    // If Poly is faster than NTT, or if input is too small for NTT overhead consistency
    // Note: NTT has high constant overhead not fully captured by simple formula for very small N.
    // The intersection of Poly vs NTT for balanced is around 3000 words.
    // Check cost:
    if (t_poly <= t_ntt) {
        return precn_mul_toom_cook(a, b, res);
    }

    return precn_mul_ntt(a, b, res);
}
int precn_mul(precn_t a, precn_t b, precn_t res) {
    return precn_mul_auto(a, b, res);
}
// Newton-Raphson Division Helper
// Computes x approx floor(2^(2*n) / b)
// b must be normalized (MSB set) of size n words
void precn_inv_recursive(precn_t b, precn_t x) {
    uint64_t n = b->rsiz;
    
    // Base Case
    if (n <= 2) {
        precn_t dividend = precn_new(0);
        precn_realloc(dividend, 2*n + 1);
        memset(dividend->n, 0, (2*n + 1)*sizeof(uint32_t));
        dividend->n[2*n] = 1; // 2^(2n)
        dividend->rsiz = 2*n + 1;
        
        precn_div_slow(dividend, b, x, NULL);
        precn_free(dividend);
        return;
    }
    
    // Recursive Step
    // k = ceil(n/2)
    uint64_t k = (n + 1) / 2;
    
    // b_high = b >> (n-k)*32 (Top k words)
    precn_t b_high = precn_new(0);
    precn_shift_right(b_high, b, (n - k) * 32);
    
    precn_t x_high = precn_new(0);
    precn_inv_recursive(b_high, x_high); 
    
    // We have x_high approx 2^(2k) / b_high
    // We want x approx 2^(2n) / b
    // Newton iteration: x_new = x_old * (2 - b * x_old)
    
    // Scaling:
    // x_high scale is 2^(2k) / 2^((n-k) + k) approx 2^k ? no.
    // Let's perform iteration using full precision integers for simplicity of logic
    // but mindful of sizes.
    
    // Initial guess X0 = X_high << (n-k words)
    // Actually we need to match bit positions.
    // X_high satisfies B_high * X_high approx 2^(2k).
    // B approx B_high * 2^(32(n-k)).
    // We want B * X approx 2^(2n).
    // B_high * 2^(32(n-k)) * X approx 2^(2n).
    // B_high * X approx 2^(2n - 32(n-k)).
    // B_high * X approx 2^(2n - 32n + 32k) = 2^(32k - 32n + 2n)?? bits vs words mixup.
    
    // Let word width W=32.
    // B approx B_high * W^(n-k).
    // B_high * X_high approx W^(2k).
    // We want B * X approx W^(2n).
    // B_high * W^(n-k) * X approx W^(2n).
    // B_high * X approx W^(2n - n + k) = W^(n+k).
    // X should be X_high * W^(n+k - 2k) = X_high * W^(n-k).
    // So X0 = X_high << 32*(n-k).
    
    precn_t x0 = precn_new(0);
    precn_shift_left(x0, x_high, (n - k) * 32);
    
    // Iteration: X = X0 + X0(2^(2n) - B*X0) / 2^(2n)
    // Note: B*X0 might be > 2^(2n) or < 2^(2n).
    
    // Compute B*X0
    precn_t bx0 = precn_new(0);
    precn_mul_auto(b, x0, bx0);
    
    // Compute Target 2^(2n)
    precn_t target = precn_new(0);
    precn_realloc(target, 2*n + 2);
    memset(target->n, 0, (2*n + 2)*sizeof(uint32_t));
    target->n[2*n] = 1;
    target->rsiz = 2*n + 1;
    
    // Diff
    precn_t diff = precn_new(0);
    bool over = false;
    
    if (precn_cmp(bx0, target) > 0) {
        precn_sub(bx0, target, diff); // bx0 - 2^2n
        over = true;
    } else {
        precn_sub(target, bx0, diff); // 2^2n - bx0
        over = false;
    }
    
    // Term = X0 * diff
    precn_t term = precn_new(0);
    precn_mul_auto(x0, diff, term);
    
    // Delta = Term >> 2n words (actually 2n bits? No, 2n words? No, scaling is 2^(2n))
    // We divided by 2^(2n) in formula. So shift right by 2*n words (64n bits).
    
    precn_t delta = precn_new(0);
    precn_shift_right(delta, term, 2 * n * 32); 
    
    if (over) {
        // X = X0 - Delta
        if (precn_cmp(x0, delta) >= 0) {
            precn_sub(x0, delta, x);
        } else {
            // Should not happen if convergence is stable
             precn_set(x, 0); // Fail safe
        }
    } else {
        // X = X0 + Delta
        precn_add(x0, delta, x);
    }
    
    precn_free(b_high); precn_free(x_high); precn_free(x0);
    precn_free(bx0); precn_free(target); precn_free(diff);
    precn_free(term); precn_free(delta);
}

int precn_div_newton(precn_t a, precn_t b, precn_t q, precn_t r) {
    if (!a || !b || b->rsiz == 0) return -1;
    
    // Fallback for very small
    if (b->rsiz < 32) return precn_div_slow(a, b, q, r);
    
    // Normalize
    uint32_t msb = b->n[b->rsiz - 1];
    int shift = 0;
    while ((msb & 0x80000000) == 0) {
        msb <<= 1;
        shift++;
    }
    
    precn_t b_norm = precn_new(0);
    precn_shift_left(b_norm, b, shift);
    precn_t a_norm = precn_new(0);
    precn_shift_left(a_norm, a, shift);
    
    // Inv
    precn_t inv = precn_new(0);
    precn_inv_recursive(b_norm, inv);
    
    // Q approx (A * Inv) >> 2n
    uint64_t n = b_norm->rsiz; // Size of B
    // Q size approx size(A) - size(B)
    
    precn_t prod = precn_new(0);
    precn_mul_auto(a_norm, inv, prod);
    
    precn_t q_est = precn_new(0);
    precn_shift_right(q_est, prod, 2 * n * 32);
    
    // Rem = A - B*Q
    precn_t bq = precn_new(0);
    precn_mul_auto(b, q_est, bq); // Use Original B for check
    
    precn_t rem = precn_new(0);
    
    // Correction with Fail-Safe
    // If mismatch is huge, use slow div to fix
    
    // Compute mismatch
    // If Overestimate: BQ > A
    if (precn_cmp(bq, a) > 0) {
        precn_t diff = precn_new(0);
        precn_sub(bq, a, diff);
        
        precn_t k = precn_new(0);
        precn_t r_waste = precn_new(0);
        
        // k = ceil(diff / B)
        // If diff is huge, simple subtraction loop is infinite.
        // Use slow div here. Correctness guaranteed.
        precn_div_slow(diff, b, k, r_waste);
        
        if (r_waste->rsiz > 0) {
             // ceil adjustment
             precn_add_shifted(k, precn_new(0), 0); // hack to ensure alloc?
             // k++
             precn_t one = precn_new(0); precn_set(one, 1);
             precn_t tmp = precn_new(0); precn_add(k, one, tmp); precn_set(k, tmp);
             precn_free(one); precn_free(tmp);
        }
        
        precn_t q_new = precn_new(0);
        precn_sub(q_est, k, q_new);
        precn_set(q_est, q_new);
        
        // Recalc Rem
        precn_free(bq);
        bq = precn_new(0);
        precn_mul_toom_cook(b, q_est, bq);
        precn_sub(a, bq, rem);
        
        precn_free(diff); precn_free(k); precn_free(r_waste);
        precn_free(q_new);
    } else {
        // Underestimate: BQ <= A. Rem = A - BQ.
        precn_sub(a, bq, rem);
        if (precn_cmp(rem, b) >= 0) {
             precn_t k = precn_new(0);
             precn_t r_new = precn_new(0);
             
             // k = rem / b
             precn_div_slow(rem, b, k, r_new);
             
             precn_t q_new = precn_new(0);
             precn_add(q_est, k, q_new);
             precn_set(q_est, q_new);
             precn_set(rem, r_new);
             
             precn_free(k); precn_free(r_new); precn_free(q_new);
        }
    }
    
    if (q) precn_set(q, q_est);
    if (r) precn_set(r, rem);
    
    precn_free(b_norm); precn_free(a_norm); precn_free(inv);
    precn_free(prod); precn_free(q_est); precn_free(bq);
    precn_free(rem);
    return 0;
}


// Wrapper
int precn_div(precn_t a, precn_t b, precn_t q, precn_t r) {
    if (b->rsiz > 128) {
        return precn_div_newton(a, b, q, r);
    }
    return precn_div_slow(a, b, q, r);
}

// Modulo: res = a % b
int precn_mod(precn_t a, precn_t b, precn_t res) {
    precn_t q = precn_new(0);
    precn_div(a, b, NULL, res);
    precn_free(q);
    return 0;
}

// Integer Square Root: res = floor(sqrt(a))
// Optimized using Inverse Square Root Newton Iteration (Division-Free Loop)
// x_{k+1} = x_k * (3 - a * x_k^2) / 2   (Converges to 1/sqrt(a))
// Sqrt(a) = a * x_final
int precn_sqrt(precn_t a, precn_t res) {
    if (!a || !res) return -1;
    if (a->rsiz == 0) { precn_set(res, 0); return 0; }
    
    // 1. Normalize A to have MSB at nice boundary or effectively fixed point?
    // We treat A as integer.
    // 1/sqrt(A) is fractional.
    // We need fixed point math.
    // Let's solve for integer X such that X approx 1/sqrt(A) * 2^K.
    
    // Easier approach for Integer SQRT of big integer N:
    // Standard Newton on Square Root: x = (x + N/x)/2.
    // This is slow due to division.
    
    // Inverse Sqrt approach:
    // We want X ~ 2^k / sqrt(N).
    // Let N be our input 'a'.
    // Let target precision be bit length L.
    // We compute InvSqrt to accuracy L.
    
    // Estimate bits of a
    uint32_t msb = a->n[a->rsiz - 1];
    int lz = 0;
    while ((msb & ((uint32_t)0x80000000 >> lz)) == 0 && lz < 32) lz++;
    int64_t a_bits = (uint64_t)a->rsiz * 32 - lz;
    
    // We want result sqrt(a) which has bits_res = ceil(a_bits / 2).
    int64_t res_bits = (a_bits + 1) / 2;
    
    // If small, use simple method or just standard float estimate
    if (a->rsiz <= 2) {
        double d = 0;
        if (a->rsiz == 1) d = a->n[0];
        else d = (double)a->n[1] * 4294967296.0 + a->n[0];
        uint64_t s = (uint64_t)sqrt(d);
        precn_set(res, s);
        return 0;
    }
    
    // Use Coupled Newton Iteration for 1/sqrt(A):
    // Let X approx 1/sqrt(A).
    // Newton: X' = X/2 * (3 - A*X^2).
    // But we are working with Integers.
    // Scaling:
    // Let A_norm = A. (We treat as fractional 0.A_norm in some virtual point?)
    // No, standard integer scaling.
    // We want X_final * X_final * A approx 2^(OutputScale).
    
    // Alternative:
    // N_sqrt approximation using Division-based Newton but with increasing precision?
    // "Newton with dynamic precision" is the standard trick.
    // Start with 1 word, then 2, 4, 8...
    // Division at size K is expensive, but we only do it constant times.
    // Total time T(N) = Div(N) + Div(N/2) + ... = 2 Div(N).
    // The previous implementation was doing Div(N) * Log(N) times!
    
    // Let's implement Dynamic Precision Newton Sqrt (Division based is fine if dynamic).
    // X_{k+1} = (X_k + A / X_k) / 2
    // But computed at precision required for step k.
    
    // Initial guess: 32-bit hardware sqrt
    // Get top 64 bits of A
    // (If fewer, handled by small case above)
    // Extract high 64 bits for estimation
    // Need to shift 'a' to get top 64 bits.
    
    // Shift amount to align MSB of A to bit 63?
    // Actually just take top 2 words.
    // If a->rsiz is large, we want top roughly.
    
    // Simple start guess:
    // We want sqrt(A). Target bits: res_bits.
    // We start with guess of length... 1 word?
    // Let's just do recursive Sqrt?
    // Sqrt(A) = Sqrt(A_high) << shift.
    // Sqrt(A) ~ Sqrt(A >> 2k) << k
    
    // Recursive Sqrt Implementation:
    // 1. If A is small (<= 2 words), use hardware.
    // 2. Normalize A so even bit length? (Optional)
    // 3. Let 'half' = A >> (2 * floor(bits/4)).  (Quarters?)
    //    Actually split A into High/Low.
    //    We want X = Sqrt(A).
    //    Let k be approx bits/2.
    //    Let A_high = A >> 2k.
    //    X_high = Sqrt(A_high).
    //    Guess X0 = X_high << k.
    //    Refine X = (X0 + A/X0)/2.
    //    This is ONE iter of Newton.
    //    If X_high is accurate to m bits, X is accurate to 2m bits.
    //    So we recursively match precision!
    
    // Recursive function not needed. Loop up from small size.
    // Size sequence: 64 bits -> 128 -> 256 ... -> full.
    
    // Top 64 bits estimation
    // We need the 'scale' (shift) such that we process top words.
    // Loop variable 'current_bits' doubles until >= res_bits.
    
    // Determine shift for top 64 bits of A
    // We want to construct precn 'start' from top words of A.
    // We need roughly 2 words of A at MSB.
    
    // Let's pick a starting size.
    uint64_t start_shift_bits = 0;
    if (a_bits > 64) start_shift_bits = a_bits - 64;
    // Align to even shift for square root property Sqrt(A * 2^2k) = Sqrt(A) * 2^k
    start_shift_bits &= ~1ULL; // Ensure even
    
    precn_t top = precn_new(0);
    precn_shift_right(res, a, start_shift_bits); // Use 'res' as temp storage for top
    precn_set(top, res);
    
    // Hardware sqrt on top (approx)
    double d = 0;
    if (top->rsiz > 0) d += top->n[0];
    if (top->rsiz > 1) d += (double)top->n[1] * 4294967296.0;
    if (top->rsiz > 2) d += (double)top->n[2] * 18446744073709551616.0; // Rare if 64 bits
    
    uint64_t hw_sqrt = (uint64_t)sqrt(d);
    precn_set(res, hw_sqrt);
    
    precn_free(top);
    
    // Current precision of 'res' relative to 'a'
    // 'res' corresponds to sqrt(a >> start_shift_bits).
    // The "exponent" of 'res' is effectively 'start_shift_bits / 2'.
    
    int64_t current_shift = start_shift_bits; // 2k
    
    precn_t t = precn_new(0);
    precn_t sum_x = precn_new(0);
    precn_t a_shifted = precn_new(0);
    
    while (current_shift > 0) {
        // We have guess 'res' for A >> current_shift.
        // We want next guess for A >> next_shift.
        // Where next_shift approx current_shift / 2.
        
        int64_t next_shift = current_shift / 2;
        // Align even
        next_shift &= ~1ULL;
        
        // Scale 'res' up to match new domain
        // res_new_guess = res << (current_shift - next_shift)/2
        int64_t shift_diff = current_shift - next_shift;
        precn_shift_left(res, res, shift_diff / 2);
        
        // Newton Step: x = (x + (A >> next_shift) / x) / 2
        // We need (A >> next_shift).
        precn_shift_right(a_shifted, a, next_shift);
        
        // t = a_shifted / res
        precn_div(a_shifted, res, t, NULL);
        
        // x = (res + t) / 2
        precn_add(res, t, sum_x);
        precn_shift_right(res, sum_x, 1);
        
        current_shift = next_shift;
    }
    
    // Final cleanup iteration at full precision to handle last bit errors?
    // The loop ends when current_shift == 0.
    // Usually one more iteration is good policy for perfect integer sqrt.
    
    // t = a / res
    precn_div(a, res, t, NULL);
    precn_add(res, t, sum_x);
    precn_shift_right(res, sum_x, 1);
    
    precn_free(t); precn_free(sum_x); precn_free(a_shifted);
    return 0;
}

// ==========================================================
// String Conversion (Base 10) - O(N log^2 N)
// ==========================================================

// Helpers for Fast Base Conversion
static std::vector<precn_t> g_pow10_cache;

// Ensure powers of 10 exist up to 10^(9 * 2^k) covering 'bits' size
// Each entry i stores 10^(9 * 2^i)
// i=0: 10^9
// i=1: 10^18
// ...
void _precn_ensure_powers(uint64_t bits) {
    // 10^9 is approx 2^29.9
    // 10^(9 * 2^k) approx 2^(30 * 2^k)
    // We need 30 * 2^k >= bits
    
    if (g_pow10_cache.empty()) {
        g_pow10_cache.push_back(precn_new(1000000000));
    }
    
    // Last power
    while (true) {
        precn_t last = g_pow10_cache.back();
        // Check size roughly
        uint64_t est_bits = (uint64_t)last->rsiz * 32;
        if (est_bits > bits + 64) break; // Enough coverage
        
        precn_t next = precn_new(0);
        precn_mul_auto(last, last, next);
        g_pow10_cache.push_back(next);
    }
}

// Helper to convert exactly, returning string
std::string _precn_to_str_inner(precn_t a, int level) {
    if (level < 0) { // Base: < 10^9
        uint32_t val = (a->rsiz > 0) ? a->n[0] : 0; 
        return std::to_string(val);
    }
    
    precn_t p = g_pow10_cache[level];
    
    if (precn_cmp(a, p) < 0) {
       return _precn_to_str_inner(a, level - 1);
    }
    
    precn_t q = precn_new(0);
    precn_t r = precn_new(0);
    precn_div(a, p, q, r);
    
    std::string s_q = _precn_to_str_inner(q, level - 1);
    std::string s_r = _precn_to_str_inner(r, level - 1);
    
    precn_free(q); precn_free(r);
    
    // Pad R
    size_t expected = 9 * (1ULL << level);
    if (s_r.length() < expected) {
        std::string pad(expected - s_r.length(), '0');
        return s_q + pad + s_r;
    }
    return s_q + s_r;
}

char* precn_to_str(precn_t a) {
    if (!a || a->rsiz == 0) {
        char* s = (char*)malloc(2); strcpy(s, "0"); return s;
    }
    
    _precn_ensure_powers(a->rsiz * 32);
    
    // Find starting level
    int level = 0;
    while ((size_t)level < g_pow10_cache.size() && precn_cmp(a, g_pow10_cache[level]) >= 0) {
        level++;
    }
    // Now a < powers[level], so we split using powers[level-1]
    
    std::string res = _precn_to_str_inner(a, level - 1);
    
    char* ret = (char*)malloc(res.length() + 1);
    strcpy(ret, res.c_str());
    return ret;
}

// Recursive Parse
// str is substring
void _precn_from_str_rec(precn_t res, const char* str, size_t len, int level) {
    // Base Case
    if (len <= 9) { // Single chunk
        if (len == 0) { precn_set(res, 0); return; }
        std::string s(str, len);
        uint32_t val = (uint32_t)strtoul(s.c_str(), NULL, 10);
        precn_set(res, val);
        return;
    }
    
    // Standard D&C:
    // val = High * 10^k + Low
    // Split at 'right' side to align with powers of 10.
    // Let's use the largest power available that divides the string.
    
    // Find largest k such that 9*2^k < len
    int k = 0;
    while ((9ULL << (k+1)) < len) k++;
    
    size_t split_len = 9 * (1ULL << k); // Length of Low part
    size_t high_len = len - split_len;
    
    precn_t high = precn_new(0);
    precn_t low = precn_new(0);
    
    _precn_from_str_rec(high, str, high_len, k); // Recurse
    _precn_from_str_rec(low, str + high_len, split_len, k);
    
    // Res = High * Pow + Low
    _precn_ensure_powers(len * 4); // ample bits
    
    // powers[k] is 10^(9*2^k), which matches split_len
    precn_t p = g_pow10_cache[k]; 
    
    precn_t term = precn_new(0);
    precn_mul_auto(high, p, term);
    
    precn_add(term, low, res);
    
    precn_free(high); precn_free(low); precn_free(term);
}

int precn_from_str(precn_t res, const char* str) {
    if (!res || !str) return -1;
    size_t len = strlen(str);
    if (len == 0) { precn_set(res, 0); return 0; }
    
    _precn_ensure_powers(len * 4); // 3.32 bits per digit is safe upper bound
    _precn_from_str_rec(res, str, len, 0);
    return 0;
}

// ==========================================================
// Arbitrary Base Conversion - O(N log^2 N)
// ==========================================================

static std::map<int, std::vector<precn_t>> g_base_powers;

// Get chunk size (number of digits in base that fit in u32)
inline int _precn_base_chunk(int base) {
    if (base < 2) return 0;
    // Max power < 2^32
    uint64_t v = base;
    int k = 1;
    while (v * base < 0xFFFFFFFFULL) {
        v *= base;
        k++;
    }
    return k;
}

void _precn_ensure_base_powers(int base, uint64_t bits) {
    auto& cache = g_base_powers[base];
    if (cache.empty()) {
        int k = _precn_base_chunk(base);
        // Compute base^k
        uint64_t v = 1; 
        for(int i=0; i<k; ++i) v *= base;
        cache.push_back(precn_new(v));
    }
    
    while (true) {
        precn_t last = cache.back();
        uint64_t est_bits = (uint64_t)last->rsiz * 32;
        if (est_bits > bits + 64) break;
        
        precn_t next = precn_new(0);
        precn_mul_auto(last, last, next);
        cache.push_back(next);
    }
}

// Digits are written to *out, returns number of digits written
// output order: Little Endian? or Big Endian? 
// Standard expected is usually Big Endian (MSD first) for printing.
// But for array access, [0] as LSD is easier math?
// Let's stick to Big Endian [0] = MSD, to match string convention.
// This matches standard "base representation".
void _precn_to_base_rec(precn_t a, int base, int level, std::vector<int>& out) {
    if (a->rsiz == 0) {
        // Zero
        return; 
    }
    
    if (level < 0) {
        // Fits in u32 chunk
        int k = _precn_base_chunk(base);
        uint32_t val = (a->rsiz > 0) ? a->n[0] : 0;
        
        // Extract digits. Wait, order?
        // We want specific number of digits 'k', usually? 
        // Or just whatever digits 'val' has?
        // Inner blocks typically need PADDING.
        // We should handle padding at parent level or produce fixed size?
        
        std::vector<int> tmp;
        if (val == 0) {
             // 0
        } else {
            while (val > 0) {
                tmp.push_back(val % base);
                val /= base;
            }
        }
        // tmp has LSD first. Reverse for MSD first.
        for(int i=(int)tmp.size()-1; i>=0; --i) out.push_back(tmp[i]);
        return;
    }
    
    auto& cache = g_base_powers[base];
    if ((size_t)level >= cache.size()) return; // Should not happen
    
    precn_t p = cache[level];
    
    // a = q * p + r
    // If a < p, high part is 0.
    if (precn_cmp(a, p) < 0) {
        // High is 0. Just recurse on Low (a).
        // BUT we need padding for High if this is not the root.
        // The recursion D&C logic: High Part, then Low Part.
        // If High Part is 0, we output nothing? 
        // Only if we are at Root. If valid inner node, High Part 0 means '000...'
        // Handling padding inside recursion is messy.
        // Simpler: _precn_to_base_rec returns digits. Caller pads.
        
        // Let's enforce: inner levels must produce exact # digits? 
        // The chunk size at level 'level' is:
        // k * 2^(level+1)? No.
        // Cache[0]: chunk_k digits.
        // Cache[level]: chunk_k * 2^level digits.
        
        // We assume this function is only called for 'a'. 
        // Padding is handled by wrapper if needed? 
        // Here, we just output 'a'.
        _precn_to_base_rec(a, base, level - 1, out);
        return;
    }
    
    precn_t q = precn_new(0);
    precn_t r = precn_new(0);
    precn_div(a, p, q, r);
    
    _precn_to_base_rec(q, base, level - 1, out);
    
    // For R, we MUST capture output and pad to left.
    size_t start_idx = out.size();
    _precn_to_base_rec(r, base, level - 1, out);
    
    // Expected digits for R block:
    int k = _precn_base_chunk(base);
    size_t expected_r_len = (size_t)k * (1ULL << level);
    size_t produced = out.size() - start_idx;
    
    if (produced < expected_r_len) {
        // Insert zeros at start_idx
        size_t pad = expected_r_len - produced;
        out.insert(out.begin() + start_idx, pad, 0);
    }
    
    precn_free(q); precn_free(r);
}

int* precn_to_base(precn_t a, size_t* out_len, int base) {
    if (!a || base < 2) return NULL;
    
    if (a->rsiz == 0) {
        if (out_len) *out_len = 1;
        int* res = (int*)malloc(sizeof(int));
        res[0] = 0;
        return res;
    }

    _precn_ensure_base_powers(base, a->rsiz * 32);
    auto& cache = g_base_powers[base];
    
    int level = 0;
    while ((size_t)level < cache.size() && precn_cmp(a, cache[level]) >= 0) {
        level++;
    }
    
    std::vector<int> buf;
    _precn_to_base_rec(a, base, level - 1, buf);
    
    if (buf.empty()) buf.push_back(0);

    if (out_len) *out_len = buf.size();
    int* res = (int*)malloc(buf.size() * sizeof(int));
    for(size_t i=0; i<buf.size(); ++i) res[i] = buf[i];
    return res;
}

void _precn_from_base_rec(precn_t res, const int* digits, size_t len, int base, int level) {
    int k_chunk = _precn_base_chunk(base);
    
    if (len <= (size_t)k_chunk) {
        // Base case: Convert directly
        uint64_t val = 0;
        for(size_t i=0; i<len; ++i) {
            val = val * base + digits[i];
        }
        precn_set(res, val);
        return;
    }
    
    // Split
    // Find largest power level 'p' such that capacity(p) <= len/2 roughly?
    // Actually we want capacity(level) < len?
    // Cache[level] capacity is k * 2^level digits.
    // We want split point aligned.
    
    // Find largest k such that chunk * 2^k < len
    int k = 0;
    while ( ((unsigned long long)k_chunk << (k+1)) < len ) k++;
    
    size_t split_capacity = (size_t)k_chunk * (1ULL << k);
    
    size_t low_len = split_capacity;
    size_t high_len = len - low_len;
    
    precn_t high = precn_new(0);
    precn_t low = precn_new(0);
    
    _precn_from_base_rec(high, digits, high_len, base, k);
    _precn_from_base_rec(low, digits + high_len, low_len, base, k);
    
    _precn_ensure_base_powers(base, len * 32); // Approximate
    precn_t p = g_base_powers[base][k];
    
    precn_t term = precn_new(0);
    precn_mul_auto(high, p, term);
    precn_add(term, low, res);
    
    precn_free(high); precn_free(low); precn_free(term);
}

int precn_from_base(precn_t res, const int* digits, size_t len, int base) {
     if (!res || !digits || len == 0) { precn_set(res, 0); return 0; }
     if (base < 2) return -1;
     
     _precn_ensure_base_powers(base, len * 16); // Safe bet
     _precn_from_base_rec(res, digits, len, base, 0);
     return 0;
}

}

 /* namespace precn_impl */
#endif /* PREC_HPP */
