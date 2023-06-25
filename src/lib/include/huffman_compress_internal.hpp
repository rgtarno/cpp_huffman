
#pragma once

#include <array>
#include <climits>
#include <memory>
#include <string>
#include <vector>

#include "src/lib/include/huffman_code_point.hpp"
#include "src/lib/include/huffman_tree_node.hpp"

namespace huffman_compress
{
  constexpr unsigned char SYMBOL_START  = 0;
  constexpr unsigned char SYMBOL_END    = UCHAR_MAX;
  constexpr std::size_t   ALPHABET_SIZE = std::size_t{SYMBOL_END} + std::size_t{1};

  /* Compression */
  std::unique_ptr<huffman_tree_node> create_tree(const std::string &filename);
  std::vector<huffman_code_point>    create_codebook(const std::unique_ptr<huffman_tree_node> &tree_root);
  std::vector<huffman_code_point>    create_canonical_codebook(const std::vector<huffman_code_point> &enc);
  void                               compress_file(const std::string &infile, const std::string &outfile,
                                                   const std::vector<huffman_code_point> &codebook);
  void write_header(std::ofstream &ofs, const std::array<huffman_code_point, ALPHABET_SIZE> &encoding);

  /* Decompression */
  std::vector<huffman_code_point> read_encoding_from_header(std::ifstream &ifs);
  void                            decompress_file(std::ifstream &&infile, const std::string &outfile,
                                                  const std::vector<huffman_code_point> &codebook, const std::uint8_t pad_bits);

} // namespace huffman_compress
