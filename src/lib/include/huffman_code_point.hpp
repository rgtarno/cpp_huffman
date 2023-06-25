
#pragma once

#include <cstdint>

namespace huffman_compress
{
  struct huffman_code_point
  {
    huffman_code_point() : code(0), num_bits(32), symbol(0) {};
    huffman_code_point(const std::uint32_t c, const std::size_t n, const unsigned char s) :
        code(c), num_bits(n), symbol(s) {};

    std::uint32_t code;     // The huffman code. First bit of the code is in the least significant bit
    std::size_t   num_bits; // Legth of code in bits
    unsigned char symbol;   // The underlying byte that this code maps to
  };
}; // namespace huffman_compress