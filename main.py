import numpy as np
import sys

# Define the size of the bit vector
# 2^20 bits
num_bits = 2**20

# Create a random bit vector (array of 0s and 1s)
# We use 'bool' dtype to store bits efficiently.
bit_vector = np.random.randint(2, size=num_bits, dtype=bool)

# Calculate the memory usage
memory_usage_bytes = sys.getsizeof(bit_vector)
memory_usage_kb = memory_usage_bytes / 1024
memory_usage_mb = memory_usage_kb / 1024

# Report the memory usage
print(f"Bit vector size: {num_bits} bits")
print(f"Memory usage: {memory_usage_bytes} bytes")
print(f"Memory usage: {memory_usage_kb:.2f} KB")
print(f"Memory usage: {memory_usage_mb:.2f} MB")
