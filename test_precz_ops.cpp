#include <iostream>
#include <cassert>
#include "prec.hpp"

using namespace std;

void test_ops() {
    precz a = 10;
    precz b = -3;
    
    // +
    precz c = a + b; // 7
    assert(c.to_string() == "7");
    
    // -
    c = a - b; // 13
    assert(c.to_string() == "13");
    
    c = b - a; // -13
    assert(c.to_string() == "-13");
    
    // *
    c = a * b; // -30
    assert(c.to_string() == "-30");
    
    // /
    c = a / b; // -3
    assert(c.to_string() == "-3");
    
    // %
    c = a % b; // 1
    assert(c.to_string() == "1");

    precz z = 0;
    precz nz = -0;
    assert(z.sign == 1);
    assert(nz.sign == 1); // normalized
    assert(z == nz);
    
    cout << "Ops tests passed" << endl;
}

int main() {
    test_ops();
    return 0;
}
