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

  auto const args = parse_args(argc, argv);

  if (!args) {
    return -1;
  }

  std::vector<double> data;
  if (!read_f64(args->infile, data)) {
    std::cerr << "error reading file\n";
    return -1;
  }

  prefilter_t prefilt;
  if (!get_prefilter(args->spline_order, &prefilt)) {
    std::cerr << "getting prefilter failed, likely spline order too large\n";
    return -1;
  }

  std::vector<int32_t> truncation_indices(prefilt.nPoles);
  compute_truncation(truncation_indices.data(), prefilt.poles, prefilt.nPoles,
                     args->epsilon);

  if (!splinter_prefilter_inplace2d(data.data(), args->width, args->height,
                                    args->ext, args->spline_order,
                                    truncation_indices.data())) {
    std::cerr << "error computing prefilter\n";
    return 0;
  }

  auto const outfile = [ext = args->ext, spline_order = args->spline_order,
                        epsilon = args->epsilon](std::filesystem::path infile) {
    infile.replace_extension(std::format("o{}.eps{:.6}.{}.f64", spline_order,
                                         epsilon, to_cstr(ext)));
    return infile;
  }(args->infile);

  if (!write_f64(outfile, data)) {
    std::cerr << "error writing data to file\n";
    return -1;
  }

  std::cout << std::format("result written to '{}'\n", outfile.string());

  return 0;
}

static void show_usage(char const *const program) {
  std::cout
      << std::format(
             "Usage: {} <file> <width> <height> <order> <boundary> <eps>\n"
             "Calculat the normalized prefiltering of a 2D image\n"
             "\n"
             "  file:     f64 float raw image data\n"
             "  width:    width of the image (row major)\n"
             "  height:   height of the image\n"
             "  order:    spline order\n"
             "  boundary: boundary extension, values: 'hsym', 'wsym', 'peri', "
             "'cons'\n"
             "  eps:      f64 value for the presicion\n",
             program

             )
      << std::endl;
}

static std::optional<Args> parse_args(int const argc,
                                      char const *const *const argv) {
  if (argc != 6 + 1) {
    show_usage(argv[0]);
    return {};
  }

  std::filesystem::path infile = argv[1];

  int width = std::stoi(argv[2]);
  int height = std::stoi(argv[3]);

  if (width <= 0 || height <= 0) {
    std::cerr << "width and height must be > 0\n";
    return {};
  }

  int order = std::stoi(argv[4]);

  if (order < 0 || order > 16) {
    std::cerr << "spline order must be in [0,16]\n";
    return {};
  }

  auto const maybe_ext = try_from(argv[5]);

  if (!maybe_ext) {
    std::cerr << "unknown boundary condition\n";
    return {};
  }

  double eps = std::stod(argv[6]);

  if (eps <= 0) {
    std::cerr << "wtf...\n";
    return {};
  }

  return Args{
      .infile = infile,
      .width = width,
      .height = height,
      .ext = *maybe_ext,
      .spline_order = static_cast<uint8_t>(order),
      .epsilon = eps,
  };
}
