/**
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * @file splinter.h
 * @brief Spline interpolation
 * @author Thibaud Briand <thibaud.briand@enpc.fr>
 *         Pascal Monasse <monasse@imagine.enpc.fr>
 *
 * Copyright (c) 2017-2023, Thibaud Briand, Pascal Monasse
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Pulic License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SPLINTER_H
#define SPLINTER_H

#include "bspline.h"
#include <stdint.h>

/// Boundary extension method used in prefiltering
typedef enum : uint8_t {
  BOUNDARY_CONSTANT = 0,   ///< constant value
  BOUNDARY_HSYMMETRIC = 1, ///< half-symmetric
  BOUNDARY_WSYMMETRIC = 2, ///< whole-symmetric
  BOUNDARY_PERIODIC = 3    ///< periodic
} BoundaryExt;

/// \brief Opaque structure, intended to be used for spline interpolation.
/// \details The usage pattern is modeled after FFTW (http://www.fftw.org).
/// To interpolate, the user must first create a plan with \ref splinter_plan.
/// Calls to function \ref splinter, returning the interpolation value at points
/// (x,y), can then be performed.
/// At the end, disposal is achieved by \ref splinter_destroy_plan.
typedef struct {
  double *prefilt;      ///< prefiltered image
  int w, h, c;          ///< width,height,channels
  int shift;            ///< shift in each channel
  Bspline *bspline;     ///< Bspline kernel
  int (*ext)(int, int); ///< get pixels of extended image
  double *xBuf, *yBuf;  ///< buffers for computation (internal usage)
} splinter_plan_t;

#ifdef __cplusplus
extern "C" {
#endif
splinter_plan_t splinter_plan(const double *in, int w, int h, int c, int order,
                              BoundaryExt e, double eps, int larger);
void splinter_destroy_plan(splinter_plan_t plan);

void splinter(double *out, double x, double y, splinter_plan_t plan);

// expose the exponential filtering method directly
void splinter_expfilter(double *data, int32_t step, int32_t n,
                        BoundaryExt boundary, double alpha, int32_t n0);

// expose the 2d prefiltering fucnction directly
bool splinter_prefilter_inplace2d(double *data, int32_t width, int32_t height,
                                  BoundaryExt ext, uint8_t spline_order,
                                  // number of truncation indices floor(spline_order/2)
                                  int32_t const *truncation_indices);

#ifdef __cplusplus
}
#endif

#endif
