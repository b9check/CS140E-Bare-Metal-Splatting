#ifndef CONFIG_H
#define CONFIG_H

// Screen
#define WIDTH  256
#define HEIGHT 256

// Camera
#define FOVX 45.0f
#define FOVY 45.0f
#define CAM_POS_X 0.0f
#define CAM_POS_Y 0.0f
#define CAM_POS_Z 5.0f
#define CAM_ANGLE_X 0.0f
#define CAM_ANGLE_Y 0.0f
#define CAM_ANGLE_Z 0.0f
#define ZNEAR 0.01f
#define ZFAR  100.0f
#define MIN_DEPTH 0.2f

// Scene limits
#define MAX_GAUSSIANS 10000

#endif
