#include <iostream>
#include "../prec.hpp"

using namespace std;

int main() {
    precn a = 123456789;
    precn b = "987654321";
    cout << "a initialized" << endl;
    precn c = a;
    cout << "c copy constructed" << endl;
    precn d = std::move(b);
    cout << "d move constructed" << endl;
    
    // Check values via internal pointer (since I haven't implemented to_string or << operator yet)
    // precn doesn't have public to_string yet.
    // But I can access p and print using precn_impl functions.
    
    char* s = precn_impl::precn_to_str(d.get());
    cout << "d value: " << s << endl;
    free(s);
    
    return 0;
}
