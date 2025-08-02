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
    int num_bits;

    // --- Rank Data Structures ---
    std::vector<unsigned int> rank_large_blocks;
    std::vector<unsigned short> rank_small_blocks;
    std::vector<unsigned int> small_block_keys;
    std::vector<unsigned char> popcount_lookup;
    int large_block_size;
    int small_block_size;

    // --- Select Data Structures ---
    std::vector<unsigned int> select_index;
    int select_block_ones;
    int total_ones_count;


public:
    BitVector(int n) : num_bits(n) {
        bits.resize(num_bits);
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        for (int i = 0; i < num_bits; ++i) {
            bits[i] = std::rand() % 2;
        }

        // --- Build Rank Indices ---
        int log_n = (num_bits > 1) ? std::log2(num_bits) : 0;
        large_block_size = log_n * log_n;
        small_block_size = std::max(1, log_n / 2);
        if (large_block_size == 0) large_block_size = 1;

        int num_small_blocks = (num_bits + small_block_size - 1) / small_block_size;
        small_block_keys.resize(num_small_blocks, 0);

        int lookup_size = 1 << small_block_size;
        popcount_lookup.resize(lookup_size, 0);
        for (int i = 0; i < lookup_size; ++i) {
            popcount_lookup[i] = __builtin_popcount(i);
        }

        int num_large_blocks = (num_bits + large_block_size - 1) / large_block_size;
        rank_large_blocks.resize(num_large_blocks + 1, 0);
        rank_small_blocks.resize(num_small_blocks + 1, 0);

        unsigned int large_rank = 0;
        unsigned short small_rank = 0;
        unsigned int current_key = 0;

        // --- Build Select Index ---
        select_block_ones = log_n * log_n;
        if (select_block_ones == 0) select_block_ones = 1;
        
        total_ones_count = 0;

        for (int i = 0; i < num_bits; ++i) {
            if (i > 0 && i % large_block_size == 0) small_rank = 0;
            if (i > 0 && i % small_block_size == 0) {
                small_block_keys[(i-1) / small_block_size] = current_key;
                current_key = 0;
            }
            if (i % large_block_size == 0) rank_large_blocks[i / large_block_size] = large_rank;
            if (i % small_block_size == 0) rank_small_blocks[i / small_block_size] = small_rank;
            
            if (bits[i]) {
                if (total_ones_count % select_block_ones == 0) {
                    select_index.push_back(i);
                }
                large_rank++;
                small_rank++;
                current_key |= (1 << (i % small_block_size));
                total_ones_count++;
            }
        }
        small_block_keys[(num_bits - 1) / small_block_size] = current_key;
    }

    size_t memory_usage_bytes() const {
        size_t bits_mem = (bits.size() + 7) / 8;
        size_t large_idx_mem = rank_large_blocks.size() * sizeof(unsigned int);
        size_t small_idx_mem = rank_small_blocks.size() * sizeof(unsigned short);
        size_t keys_mem = small_block_keys.size() * sizeof(unsigned int);
        size_t lookup_mem = popcount_lookup.size() * sizeof(unsigned char);
        size_t select_mem = select_index.size() * sizeof(unsigned int);
        return bits_mem + large_idx_mem + small_idx_mem + keys_mem + lookup_mem + select_mem;
    }

    // --- Rank Functions ---
    int rank(int i) const {
        if (i < 0 || i >= num_bits) return -1;
        int large_idx = i / large_block_size;
        int small_idx = i / small_block_size;
        int pos_in_small_block = i % small_block_size;
        int count = rank_large_blocks[large_idx] + rank_small_blocks[small_idx];
        unsigned int key = small_block_keys[small_idx];
        unsigned int mask = (pos_in_small_block == 31) ? -1U : (1U << (pos_in_small_block + 1)) - 1;
        return count + popcount_lookup[key & mask];
    }
    int rank_naive(int i) const {
        if (i < 0 || i >= num_bits) return -1;
        int count = 0;
        for (int j = 0; j <= i; ++j) if (bits[j]) count++;
        return count;
    }

    // --- Select Functions ---
    int select(int k) const {
        if (k <= 0 || k > total_ones_count) return -1; // k is 1-based
        
        int block_idx = (k - 1) / select_block_ones;
        int low = select_index[block_idx];
        int high = (block_idx + 1 < select_index.size()) ? select_index[block_idx + 1] : num_bits - 1;
        
        int ans = -1;

        while (low <= high) {
            int mid = low + (high - low) / 2;
            if (rank(mid) >= k) {
                ans = mid;
                high = mid - 1;
            } else {
                low = mid + 1;
            }
        }
        return ans;
    }

    int select_naive(int k) const {
        if (k <= 0) return -1; // k is 1-based
        int ones_counted = 0;
        for (int i = 0; i < num_bits; ++i) {
            if (bits[i]) {
                ones_counted++;
                if (ones_counted == k) {
                    return i;
                }
            }
        }
        return -1;
    }

    size_t size() const { return num_bits; }
    int total_ones() const { return total_ones_count; }
};

void print_rank_header() {
    std::cout << std::left 
              << std::setw(12) << "Index"
              << std::setw(18) << "Indexed Result"
              << std::setw(18) << "Naive Result"
              << std::setw(25) << "Indexed Time (us)"
              << std::setw(25) << "Naive Time (us)"
              << std::endl;
    std::cout << std::string(98, '-') << std::endl;
}

void print_select_header() {
    std::cout << std::left 
              << std::setw(12) << "K-th One"
              << std::setw(18) << "Indexed Result"
              << std::setw(18) << "Naive Result"
              << std::setw(25) << "Indexed Time (us)"
              << std::setw(25) << "Naive Time (us)"
              << std::endl;
    std::cout << std::string(98, '-') << std::endl;
}

int main() {
    const int num_bits = 1 << 20;
    BitVector bit_vector(num_bits);

    size_t memory_bytes = bit_vector.memory_usage_bytes();
    double memory_kb = static_cast<double>(memory_bytes) / 1024;
    double memory_mb = memory_kb / 1024;

    std::cout << "Bit vector size: " << bit_vector.size() << " bits" << std::endl;
    std::cout << "Total ones: " << bit_vector.total_ones() << std::endl;
    std::cout << "Memory usage (all indices + lookup): " << memory_bytes << " bytes (" 
              << std::fixed << std::setprecision(2) << memory_kb << " KB, " 
              << memory_mb << " MB)" << std::endl;
    
    // --- Rank Performance Comparison ---
    std::cout << "\n--- Rank Performance Comparison ---" << std::endl;
    print_rank_header();
    std::vector<int> rank_test_indices = {
        0, num_bits / 4, num_bits / 2, 3 * num_bits / 4, num_bits - 1
    };
    for (int index : rank_test_indices) {
        auto start_indexed = std::chrono::high_resolution_clock::now();
        int rank_result_indexed = bit_vector.rank(index);
        auto end_indexed = std::chrono::high_resolution_clock::now();
        auto time_indexed = std::chrono::duration_cast<std::chrono::microseconds>(end_indexed - start_indexed);

        auto start_naive = std::chrono::high_resolution_clock::now();
        int rank_result_naive = bit_vector.rank_naive(index);
        auto end_naive = std::chrono::high_resolution_clock::now();
        auto time_naive = std::chrono::duration_cast<std::chrono::microseconds>(end_naive - start_naive);

        std::cout << std::left << std::setw(12) << index
                  << std::setw(18) << rank_result_indexed
                  << std::setw(18) << rank_result_naive
                  << std::setw(25) << time_indexed.count()
                  << std::setw(25) << time_naive.count()
                  << std::endl;
    }

    // --- Select Performance Comparison ---
    std::cout << "\n--- Select Performance Comparison ---" << std::endl;
    print_select_header();

    int total_ones = bit_vector.total_ones();
    std::vector<int> select_test_indices;
    if (total_ones > 0) {
        select_test_indices.push_back(1);
        if (total_ones > 4) select_test_indices.push_back(total_ones / 4);
        if (total_ones > 2) select_test_indices.push_back(total_ones / 2);
        if (total_ones > 4) select_test_indices.push_back(3 * total_ones / 4);
        select_test_indices.push_back(total_ones);
    }

    for (int k : select_test_indices) {
        auto start_indexed = std::chrono::high_resolution_clock::now();
        int result_indexed = bit_vector.select(k);
        auto end_indexed = std::chrono::high_resolution_clock::now();
        auto time_indexed = std::chrono::duration_cast<std::chrono::microseconds>(end_indexed - start_indexed);

        auto start_naive = std::chrono::high_resolution_clock::now();
        int result_naive = bit_vector.select_naive(k);
        auto end_naive = std::chrono::high_resolution_clock::now();
        auto time_naive = std::chrono::duration_cast<std::chrono::microseconds>(end_naive - start_naive);

        std::cout << std::left 
                  << std::setw(12) << k
                  << std::setw(18) << result_indexed
                  << std::setw(18) << result_naive
                  << std::setw(25) << time_indexed.count()
                  << std::setw(25) << time_naive.count()
                  << std::endl;
    }

    return 0;
}