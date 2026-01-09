#include <iostream>
#include "prec.hpp"

int main() {
    precz z("-123");
    // precz doesn't have accessors to print?
    // precz has public p.
    std::cout << z.p.to_string() << std::endl;
    return 0;
}
