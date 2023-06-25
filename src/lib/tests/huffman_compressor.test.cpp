#include <catch2/catch_all.hpp>

#include <climits>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>

#include "src/lib/include/debug_macros.hpp"
#include "src/lib/include/huffman_code_point.hpp"
#include "src/lib/include/huffman_compress.hpp"
#include "src/lib/include/huffman_compress_internal.hpp"
#include "src/lib/include/huffman_tree_node.hpp"

using namespace huffman_compress;

namespace
{
  const char FILENAME[]            = "huffman_compressor_test.bin";
  const char COMPRESSED_FILENAME[] = "huffman_compressor_test.compressed.bin";

  /* Test helpers */
  //========================================================================================
  bool does_encoding_contain(const unsigned char c, const std::vector<huffman_code_point> &vec)
  {
    return std::count_if(vec.begin(), vec.end(), [&](const huffman_code_point &e) { return e.symbol == c; }) > 0;
  }
  //========================================================================================
  huffman_code_point get_encoding_from_dictionary(const unsigned char c, const std::vector<huffman_code_point> &vec)
  {
    auto iter = std::find_if(vec.begin(), vec.end(), [&](const huffman_code_point &e) { return e.symbol == c; });

    if (iter == vec.end())
    {
      throw std::runtime_error("Failed to find symbol");
    }
    return *iter;
  }
  //========================================================================================
  std::vector<unsigned char> get_random_vector(const std::size_t size_bytes)
  {
    std::srand(1995);
    std::vector<unsigned char> ret(size_bytes);
    std::generate(ret.begin(), ret.end(), []() { return std::rand() % (UCHAR_MAX + 1); });
    return ret;
  }
  //========================================================================================
  void write_vector_to_file(const std::vector<unsigned char> &vec, const std::string &filename)
  {
    std::ofstream ofs(filename);
    if (!ofs.good())
    {
      throw std::runtime_error("Failed to open output file");
    }
    ofs.write(reinterpret_cast<const char *>(vec.data()), vec.size());
  }
  //========================================================================================
  std::vector<unsigned char> read_vector_from_file(const std::string &filename)
  {
    std::ifstream infile(filename, std::ios::in | std::ios::binary);
    if (!infile.good())
    {
      throw std::runtime_error("Failed to open file");
    }
    std::vector<unsigned char> buffer;
    infile.seekg(0, std::ios::end);
    auto end_pos = infile.tellg();
    infile.seekg(0, std::ios::beg);

    buffer.resize(end_pos);
    infile.read(reinterpret_cast<char *>(buffer.data()), end_pos);
    return buffer;
  }

} // namespace

/* Tests */
//========================================================================================
TEST_CASE("create_codebook")
{
  /* Preconditions */

  SECTION("The correct codes are generated from the tree")
  {
    /* Test procedure
             root
            /   \
          e      d
         / \
        a  f
          / \
         b   c
    */

    // Leafs
    auto a    = std::make_unique<huffman_tree_node>();
    a->symbol = 'a';
    auto b    = std::make_unique<huffman_tree_node>();
    b->symbol = 'b';
    auto c    = std::make_unique<huffman_tree_node>();
    c->symbol = 'c';
    auto d    = std::make_unique<huffman_tree_node>();
    d->symbol = 'd';

    // Internals
    auto e = std::make_unique<huffman_tree_node>();
    auto f = std::make_unique<huffman_tree_node>();
    // Root
    auto root = std::make_unique<huffman_tree_node>();

    f->left     = std::move(b);
    f->right    = std::move(c);
    e->left     = std::move(a);
    e->right    = std::move(f);
    root->left  = std::move(e);
    root->right = std::move(d);
    /* Test procedure */
    const std::vector<huffman_code_point> encoding = create_codebook(root);
    /* Pass criteria */
    REQUIRE(does_encoding_contain('a', encoding));
    REQUIRE(does_encoding_contain('b', encoding));
    REQUIRE(does_encoding_contain('c', encoding));
    REQUIRE(does_encoding_contain('d', encoding));

    const auto a_encoding = get_encoding_from_dictionary('a', encoding);
    REQUIRE(a_encoding.num_bits == 2);
    REQUIRE(a_encoding.code == 0b00);

    const auto b_encoding = get_encoding_from_dictionary('b', encoding);
    REQUIRE(b_encoding.num_bits == 3);
    REQUIRE(b_encoding.code == 0b010);

    const auto c_encoding = get_encoding_from_dictionary('c', encoding);
    REQUIRE(c_encoding.num_bits == 3);
    REQUIRE(c_encoding.code == 0b110);

    const auto d_encoding = get_encoding_from_dictionary('d', encoding);
    REQUIRE(d_encoding.num_bits == 1);
    REQUIRE(d_encoding.code == 0b1);
  }
}

//========================================================================================
TEST_CASE("create_canonical_codebook")
{
  SECTION("For a given codebook, it is transformed in to canonical form")
  {
    /* Preconditions */
    const std::vector<huffman_code_point> encoding = {{3, 2, 'a'}, {0, 1, 'b'}, {5, 3, 'c'}, {4, 3, 'd'}};
    /* Test procedure */
    const std::vector<huffman_code_point> canonical_encoding = create_canonical_codebook(encoding);

    /* Pass criteria */
    REQUIRE(does_encoding_contain('a', encoding));
    REQUIRE(does_encoding_contain('b', encoding));
    REQUIRE(does_encoding_contain('c', encoding));
    REQUIRE(does_encoding_contain('d', encoding));

    REQUIRE(canonical_encoding.size() == encoding.size());
    const bool code_length_sorted = std::is_sorted(
        canonical_encoding.begin(), canonical_encoding.end(),
        [](const huffman_code_point &a, const huffman_code_point &b) { return a.num_bits < b.num_bits; });
    REQUIRE(code_length_sorted == true);

    const bool code_sorted =
        std::is_sorted(canonical_encoding.begin(), canonical_encoding.end(),
                       [](const huffman_code_point &a, const huffman_code_point &b) { return a.code < b.code; });
    REQUIRE(code_sorted == true);

    REQUIRE(canonical_encoding[0].symbol == 'b');
    REQUIRE(canonical_encoding[1].symbol == 'a');
    REQUIRE(canonical_encoding[2].symbol == 'c');
    REQUIRE(canonical_encoding[3].symbol == 'd');

    REQUIRE(canonical_encoding[0].code == 0);
    REQUIRE(canonical_encoding[0].num_bits == 1);

    REQUIRE(canonical_encoding[1].code == 1);
    REQUIRE(canonical_encoding[1].num_bits == 2);

    REQUIRE(canonical_encoding[2].code == 3);
    REQUIRE(canonical_encoding[2].num_bits == 3);

    REQUIRE(canonical_encoding[3].code == 7);
    REQUIRE(canonical_encoding[3].num_bits == 3);
  }
}

//========================================================================================
TEST_CASE("Test compress decompress data")
{
  SECTION("1000 random bytes")
  {
    /* Preconditions */
    const std::size_t                SIZE_BYTES = 1000;
    const std::vector<unsigned char> input      = get_random_vector(SIZE_BYTES);
    write_vector_to_file(input, FILENAME);

    /* Test procedure */
    compress(FILENAME, COMPRESSED_FILENAME);
    decompress(COMPRESSED_FILENAME, FILENAME);

    const std::vector<unsigned char> output = read_vector_from_file(FILENAME);

    /* Pass criteria */
    REQUIRE(input.size() == output.size());
    REQUIRE(std::memcmp(input.data(), output.data(), input.size()) == 0);
  }

  SECTION("1 MB random bytes")
  {
    /* Preconditions */
    const std::size_t                SIZE_BYTES = 1024 * 1024;
    const std::vector<unsigned char> input      = get_random_vector(SIZE_BYTES);
    write_vector_to_file(input, FILENAME);

    /* Test procedure */
    compress(FILENAME, COMPRESSED_FILENAME);
    decompress(COMPRESSED_FILENAME, FILENAME);

    const std::vector<unsigned char> output = read_vector_from_file(FILENAME);

    /* Pass criteria */
    REQUIRE(input.size() == output.size());
    REQUIRE(std::memcmp(input.data(), output.data(), input.size()) == 0);
  }

  SECTION("10 MB random bytes")
  {
    /* Preconditions */
    const std::size_t                SIZE_BYTES = 10 * 1024 * 1024;
    const std::vector<unsigned char> input      = get_random_vector(SIZE_BYTES);
    write_vector_to_file(input, FILENAME);

    /* Test procedure */
    compress(FILENAME, COMPRESSED_FILENAME);
    decompress(COMPRESSED_FILENAME, FILENAME);

    const std::vector<unsigned char> output = read_vector_from_file(FILENAME);

    /* Pass criteria */
    REQUIRE(input.size() == output.size());
    REQUIRE(std::memcmp(input.data(), output.data(), input.size()) == 0);
  }

  SECTION("1 KB of zeros then 1KB of ones")
  {
    /* Preconditions */
    const std::size_t          SIZE_BYTES = 2 * 1024;
    std::vector<unsigned char> input(SIZE_BYTES, 0);
    std::fill_n(input.begin(), SIZE_BYTES / 2, 0);
    std::fill_n(input.begin() + SIZE_BYTES / 2, SIZE_BYTES / 2, 1);
    write_vector_to_file(input, FILENAME);

    /* Test procedure */
    compress(FILENAME, COMPRESSED_FILENAME);
    decompress(COMPRESSED_FILENAME, FILENAME);

    const std::vector<unsigned char> output = read_vector_from_file(FILENAME);

    /* Pass criteria */
    REQUIRE(input.size() == output.size());
    REQUIRE(std::memcmp(input.data(), output.data(), input.size()) == 0);
  }

  SECTION("1 KB of zeros")
  {
    /* Preconditions */
    const std::size_t                SIZE_BYTES = 1 * 1024;
    const std::vector<unsigned char> input(SIZE_BYTES, 0);
    write_vector_to_file(input, FILENAME);

    /* Test procedure */
    compress(FILENAME, COMPRESSED_FILENAME);
    decompress(COMPRESSED_FILENAME, FILENAME);

    const std::vector<unsigned char> output = read_vector_from_file(FILENAME);

    /* Pass criteria */
    REQUIRE(input.size() == output.size());
    REQUIRE(std::memcmp(input.data(), output.data(), input.size()) == 0);
  }

  SECTION("Empty file")
  {
    /* Preconditions */
    const std::size_t                SIZE_BYTES = 0;
    const std::vector<unsigned char> input(SIZE_BYTES, 0);
    write_vector_to_file(input, FILENAME);

    /* Test procedure */
    compress(FILENAME, COMPRESSED_FILENAME);
    decompress(COMPRESSED_FILENAME, FILENAME);

    const std::vector<unsigned char> output = read_vector_from_file(FILENAME);

    /* Pass criteria */
    REQUIRE(input.size() == output.size());
    REQUIRE(std::memcmp(input.data(), output.data(), input.size()) == 0);
  }

  SECTION("One byte file")
  {
    /* Preconditions */
    const std::size_t                SIZE_BYTES = 1;
    const std::vector<unsigned char> input(SIZE_BYTES, 0x5A);
    write_vector_to_file(input, FILENAME);

    /* Test procedure */
    compress(FILENAME, COMPRESSED_FILENAME);
    decompress(COMPRESSED_FILENAME, FILENAME);

    const std::vector<unsigned char> output = read_vector_from_file(FILENAME);

    /* Pass criteria */
    REQUIRE(input.size() == output.size());
    REQUIRE(std::memcmp(input.data(), output.data(), input.size()) == 0);
  }

  SECTION("Equal frequency of each symbol")
  {
    /* Preconditions */
    // 512 bytes of each byte value
    const std::size_t          SIZE_BYTES = 256 * 512;
    std::vector<unsigned char> input;
    input.reserve(SIZE_BYTES);
    for (std::uint32_t value = 0; value <= UCHAR_MAX; ++value)
    {
      std::vector<unsigned char> tmp(512, static_cast<unsigned char>(value));
      input.insert(input.end(), tmp.begin(), tmp.end());
    }
    REQUIRE(input.size() == SIZE_BYTES);
    write_vector_to_file(input, FILENAME);

    /* Test procedure */
    compress(FILENAME, COMPRESSED_FILENAME);
    decompress(COMPRESSED_FILENAME, FILENAME);

    const std::vector<unsigned char> output = read_vector_from_file(FILENAME);

    /* Pass criteria */
    REQUIRE(input.size() == output.size());
    REQUIRE(std::memcmp(input.data(), output.data(), input.size()) == 0);
  }

  if (std::filesystem::exists(FILENAME))
  {
    std::filesystem::remove_all(FILENAME);
  }
  if (std::filesystem::exists(COMPRESSED_FILENAME))
  {
    std::filesystem::remove_all(COMPRESSED_FILENAME);
  }
}

//========================================================================================
TEST_CASE("Test compress decompress exceptions")
{
  SECTION("No input file throws")
  {
    /* Preconditions */
    if (std::filesystem::exists(FILENAME))
    {
      std::filesystem::remove_all(FILENAME);
    }
    /* Pass criteria */
    REQUIRE_THROWS(compress(FILENAME, COMPRESSED_FILENAME));
  }

  SECTION("Empty input filename throws")
  {
    /* Pass criteria */
    REQUIRE_THROWS(compress("", COMPRESSED_FILENAME));
  }
}
