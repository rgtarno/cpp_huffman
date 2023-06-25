
#include "catch2/catch_all.hpp"

#include "src/lib/include/bit_writer.hpp"

#include <catch2/catch_test_macros.hpp>
#include <climits>
#include <cstdint>
#include <filesystem>
#include <fstream>

#include "src/lib/include/debug_macros.hpp"

namespace
{
  const std::string FILENAME = "bit_writer_test.bin";
}

SCENARIO("single bits can be written")
{
  GIVEN("An empty file")
  {

    std::ofstream file(FILENAME, std::ios::trunc);
    REQUIRE(file.good());

    WHEN("8 bits are written")
    {
      {
        huffman_compress::bit_writer bw(file);
        bw.write_bit(0);
        bw.write_bit(1);
        bw.write_bit(0);
        bw.write_bit(1);
        bw.write_bit(0);
        bw.write_bit(1);
        bw.write_bit(0);
        bw.write_bit(1);
      }
      file.close();

      THEN("the file contains 1 byte and the value of the byte matches the bits written")
      {
        REQUIRE(std::filesystem::file_size(FILENAME) == 1);

        std::ifstream ifs(FILENAME, std::ios::binary);
        std::uint8_t  test = 0;
        ifs.read(reinterpret_cast<char *>(&test), 1);

        REQUIRE(test == 0xAA);
      }
    }

    WHEN("24 bits are written")
    {
      const std::uint8_t aa = 0xaa;
      const std::uint8_t ff = 0xff;
      const std::uint8_t c4 = 0xc4;

      {
        huffman_compress::bit_writer bw(file);

        auto write_byte = [&bw](std::uint8_t val) {
          for (std::size_t i = 0; i < 8; ++i)
          {
            bw.write_bit((val >> i) & 0x01);
          }
        };

        write_byte(aa);
        write_byte(ff);
        write_byte(c4);
      }
      file.close();

      THEN("the file contains 3 bytes and the value of the bytes matches the bits written")
      {

        REQUIRE(std::filesystem::file_size(FILENAME) == 3);

        std::ifstream ifs(FILENAME, std::ios::binary);
        std::uint8_t  test = 0;

        ifs.read(reinterpret_cast<char *>(&test), 1);
        REQUIRE(test == aa);
        ifs.read(reinterpret_cast<char *>(&test), 1);
        REQUIRE(test == ff);
        ifs.read(reinterpret_cast<char *>(&test), 1);
        REQUIRE(test == c4);
      }
    }

    std::filesystem::remove_all(FILENAME);
  }
}

TEST_CASE("Test bit_writer write multiple bits")
{
  /* Preconditions */
  std::ofstream file(FILENAME, std::ios::trunc);
  REQUIRE(file.good());

  SECTION("Write 16 bits")
  {
    /* Test procedure */
    const std::uint16_t VALUE    = 0xdead;
    const std::size_t   NUM_BITS = sizeof(VALUE) * CHAR_BIT;
    {
      huffman_compress::bit_writer bw(file);
      bw.write_bits(VALUE, NUM_BITS);
    }
    file.close();

    /* Pass criteria */
    REQUIRE(std::filesystem::file_size(FILENAME) == sizeof(VALUE));

    std::ifstream ifs(FILENAME, std::ios::binary);
    REQUIRE(ifs.good());
    std::uint16_t test = 0;
    ifs.read(reinterpret_cast<char *>(&test), sizeof(test));

    /* Clean up */
    REQUIRE(test == VALUE);
  }

  SECTION("Write 12 bits")
  {
    /* Test procedure */
    const std::uint16_t VALUE    = 0x0AAA;
    const std::size_t   NUM_BITS = 12;
    {
      huffman_compress::bit_writer bw(file);
      bw.write_bits(VALUE, NUM_BITS);
    }
    file.close();

    /* Pass criteria */
    REQUIRE(std::filesystem::file_size(FILENAME) == sizeof(VALUE));

    std::ifstream ifs(FILENAME, std::ios::binary);
    REQUIRE(ifs.good());
    std::uint16_t test = 0;
    ifs.read(reinterpret_cast<char *>(&test), sizeof(test));

    REQUIRE(test == VALUE);
  }

  SECTION("Write 0 bits")
  {
    /* Test procedure */
    {
      huffman_compress::bit_writer bw(file);
    }
    file.close();

    /* Pass criteria */
    REQUIRE(std::filesystem::file_size(FILENAME) == 0);
  }

  SECTION("Write invalid number of bits")
  {
    /* Test procedure */
    const std::uint32_t VALUE    = 0x12345678;
    const std::size_t   NUM_BITS = 50;
    {
      huffman_compress::bit_writer bw(file);
      /* Pass criteria */
      REQUIRE_THROWS(bw.write_bits(VALUE, NUM_BITS));
    }
    file.close();
  }

  SECTION("Write several bytes with partial last byte")
  {
    /* Test procedure */
    const std::uint32_t VALUE1     = 0x12345678;
    const std::uint32_t VALUE2     = 0xcafebabe;
    const std::uint32_t VALUE3     = 0xfee1dead;
    const std::uint32_t VALUE4     = 0xdeadbeef;
    const std::uint32_t VALUE_LAST = 0x0000000d;

    {
      huffman_compress::bit_writer bw(file);
      for (std::size_t i = 0; i < 10; ++i)
      {
        bw.write_bits(VALUE1, 32);
        bw.write_bits(VALUE2, 32);
        bw.write_bits(VALUE3, 32);
        bw.write_bits(VALUE4, 32);
      }
      bw.write_bits(VALUE_LAST, 4);
    }
    file.close();

    /* Pass criteria */
    REQUIRE(std::filesystem::file_size(FILENAME) == 161); // (4 * 4 * 10) + 1

    std::ifstream ifs(FILENAME, std::ios::binary);
    REQUIRE(ifs.good());
    std::uint32_t test = 0;

    for (std::size_t i = 0; i < 10; ++i)
    {
      ifs.read(reinterpret_cast<char *>(&test), sizeof(test));
      REQUIRE(test == VALUE1);
      ifs.read(reinterpret_cast<char *>(&test), sizeof(test));
      REQUIRE(test == VALUE2);
      ifs.read(reinterpret_cast<char *>(&test), sizeof(test));
      REQUIRE(test == VALUE3);
      ifs.read(reinterpret_cast<char *>(&test), sizeof(test));
      REQUIRE(test == VALUE4);
    }
    std::uint8_t last = 0;
    ifs.read(reinterpret_cast<char *>(&last), sizeof(last));
    REQUIRE(last == VALUE_LAST);
  }

  /* Clean up */
  std::filesystem::remove_all(FILENAME);
}

TEST_CASE("Test bit_writer pad bits reporting")
{
  /* Preconditions */
  std::ofstream file(FILENAME, std::ios::trunc);
  REQUIRE(file.good());

  SECTION("Write 1 bit, expect 7 pad bits")
  {
    /* Test procedure */
    const std::uint16_t VALUE    = 0x0001;
    const std::size_t   NUM_BITS = 1;
    {
      huffman_compress::bit_writer bw(file);
      bw.write_bits(VALUE, NUM_BITS);

      /* Pass criteria */
      REQUIRE(bw.flush() == 7);
    }
    file.close();
  }

  SECTION("Write 12 bits, expect 4 pad bits")
  {
    /* Test procedure */
    const std::uint16_t VALUE    = 0x0AAA;
    const std::size_t   NUM_BITS = 12;
    {
      huffman_compress::bit_writer bw(file);
      bw.write_bits(VALUE, NUM_BITS);

      /* Pass criteria */
      REQUIRE(bw.flush() == 4);
    }
    file.close();
  }

  SECTION("Write 29 bits, expect 3 pad bits")
  {
    /* Test procedure */
    const std::uint32_t VALUE    = 0xFFFFFFFF;
    const std::size_t   NUM_BITS = 29;
    {
      huffman_compress::bit_writer bw(file);
      bw.write_bits(VALUE, NUM_BITS);

      /* Pass criteria */
      REQUIRE(bw.flush() == 3);
    }
    file.close();
  }

  /* Clean up */
  std::filesystem::remove_all(FILENAME);
}
