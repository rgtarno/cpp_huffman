
#pragma once

#include <cstdint>
#include <memory>
#include <optional>

namespace huffman_compress
{
  struct huffman_tree_node
  {

    huffman_tree_node();

    std::unique_ptr<huffman_tree_node> left;
    std::unique_ptr<huffman_tree_node> right;
    std::uint64_t                      weight;
    std::optional<unsigned char>       symbol;
  };

}; // namespace huffman_compress