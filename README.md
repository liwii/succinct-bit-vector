# High-Performance Bit Vector with Rank Operation

This project provides a C++ implementation of a memory-efficient bit vector that supports a highly optimized, constant-time `rank` operation. It serves as a practical demonstration of concepts from succinct data structures, showing the evolution from a naive implementation to a sophisticated, multi-level indexed structure.

## Features

- **Constant-Time Rank:** The `rank(i)` operation, which counts the number of set bits (1s) up to index `i`, is implemented to run in O(1) time.
- **Memory Efficient:** The data structure uses a fraction of the memory that a simple array of booleans would require, thanks to bit-packing and carefully sized auxiliary indexes.
- **Dynamic and Scalable:** The block sizes and lookup tables are dynamically sized based on the input size (`n`), using `log2(n)` to automatically balance the time-space trade-off.
- **Comparative Analysis:** The `main` function includes a `rank_naive` method and a performance comparison to clearly demonstrate the effectiveness of the optimizations.

## How to Build and Run

### Prerequisites

- A C++ compiler (like g++ or Clang)
- CMake (version 3.10 or higher)

### Steps

1.  **Clone the repository:**
    ```bash
    git clone <repository-url>
    cd bitvector
    ```

2.  **Create a build directory:**
    ```bash
    mkdir build
    cd build
    ```

3.  **Configure with CMake and compile:**
    ```bash
    cmake ..
    make
    ```

4.  **Run the executable:**
    The compiled binary will be placed in the `bin` directory at the project root.
    ```bash
    ./../bin/bitvector_test
    ```

## Implementation Details

The final implementation achieves its O(1) rank time by using a multi-level indexing strategy on top of a `std::vector<bool>`.

1.  **`std::vector<bool>`:** The base bit vector is stored in a `std::vector<bool>`, which is a space-efficient C++ specialization that packs boolean values into bits.

2.  **Block-Based Indexing:** The bit vector is conceptually divided into blocks of different sizes to create a lookup hierarchy.
    -   **Large Blocks:** The vector is first divided into large blocks of size `(log2(n))^2`. An index (`rank_large_blocks`) stores the pre-computed rank up to the start of each large block.
    -   **Small Blocks:** Each large block is further subdivided into small blocks of size `0.5 * log2(n)`. A second index (`rank_small_blocks`) stores the rank from the start of the current large block to the start of each small block.

3.  **Popcount Lookup Table:** To eliminate the final loop, a pre-computed lookup table (`popcount_lookup`) is created.
    -   The size of this table is `2^(small_block_size)`.
    -   `popcount_lookup[i]` stores the number of set bits (the population count) in the binary representation of `i`.

### How `rank(i)` Works

The `rank(i)` operation is a series of fast lookups and additions:

1.  It finds the rank up to the start of the large block containing `i` from `rank_large_blocks`.
2.  It adds the rank from the start of that large block to the start of the small block containing `i` from `rank_small_blocks`.
3.  It constructs an integer key from the remaining bits within the small block (from its start to position `i`).
4.  It uses this key to look up the final count in the `popcount_lookup` table and adds it to the total.

This combination of lookups replaces a linear scan with a few memory accesses, achieving constant-time performance.

## Performance

The output of the program demonstrates the effectiveness of this approach. The indexed `rank` operation consistently takes 0-1 microseconds, regardless of the index, while the `rank_naive` operation's time increases linearly with the index.

```
--- Rank Performance Comparison ---
Index       Indexed Result    Naive Result      Indexed Time (us)        Naive Time (us)
--------------------------------------------------------------------------------------------------
0           0                 0                 0                        0
262144      131014            131014            0                        3732
524288      262055            262055            0                        7064
786432      393606            393606            0                        10047
1048575     524941            524941            0                        12654
```
