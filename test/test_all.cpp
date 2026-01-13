#include <iostream>
#include <sstream>
#include <iomanip>
#include <cassert>
#include <vector>
#include <random>
#include "../prec.hpp"

using namespace precn_impl;

static std::string to_hex(precn_t p) {
  if (p == NULL || p->rsiz == 0) return "0";
  std::ostringstream oss;
  oss << std::hex;
  for (int i = (int)p->rsiz - 1; i >= 0; --i) {
    if (i == (int)p->rsiz - 1)
      oss << p->n[i];
    else
      oss << std::setw(8) << std::setfill('0') << p->n[i];
  }
  return oss.str();
}

int main(){
  int failures = 0;
  std::cout << "Starting comprehensive tests..." << std::endl;

  // --- Test precn_new and precn_free ---
  {
    precn_t p1 = precn_new(12345);
    if (!p1) { std::cerr << "precn_new failed" << std::endl; failures++; }
    else {
        if (to_hex(p1) != "3039") { std::cerr << "precn_new(12345) failed. Got " << to_hex(p1) << std::endl; failures++; }
        precn_free(p1);
    }

    precn_t p2 = precn_new(-1); // Should be 0
    if (!p2) { std::cerr << "precn_new failed" << std::endl; failures++; }
    else {
        if (to_hex(p2) != "0") { std::cerr << "precn_new(-1) failed. Got " << to_hex(p2) << std::endl; failures++; }
        precn_free(p2);
    }
    
    precn_t p3 = precn_new(0x1234567890ABCDEFULL);
    if (!p3) { std::cerr << "precn_new failed" << std::endl; failures++; }
    else {
        if (to_hex(p3) != "1234567890abcdef") { std::cerr << "precn_new(large) failed. Got " << to_hex(p3) << std::endl; failures++; }
        precn_free(p3);
    }
  }

  // --- Test precn_set ---
  {
      precn_t p1 = precn_new(0);
      precn_set(p1, 12345);
      if (to_hex(p1) != "3039") { std::cerr << "precn_set(int) failed. Got " << to_hex(p1) << std::endl; failures++; }

      precn_t p2 = precn_new(0);
      precn_set(p2, p1);
      if (to_hex(p2) != "3039") { std::cerr << "precn_set(precn_t) failed. Got " << to_hex(p2) << std::endl; failures++; }
      
      precn_free(p1);
      precn_free(p2);
  }

  // --- Test precn_cmp ---
  {
      precn_t p1 = precn_new(100);
      precn_t p2 = precn_new(100);
      precn_t p3 = precn_new(200);
      precn_t p4 = precn_new(50);

      if (precn_cmp(p1, p2) != 0) { std::cerr << "precn_cmp(equal) failed" << std::endl; failures++; }
      if (precn_cmp(p1, p3) != -1) { std::cerr << "precn_cmp(less) failed" << std::endl; failures++; }
      if (precn_cmp(p1, p4) != 1) { std::cerr << "precn_cmp(greater) failed" << std::endl; failures++; }

      precn_free(p1); precn_free(p2); precn_free(p3); precn_free(p4);
  }

  // --- Test precn_add ---
  {
      precn_t a = precn_new(100);
      precn_t b = precn_new(200);
      precn_t res = precn_new(0);

      if (precn_add(a, b, res) != 0) { std::cerr << "precn_add failed" << std::endl; failures++; }
      if (to_hex(res) != "12c") { std::cerr << "precn_add(100, 200) failed. Got " << to_hex(res) << std::endl; failures++; } // 300 = 0x12c

      // Test carry
      precn_set(a, 0xFFFFFFFF);
      precn_set(b, 1);
      precn_add(a, b, res);
      if (to_hex(res) != "100000000") { std::cerr << "precn_add(carry) failed. Got " << to_hex(res) << std::endl; failures++; }

      precn_free(a); precn_free(b); precn_free(res);
  }

  // --- Test precn_sub ---
  {
      precn_t a = precn_new(300);
      precn_t b = precn_new(100);
      precn_t res = precn_new(0);

      if (precn_sub(a, b, res) != 0) { std::cerr << "precn_sub failed" << std::endl; failures++; }
      if (to_hex(res) != "c8") { std::cerr << "precn_sub(300, 100) failed. Got " << to_hex(res) << std::endl; failures++; } // 200 = 0xc8

      // Test borrow
      precn_set(a, 0x100000000ULL);
      precn_set(b, 1);
      precn_sub(a, b, res);
      if (to_hex(res) != "ffffffff") { std::cerr << "precn_sub(borrow) failed. Got " << to_hex(res) << std::endl; failures++; }

      // Test negative result error
      precn_set(a, 100);
      precn_set(b, 200);
      if (precn_sub(a, b, res) != -1) { std::cerr << "precn_sub(negative) failed to return error" << std::endl; failures++; }

      precn_free(a); precn_free(b); precn_free(res);
  }

  // --- Test precn_mul_u32 ---
  {
      precn_t a = precn_new(12345);
      if (precn_mul_u32(a, 0) != 0) { std::cerr << "precn_mul_u32 failed" << std::endl; failures++; }
      if (to_hex(a) != "0") { std::cerr << "precn_mul_u32(0) failed. Got " << to_hex(a) << std::endl; failures++; }

      precn_set(a, 12345);
      precn_mul_u32(a, 1);
      if (to_hex(a) != "3039") { std::cerr << "precn_mul_u32(1) failed. Got " << to_hex(a) << std::endl; failures++; }

      precn_set(a, 2);
      precn_mul_u32(a, 0xFFFFFFFF);
      if (to_hex(a) != "1fffffffe") { std::cerr << "precn_mul_u32(large) failed. Got " << to_hex(a) << std::endl; failures++; }

      precn_free(a);
  }

  // --- Test random add/sub ---
  {
      std::mt19937_64 rng(12345);
      std::uniform_int_distribution<uint32_t> dist_u32;
      std::uniform_int_distribution<int> dist_size(1, 9999999);

      auto make_random = [&](int size) {
          precn_t p = __precn_new_size(size, size);
          for(int i=0; i<size; ++i) {
              p->n[i] = dist_u32(rng);
          }
          // Normalize rsiz
          while(p->rsiz > 0 && p->n[p->rsiz-1] == 0) {
              p->rsiz--;
          }
          return p;
      };

      for (int i = 0; i < 100; ++i) {
          precn_t a = make_random(dist_size(rng));
          precn_t b = make_random(dist_size(rng));
          precn_t c = precn_new(0);
          precn_t d = precn_new(0);

          // c = a + b
          if (precn_add(a, b, c) != 0) { std::cerr << "precn_add failed in random test" << std::endl; failures++; }
          
          // d = c - b
          if (precn_sub(c, b, d) != 0) { std::cerr << "precn_sub failed in random test" << std::endl; failures++; }

          // check d == a
          if (precn_cmp(d, a) != 0) {
              std::cerr << "Random test failed: (a + b) - b != a" << std::endl;
              std::cerr << "a size: " << a->rsiz << std::endl;
              std::cerr << "b size: " << b->rsiz << std::endl;
              failures++;
          }

          precn_free(a); precn_free(b); precn_free(c); precn_free(d);
      }
  }

  if (failures == 0) std::cout << "All tests passed!" << std::endl;
  else std::cout << failures << " tests failed." << std::endl;

  return failures > 0 ? 1 : 0;
}
