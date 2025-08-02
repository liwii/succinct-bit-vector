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
    int large_block_size;
    int small_block_size;

public:
    BitVector(int num_bits) {
        bits.resize(num_bits);
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        for (int i = 0; i < num_bits; ++i) {
            bits[i] = std::rand() % 2;
        }

        int log_n = (num_bits > 1) ? std::log2(num_bits) : 0;
        large_block_size = log_n * log_n;
        small_block_size = std::max(1, log_n / 2);

        int num_large_blocks = (num_bits + large_block_size - 1) / large_block_size;
        rank_large_blocks.resize(num_large_blocks + 1, 0);

        int num_small_blocks = (num_bits + small_block_size - 1) / small_block_size;
        rank_small_blocks.resize(num_small_blocks + 1, 0);

        unsigned int current_large_rank = 0;
        unsigned short current_small_rank = 0;
        for (int i = 0; i < num_bits; ++i) {
            if (i % large_block_size == 0) {
                rank_large_blocks[i / large_block_size] = current_large_rank;
                current_small_rank = 0;
            }
            if (i % small_block_size == 0) {
                rank_small_blocks[i / small_block_size] = current_small_rank;
            }
            if (bits[i]) {
                current_large_rank++;
                current_small_rank++;
            }
        }
    }

    size_t memory_usage_bytes() const {
        size_t bits_mem = bits.capacity() / 8;
        size_t large_idx_mem = rank_large_blocks.size() * sizeof(unsigned int);
        size_t small_idx_mem = rank_small_blocks.size() * sizeof(unsigned short);
        return bits_mem + large_idx_mem + small_idx_mem;
    }

    int rank(int i) const {
        if (i < 0 || i >= bits.size()) return -1;

        int large_idx = i / large_block_size;
        int small_idx = i / small_block_size;
        int scan_start = small_idx * small_block_size;

        int count = rank_large_blocks[large_idx] + rank_small_blocks[small_idx];
        
        for (int k = scan_start; k <= i; ++k) {
            if (bits[k]) {
                count++;
            }
        }
        return count;
    }

    int rank_naive(int i) const {
        if (i < 0 || i >= bits.size()) return -1;
        return std::count(bits.begin(), bits.begin() + i + 1, true);
    }

    size_t size() const {
        return bits.size();
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
    std::cout << "Memory usage (2-level index): " << memory_bytes << " bytes (" 
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