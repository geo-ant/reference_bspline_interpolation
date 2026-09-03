#ifndef COMMON_UTILITY_HPP_INCLUDED
#define COMMON_UTILITY_HPP_INCLUDED

#include "splinter.h"
#include <filesystem>
#include <optional>
#include <span>
#include <vector>

bool read_f64(std::filesystem::path path, std::vector<double> &out);
bool write_f64(std::filesystem::path path, std::span<double const> data);
char const *to_cstr(BoundaryExt ext);

std::optional<BoundaryExt> try_from(int value);
std::optional<BoundaryExt> try_from(char const* const str);

#endif
