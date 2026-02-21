#include "rpi.h"
#include <math.h>
#include "main-helpers.h"

// Run steps 3-6 per Gaussian. cov3d precomputed once by caller.
// Returns number of valid splats (skips behind-camera and degenerate ones).
int preprocess(const Gaussian *gaussians, const Mat33 *cov3d, int n,
               const float cam_pos[3], const float cam_angles[3],
               float fovx, float fovy, int W, int H,
               Splat *out_splats) {
    Mat44 view = build_view_matrix(cam_angles, cam_pos);
    Mat44 proj = build_proj_matrix(&view, fovx, fovy, ZNEAR, ZFAR);

    int count = 0;
    for (int i = 0; i < n; i++) {
        const Gaussian *g = &gaussians[i];

        Vec3 p_cam = transform_to_camera(g->pos, &view);
        float depth = p_cam.v[2];
        if (depth < MIN_DEPTH)
            continue;

        Vec2 screen = project_to_screen(g->pos, &proj, W, H);
        Mat22 cov2d = compute_cov2d(g->pos, &cov3d[i], &view, fovx, fovy, W, H);

        // Invert covariance to get conic + screen radius; skip degenerate
        ConicRadius cr = prepare_conic_and_radius(&cov2d);
        if (cr.radius == 0)
            continue;

        // Pack into output splat
        Splat *s = &out_splats[count++];
        s->px = screen.v[0];
        s->py = screen.v[1];
        s->depth = depth;
        s->conic[0] = cr.conic[0];
        s->conic[1] = cr.conic[1];
        s->conic[2] = cr.conic[2];
        s->opacity = g->opacity;
        s->color[0] = g->color[0];
        s->color[1] = g->color[1];
        s->color[2] = g->color[2];
        s->radius = cr.radius;
    }
    return count;
}
