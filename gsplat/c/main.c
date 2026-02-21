#include "rpi.h"
#include "config.h"
#include "main-helpers.h"
#include "scene-data.h"

void notmain(void) {
    // read first 4 bytes to get number of gaussians, check if too many
    uint32_t n = *(const uint32_t *)gsplat_scene_gsplat;
    if (n > MAX_GAUSSIANS) {
        printk("Too many gaussians (%u > %d)\n", (unsigned)n, MAX_GAUSSIANS);
        return;
    }
    // read the rest of the file, cast them as Gaussian pointers
    const Gaussian *gaussians = (const Gaussian *)&gsplat_scene_gsplat[4];

    // create a splat array of size n to store splats after preprocessing
    Splat splats[n];
    printk("Loaded %u gaussians\n", (unsigned)n);

    // load camera params
    float cam_pos[3]    = { CAM_POS_X, CAM_POS_Y, CAM_POS_Z };
    float cam_angles[3] = { CAM_ANGLE_X, CAM_ANGLE_Y, CAM_ANGLE_Z };

    // preprocess the gaussians: modifies the splats array in place
    int num_splats = preprocess(gaussians, (int)n, cam_pos, cam_angles,
                                FOVX, FOVY, WIDTH, HEIGHT, splats);
    printk("Preprocessed: %d visible splats\n", num_splats);

    // TODO: sort splats by depth
    // TODO: rasterize

    printk("done!\n");
}
