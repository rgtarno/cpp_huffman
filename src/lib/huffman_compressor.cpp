
#include "src/lib/include/huffman_compress_internal.hpp"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <memory>
#include <queue>
#include <unordered_map>
#include <vector>

#include "src/lib/include/bit_writer.hpp"
#include "src/lib/include/debug_macros.hpp"
#include "src/lib/include/huffman_tree_node.hpp"

namespace
{
  //==================================================================
  // https://graphics.stanford.edu/~seander/bithacks.html#BitReverseObvious
  template <std::integral T>
  T bit_reverse(T input)
  {
    T   ret = input;                        // r will be reversed bits of v; first get LSB of v
    int s   = sizeof(input) * CHAR_BIT - 1; // extra shift needed at end

    for (input >>= 1; input; input >>= 1)
    {
      ret <<= 1;
      ret |= input & 1;
      s--;
    }
    ret <<= s; // shift when v's highest bits are zero
    return ret;
  }

  //==================================================================
  std::vector<unsigned char> read_file_to_byte_vector(std::ifstream &&infile)
  {
    if (!infile.good())
    {
      throw std::runtime_error("Failed to read file");
    }

    const auto start_pos = infile.tellg();

    std::vector<unsigned char> input_buffer;
    infile.seekg(0, std::ios::end);
    const auto end_pos = infile.tellg();
    infile.seekg(start_pos, std::ios::beg);

    input_buffer.resize(end_pos - start_pos);
    infile.read(reinterpret_cast<char *>(input_buffer.data()), end_pos - start_pos);

    return input_buffer;
  }

  //==================================================================
  /**
   * @brief Sort symbol encodings prior to calculation of canonical codes
   *
   * Sort in order of increasing code length, with ties broken by lexicographical
   * the symbol values.
   */
  bool canonical_encoding_compare(const huffman_compress::huffman_code_point &a,
                                  const huffman_compress::huffman_code_point &b)
  {
    // Sort in ascending code length, use alphabetical order to break ties
    if (a.num_bits < b.num_bits)
      return true;
    if (b.num_bits < a.num_bits)
      return false;
    if (a.symbol < b.symbol)
      return true;
    if (b.symbol < a.symbol)
      return false;
    return false;
  }

  //==================================================================
  /**
   * @brief Recursively traverse the tree, assigning a code to each leaf node
   *
   * The left branch encodes a 0 and right branch encodes a 1
   *
   */
  void label_internal(const std::unique_ptr<huffman_compress::huffman_tree_node> &node, std::uint32_t value,
                      std::size_t depth, std::vector<huffman_compress::huffman_code_point> &codebook)
  {
    if (!node)
    {
      return;
    }

    if (!node->symbol.has_value())
    {
      label_internal(node->left, value, depth + 1, codebook);
      label_internal(node->right, value + (1 << depth), depth + 1, codebook);
    }
    else
    {
      codebook.emplace_back(value, std::max(depth, std::size_t(1)), node->symbol.value());
    }
  }

} // namespace

namespace huffman_compress
{
  //========================================================================================
  void compress(const std::string &infile, const std::string &outfile)
  {
    const auto                            root_node = create_tree(infile);
    const std::vector<huffman_code_point> codebook  = create_canonical_codebook(create_codebook(root_node));
    compress_file(infile, outfile, codebook);
  }

  //========================================================================================
  void decompress(const std::string &infile, const std::string &outfile)
  {
    std::ifstream ifs(infile);
    if (!ifs.good())
    {
      throw std::runtime_error("Failed to open input file");
    }

    std::vector<huffman_code_point> encoding = read_encoding_from_header(ifs);
    std::uint8_t                    pad_bits = 0;
    ifs.read(reinterpret_cast<char *>(&pad_bits), sizeof(pad_bits));

    decompress_file(std::move(ifs), outfile, encoding, pad_bits);
  }

  //========================================================================================
  /**
   * @brief Create a Huffman tree where each leaf node contains a symbol
   *
   * Each node has the number of occurrences of all symbols below it. Returns a nullptr if the tree
   * is empty.
   *
   * @param filename File to generate tree for
   * @return std::unique_ptr<huffman_tree_node>
   */
  std::unique_ptr<huffman_tree_node> create_tree(const std::string &filename)
  {
    const auto input_buffer = read_file_to_byte_vector(std::ifstream(filename, std::ios::in | std::ios::binary));

    if (input_buffer.empty())
    {
      return {};
    }

    // Create map of symbols to occurrences
    // Use an array for speed
    std::array<uint64_t, ALPHABET_SIZE> frequency_map;
    frequency_map.fill(0);
    for (const auto &c : input_buffer)
    {
      frequency_map[c] += 1;
    }

    std::queue<std::unique_ptr<huffman_tree_node>> a;
    std::queue<std::unique_ptr<huffman_tree_node>> b;

    // 1) Fill queue a with the symbols sorted in ascending order of weight
    std::vector<std::unique_ptr<huffman_tree_node>> sorted_nodes;

    for (std::size_t i = 0; i < ALPHABET_SIZE; ++i)
    {
      if (frequency_map[i] != 0)
      {
        auto node    = std::make_unique<huffman_tree_node>();
        node->weight = frequency_map[i];
        node->symbol = static_cast<unsigned char>(i);
        sorted_nodes.push_back(std::move(node));
      }
    }

    std::sort(sorted_nodes.begin(), sorted_nodes.end(),
              [](const std::unique_ptr<huffman_tree_node> &z1, const std::unique_ptr<huffman_tree_node> &z2) {
                return z1->weight < z2->weight;
              });

    for (auto &node : sorted_nodes)
    {
      a.push(std::move(node));
    }

    auto dequeue_lowest_weight = [](std::queue<std::unique_ptr<huffman_tree_node>> &q1,
                                    std::queue<std::unique_ptr<huffman_tree_node>> &q2) {
      if (!(q1.empty() || q2.empty()))
      {
        if (q1.front()->weight <= q2.front()->weight)
        {
          auto ret = std::move(q1.front());
          q1.pop();
          return ret;
        }
        else
        {
          auto ret = std::move(q2.front());
          q2.pop();
          return ret;
        }
      }
      else if (q2.empty())
      {
        auto ret = std::move(q1.front());
        q1.pop();
        return ret;
      }
      else
      {
        auto ret = std::move(q2.front());
        q2.pop();
        return ret;
      }
    };

    // 2) Construct the binary tree
    while ((a.size() > 1) || (b.size() > 1))
    {
      auto n1 = dequeue_lowest_weight(a, b);
      auto n2 = dequeue_lowest_weight(a, b);

      auto new_node    = std::make_unique<huffman_tree_node>();
      new_node->weight = n1->weight + n2->weight;
      new_node->left   = std::move(n1);
      new_node->right  = std::move(n2);

      b.push(std::move(new_node));
    }

    return dequeue_lowest_weight(a, b);
  }

  //==================================================================
  /**
   * @brief Create a codebook containing the symbol to Huffman code mapping
   *
   * @param tree_root Root node of a Huffman tree describing the frequency of symbol occurrences
   * @return std::vector<huffman_code_point> Huffman encoding
   */
  std::vector<huffman_code_point> create_codebook(const std::unique_ptr<huffman_tree_node> &tree_root)
  {
    std::vector<huffman_code_point> encoding;
    label_internal(tree_root, 0, 0, encoding);
    return encoding;
  }

  //==================================================================
  /**
   * @brief Transform Huffman encoding in to canonical form
   *
   * See https://en.wikipedia.org/wiki/Canonical_Huffman_code#Algorithm
   *
   * @param non_canonical_codebook Non-canonical Huffman codebook
   * @return std::vector<huffman_code_point> canonical encoding
   */
  std::vector<huffman_code_point> create_canonical_codebook(
      const std::vector<huffman_code_point> &non_canonical_codebook)
  {
    auto codebook = non_canonical_codebook;
    std::sort(codebook.begin(), codebook.end(), canonical_encoding_compare);

    // Transform codes in to canonical form
    std::size_t   current_length  = codebook.front().num_bits;
    std::uint32_t next_code_value = 0;
    for (auto &symb : codebook)
    {
      symb.code = next_code_value;
      if (symb.num_bits > current_length)
      {
        // left shift until code is the same length as the original code
        symb.code      = (symb.code << (symb.num_bits - current_length));
        current_length = symb.num_bits;
      }
      next_code_value = symb.code + 1;
      // Bit reverse the code as the bit_writer works from the least significant bit
      symb.code = bit_reverse(symb.code) >> (32 - symb.num_bits);
    }
#if DEBUG
    debug("---------------------------------------------");
    debug("Canonical codebook (bits right to left -->)");
    for (auto &e : codebook)
    {
      debug("0x{:02x} : {:0{}b} ({})", e.symbol, bit_reverse(e.code) >> (32 - e.num_bits), e.num_bits, e.num_bits);
    }
#endif
    return codebook;
  }

  //==================================================================
  void compress_file(const std::string &infile, const std::string &outfile,
                     const std::vector<huffman_code_point> &codebook)
  {
    // Populate array based lookup
    std::array<huffman_code_point, ALPHABET_SIZE> symbol_lookup;
    for (const auto &e : codebook)
    {
      symbol_lookup[e.symbol].code     = e.code;
      symbol_lookup[e.symbol].num_bits = e.num_bits;
    }

    std::ifstream ifs(infile);
    if (!ifs.good())
    {
      throw std::runtime_error("Failed to open input file");
    }

    std::ofstream ofs(outfile);
    if (!ofs.good())
    {
      throw std::runtime_error("Failed to open output file");
    }

    write_header(ofs, symbol_lookup);

    std::vector<unsigned char> input_buffer;
    ifs.seekg(0, std::ios::end);
    const auto end_pos = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    input_buffer.resize(end_pos);
    ifs.read(reinterpret_cast<char *>(input_buffer.data()), end_pos);

    bit_writer writer(ofs);

    for (const auto &c : input_buffer)
    {
      writer.write_bits(symbol_lookup[c].code, symbol_lookup[c].num_bits);
    }

    const std::uint8_t pad_length = writer.flush();
    if (pad_length)
    {
      ofs.seekp(256, std::ios::beg);
      ofs.write(reinterpret_cast<const char *>(&pad_length), sizeof(pad_length));
    }
  }

  //==================================================================
  /**
   * @brief Write the symbol encoding to the start of the file
   *
   * Write out the code lengths of the entire alphabet (0 --> 255) and 1 byte to store
   * the number of pad bits. Set pad bits to 0, so if we get lucky and there are none
   * then we don't have to fseek.
   *
   * @param ofs File stream to write to
   * @param encoding Full symbol encoding
   */
  void write_header(std::ofstream &ofs, const std::array<huffman_code_point, ALPHABET_SIZE> &encoding)
  {
    // Full alphabet
    for (const auto &symbol : encoding)
    {
      const std::uint8_t code_length = static_cast<std::uint8_t>(symbol.num_bits);
      ofs.write(reinterpret_cast<const char *>(&code_length), sizeof(code_length));
    }
    const std::uint8_t pad_length = 0;
    ofs.write(reinterpret_cast<const char *>(&pad_length), sizeof(pad_length));
  }

  //==================================================================
  /**
   * @brief Reconstruct the symbol encoding from the compressed files header
   *
   * The first 256 bytes of the file are the code lengths of each respective byte.
   * From this we can determine the canonical Huffman code for each symbol.
   *
   * @param ifs Input file stream for the compressed file
   * @return std::vector<huffman_code_point> regenerated canonical encoding
   */
  std::vector<huffman_code_point> read_encoding_from_header(std::ifstream &ifs)
  {
    // Map of symbol lengths to symbols
    std::unordered_map<std::uint8_t, std::vector<unsigned char>> depth_map;
    unsigned char                                                i = 0;
    while (true)
    {
      std::uint8_t depth = 0;
      ifs.read(reinterpret_cast<char *>(&depth), sizeof(depth));
      depth_map[depth].push_back(i);
      if (i == UCHAR_MAX)
        break;
      ++i;
    }

    assert(depth_map.count(0) == 0);

    // Regenerate canonical codes - uses the same algorithm as create_canonical_codebook()
    std::vector<huffman_code_point> encoding;
    std::uint32_t                   current_depth   = 0;
    std::uint32_t                   next_code_value = 0;
    for (std::uint32_t depth = 1; depth < 32; ++depth)
    {
      const auto iter = depth_map.find(static_cast<std::uint8_t>(depth));
      if (iter != depth_map.end())
      {
        auto symbols = iter->second;
        if (!current_depth)
        {
          current_depth = depth;
        }
        if (depth > current_depth)
        {
          next_code_value = next_code_value << (depth - current_depth);
          current_depth   = depth;
        }

        for (auto &symb : symbols)
        {
          const std::uint32_t reversed_code = bit_reverse(next_code_value) >> (32 - depth);
          encoding.emplace_back(reversed_code, depth, symb);
          next_code_value = next_code_value + 1;
        }
      }
    }

    std::sort(encoding.begin(), encoding.end(), canonical_encoding_compare);
    return encoding;
  }

  //==================================================================
  void decompress_file(std::ifstream &&infile, const std::string &outfile,
                       const std::vector<huffman_code_point> &codebook, const std::uint8_t pad_bits)
  {
    const auto                 input_buffer = read_file_to_byte_vector(std::move(infile));
    std::vector<unsigned char> output_buffer;
    output_buffer.reserve(input_buffer.size() * 2);

    // Decode loop
    constexpr std::size_t UCHAR_NUM_BITS = CHAR_BIT;
    const std::size_t     total_bits     = (input_buffer.size() * 8) - pad_bits;
    std::size_t           bit_select     = 0;
    std::uint32_t         current_code   = 0;
    std::size_t           current_length = 0;
    std::size_t           bit_counter    = 0;
    auto                  symbol_iter    = codebook.cbegin();

    /* Build up the current_code bit by bit, on each bit check if any of the
       symbols in the codebook vector with the same length match. As the vector
       is sorted by increasing code length, the iterator in to that vector is incremented
       until we get to either a matching code, or a code with a bigger length then the current_code.
     */
    for (const auto &c : input_buffer)
    {
      while (bit_select < UCHAR_NUM_BITS)
      {
        if (bit_counter == total_bits)
        {
          break;
        }

        // Shift in next bit
        const std::uint32_t next_bit = (c >> bit_select) & 0x01;
        current_code |= (next_bit << current_length);
        ++current_length;

        // Lookup code
        while ((symbol_iter != codebook.cend()))
        {
          // Have a code match, reset the state.
          if ((current_length == symbol_iter->num_bits) && (current_code == symbol_iter->code))
          {
            output_buffer.push_back(symbol_iter->symbol);
            current_code   = 0;
            current_length = 0;
            symbol_iter    = codebook.cbegin();
            break;
          }
          else if (current_length < symbol_iter->num_bits)
          {
            // Haven't matched the code, read next bit before comparing with the longer codes
            break;
          }
          ++symbol_iter;
        }
        ++bit_select;
        ++bit_counter;
      }
      if (bit_counter == total_bits)
      {
        break;
      }
      bit_select = 0;
    }

    std::ofstream ofs(outfile);
    if (!ofs.good())
    {
      throw std::runtime_error("Failed to open output file");
    }
    ofs.write(reinterpret_cast<char *>(output_buffer.data()), output_buffer.size());
  }

} // namespace huffman_compress
