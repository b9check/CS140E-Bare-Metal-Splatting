#include <stdio.h>
#include <math.h>
#include "main-helpers.h"

// Load .gsplat into a static buffer. Returns count, or -1 if too many.
// Struct layout matches binary format so fread works directly.
int load_gaussians(const char *path, Gaussian *out, int max) {
    FILE *f = fopen(path, "rb");
    unsigned int n;
    // Get number of Gaussians
    fread(&n, sizeof(unsigned int), 1, f);
    if (n > max) {
        fclose(f);
        return -1;
    }
    // Read Gaussians to static buffer (out)
    fread(out, sizeof(Gaussian), n, f);
    fclose(f);
    return (int)n;
}


// Run steps 2-6 on each Gaussian. Writes visible Splats into static buffer out_splats.
// Returns number of valid splats (skips behind-camera and degenerate ones).
// Caller provides buffer sized to n (worst case: all Gaussians visible).
int preprocess(const Gaussian *gaussians, int n,
               const float cam_pos[3], const float cam_angles[3],
               float fovx, float fovy, int W, int H,
               Splat *out_splats) {
    // Build per-frame matrices from camera params
    Mat44 view = build_view_matrix(cam_angles, cam_pos);
    Mat44 proj = build_proj_matrix(&view, fovx, fovy, ZNEAR, ZFAR);
    
    // Per Gaussian per frame preprocessing
    int count = 0;
    for (int i = 0; i < n; i++) {
        const Gaussian *g = &gaussians[i];

        // Cull if behind camera
        Vec3 p_cam = transform_to_camera(g->pos, &view);
        float depth = p_cam.v[2];
        if (depth < MIN_DEPTH)
            continue;

        // Project center to pixel coords
        Vec2 screen = project_to_screen(g->pos, &proj, W, H);

        // Build 3D covariance from scale + rotation, then project to 2D
        Mat33 cov3d = compute_cov_3d((float *)g->scale, (float *)g->rot);
        Mat22 cov2d = compute_cov2d(g->pos, &cov3d, &view, fovx, fovy, W, H);

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
