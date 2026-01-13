from sympy import invert

P1 = 998244353
P2 = 167772161
P3 = 469762049

M = P1 * P2 * P3

M1 = M // P1
M2 = M // P2
M3 = M // P3

inv1 = invert(M1, P1)
inv2 = invert(M2, P2)
inv3 = invert(M3, P3)

C1 = M1 * inv1
C2 = M2 * inv2
C3 = M3 * inv3

# For explicit CRT (float approximation method)
# We want doubles roughly equal to inv_i / Pi
# Actually in the code I wrote: inv_Pi = (double)i1 / P1;
# where i1 is the modular inverse.

d_inv1 = float(inv1) / P1
d_inv2 = float(inv2) / P2
d_inv3 = float(inv3) / P3

print(f"P1: {P1}")
print(f"P2: {P2}")
print(f"P3: {P3}")
print(f"M (hex): 0x{M:x}")
print(f"C1 (hex): 0x{C1:x}")
print(f"C2 (hex): 0x{C2:x}")
print(f"C3 (hex): 0x{C3:x}")
print(f"D_INV1: {d_inv1:.20e}")
print(f"D_INV2: {d_inv2:.20e}")
print(f"D_INV3: {d_inv3:.20e}")
