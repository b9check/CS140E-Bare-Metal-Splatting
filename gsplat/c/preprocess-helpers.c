#include <math.h>
#include "matrix-helpers.h"
#include "preprocess-helpers.h"

#define DEG_TO_RAD (3.14159265f / 180.0f)

// Compute 3x3 covariance matrix of gaussian from scale and rotation
Mat33 compute_cov_3d(float scale[3], float rot[4]) {
    Mat33 R = quat_to_rotation(rot);
    Mat33 S = {{
        { scale[0], 0.0f, 0.0f },
        { 0.0f, scale[1], 0.0f },
        { 0.0f, 0.0f, scale[2] }
    }};
    Mat33 M = mult_33_33(&R, &S);
    Mat33 MT = transpose_33(&M);
    return mult_33_33(&M, &MT);
}

// Build view matrix (world-to-camera transform) from camera pose. Call once per frame.
// Match Python: R includes z-flip so camera at +z looks toward -z (scene at negative z).
Mat44 build_view_matrix(const float cam_angles[3], const float cam_pos[3]) {
    Mat33 R = euler_to_rotation(cam_angles);
    Mat33 Z_flip = { .m = {{1,0,0}, {0,1,0}, {0,0,-1}} };
    Mat33 R_view = mult_33_33(&R, &Z_flip);
    return homo_from_r_t(&R_view, cam_pos);
}

// Transform Gaussian's world position into camera frame. 
Vec3 transform_to_camera(const float gauss_pos[3], const Mat44 *view_matrix) {
    float p[4] = { gauss_pos[0], gauss_pos[1], gauss_pos[2], 1.0f };
    Vec4 tmp = mult_14_44(p, view_matrix);
    return (Vec3){ .v = { tmp.v[0], tmp.v[1], tmp.v[2] } };
}

// Combine camera transform + FOV/depth range into one homogeneous transform matrix. Used by project_to_screen.
Mat44 build_proj_matrix(const Mat44 *view_matrix, float fovx, float fovy,
                        float znear, float zfar) {
    float tan_fovx = tanf(fovx * 0.5f * DEG_TO_RAD);
    float tan_fovy = tanf(fovy * 0.5f * DEG_TO_RAD);

    // Rearrange view_matrix for row-vector multiply (translation to bottom row)
    Mat44 w2c = {{{0}}};
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            w2c.m[i][j] = view_matrix->m[i][j];
    for (int j = 0; j < 3; j++)
        w2c.m[3][j] = view_matrix->m[j][3];
    w2c.m[3][3] = 1.0f;

    // Perspective matrix (pre-transposed): scales xy by 1/tan(fov/2), maps z to [0,1]
    float range = zfar - znear;
    Mat44 PT = {{{0}}};
    PT.m[0][0] = 1.0f / tan_fovx;
    PT.m[1][1] = 1.0f / tan_fovy;
    PT.m[2][2] = zfar / range;
    PT.m[2][3] = 1.0f;
    PT.m[3][2] = -(zfar * znear) / range;

    return mult_44_44(&w2c, &PT);
}

// Transform Gaussian's world position to scren frame (pixel_x, pixel_y)
Vec2 project_to_screen(const float pos[3], const Mat44 *proj_matrix, int W, int H) {
    // Homogeneous point: (x, y, z, 1) 
    float p[4] = { pos[0], pos[1], pos[2], 1.0f };
    Vec4 p_hom = mult_14_44(p, proj_matrix);

    // Perspective divide: normalize by w to get NDC in [-1, 1] 
    float p_w = 1.0f / (p_hom.v[3] + 1e-7f);
    float ndc_x = p_hom.v[0] * p_w;
    float ndc_y = p_hom.v[1] * p_w;

    // Normalized coords -> pixel [0, size-1]: center 0 maps to (size-1)/2, edges ±1 map to 0 and size-1 
    Vec2 out;
    out.v[0] = ((ndc_x + 1.0f) * (float)W - 1.0f) * 0.5f;
    out.v[1] = ((ndc_y + 1.0f) * (float)H - 1.0f) * 0.5f;
    return out;
}

// Project 3D covariance to 2D (EWA splatting from Eq. 5 in paper). Returns 2x2 covariance matrix. fovx, fovy in degrees.
Mat22 compute_cov2d(const float pos[3], const Mat33 *cov3d, const Mat44 *view_matrix,
                    float fovx, float fovy, int W, int H) {
    // tangent of half fov is used in later calculations
    float tan_fovx = tanf(fovx * 0.5f * DEG_TO_RAD);
    float tan_fovy = tanf(fovy * 0.5f * DEG_TO_RAD);

    // Transform Gaussian center to camera space; t.z is depth
    Vec3 t = transform_to_camera(pos, view_matrix);

    // Clamp t.x/t.z and t.y/t.z to avoid numerical instability for points near viewing cone edges
    float limx = 1.3f * tanf(fovx * 0.5f * DEG_TO_RAD);
    float limy = 1.3f * tanf(fovy * 0.5f * DEG_TO_RAD);
    float txtz = t.v[0] / t.v[2];
    float tytz = t.v[1] / t.v[2];
    float clamp_x = (txtz < -limx) ? -limx : ((txtz > limx) ? limx : txtz);
    float clamp_y = (tytz < -limy) ? -limy : ((tytz > limy) ? limy : tytz);
    t.v[0] = clamp_x * t.v[2];
    t.v[1] = clamp_y * t.v[2];

    // Focal length in pixels from FOV and image size
    float focal_x = (float)W / (2.0f * tan_fovx);
    float focal_y = (float)H / (2.0f * tan_fovy);

    // Jacobian of perspective projection: d(screen) / d(world)
    float t2_sq = t.v[2] * t.v[2];
    Mat33 J = { .m = {
        { focal_x / t.v[2], 0.0f,                    -(focal_x * t.v[0]) / t2_sq },
        { 0.0f,             focal_y / t.v[2],        -(focal_y * t.v[1]) / t2_sq },
        { 0.0f,             0.0f,                    0.0f }
    }};

    // Propagate 3D covariance through projection: cov2d = J * W_rot * cov3d * (J * W_rot)^T
    Mat33 W_rot = mat44_extract_33(view_matrix);
    Mat33 T = mult_33_33(&J, &W_rot);
    Mat33 Tt = transpose_33(&T);
    Mat33 cov = mult_33_33(&T, cov3d);
    cov = mult_33_33(&cov, &Tt);

    // Extract upper-left 2x2 (screen-space covariance)
    Mat22 out;
    out.m[0][0] = cov.m[0][0];
    out.m[0][1] = cov.m[0][1];
    out.m[1][0] = cov.m[1][0];
    out.m[1][1] = cov.m[1][1];
    return out;
}

// Invert 2x2 covariance (with blur 0.3), compute conic and screen radius. radius==0 on failure (covariance is singular or not positive definite - aka 3D gaussian doesnt splat well, is a line or point).
ConicRadius prepare_conic_and_radius(const Mat22 *cov2d) {
    // initialize to zero in case it's invalid
    ConicRadius out = { .conic = {0, 0, 0}, .radius = 0 };

    // Blur diagonal to ensure size > 1 pixel
    Mat22 blurred = *cov2d;
    blurred.m[0][0] += 0.3f;
    blurred.m[1][1] += 0.3f;

    // if det < 0, return 
    float d = det_22(&blurred);
    if (d <= 0.0f)
        return out;

    // Conic = inverse of blurred covariance, packed as [inv_00, inv_01, inv_11]
    Mat22 inv = inv_22(&blurred, d);
    out.conic[0] = inv.m[0][0];
    out.conic[1] = inv.m[0][1];
    out.conic[2] = inv.m[1][1];

    // Bounding radius from largest eigenvalue of 2x2 covariance.
    // Note: for covariance, eigenvalues = variance along major axes
    // For 2x2 symmetric: eigenvalues = mid ± sqrt(mid² - det)
    float mid = 0.5f * (blurred.m[0][0] + blurred.m[1][1]);  // avg eigenvalue
    float disc = mid * mid - d;                                 // gap between eigenvalues; clamp for float safety
    float lambda_max = mid + sqrtf(disc > 0.0f ? disc : 0.0f); // larger eigenvalue (variance along major axis)
    float r = 3.0f * sqrtf(lambda_max > 0.0f ? lambda_max : 0.0f); // 3-sigma bound using larger eigenvalue
    out.radius = (int)ceilf(r);
    return out;
}








