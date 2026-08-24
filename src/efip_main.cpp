#include "splinter.h"
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// quick and dirty tool to apply the expontial filter in-place to some
// double precision floating point data.

struct Args {
  BoundaryExt extension;
  int n_trunc;
  double alpha;
  std::filesystem::path file;
};

static void show_usage(char const *appname);
static bool read_f64(std::filesystem::path path, std::vector<double> &out);
static bool write_f64(std::filesystem::path path, std::span<double const> data);
static bool parse_args(int argc, char const *const *argv, Args &out_args);

int main(int argc, char const *const *argv) {

  Args args;
  if (!parse_args(argc, argv, args)) {
    return -1;
  }

  std::vector<double> data;
  if (!read_f64(args.file, data)) {
    return -1;
  }

  splinter_expfilter(data.data(), 1, data.size(), args.extension, args.alpha, args.n_trunc);

  std::filesystem::path const out = [args](){
    auto temp(args.file);
    temp.replace_extension("filtered.f64");
    return temp;
  }();
  
  write_f64(out, data);
  std::cout << std::format("written to: '{}'\n",out.string());

  return 0;
}

static bool parse_args(int argc, char const *const *argv, Args &out_args) {
  if (argc - 1 != 4) {
    show_usage(argv[0]);
    return -1;
  }

  using BoundaryExtUnderlying = std::underlying_type_t<BoundaryExt>;
  BoundaryExtUnderlying const ext_val = std::stoi(argv[1]);
  BoundaryExt extension;

  switch (static_cast<BoundaryExtUnderlying>(ext_val)) {
    case BOUNDARY_PERIODIC: { extension = BOUNDARY_PERIODIC; } break;
    case BOUNDARY_WSYMMETRIC: { extension = BOUNDARY_WSYMMETRIC; } break;
    case BOUNDARY_CONSTANT: { extension = BOUNDARY_CONSTANT; } break;
    case BOUNDARY_HSYMMETRIC: { extension = BOUNDARY_HSYMMETRIC; } break;
  default: {
    std::cerr << "Illegal value for boundary extension" << std::endl;
    return false;
  }
  }

  int const n_trunc = std::stoi(argv[2]);
  if (n_trunc <= 0) {
    std::cerr << "truncation n must be > 0" << std::endl;
    return false;
  }

  double const alpha = std::stof(argv[3]);
  if (alpha >= 0 || alpha <= -1) {
    std::cerr << "alpha must be in (-1,0)" << std::endl;
    return false;
  }

  std::string filename(argv[4]);
  if (filename.empty()) {
    std::cerr << "filename must not be empty" << std::endl;
    return false;
  }

  out_args = Args{.extension = extension,
                  .n_trunc = n_trunc,
                  .alpha = alpha,
                  .file = filename};

  return true;
}

static void show_usage(char const *appname) {
  std::cout << std::format(
      "Usage\n"
      "{} <boundary> <N_trunc> <alpha> <file>\n"
      "\n"
      "Arguments:\n"
      "   boundary: boundary extension. "
      "{} = constant, {} = half-sym, {} = whole-sym, {} = periodic\n"
      "   N_trunc:  trunctation N\n"
      "   alpha:    float64 value in (-1,0) for the pole\n"
      "   file:     file with the signal of float64 values\n",
      appname, std::to_underlying(BoundaryExt::BOUNDARY_CONSTANT),
      std::to_underlying(BoundaryExt::BOUNDARY_HSYMMETRIC),
      std::to_underlying(BoundaryExt::BOUNDARY_WSYMMETRIC),
      std::to_underlying(BoundaryExt::BOUNDARY_PERIODIC));
}

static bool read_f64(std::filesystem::path path, std::vector<double> &out) {
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

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <limits>
#include <span>

static bool write_f64(std::filesystem::path path,
                      std::span<double const> data)
{
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file)
        return false;

    const auto bytes = std::as_bytes(data);
    std::size_t offset = 0;

    constexpr auto max_write =
        static_cast<std::size_t>(
            std::numeric_limits<std::streamsize>::max());

    while (offset < bytes.size()) {
        const std::size_t remaining = bytes.size() - offset;
        const auto count = static_cast<std::streamsize>(
            remaining < max_write ? remaining : max_write);

        file.write(
            reinterpret_cast<char const*>(bytes.data() + offset),
            count);

        if (!file)
            return false;

        offset += static_cast<std::size_t>(count);
    }

    return true;
}
