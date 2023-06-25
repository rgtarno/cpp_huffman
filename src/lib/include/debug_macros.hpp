
#pragma once

#include "fmt/core.h"

constexpr const char *get_basename(const char *path)
{
  const char *file = path;
  while (*path)
  {
    if (*path++ == '/')
    {
      file = path;
    }
  }
  return file;
}

#define debug(...)                                                                                                     \
  fmt::print("[{}:{}] ", get_basename(__FILE__), __LINE__);                                                            \
  fmt::print(__VA_ARGS__);                                                                                             \
  fmt::print("\n")

#define warn(...)                                                                                                      \
  fmt::print("ERROR: ");                                                                                               \
  fmt::print(__VA_ARGS__);                                                                                             \
  fmt::print("\n")

#define error(...)                                                                                                     \
  fmt::print("ERROR: ");                                                                                               \
  fmt::print(__VA_ARGS__);                                                                                             \
  fmt::print("\n")
