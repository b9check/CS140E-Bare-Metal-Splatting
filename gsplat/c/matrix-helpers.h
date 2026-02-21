#ifndef MATRIX_HELPERS_H
#define MATRIX_HELPERS_H

// Types
typedef struct { float m[2][2]; } Mat22;
typedef struct { float m[3][3]; } Mat33;
typedef struct { float m[4][4]; } Mat44;
typedef struct { float v[2]; } Vec2;
typedef struct { float v[3]; } Vec3;
typedef struct { float v[4]; } Vec4;

// Multiply
Mat33 mult_33_33(const Mat33 *m1, const Mat33 *m2);
Mat44 mult_44_44(const Mat44 *m1, const Mat44 *m2);
Vec3  mult_33_31(const Mat33 *m, const float v[3]);
Vec4  mult_14_44(const float v[4], const Mat44 *m);

// Transpose / Extract
Mat33 transpose_33(const Mat33 *m);
Mat33 mat44_extract_33(const Mat44 *m);

// Determinant / Inverse
float det_22(const Mat22 *m);
Mat22 inv_22(const Mat22 *m, float det);

// Eigenvalues
float max_eigenvalue_sym_22(const Mat22 *m);

// Conversions
Mat33 quat_to_rotation(const float q[4]);
Mat33 euler_to_rotation(const float angles[3]);
Mat44 homo_from_r_t(const Mat33 *R, const float t[3]);

#endif
