
#pragma once

#include <cstdint>
#include <fstream>

namespace huffman_compress
{
  /**
   * @brief Write bits to a file
   *
   * If a non integer number of bytes have been written at destruction,
   * the file will be padded with zeros.
   *
   */
  class bit_writer
  {
  public:
    explicit bit_writer(std::ofstream &ofs) : _buffer(0), _size(0), _ofs(ofs) {};
    bit_writer()                              = delete;
    bit_writer(const bit_writer &)            = delete;
    bit_writer(bit_writer &&)                 = delete;
    bit_writer &operator=(const bit_writer &) = delete;
    bit_writer &operator=(bit_writer &&)      = delete;

    ~bit_writer();

    void         write_bits(const std::uint32_t &value, const std::size_t &num_bits);
    void         write_bit(const bool bit);
    std::uint8_t flush();

  private:
    inline void write_buffer()
    {
      _ofs.write(reinterpret_cast<char *>(&_buffer), sizeof(_buffer));
    }

    inline void write_byte(const std::uint8_t byte)
    {
      _ofs.write(reinterpret_cast<const char *>(&byte), sizeof(byte));
    }

    std::uint32_t  _buffer;
    std::size_t    _size;
    std::ofstream &_ofs;
  };
}; // namespace huffman_compress
