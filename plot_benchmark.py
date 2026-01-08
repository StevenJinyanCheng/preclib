import matplotlib.pyplot as plt
import numpy as np

# Data from the C++ Benchmark Output
sizes = [100, 1000, 4000, 8000, 16000, 32000, 64000, 128000, 256000, 512000, 1024000]

times_kara = [
    0.000018, 0.000719, 0.007701, 0.020384, 0.062531, 
    0.181260, 0.533652, 2.370289, 7.052260, 22.383721, 66.200823
]

times_ntt = [
    0.000080, 0.000716, 0.003485, 0.007028, 0.015563,
    0.037507, 0.072285, 0.228028, 0.496697, 1.046450, 2.354392
]

times_dispatch = [
    0.000017, 0.000830, 0.003382, 0.007566, 0.015221,
    0.034333, 0.107896, 0.231448, 0.510528, 1.068885, 2.570973
]

times_toom = [
    0.000013, 0.000650, 0.004135, 0.015411, 0.042758,
    0.097562, 0.488422, 1.516223, 3.307576, 10.145013, 23.843920
]

plt.figure(figsize=(12, 8))

# Log-Log Plot
plt.plot(sizes, times_kara, marker='o', label='Karatsuba O(N^1.58)', linestyle='-')
plt.plot(sizes, times_toom, marker='s', label='Toom-Cook 3 O(N^1.46)', linestyle='--')
plt.plot(sizes, times_ntt, marker='^', label='NTT O(N log N)', linestyle='-')
plt.plot(sizes, times_dispatch, marker='x', label='Auto Dispatch', linestyle=':')

plt.xscale('log')
plt.yscale('log')

plt.xlabel('Input Size (Words)')
plt.ylabel('Time (seconds)')
plt.title('Multiplication Algorithm Performance (Log-Log Scale)')
plt.grid(True, which="both", ls="-", alpha=0.5)
plt.legend()

# Save the plot
output_file = 'mul_benchmark_graph.png'
plt.savefig(output_file)
print(f"Graph saved to {output_file}")
