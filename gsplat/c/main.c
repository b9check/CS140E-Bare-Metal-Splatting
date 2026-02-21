#include "rpi.h"
#include "config.h"
#include "main-helpers.h"
#include "scene-data.h"

void notmain(void) {
    // compile-time (done in makefile): scene embedded (xxd) in scene-data.h, config (FOV, camera params, etc.) in config.h
    

    /*
    =======================================================================================
    SETUP: RUNS ONCE AT START OF PROGRAM
    - load gaussians
    - precompute 3D covariance
    - initialize splat array
    - load camera params
    =======================================================================================
    */
    // load gaussians from embedded scene data
    uint32_t n = *(const uint32_t *)gsplat_scene_gsplat;
    if (n > MAX_GAUSSIANS) {
        printk("Too many gaussians (%u > %d)\n", (unsigned)n, MAX_GAUSSIANS);
        return;
    }
    const Gaussian *gaussians = (const Gaussian *)&gsplat_scene_gsplat[4];
    printk("Loaded %u gaussians\n", (unsigned)n);

    // precompute 3D covariance (depends only on Gaussian, not camera)
    Mat33 cov3d[n];
    for (int i = 0; i < (int)n; i++)
        cov3d[i] = compute_cov_3d((float *)gaussians[i].scale, (float *)gaussians[i].rot);
    printk("Precomputed %u 3D covariances\n", (unsigned)n);

    // initialize splat array, load camera params
    Splat splats[n];
    float cam_pos[3]    = { CAM_POS_X, CAM_POS_Y, CAM_POS_Z };
    float cam_angles[3] = { CAM_ANGLE_X, CAM_ANGLE_Y, CAM_ANGLE_Z };

    
    /* 
    =======================================================================================
    FRAME LOOP 
    WHEN RUNNING MULTIPLE FRAMES, LOOP THROUGH THESE STEPS
    - preprocess (transform, project, cov2d, conic)
    - sort (by depth)
    - rasterize
    - output
    =======================================================================================
    */
    int i = 0; // will be looping through frames in final code
    while (i<1) {
        // preprocess (transform, project, cov2d, conic)
        int num_splats = preprocess(gaussians, cov3d, (int)n, cam_pos, cam_angles,
                                    FOVX, FOVY, WIDTH, HEIGHT, splats);
        printk("Preprocessed: %d visible splats\n", num_splats);
        // runtime: TODO sort splats by depth, rasterize
        i += 1;
    }

    printk("done!\n");
}
