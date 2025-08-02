#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <numeric>
#include <chrono>
#include <cmath>
#include <iomanip>

// --- Data structure for the k-ary select search tree ---
struct SelectNode {
    std::vector<int> child_counts; 
    std::vector<SelectNode*> children;
    int start_pos;
    int size;

    SelectNode(int s_pos, int s) : start_pos(s_pos), size(s) {}

    ~SelectNode() {
        for (SelectNode* child : children) {
            delete child;
        }
    }
};


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
    std::vector<SelectNode*> select_search_trees;
    int select_block_ones;
    int total_ones_count;
    int k_ary_branch_factor;

    SelectNode* build_select_tree(int start, int end, int level_rank_base) {
        SelectNode* node = new SelectNode(start, end - start + 1);
        if (node->size <= k_ary_branch_factor) {
            return node; // Leaf node
        }

        int sub_block_size = (node->size + k_ary_branch_factor - 1) / k_ary_branch_factor;
        int current_rank = 0;

        for (int i = 0; i < k_ary_branch_factor; ++i) {
            int child_start = start + i * sub_block_size;
            if (child_start > end) break;
            
            int child_end = std::min(end, child_start + sub_block_size - 1);
            
            node->child_counts.push_back(current_rank);
            
            int child_rank_base = level_rank_base + current_rank;
            int ones_in_child = (child_start > 0 ? rank(child_end) - rank(child_start - 1) : rank(child_end));
            
            node->children.push_back(build_select_tree(child_start, child_end, child_rank_base));
            
            current_rank += ones_in_child;
        }
        return node;
    }


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
        int lookup_size = 1 << small_block_size;
        popcount_lookup.resize(lookup_size, 0);
        for (int i = 0; i < lookup_size; ++i) popcount_lookup[i] = __builtin_popcount(i);
        int num_large_blocks = (num_bits + large_block_size - 1) / large_block_size;
        rank_large_blocks.resize(num_large_blocks + 1, 0);
        rank_small_blocks.resize(num_small_blocks + 1, 0);
        unsigned int large_rank = 0, small_rank = 0, current_key = 0;
        
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
                if (total_ones_count % select_block_ones == 0) select_index.push_back(i);
                large_rank++; small_rank++;
                current_key |= (1 << (i % small_block_size));
                total_ones_count++;
            }
        }
        small_block_keys[(num_bits - 1) / small_block_size] = current_key;

        k_ary_branch_factor = std::max(2, (int)sqrt(log_n));
        for (size_t i = 0; i < select_index.size(); ++i) {
            int start = select_index[i];
            int end = (i + 1 < select_index.size()) ? select_index[i+1] - 1 : num_bits - 1;
            int base_rank = i * select_block_ones;
            select_search_trees.push_back(build_select_tree(start, end, base_rank));
        }
    }

    ~BitVector() {
        for (SelectNode* tree : select_search_trees) {
            delete tree;
        }
    }

    size_t memory_usage_bytes() const {
        size_t bits_mem = (bits.size() + 7) / 8;
        size_t rank_mem = rank_large_blocks.size() * sizeof(unsigned int) +
                          rank_small_blocks.size() * sizeof(unsigned short) +
                          small_block_keys.size() * sizeof(unsigned int) +
                          popcount_lookup.size() * sizeof(unsigned char);
        size_t select_idx_mem = select_index.size() * sizeof(unsigned int);
        size_t select_tree_mem = select_index.size() * sizeof(SelectNode) * k_ary_branch_factor; 
        return bits_mem + rank_mem + select_idx_mem + select_tree_mem;
    }

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

    int select(int k) const {
        if (k <= 0 || k > total_ones_count) return -1;

        int block_idx = (k - 1) / select_block_ones;
        int rank_in_block = (k - 1) % select_block_ones;
        
        SelectNode* current_node = select_search_trees[block_idx];

        while (!current_node->children.empty()) {
            int child_idx = 0;
            for (size_t i = 1; i < current_node->child_counts.size(); ++i) {
                if (current_node->child_counts[i] <= rank_in_block) {
                    child_idx = i;
                } else {
                    break;
                }
            }
            rank_in_block -= current_node->child_counts[child_idx];
            current_node = current_node->children[child_idx];
        }

        for (int i = 0; i < current_node->size; ++i) {
            int pos = current_node->start_pos + i;
            if (pos >= num_bits) break;
            if (bits[pos]) {
                if (rank_in_block == 0) {
                    return pos;
                }
                rank_in_block--;
            }
        }
        return -1;
    }

    int select_naive(int k) const {
        if (k <= 0) return -1;
        int ones_counted = 0;
        for (int i = 0; i < num_bits; ++i) {
            if (bits[i]) {
                ones_counted++;
                if (ones_counted == k) return i;
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
    std::cout << "Memory usage (k-ary select trees): " << memory_bytes << " bytes (" 
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
        if (total_ones > 2) select_test_indices.push_back(total_ones / 2);
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