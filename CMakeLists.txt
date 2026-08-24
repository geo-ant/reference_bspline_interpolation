cmake_minimum_required(VERSION 3.13)
project(Splinter LANGUAGES C CXX)

# Enable C99
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 23 REQUIRED)
set(CMAKE_EXPORT_COMPILE_COMMANDS On)

option(EXTRAPOLATE "Extrapolate for pixels outside image" OFF)

set(GSL_FIND_QUIETLY TRUE)
find_package(GSL)

# IIO
add_subdirectory(src/iio)

add_library(Splinter src/bspline.c src/bspline.h src/splinter.c src/splinter.h)
set_property(TARGET Splinter PROPERTY COMPILE_WARNING_AS_ERROR ON)

if(GSL_FOUND)
  target_compile_definitions(Splinter PRIVATE GSL_SUPPORT)
  target_link_libraries(Splinter PUBLIC GSL::gsl)
endif()
target_link_libraries(Splinter PUBLIC m)
if(EXTRAPOLATE)
  target_compile_definitions(Splinter PRIVATE EXTRAPOLATE)
endif()

add_executable(bspline src/bspline_main.c src/splinter_transform.c src/homography_tools.c)
target_link_libraries(bspline PRIVATE IIOLIB Splinter)

# there's an error in here...
add_executable(hom4p src/hom4p.c src/homography_tools.c)
target_link_libraries(hom4p PRIVATE m)

# (e)xponential (f)ilter (i)n (p)lace
add_executable(efip src/efip_main.cpp)
target_link_libraries(efip PUBLIC Splinter)

# if(CMAKE_CXX_COMPILER_ID MATCHES "(GNU)|(CLANG)")
#   target_compile_options(Splinter "-Wall -Wextra")
#   target_compile_options(bspline "-Wall -Wextra")
#   target_compile_options(hom4p "-Wall -Wextra")
# endif()

if(GSL_FOUND)
  add_executable (compute_bspline compute_bspline.c bspline.c bspline.h)
  target_compile_definitions(compute_bspline PRIVATE GSL_SUPPORT)
  target_link_libraries(compute_bspline PRIVATE GSL::gsl m)
endif()
