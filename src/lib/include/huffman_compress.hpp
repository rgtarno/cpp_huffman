
#pragma once

#include <string>

namespace huffman_compress
{
  /**
   * @brief Compress a file using Huffman encoding
   *
   * Compressed file will be written with a 267 byte header.
   * 256 bytes for the symbol encoding, and 1 byte for the number
   * of pad bits.
   *
   * @param infile File to compress
   * @param outfile File to write compressed data to
   */
  void compress(const std::string &infile, const std::string &outfile);

  /**
   * @brief Decompress a file that has been compressed using the compress function
   *
   * @param infile Compressed file
   * @param outfile File to write uncompressed data to
   */
  void decompress(const std::string &infile, const std::string &outfile);

} // namespace huffman_compress
