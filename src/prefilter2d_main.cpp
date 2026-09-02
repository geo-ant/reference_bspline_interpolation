#include "bspline.h"
#include "common_utils.hpp"
#include "splinter.h"
#include <cstdint>
#include <filesystem>
#include <format>
#include <iostream>
#include <optional>
#include <string>

struct Args {
  std::filesystem::path infile;
  int width;
  int height;
  BoundaryExt ext;
  uint8_t spline_order;
  double epsilon;
};

static std::optional<Args> parse_args(int argc, char const *const *argv);

int main(int const argc, char const *const *const argv) {

  std::vector<double> data;
  std::filesystem::path infile;
  if (!read_f64(infile, data)) {
    std::cerr << "error reading file\n";
    return -1;
  }

  int width;
  int height;
  BoundaryExt ext;
  uint8_t spline_order;
  double epsilon;

  prefilter_t prefilt;
  if (!get_prefilter(spline_order, &prefilt)) {
    std::cerr << "getting prefilter failed, likely spline order too large\n";
    return -1;
  }

  std::vector<int32_t> truncation_indices(prefilt.nPoles);
  compute_truncation(truncation_indices.data(), prefilt.poles, prefilt.nPoles,
                     epsilon);

  if (!splinter_prefilter_inplace2d(data.data(), width, height, ext,
                                    spline_order, truncation_indices.data())) {
    std::cerr << "error computing prefilter\n";
    return 0;
  }

  auto const outfile = [ext, spline_order,
                        epsilon](std::filesystem::path infile) {
    infile.replace_extension(std::format("o{}.eps{:.6}.{}.f64", spline_order,
                                         epsilon, to_cstr(ext)));
    return infile;
  }(infile);

  if (!write_f64(outfile, data)) {
    std::cerr << "error writing data to file\n";
    return -1;
  }

  std::cout << std::format("result written to '{}'\n", outfile.string());

  return 0;
}

static std::optional<Args> parse_args(int const argc,
                                      char const *const *const argv) {

  // struct Args {
  //   int width;
  //   int height;
  //   BoundaryExt ext;
  //   uint8_t spline_order;
  //   double epsilon;
  // };

  if (argc != 6 + 1) {
    todo show usage return {};
  }


  
}
