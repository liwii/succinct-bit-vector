#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <numeric>
#include <chrono>
#include <cmath>
#include <iomanip>

class BitVector {
private:
    std::vector<bool> bits;
    std::vector<int> rank_index;
    int block_size;

public:
    BitVector(int num_bits) {
        // Initialize bits
        bits.resize(num_bits);
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        for (int i = 0; i < num_bits; ++i) {
            bits[i] = std::rand() % 2;
        }

        // Initialize rank index
        int log_num_bits = std::log2(num_bits);
        block_size = log_num_bits * log_num_bits;
        if (block_size == 0) block_size = 1; // Avoid division by zero for small num_bits
        int num_blocks = (num_bits + block_size - 1) / block_size;
        rank_index.resize(num_blocks, 0);

        int current_rank = 0;
        for (int i = 0; i < num_bits; ++i) {
            if (i % block_size == 0) {
                rank_index[i / block_size] = current_rank;
            }
            if (bits[i]) {
                current_rank++;
            }
        }
    }

    size_t memory_usage_bytes() const {
        size_t bits_memory = bits.capacity() / 8;
        size_t index_memory = rank_index.size() * sizeof(int);
        return bits_memory + index_memory;
    }

    int rank(int i) const {
        if (i < 0 || i >= bits.size()) return -1;

        int block_idx = i / block_size;
        int rank_at_block_start = rank_index[block_idx];
        
        int rank_in_block = 0;
        int start_of_block = block_idx * block_size;
        // This loop is much shorter than rank_naive
        for (int j = start_of_block; j <= i; ++j) {
            if (bits[j]) {
                rank_in_block++;
            }
        }
        return rank_at_block_start + rank_in_block;
    }

    int rank_naive(int i) const {
        if (i < 0 || i >= bits.size()) return -1;
        // The original, simple implementation
        return std::count(bits.begin(), bits.begin() + i + 1, true);
    }

    size_t size() const {
        return bits.size();
    }
};

// Helper to format the output table
void print_header() {
    std::cout << std::left 
              << std::setw(12) << "Index"
              << std::setw(18) << "Indexed Result"
              << std::setw(18) << "Naive Result"
              << std::setw(25) << "Indexed Time (us)"
              << std::setw(25) << "Naive Time (us)"
              << std::endl;
    std::cout << std::string(98, '-') << std::endl;
}

int main() {
    const int num_bits = 1 << 20; // 2^20
    BitVector bit_vector(num_bits);

    // Report memory usage
    size_t memory_bytes = bit_vector.memory_usage_bytes();
    double memory_kb = static_cast<double>(memory_bytes) / 1024;
    double memory_mb = memory_kb / 1024;

    std::cout << "Bit vector size: " << bit_vector.size() << " bits" << std::endl;
    std::cout << "Memory usage (with index): " << memory_bytes << " bytes (" 
              << std::fixed << std::setprecision(2) << memory_kb << " KB, " 
              << memory_mb << " MB)" << std::endl;
    
    std::cout << "\n--- Rank Performance Comparison ---" << std::endl;
    print_header();

    std::vector<int> test_indices = {
        0,
        num_bits / 4,
        num_bits / 2,
        3 * num_bits / 4,
        num_bits - 1
    };

    for (int index : test_indices) {
        // Time the indexed rank
        auto start_indexed = std::chrono::high_resolution_clock::now();
        int rank_result_indexed = bit_vector.rank(index);
        auto end_indexed = std::chrono::high_resolution_clock::now();
        auto time_indexed = std::chrono::duration_cast<std::chrono::microseconds>(end_indexed - start_indexed);

        // Time the naive rank
        auto start_naive = std::chrono::high_resolution_clock::now();
        int rank_result_naive = bit_vector.rank_naive(index);
        auto end_naive = std::chrono::high_resolution_clock::now();
        auto time_naive = std::chrono::duration_cast<std::chrono::microseconds>(end_naive - start_naive);

        std::cout << std::left 
                  << std::setw(12) << index
                  << std::setw(18) << rank_result_indexed
                  << std::setw(18) << rank_result_naive
                  << std::setw(25) << time_indexed.count()
                  << std::setw(25) << time_naive.count()
                  << std::endl;
    }

    return 0;
}