#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include "../prec.hpp"

using namespace precn_impl;

// Hex string to precn_t
precn_t from_hex(const std::string& s) {
    if (s == "0") return precn_new(0);
    
    // This is a bit hacky, but we can parse 8 chars at a time from the end.
    int len = s.length();
    int num_words = (len + 7) / 8;
    precn_t p = __precn_new_size(num_words, num_words);
    
    for (int i = 0; i < num_words; ++i) {
        int end = len - i * 8;
        int start = end - 8;
        if (start < 0) start = 0;
        std::string chunk = s.substr(start, end - start);
        p->n[i] = std::stoul(chunk, nullptr, 16);
    }
    // Adjust rsiz
    while (p->rsiz > 0 && p->n[p->rsiz - 1] == 0) {
        p->rsiz--;
    }
    return p;
}

std::string to_hex(precn_t p) {
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

int main() {
    std::string op, a_str, b_str;
    while (std::cin >> op >> a_str >> b_str) {
        precn_t a = from_hex(a_str);
        precn_t b = from_hex(b_str);
        precn_t res = precn_new(0);
        
        if (op == "add") {
            precn_add(a, b, res);
        } else if (op == "mul") {
            precn_mul_slow(a, b, res);
        } else {
            std::cout << "error" << std::endl;
            continue;
        }
        
        std::cout << to_hex(res) << std::endl;
        
        precn_free(a);
        precn_free(b);
        precn_free(res);
    }
    return 0;
}
