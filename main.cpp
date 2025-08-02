#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <numeric>
#include <chrono>
#include <cmath>
#include <iomanip>

struct SelectNode {
    std::vector<int> child_counts; 
    std::vector<SelectNode*> children;
    int start_pos;
    int size;
    SelectNode(int s_pos, int s) : start_pos(s_pos), size(s) {}
    ~SelectNode() { for (SelectNode* child : children) delete child; }
};

class BitVector {
private:
    std::vector<bool> bits;
    int num_bits;

    // Rank Data Structures
    std::vector<unsigned int> rank_large_blocks;
    std::vector<unsigned short> rank_small_blocks;
    std::vector<unsigned int> small_block_keys;
    std::vector<unsigned char> popcount_lookup;
    int large_block_size;
    int small_block_size;

    // Select Data Structures
    std::vector<unsigned int> select_index;
    std::vector<SelectNode*> dense_block_search_trees;
    std::vector<std::vector<unsigned int>> sparse_block_lookups;
    std::vector<int> sparse_block_map;
    std::vector<bool> is_sparse_block;
    int select_block_ones;
    int total_ones_count;
    int k_ary_branch_factor;

    SelectNode* build_select_tree(int start, int end) {
        SelectNode* node = new SelectNode(start, end - start + 1);
        if (node->size <= k_ary_branch_factor) return node;

        int sub_block_size = (node->size + k_ary_branch_factor - 1) / k_ary_branch_factor;
        int current_rank_in_block = 0;

        for (int i = 0; i < k_ary_branch_factor; ++i) {
            int child_start = start + i * sub_block_size;
            if (child_start > end) break;
            int child_end = std::min(end, child_start + sub_block_size - 1);
            
            node->child_counts.push_back(current_rank_in_block);
            node->children.push_back(build_select_tree(child_start, child_end));
            
            current_rank_in_block += (child_start > 0 ? rank(child_end) - rank(child_start - 1) : rank(child_end));
        }
        return node;
    }
    
    size_t get_tree_size(SelectNode* node) const {
        if (!node) return 0;
        size_t current_size = sizeof(SelectNode) + node->child_counts.capacity() * sizeof(int) + node->children.capacity() * sizeof(SelectNode*);
        for (SelectNode* child : node->children) {
            current_size += get_tree_size(child);
        }
        return current_size;
    }

public:
    BitVector(int n) : num_bits(n) {
        bits.resize(num_bits);
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        for (int i = 0; i < num_bits; ++i) bits[i] = std::rand() % 2;

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
        total_ones_count = 0;
        select_block_ones = log_n * log_n;
        if (select_block_ones == 0) select_block_ones = 1;

        for (int i = 0; i < num_bits; ++i) {
            if (i > 0 && i % large_block_size == 0) small_rank = 0;
            if (i > 0 && i % small_block_size == 0) { small_block_keys[(i-1)/small_block_size] = current_key; current_key = 0; }
            if (i % large_block_size == 0) rank_large_blocks[i/large_block_size] = large_rank;
            if (i % small_block_size == 0) rank_small_blocks[i/small_block_size] = small_rank;
            if (bits[i]) {
                if (total_ones_count % select_block_ones == 0) select_index.push_back(i);
                large_rank++; small_rank++;
                current_key |= (1 << (i % small_block_size));
                total_ones_count++;
            }
        }
        small_block_keys[(num_bits - 1) / small_block_size] = current_key;

        k_ary_branch_factor = std::max(2, (int)sqrt(log_n));
        long long sparse_threshold = log_n * log_n * log_n * log_n;
        is_sparse_block.resize(select_index.size(), false);
        sparse_block_map.resize(select_index.size(), -1);
        dense_block_search_trees.resize(select_index.size(), nullptr);
        int sparse_counter = 0;

        for (size_t i = 0; i < select_index.size(); ++i) {
            int start = select_index[i];
            int end = (i + 1 < select_index.size()) ? select_index[i+1] - 1 : num_bits - 1;
            if ((end - start + 1) > sparse_threshold) {
                is_sparse_block[i] = true;
                sparse_block_map[i] = sparse_counter++;
                std::vector<unsigned int> positions;
                for (int j = start; j <= end; ++j) {
                    if (bits[j]) positions.push_back(j);
                }
                sparse_block_lookups.push_back(positions);
            } else {
                is_sparse_block[i] = false;
                dense_block_search_trees[i] = build_select_tree(start, end);
            }
        }
    }

    ~BitVector() { for (SelectNode* tree : dense_block_search_trees) if(tree) delete tree; }

    size_t mem_bits() const { return (bits.capacity() + 7) / 8; }
    size_t mem_rank_large_blocks() const { return rank_large_blocks.capacity() * sizeof(unsigned int); }
    size_t mem_rank_small_blocks() const { return rank_small_blocks.capacity() * sizeof(unsigned short); }
    size_t mem_small_block_keys() const { return small_block_keys.capacity() * sizeof(unsigned int); }
    size_t mem_popcount_lookup() const { return popcount_lookup.capacity() * sizeof(unsigned char); }
    size_t mem_select_index() const { return select_index.capacity() * sizeof(unsigned int); }
    size_t mem_dense_block_search_trees() const {
        size_t total_size = 0;
        for (SelectNode* tree : dense_block_search_trees) total_size += get_tree_size(tree);
        return total_size;
    }
    size_t mem_sparse_block_lookups() const {
        size_t total_size = 0;
        for (const auto& vec : sparse_block_lookups) total_size += vec.capacity() * sizeof(unsigned int);
        return total_size;
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
        if (is_sparse_block[block_idx]) {
            int sparse_map_idx = sparse_block_map[block_idx];
            return sparse_block_lookups[sparse_map_idx][rank_in_block];
        } else {
            SelectNode* current_node = dense_block_search_trees[block_idx];
            while (current_node && !current_node->children.empty()) {
                int child_idx = 0;
                for (size_t i = 1; i < current_node->child_counts.size(); ++i) {
                    if (current_node->child_counts[i] <= rank_in_block) child_idx = i;
                    else break;
                }
                rank_in_block -= current_node->child_counts[child_idx];
                current_node = current_node->children[child_idx];
            }
            if (!current_node) return -1;
            for (int i = 0; i < current_node->size; ++i) {
                int pos = current_node->start_pos + i;
                if (pos >= num_bits) break;
                if (bits[pos]) {
                    if (rank_in_block == 0) return pos;
                    rank_in_block--;
                }
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

void print_header(const std::string& title) { std::cout << "\n--- " << title << " ---" << std::endl; }
void print_mem_line(const std::string& name, size_t bytes) {
    std::cout << std::left << std::setw(35) << name << std::setw(15) << bytes << " bytes" << "(" << std::fixed << std::setprecision(2) << static_cast<double>(bytes) / 1024.0 << " KB)" << std::endl;
}
void print_rank_header() {
    std::cout << std::left << std::setw(12) << "Index" << std::setw(18) << "Indexed Result" << std::setw(18) << "Naive Result" << std::setw(25) << "Indexed Time (us)" << std::setw(25) << "Naive Time (us)" << std::endl;
    std::cout << std::string(98, '-') << std::endl;
}
void print_select_header() {
    std::cout << std::left << std::setw(12) << "K-th One" << std::setw(18) << "Indexed Result" << std::setw(18) << "Naive Result" << std::setw(25) << "Indexed Time (us)" << std::setw(25) << "Naive Time (us)" << std::endl;
    std::cout << std::string(98, '-') << std::endl;
}

int main() {
    const int num_bits = 1 << 20;
    BitVector bit_vector(num_bits);

    std::cout << "Bit vector size: " << bit_vector.size() << " bits" << std::endl;
    std::cout << "Total ones: " << bit_vector.total_ones() << std::endl;

    print_header("Memory Usage Breakdown");
    size_t mem_raw = bit_vector.mem_bits();
    size_t mem_rank_large = bit_vector.mem_rank_large_blocks();
    size_t mem_rank_small = bit_vector.mem_rank_small_blocks();
    size_t mem_rank_keys = bit_vector.mem_small_block_keys();
    size_t mem_rank_popcount = bit_vector.mem_popcount_lookup();
    size_t total_rank_mem = mem_rank_large + mem_rank_small + mem_rank_keys + mem_rank_popcount;
    size_t mem_select_idx = bit_vector.mem_select_index();
    size_t mem_select_trees = bit_vector.mem_dense_block_search_trees();
    size_t mem_select_lookups = bit_vector.mem_sparse_block_lookups();
    size_t total_select_mem = mem_select_idx + mem_select_trees + mem_select_lookups;
    size_t total_mem = mem_raw + total_rank_mem + total_select_mem;
    print_mem_line("Raw Bit Vector", mem_raw);
    print_header("Rank Structures");
    print_mem_line("rank_large_blocks", mem_rank_large);
    print_mem_line("rank_small_blocks", mem_rank_small);
    print_mem_line("small_block_keys", mem_rank_keys);
    print_mem_line("popcount_lookup", mem_rank_popcount);
    std::cout << std::string(70, '-') << std::endl;
    print_mem_line("Total for Rank()", total_rank_mem);
    print_header("Select Structures");
    print_mem_line("select_index", mem_select_idx);
    print_mem_line("dense_block_search_trees", mem_select_trees);
    print_mem_line("sparse_block_lookups", mem_select_lookups);
    std::cout << std::string(70, '-') << std::endl;
    print_mem_line("Total for Select()", total_select_mem);
    print_header("Grand Total");
    print_mem_line("Total Calculated Memory", total_mem);

    print_header("Rank Performance Comparison");
    print_rank_header();
    std::vector<int> rank_test_indices = { 0, num_bits / 4, num_bits / 2, 3 * num_bits / 4, num_bits - 1 };
    for (int index : rank_test_indices) {
        auto start_indexed = std::chrono::high_resolution_clock::now();
        int rank_result_indexed = bit_vector.rank(index);
        auto end_indexed = std::chrono::high_resolution_clock::now();
        auto time_indexed = std::chrono::duration_cast<std::chrono::microseconds>(end_indexed - start_indexed);
        auto start_naive = std::chrono::high_resolution_clock::now();
        int rank_result_naive = bit_vector.rank_naive(index);
        auto end_naive = std::chrono::high_resolution_clock::now();
        auto time_naive = std::chrono::duration_cast<std::chrono::microseconds>(end_naive - start_naive);
        std::cout << std::left << std::setw(12) << index << std::setw(18) << rank_result_indexed << std::setw(18) << rank_result_naive << std::setw(25) << time_indexed.count() << std::setw(25) << time_naive.count() << std::endl;
    }

    print_header("Select Performance Comparison");
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
        std::cout << std::left << std::setw(12) << k << std::setw(18) << result_indexed << std::setw(18) << result_naive << std::setw(25) << time_indexed.count() << std::setw(25) << time_naive.count() << std::endl;
    }

    return 0;
}