import os
import sys
import gmpy2

# Increase limit for large integer string conversion
sys.set_int_max_str_digits(0) # 0 means no limit

def verify():
    print("Reading A.txt...")
    with open("test/A.txt", "r") as f:
        a_hex = f.read().strip()
    
    print("Reading B.txt...")
    with open("test/B.txt", "r") as f:
        b_hex = f.read().strip()
        
    print("Reading Res.txt...")
    with open("test/Res.txt", "r") as f:
        res_hex = f.read().strip()
        
    print("Converting to gmpy2 integers...")
    a = gmpy2.mpz(a_hex, 16)
    b = gmpy2.mpz(b_hex, 16)
    res_actual = gmpy2.mpz(res_hex, 16)
    
    print("Computing Expected = A * B ...")
    res_expected = a * b

    
    print("Comparing...")
    if res_actual == res_expected:
        print("SUCCESS: Python verification passed!")
        return 0
    else:
        print("FAILURE: Mismatch found!")
        print(f"Expected bits: {res_expected.bit_length()}")
        print(f"Actual bits:   {res_actual.bit_length()}")
        return 1

if __name__ == "__main__":
    sys.exit(verify())
