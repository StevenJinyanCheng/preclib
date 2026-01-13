import subprocess
import random
import sys
import os

def to_hex(n):
    return hex(n)[2:]

def run_tests():
    driver_path = os.path.abspath("test_driver.exe")
    if not os.path.exists(driver_path):
        print("test_driver.exe not found")
        return

    proc = subprocess.Popen([driver_path], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

    num_tests = 100
    failures = 0

    print(f"Running {num_tests} tests using Python (GMP-based integers) to verify...")

    for i in range(num_tests):
        # Generate random numbers
        # Mix of sizes
        bits = random.choice([64, 128, 256, 512, 1024, 2048, 4096, 8192])
        a = random.getrandbits(bits)
        b = random.getrandbits(bits)
        
        # Print examples for the first 3 iterations
        if i < 3:
            print(f"\n--- Example {i+1} (bits={bits}) ---")
            print(f"A: {to_hex(a)}")
            print(f"B: {to_hex(b)}")

        # Test Addition
        op = "add"
        expected = a + b
        
        input_str = f"{op} {to_hex(a)} {to_hex(b)}\n"
        proc.stdin.write(input_str)
        proc.stdin.flush()
        
        result_hex = proc.stdout.readline().strip()
        if not result_hex:
            print("Process crashed or closed stream")
            break
            
        result = int(result_hex, 16)
        
        if i < 3:
            print(f"Add Result: {result_hex}")

        if result != expected:
            print(f"Addition failed at iter {i}")
            print(f"A: {to_hex(a)}")
            print(f"B: {to_hex(b)}")
            print(f"Expected: {to_hex(expected)}")
            print(f"Got:      {result_hex}")
            failures += 1

        # Test Multiplication
        op = "mul"
        expected = a * b
        
        input_str = f"{op} {to_hex(a)} {to_hex(b)}\n"
        proc.stdin.write(input_str)
        proc.stdin.flush()
        
        result_hex = proc.stdout.readline().strip()
        if not result_hex:
            print("Process crashed or closed stream")
            break
            
        result = int(result_hex, 16)

        if i < 3:
            print(f"Mul Result: {result_hex}")
        
        if result != expected:
            print(f"Multiplication failed at iter {i}")
            print(f"A: {to_hex(a)}")
            print(f"B: {to_hex(b)}")
            print(f"Expected: {to_hex(expected)}")
            print(f"Got:      {result_hex}")
            failures += 1

    proc.stdin.close()
    proc.terminate()
    
    if failures == 0:
        print("All tests passed!")
    else:
        print(f"{failures} tests failed.")

if __name__ == "__main__":
    run_tests()
