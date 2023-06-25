
#include "src/lib/include/bit_writer.hpp"

#include <cassert>
#include <climits>
#include <cmath>

namespace
{
  constexpr std::size_t BUFFER_SIZE_BITS = sizeof(std::uint32_t) * 8;
}

//==================================================================
huffman_compress::bit_writer::~bit_writer()
{
  flush();
  assert(_size == 0);
}

//==================================================================
/**
 * @brief Write any buffered output to file
 *
 * If a non integer number of bytes remain in the buffer, zeros are padded
 * until the next byte is reached.
 *
 * @return std::uint8_t Number of pad zero bits written
 */
std::uint8_t huffman_compress::bit_writer::flush()
{
  if (_size % 8 == 0)
  {
    const std::size_t bytes_left = _size / 8;
    if (!bytes_left)
    {
      return 0;
    }
    std::size_t         pos  = 0;
    const std::uint32_t mask = 0xFF;
    while ((pos / 8) < bytes_left)
    {
      std::uint8_t byte = static_cast<std::uint8_t>(((mask << pos) & _buffer) >> pos);
      write_byte(byte);
      pos += 8;
    }
    _size   = 0;
    _buffer = 0;
    return 0;
  }
  else if (_size != 0)
  {
    const float        next_byte = std::ceil(static_cast<float>(_size) / 8.0f);
    const std::uint8_t pad_bits  = static_cast<std::uint8_t>(next_byte * CHAR_BIT) - static_cast<std::uint8_t>(_size);
    write_bits(0, static_cast<std::size_t>(pad_bits));
    flush();
    return pad_bits;
  }
  return 0;
}

//==================================================================
/**
 * @brief Write num_bits bits starting from the LSB of value to file
 *
 * Bits are read from the LSB of value and are packed in to bytes starting from the LSB before they are written
 * to file.
 *
 * @param value Contains the data to be written
 * @param num_bits Number of bits
 *
 * @throw std::logic_error If num_bits if greater than 32
 */
void huffman_compress::bit_writer::write_bits(const std::uint32_t &value, const std::size_t &num_bits)
{
  const std::size_t buffer_space_bits       = BUFFER_SIZE_BITS - _size;
  std::size_t       remaining_bits_to_write = num_bits;

  if (num_bits > BUFFER_SIZE_BITS)
  {
    throw std::logic_error("Invalid number of bits");
  }

  if (num_bits >= buffer_space_bits)
  {
    _buffer |= (value << _size);
    // Full now
    write_buffer();
    _buffer = 0;
    _size   = 0;
    remaining_bits_to_write -= buffer_space_bits;
    if (remaining_bits_to_write)
    {
      // Empty so can assign
      _buffer = (value >> buffer_space_bits);
      _size += remaining_bits_to_write;
    }
  }
  else
  {
    _buffer |= (value << _size);
    _size += num_bits;
    assert(_size <= BUFFER_SIZE_BITS);
    if (_size == BUFFER_SIZE_BITS)
    {
      write_buffer();
      _buffer = 0;
      _size   = 0;
    }
  }
}

//==================================================================
/**
 * @brief Write a single bit to file
 *
 * @param bit
 */
void huffman_compress::bit_writer::write_bit(const bool bit)
{
  _buffer |= (bit << _size);
  ++_size;

  if (_size == BUFFER_SIZE_BITS)
  {
    write_buffer();
    _buffer = 0;
    _size   = 0;
  }
}
