#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

int main() {
    // Define the size of the bit vector
    const int num_bits = 1 << 20; // 2^20

    // Use std::vector<bool>, which is a space-efficient specialization
    // of std::vector for boolean values. It often packs bits together.
    std::vector<bool> bit_vector;
    bit_vector.reserve(num_bits);

    // Seed the random number generator
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // Create a random bit vector
    for (int i = 0; i < num_bits; ++i) {
        bit_vector.push_back(std::rand() % 2);
    }

    // Calculate the memory usage
    // Note: sizeof(bit_vector) gives the size of the std::vector object itself,
    // not the dynamically allocated memory for its elements.
    // The actual memory for the elements is managed internally by the vector.
    // For std::vector<bool>, this is implementation-defined but is typically
    // num_bits / 8 bytes.
    size_t memory_usage_bytes = bit_vector.capacity() / 8;
    double memory_usage_kb = static_cast<double>(memory_usage_bytes) / 1024;
    double memory_usage_mb = memory_usage_kb / 1024;

    // Report the memory usage
    std::cout << "Bit vector size: " << num_bits << " bits" << std::endl;
    std::cout << "Calculated memory usage: " << memory_usage_bytes << " bytes" << std::endl;
    std::cout << "Calculated memory usage: " << memory_usage_kb << " KB" << std::endl;
    std::cout << "Calculated memory usage: " << memory_usage_mb << " MB" << std::endl;

    return 0;
}
