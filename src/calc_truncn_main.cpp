#include <cstdlib>
#include <format>
#include <iostream>
#include <string>
#include <vector>

#include "bspline.h"

static void show_usage(char const *appname);

int main(int const argc, char const *const *const argv) {
  if (argc != 2 + 1) {
    show_usage(argv[0]);
    return -1;
  }

  std::cout << std::format("alpha = {}, eps = {}\n", argv[1], argv[2]);
  int order = std::stoi(argv[1]);
  if (order < 0 || order > 11) {
    std::cerr << "order must be integer in [0,11]" << std::endl;
    return -1;
  }

  double eps = std::stod(argv[2]);
  if (eps <= 0) {
    std::cerr << "epsilon must be > 0" << std::endl;
    return -1;
  }

  prefilter_t p;
  if (!get_prefilter(order, &p)) {
    std::cerr << "error getting spline args, likely wrong order\n";
    return -1;
  }

  std::vector<int> indices(p.nPoles);

  compute_truncation(indices.data(), p.poles, p.nPoles, eps);

  std::cout << "truncation indices: [";
  for (int j = 0; j < indices.size()-1; j++) {
    std::cout << indices[j] << ',';
  }
  std::cout << indices.back() << ']' << '\n';

  return 0;
}

static void show_usage(char const *const appname) {
  std::cout
      << std::format(
             "Calculate the trunctation indices for a given spline order\n\n"
             "Usage:\n"
             "{} <order> <epsilon>\n"
             "\n"
             "Arguments:\n"
             "   order:   spline order in [0,11]\n"
             "   epsilon: precision > 0\n",
             appname)
      << std::endl;
}
