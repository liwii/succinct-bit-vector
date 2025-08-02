# High-Performance Bit Vector with Rank and Select

This project provides a C++ implementation of a memory-efficient bit vector that supports highly optimized, constant-time `rank` and `select` operations. It serves as a practical, hands-on demonstration of the concepts behind succinct data structures, showing the evolution from naive implementations to a sophisticated, multi-level indexed structure with theoretical performance guarantees.

## Features

- **Constant-Time Rank:** The `rank(i)` operation, which counts the number of set bits (1s) up to index `i`, is implemented to run in O(1) time.
- **Constant-Time Select:** The `select(k)` operation, which finds the position of the k-th set bit, is also implemented to run in O(1) time.
- **Memory Efficient:** The data structure uses a fraction of the memory that a simple array of booleans would require, thanks to bit-packing and carefully sized auxiliary indexes.
- **Dynamic and Scalable:** All auxiliary data structures (block sizes, lookup tables, search trees) are dynamically sized based on the input size (`n`), using `log2(n)` to automatically balance the time-space trade-off.
- **Comparative Analysis:** The `main` function includes naive implementations of both `rank` and `select` to clearly demonstrate the effectiveness of the optimizations.

## How to Build and Run

### Prerequisites

- A C++ compiler (like g++ or Clang)
- CMake (version 3.10 or higher)

### Steps

1.  **Create a build directory:**
    ```bash
    mkdir -p build
    cd build
    ```

2.  **Configure with CMake and compile:**
    ```bash
    cmake ..
    make
    ```

3.  **Run the executable:**
    The compiled binary will be placed in the `bin` directory at the project root.
    ```bash
    ./../bin/bitvector_test
    ```

## Implementation Details

The final implementation achieves its O(1) query times by using a hierarchy of indexes.

### O(1) Rank

The `rank(i)` operation uses a three-level lookup strategy:
1.  **Large Blocks:** The bit vector is divided into large blocks of size `(log2(n))^2`. An index stores the pre-computed rank up to the start of each large block.
2.  **Small Blocks:** Each large block is subdivided into small blocks of size `0.5 * log2(n)`. A second index stores the rank from the start of the large block to the start of each small block.
3.  **Popcount Lookup Table:** A pre-computed table stores the population count (number of set bits) for every possible bit pattern within a small block. The final rank is calculated by combining these three lookups with a bitmask, eliminating any loops.

### O(1) Select

The `select(k)` operation uses a different, powerful strategy to find the k-th '1':
1.  **Coarse-Grained Index:** An initial index (`select_index`) points to the start of blocks that each contain `(log2(n))^2` set bits. This quickly narrows down the search to a specific block.
2.  **Dense vs. Sparse Block Strategy:** The algorithm then uses a different strategy depending on the nature of that block:
    -   **Dense Blocks:** If the block's width in bits is small (less than `(log2(n))^4`), it's considered "dense". For these, we build a direct lookup table containing the exact position of every '1'. `select` becomes a single array lookup.
    -   **Sparse Blocks:** If the block's width is large, it's considered "sparse". For these, we build a **k-ary search tree** (where `k = sqrt(log2(n))`) over the block. This allows us to navigate the huge, sparse range in a constant number of steps.
3.  **Dispatch:** The `select` function first checks if the target block is dense or sparse and dispatches to the appropriate, O(1) strategy.

## Performance

The output of the program demonstrates the effectiveness of this approach. Both indexed `rank` and `select` operations consistently take 0-1 microseconds, regardless of the index, while the naive versions' times increase linearly.

```
--- Rank Performance Comparison ---
Index       Indexed Result    Naive Result      Indexed Time (us)        Naive Time (us)
--------------------------------------------------------------------------------------------------
0           1                 1                 0                        0
262144      131347            131347            0                        3135
...

--- Select Performance Comparison ---
K-th One    Indexed Result    Naive Result      Indexed Time (us)        Naive Time (us)
--------------------------------------------------------------------------------------------------
1           0                 0                 0                        0
261918      524160            524160            0                        5870
...
```

## Development Note

This codebase was written iteratively with the **Gemini CLI**. The process involved progressively building and refining the data structures, from initial naive implementations to the final, theoretically optimal versions, demonstrating an interactive and collaborative development workflow.

You can find the Gemini CLI project here: [https://github.com/google/gemini-cli](https://github.com/google/gemini-cli)