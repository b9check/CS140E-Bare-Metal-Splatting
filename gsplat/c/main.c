#include <stdio.h>
#include "main-helpers.h"

// Static buffers — no malloc, sized for worst case
static Gaussian gaussians[MAX_GAUSSIANS];
static Splat splats[MAX_GAUSSIANS];

int main(int argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : "scene.gsplat";

    // Load gaussians from .gsplat file (once)
    int n = load_gaussians(path, gaussians, MAX_GAUSSIANS);
    if (n < 0) {
        printf("Too many gaussians or failed to load %s\n", path);
        return 1;
    }
    printf("Loaded %d gaussians from %s\n", n, path);

    // Camera params
    float cam_pos[3]    = { 0.0f, 0.0f, 5.0f };
    float cam_angles[3] = { 0.0f, 0.0f, 0.0f };

    // Preprocess (per frame — re-run if camera moves)
    int num_splats = preprocess(gaussians, n, cam_pos, cam_angles,
                                FOVX, FOVY, WIDTH, HEIGHT, splats);
    printf("Preprocessed: %d visible splats\n", num_splats);

    // TODO: sort splats by depth
    // TODO: rasterize

    return 0;
}
