#include "common_utils.hpp"
#include "splinter.h"
#include <fstream>
#include <iostream>
#include <optional>
#include <type_traits>

bool read_f64(std::filesystem::path path, std::vector<double> &out) {
  try {
    std::ifstream file(path);
    file.seekg(0, std::ios::end);
    const auto bytes = file.tellg();

    if (bytes < 0 || bytes % sizeof(double) != 0) {
      std::cerr << "Invalid double file" << std::endl;
      return false;
    }

    const auto count = static_cast<std::size_t>(bytes) / sizeof(double);

    file.seekg(0);
    out.resize(count);
    file.read(reinterpret_cast<char *>(out.data()),
              static_cast<std::streamsize>(out.size() * sizeof(double)));
    return true;
  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return false;
  }
}

bool write_f64(std::filesystem::path path, std::span<double const> data) {
  std::ofstream file(path, std::ios::binary | std::ios::trunc);
  if (!file)
    return false;

  const auto bytes = std::as_bytes(data);
  std::size_t offset = 0;

  constexpr auto max_write =
      static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max());

  while (offset < bytes.size()) {
    const std::size_t remaining = bytes.size() - offset;
    const auto count = static_cast<std::streamsize>(
        remaining < max_write ? remaining : max_write);

    file.write(reinterpret_cast<char const *>(bytes.data() + offset), count);

    if (!file)
      return false;

    offset += static_cast<std::size_t>(count);
  }

  return true;
}

char const *to_cstr(BoundaryExt ext) {
  switch (ext) {
  case BOUNDARY_CONSTANT:
    return "cons";
  case BOUNDARY_HSYMMETRIC:
    return "hsym";
  case BOUNDARY_WSYMMETRIC:
    return "wsym";
  case BOUNDARY_PERIODIC:
    return "peri";
  default:
    // shouldn't happen
    return "UNKNOWN";
  }
}

std::optional<BoundaryExt> try_from(int const value) {
  using BoundaryExtUnderlying = std::underlying_type_t<BoundaryExt>;
  switch (static_cast<BoundaryExtUnderlying>(value)) {
  case BOUNDARY_PERIODIC: {
    return BOUNDARY_PERIODIC;
  } break;
  case BOUNDARY_WSYMMETRIC: {
    return BOUNDARY_WSYMMETRIC;
  } break;
  case BOUNDARY_CONSTANT: {
    return BOUNDARY_CONSTANT;
  } break;
  case BOUNDARY_HSYMMETRIC: {
    return BOUNDARY_HSYMMETRIC;
  } break;
  default: {
    return {};
  }
  }
}
