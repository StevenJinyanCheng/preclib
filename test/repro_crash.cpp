#include "../prec.hpp"
#include <iostream>
#include <string>

int main() {
    std::cout << "Pointer size: " << sizeof(void*) << " bytes" << std::endl;
    try {
        std::string input = "123456789"; 
        // User input seems to be "123456789..." repeated or just sequential integers
        // Let's use a reasonably long string.
        for(int i=0; i<5; ++i) input += input; // Make it longer, length 9*32 = 288 approx
        
        precz A(input.c_str());
        std::cout << "Initial A digits: " << A.to_string().length() << std::endl;
        
        for (int i = 1; i <= 10; ++i) {
            std::cout << "Iteration " << i << ": A = A ^ 10" << std::endl;
            
            // A = A ^ 10
            long long exp = 10;
            precz res(1);
            precz b = A;
            long long e = exp;
            
            int p_ops = 0;
            while(e > 0) {
                if(e&1) res = res*b;
                b = b*b;
                e>>=1;
            }
            A = res;
            
            std::string s = A.to_string();
            std::cout << "Result digits: " << s.length() << std::endl;
            // std::cout << "Result: " << s.substr(0, 50) << "..." << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Caught exception: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Caught unknown exception" << std::endl;
    }
    return 0;
}
