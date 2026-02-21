#ifndef MAIN_HELPERS_H
#define MAIN_HELPERS_H

#include "matrix-helpers.h"
#include "preprocess-helpers.h"
#include "config.h"

int preprocess(const Gaussian *gaussians, const Mat33 *cov3d, int n,
               const float cam_pos[3], const float cam_angles[3],
               float fovx, float fovy, int W, int H,
               Splat *out_splats);

#endif
