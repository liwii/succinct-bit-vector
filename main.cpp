#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <numeric>
#include <chrono>

class BitVector {
private:
    std::vector<bool> bits;

public:
    BitVector(int num_bits) {
        bits.reserve(num_bits);
        // Seed the random number generator
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        // Create a random bit vector
        for (int i = 0; i < num_bits; ++i) {
            bits.push_back(std::rand() % 2);
        }
    }

    size_t memory_usage_bytes() const {
        return bits.capacity() / 8;
    }

    int rank(int i) const {
        if (i < 0 || i >= bits.size()) {
            return -1; // Or throw an exception
        }
        return std::count(bits.begin(), bits.begin() + i + 1, true);
    }

    size_t size() const {
        return bits.size();
    }
};

int main() {
    const int num_bits = 1 << 20; // 2^20
    BitVector bit_vector(num_bits);

    // Report memory usage
    size_t memory_bytes = bit_vector.memory_usage_bytes();
    double memory_kb = static_cast<double>(memory_bytes) / 1024;
    double memory_mb = memory_kb / 1024;

    std::cout << "Bit vector size: " << bit_vector.size() << " bits" << std::endl;
    std::cout << "Memory usage: " << memory_bytes << " bytes (" << memory_kb << " KB, " << memory_mb << " MB)" << std::endl;
    std::cout << "\n--- Rank Operation Performance ---" << std::endl;

    // Define indices to test
    std::vector<int> test_indices = {
        0,
        num_bits / 4,
        num_bits / 2,
        3 * num_bits / 4,
        num_bits - 1
    };

    for (int index : test_indices) {
        auto start = std::chrono::high_resolution_clock::now();
        int rank_result = bit_vector.rank(index);
        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        std::cout << "Rank at index " << index << ": " << rank_result
                  << " (took " << duration.count() << " microseconds)" << std::endl;
    }

    return 0;
}
