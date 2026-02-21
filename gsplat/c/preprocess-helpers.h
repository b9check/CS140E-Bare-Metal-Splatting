#ifndef PREPROCESS_HELPERS
#define PREPROCESS_HELPERS

#include "matrix-helpers.h"

// Matches .gsplat binary layout: 14 floats = 56 bytes per record.
typedef struct {
    float pos[3];
    float scale[3];
    float opacity;
    float rot[4];
    float color[3];
} Gaussian;

// Use radius == 0 as sentinel for invalid/singular covariance (no separate bool).
typedef struct {
    float conic[3];
    int radius;
} ConicRadius;

typedef struct {
    float px, py;
    float depth;
    float conic[3];
    float opacity;
    float color[3];
    int radius;
} Splat;

Mat33 compute_cov_3d(float scale[3], float rot[4]);
Mat44 build_view_matrix(const float cam_angles[3], const float cam_pos[3]);
Vec3 transform_to_camera(const float gauss_pos[3], const Mat44 *view_matrix);
Vec2 project_to_screen(const float pos[3], const Mat44 *proj_matrix, int W, int H);
Mat22 compute_cov2d(const float pos[3], const Mat33 *cov3d, const Mat44 *view_matrix,
                    float fovx, float fovy, int W, int H);
ConicRadius prepare_conic_and_radius(const Mat22 *cov2d);
Mat44 build_proj_matrix(const Mat44 *view_matrix, float fovx, float fovy,
                        float znear, float zfar);

#endif