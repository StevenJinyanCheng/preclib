#include <iostream>
#include <sstream>
#include <iomanip>
#include <cassert>
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

  // Test 1: multiply by 0 -> result zero
  precn_t a = precn_new<uint64_t>(12345);
  if (!a) { std::cerr<<"alloc failed"<<std::endl; return 2; }
  if (precn_mul_u32(a, 0) != 0) { std::cerr<<"mul returned error"<<std::endl; failures++; }
  if (a->rsiz != 0) { std::cerr<<"Test1 failed: expected rsiz 0 got "<<a->rsiz<<"\n"; failures++; }
  std::cout<<"Test1 multiply by 0 => "<<to_hex(a)<<"\n";

  // Test 2: multiply by 1 -> unchanged
  precn_set(a, (uint64_t)0x12345678);
  if (precn_mul_u32(a, 1) != 0) { std::cerr<<"mul returned error"<<std::endl; failures++; }
  if (to_hex(a) != "12345678") { std::cerr<<"Test2 failed: expected 12345678 got "<<to_hex(a)<<"\n"; failures++; }
  std::cout<<"Test2 multiply by 1 => "<<to_hex(a)<<"\n";

  // Test 3: multiply small value by 0xFFFFFFFF
  precn_set(a, (uint64_t)0x2);
  if (precn_mul_u32(a, 0xFFFFFFFFu) != 0) { std::cerr<<"mul returned error"<<std::endl; failures++; }
  // expected 0x1FFFFFFFE -> hex: 1fffffffe
  std::cout<<"Test3 multiply 2 * 0xFFFFFFFF => "<<to_hex(a)<<"\n";

  // Test 4: carry propagation: (1<<32) * 2 = 0x2_00000000
  precn_set(a, (uint64_t)0x100000000ull);
  if (precn_mul_u32(a, 2) != 0) { std::cerr<<"mul returned error"<<std::endl; failures++; }
  std::cout<<"Test4 (2^32)*2 => "<<to_hex(a)<<"\n";

  // Test 5: precn_mul_slow 10 * 10 = 100
  precn_t m1 = precn_new(10);
  precn_t m2 = precn_new(10);
  precn_t res = precn_new(0);
  if (precn_mul_slow(m1, m2, res) != 0) { std::cerr<<"mul_slow returned error"<<std::endl; failures++; }
  if (to_hex(res) != "64") { std::cerr<<"Test5 failed: expected 64 got "<<to_hex(res)<<"\n"; failures++; }
  std::cout<<"Test5 mul_slow 10 * 10 => "<<to_hex(res)<<"\n";

  // Test 6: precn_mul_slow large numbers
  // 0x12345678 * 0x10000 = 0x123456780000
  precn_set(m1, (uint64_t)0x12345678);
  precn_set(m2, (uint64_t)0x10000);
  if (precn_mul_slow(m1, m2, res) != 0) { std::cerr<<"mul_slow returned error"<<std::endl; failures++; }
  if (to_hex(res) != "123456780000") { std::cerr<<"Test6 failed: expected 123456780000 got "<<to_hex(res)<<"\n"; failures++; }
  std::cout<<"Test6 mul_slow large => "<<to_hex(res)<<"\n";

  precn_free(m1);
  precn_free(m2);
  precn_free(res);

  precn_free(a);

  if (failures == 0) std::cout<<"All tests completed (see output)."<<std::endl;
  else std::cout<<failures<<" failures."<<std::endl;
  return failures ? 1 : 0;
}
