// quick and dirty tool to apply the expontial filter in-place to some
// f64 data.

#include "common_utils.hpp"
#include "splinter.h"
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <span>
#include <string>
#include <utility>
#include <vector>

struct Args {
  BoundaryExt extension;
  int n_trunc;
  double alpha;
  std::filesystem::path file;
};

static void show_usage(char const *appname);
static bool parse_args(int argc, char const *const *argv, Args &out_args);

int main(int const argc, char const *const *const argv) {

  Args args;
  if (!parse_args(argc, argv, args)) {
    return -1;
  }

  std::vector<double> data;
  if (!read_f64(args.file, data)) {
    return -1;
  }

  splinter_expfilter(data.data(), 1, data.size(), args.extension, args.alpha,
                     args.n_trunc);

  std::filesystem::path const out = [args]() {
    auto temp(args.file);
    temp.replace_extension(std::format("filtered.n{}.{}.alpha{:.4}.f64",
                                       args.n_trunc, to_cstr(args.extension),
                                       args.alpha));
    return temp;
  }();

  write_f64(out, data);
  std::cout << std::format("written to: '{}'\n", out.string());

  return 0;
}

static bool parse_args(int argc, char const *const *argv, Args &out_args) {
  if (argc - 1 != 4) {
    show_usage(argv[0]);
    return -1;
  }

  auto const ext_val = std::stoi(argv[1]);
  std::optional<BoundaryExt> maybe_extension = try_from(ext_val);

  if (!maybe_extension) {
    std::cerr << "illegal value for boundary extension" << std::endl;
    return false;
  }

  int const n_trunc = std::stoi(argv[2]);
  if (n_trunc <= 0) {
    std::cerr << "truncation n must be > 0" << std::endl;
    return false;
  }

  double const alpha = std::stod(argv[3]);
  if (alpha >= 0 || alpha <= -1) {
    std::cerr << "alpha must be in (-1,0)" << std::endl;
    return false;
  }

  std::string filename(argv[4]);
  if (filename.empty()) {
    std::cerr << "filename must not be empty" << std::endl;
    return false;
  }

  out_args = Args{.extension = *maybe_extension,
                  .n_trunc = n_trunc,
                  .alpha = alpha,
                  .file = filename};

  return true;
}

static void show_usage(char const *const appname) {
  std::cout << std::format(
      "Apply the exponential filter for pole alpha to float64 data\n\n"
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
