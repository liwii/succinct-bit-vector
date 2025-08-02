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
    std::vector<unsigned int> rank_large_blocks;
    std::vector<unsigned short> rank_small_blocks;
    std::vector<unsigned int> small_block_keys; // New: Stores the integer representation of each small block
    std::vector<unsigned char> popcount_lookup;
    int large_block_size;
    int small_block_size;
    int num_bits;

public:
    BitVector(int n) : num_bits(n) {
        bits.resize(num_bits);
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        for (int i = 0; i < num_bits; ++i) {
            bits[i] = std::rand() % 2;
        }

        int log_n = (num_bits > 1) ? std::log2(num_bits) : 0;
        large_block_size = log_n * log_n;
        small_block_size = std::max(1, log_n / 2);
        
        if (large_block_size == 0) large_block_size = 1;

        int num_small_blocks = (num_bits + small_block_size - 1) / small_block_size;
        small_block_keys.resize(num_small_blocks, 0);

        // Initialize popcount lookup table
        int lookup_size = 1 << small_block_size;
        popcount_lookup.resize(lookup_size, 0);
        for (int i = 0; i < lookup_size; ++i) {
            popcount_lookup[i] = __builtin_popcount(i);
        }

        // Initialize rank indices and small_block_keys
        int num_large_blocks = (num_bits + large_block_size - 1) / large_block_size;
        rank_large_blocks.resize(num_large_blocks + 1, 0);
        rank_small_blocks.resize(num_small_blocks + 1, 0);

        unsigned int large_rank = 0;
        unsigned short small_rank = 0;
        unsigned int current_key = 0;
        for (int i = 0; i < num_bits; ++i) {
            if (i > 0 && i % large_block_size == 0) {
                small_rank = 0;
            }
            if (i > 0 && i % small_block_size == 0) {
                small_block_keys[(i-1) / small_block_size] = current_key;
                current_key = 0;
            }
            if (i % large_block_size == 0) {
                rank_large_blocks[i / large_block_size] = large_rank;
            }
            if (i % small_block_size == 0) {
                rank_small_blocks[i / small_block_size] = small_rank;
            }
            
            if (bits[i]) {
                large_rank++;
                small_rank++;
                current_key |= (1 << (i % small_block_size));
            }
        }
        small_block_keys[(num_bits - 1) / small_block_size] = current_key; // Store the last key
    }

    size_t memory_usage_bytes() const {
        size_t bits_mem = (bits.size() + 7) / 8;
        size_t large_idx_mem = rank_large_blocks.size() * sizeof(unsigned int);
        size_t small_idx_mem = rank_small_blocks.size() * sizeof(unsigned short);
        size_t keys_mem = small_block_keys.size() * sizeof(unsigned int);
        size_t lookup_mem = popcount_lookup.size() * sizeof(unsigned char);
        return bits_mem + large_idx_mem + small_idx_mem + keys_mem + lookup_mem;
    }

    int rank(int i) const {
        if (i < 0 || i >= num_bits) return -1;

        int large_idx = i / large_block_size;
        int small_idx = i / small_block_size;
        int pos_in_small_block = i % small_block_size;

        int count = rank_large_blocks[large_idx] + rank_small_blocks[small_idx];

        // Fetch the pre-computed key for the small block
        unsigned int key = small_block_keys[small_idx];
        
        // Create a mask to isolate the bits up to the position
        unsigned int mask = (pos_in_small_block == 31) ? -1U : (1U << (pos_in_small_block + 1)) - 1;
        unsigned int masked_key = key & mask;
        
        return count + popcount_lookup[masked_key];
    }

    int rank_naive(int i) const {
        if (i < 0 || i >= num_bits) return -1;
        int count = 0;
        for (int j = 0; j <= i; ++j) {
            if (bits[j]) {
                count++;
            }
        }
        return count;
    }

    size_t size() const {
        return num_bits;
    }
};

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

    size_t memory_bytes = bit_vector.memory_usage_bytes();
    double memory_kb = static_cast<double>(memory_bytes) / 1024;
    double memory_mb = memory_kb / 1024;

    std::cout << "Bit vector size: " << bit_vector.size() << " bits" << std::endl;
    std::cout << "Memory usage (all indices + lookup): " << memory_bytes << " bytes (" 
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
        auto start_indexed = std::chrono::high_resolution_clock::now();
        int rank_result_indexed = bit_vector.rank(index);
        auto end_indexed = std::chrono::high_resolution_clock::now();
        auto time_indexed = std::chrono::duration_cast<std::chrono::microseconds>(end_indexed - start_indexed);

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