#include <math.h>
#include "matrix-helpers.h"

// 3x3 matrix multiply: out = m1 * m2
Mat33 mult_33_33(const Mat33 *m1, const Mat33 *m2) {
    Mat33 out;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            out.m[i][j] = 0.0f;
            for (int k = 0; k < 3; k++) {
                out.m[i][j] += m1->m[i][k] * m2->m[k][j];
            }
        }
    }
    return out;
}

// 4x4 matrix multiply: out = m1 * m2
Mat44 mult_44_44(const Mat44 *m1, const Mat44 *m2) {
    Mat44 out;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            out.m[i][j] = 0.0f;
            for (int k = 0; k < 4; k++) {
                out.m[i][j] += m1->m[i][k] * m2->m[k][j];
            }
        }
    }
    return out;
}

// Convert quaternion [x,y,z,w] to 3x3 rotation matrix
Mat33 quat_to_rotation(const float q[4]) {
    float x = q[0]; float y = q[1]; float z = q[2]; float w = q[3];
    Mat33 out;
    out.m[0][0] = 1 - 2*(y*y + z*z);
    out.m[0][1] = 2*(x*y - w*z);
    out.m[0][2] = 2*(x*z + w*y);
    out.m[1][0] = 2*(x*y + w*z);
    out.m[1][1] = 1 - 2*(x*x + z*z);
    out.m[1][2] = 2*(y*z - w*x);
    out.m[2][0] = 2*(x*z - w*y);
    out.m[2][1] = 2*(y*z + w*x);
    out.m[2][2] = 1 - 2*(x*x + y*y);
    return out;
}

// Transpose of 3x3 matrix
Mat33 transpose_33(const Mat33 *m) {
    Mat33 out;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            out.m[i][j] = m->m[j][i];
        }
    }
    return out;
}

// Extract upper-left 3x3 block from 4x4 matrix
Mat33 mat44_extract_33(const Mat44 *m) {
    Mat33 out;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            out.m[i][j] = m->m[i][j];
        }
    }
    return out;
}

// Euler angles [rx,ry,rz] (radians) to 3x3 rotation matrix; order Rz*Ry*Rx
Mat33 euler_to_rotation(const float angles[3]) {
    float cx = cosf(angles[0]), sx = sinf(angles[0]);
    float cy = cosf(angles[1]), sy = sinf(angles[1]);
    float cz = cosf(angles[2]), sz = sinf(angles[2]);
    Mat33 Rx = { .m = {{1,0,0}, {0,cx,-sx}, {0,sx,cx}} };
    Mat33 Ry = { .m = {{cy,0,sy}, {0,1,0}, {-sy,0,cy}} };
    Mat33 Rz = { .m = {{cz,-sz,0}, {sz,cz,0}, {0,0,1}} };
    Mat33 RyRx = mult_33_33(&Ry, &Rx);
    return mult_33_33(&Rz, &RyRx);
}

// 3x3 matrix times 3x1 column vector
Vec3 mult_33_31(const Mat33 *m, const float v[3]) {
    Vec3 out;
    for (int i = 0; i < 3; i++) {
        out.v[i] = 0.0f;
        for (int j = 0; j < 3; j++) {
            out.v[i] += m->m[i][j] * v[j];
        }
    }
    return out;
}

// Determinant of 2x2 matrix
float det_22(const Mat22 *m) {
    return m->m[0][0] * m->m[1][1] - m->m[0][1] * m->m[1][0];
}

// Inverse of 2x2 matrix given its determinant (caller must ensure det != 0)
Mat22 inv_22(const Mat22 *m, float det) {
    float inv = 1.0f / det;
    Mat22 out;
    out.m[0][0] =  m->m[1][1] * inv;
    out.m[0][1] = -m->m[0][1] * inv;
    out.m[1][0] = -m->m[1][0] * inv;
    out.m[1][1] =  m->m[0][0] * inv;
    return out;
}

// Max eigenvalue of a symmetric 2x2 matrix
float max_eigenvalue_sym_22(const Mat22 *m) {
    float a = m->m[0][0], b = m->m[0][1], c = m->m[1][1];
    float mid = 0.5f * (a + c);
    float disc = mid * mid - (a * c - b * b);
    if (disc < 0.0f) disc = 0.0f;
    return mid + sqrtf(disc);
}

// 1x4 row vector times 4x4 matrix: out = v @ m
Vec4 mult_14_44(const float v[4], const Mat44 *m) {
    Vec4 out;
    for (int j = 0; j < 4; j++) {
        out.v[j] = 0.0f;
        for (int i = 0; i < 4; i++) {
            out.v[j] += v[i] * m->m[i][j];
        }
    }
    return out;
}

// Build 4x4 homogeneous matrix from rotation R and translation t; returns [R^T | -R^T*t; 0 0 0 1]
Mat44 homo_from_r_t(const Mat33 *R, const float t[3]) {
    Mat33 Rt = transpose_33(R);
    Mat44 out;

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++)
            out.m[i][j] = Rt.m[i][j];
    }

    Vec3 neg_t = mult_33_31(&Rt, t);
    out.m[0][3] = -neg_t.v[0];
    out.m[1][3] = -neg_t.v[1];
    out.m[2][3] = -neg_t.v[2];

    out.m[3][0] = out.m[3][1] = out.m[3][2] = 0.0f;
    out.m[3][3] = 1.0f;

    return out;
}
