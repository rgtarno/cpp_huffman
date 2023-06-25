# Huffman encoding based compression and decompresion

A C++ implementation of a Huffman encoder and decoder.

The encoder reads the input file twice. The first time is creates a symbol frequency map,
and transforms that in to a canonical Huffman encoding for each symbol. On the second read
it substitutes each input byte using the encoding and writes out the compressed file.

The 256 byte encoding is prepended to the file, along with 1 byte containing the number of pad bits.

The decoder reads the prepended encoding and regenerates the symbol mapping, substituting each symbol for the
original bytes to recover the file.

Only tested on linux.

## Bazel build

1. [Install Bazel](https://bazel.build/install)
2. Build the compress / decompress util: `bazel  build --compilation_mode=dbg //src/app:huffman_compressor`
3. Run tests: `bazel test --verbose_failures --compilation_mode=opt //src/lib/tests/...`
