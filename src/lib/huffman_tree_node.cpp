
#include "src/lib/include/huffman_tree_node.hpp"

namespace huffman_compress
{
  //==================================================================
  huffman_tree_node::huffman_tree_node() : left{}, right{}, weight{0}, symbol{}
  {
  }
} // namespace huffman_compress