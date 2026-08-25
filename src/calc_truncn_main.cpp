#include <cstdlib>
#include <format>
#include <iostream>
#include <string>

#include "bspline.h"

static void show_usage(char const *appname);

int main(int const argc, char const *const * const argv) {
  if (argc != 2+1) {
    show_usage(argv[0]);
    return -1;
  }

  std::cout << std::format("alpha = {}, eps = {}\n", argv[1], argv[2]);
  double alpha = std::stod(argv[1]);
  if (alpha <= -1 || alpha > 0) {
    std::cerr << "pole must be in (-1,0)" << std::endl;
    return -1;
  } 

  double eps = std::stod(argv[2]);
  if (eps <= 0) {
    std::cerr << "epsilon must be > 0" << std::endl;
    return -1;
  }

  int trunc = -1;
  compute_truncation(&trunc, &alpha, 1, eps);
  if (trunc < 0) {
    std::cerr << "something went horribly wrong!" << std::endl;
    return -1;
  }

  std::cout << "truncation index: " << trunc << std::endl;
  return 0;
}

static void show_usage(char const *const appname) {
  std::cout << std::format("Calculate the truncation index for a given pole in "
                           "(-1,0) with precision epsilon > 0\n\n"
                           "Usage:\n"
                           "{} <pole> <epsilon>\n"
                           "\n"
                           "Arguments:\n"
                           "   pole:    pole in (-1,0)\n"
                           "   epsilon: precision > 0\n",
                           appname)
            << std::endl;
}
