#include <string>
#include <unistd.h>
#include <vector>

#include "fmt/core.h"

#include "src/lib/include/debug_macros.hpp"
#include "src/lib/include/huffman_compress.hpp"

//==================================================================
void print_usage(char *program_name)
{
  fmt::print("Usage: {} [OPTIONS]... [FILE]...\n\n", program_name);
  fmt::print("Compresses the files specified by [FILE] using huffman compression.\n");
  fmt::print("By default a new file with the extension .compressed will be created unless otherwise specified by the "
             "options below.\n\n");
  fmt::print("-o  File to write output to\n");
  fmt::print("-d  Decompress\n");
}

//==================================================================
int main(int argc, char **argv)
{
  int c;
  opterr = 0;

  std::string output_file;
  bool        decompress = false;

  std::vector<std::string> input_files;

  while ((c = getopt(argc, argv, "o:d")) != -1)
  {
    switch (c)
    {
    case 'o': {
      output_file = std::string(optarg);
      break;
    }
    case 'd': {
      decompress = true;
      break;
    }
    case '?':
    default: {
      print_usage(argv[0]);
      return 1;
    }
    }
  }

  while (argv[optind] != nullptr)
  {
    input_files.push_back(argv[optind]);
    ++optind;
  }

  if (input_files.size() > 1 && !output_file.empty())
  {
    error("Unable to specify output filename when multiple input files are given");
    return 1;
  }

  const std::string COMPRESSION_SUFFIX(".compressed");

  for (const auto &input_file : input_files)
  {
    if (decompress)
    {
      if (output_file.empty())
      {
        if (input_file.find(COMPRESSION_SUFFIX) != std::string::npos)
        {
          output_file = input_file.substr(0, input_file.length() - COMPRESSION_SUFFIX.length());
        }
      }
    }
    else
    {
      if (output_file.empty())
      {
        output_file = input_file + COMPRESSION_SUFFIX;
      }
    }

    try
    {
      if (decompress)
      {
        huffman_compress::decompress(input_file, output_file);
      }
      else
      {
        huffman_compress::compress(input_file, output_file);
      }
    }
    catch (const std::exception &e)
    {
      error("{}", e.what());
      return 1;
    }
  }

  return 0;
}