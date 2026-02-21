# Gaussian Splatting Rendering Pipeline

Based on "3D Gaussian Splatting for Real-Time Radiance Field Rendering" (Kerbl et al., SIGGRAPH 2023).
See [README.md](README.md) for the `.gsplat` input format.

---

## When Each Step Runs (C)

| Step | When |
|------|------|
| Scene, config | Compile (xxd → scene-data.h, config.h) |
| 1. Load | Once (parse count + cast from embedded data) |
| 2. Cov3d | Once (before frame loop; depends only on Gaussian) |
| 3–6. Preprocess | Per frame (transform, project, cov2d, conic) |
| 7. Sort | Per frame |
| 8. Rasterize | Per frame |
| 9. Output | Per frame |

---

**1. Load** — Read `.gsplat` binary: count N, then N x 56 bytes of packed Gaussian data. (Bare-metal: embedded at compile; parse at runtime.)

**2. Build 3D covariance** (per Gaussian) — *Paper Eq. 6* — *Runs once before frame loop.*
Each Gaussian is an oriented ellipsoid. We store scale (3 radii) and rotation (quaternion) separately, and combine them into a 3x3 covariance matrix:
`Sigma = R*S*S^T*R^T`.
This decomposition guarantees the matrix is always valid (positive semi-definite).

**3. Transform to camera space** (per Gaussian, per frame) — *Standard 3D graphics*
Multiply position by the view matrix to get the Gaussian's position relative to the camera.
If it's behind the camera (depth < 0.2), skip it.

**4. Project to screen** (per Gaussian, per frame) — *Standard 3D graphics*
Apply perspective projection to get a 2D pixel coordinate. Things farther away appear smaller.

**5. Project covariance to 2D** (per Gaussian, per frame) — *Paper Eq. 5, from Zwicker et al. 2001*
The 3D ellipsoid, seen through the camera, becomes a 2D ellipse on screen.
`Sigma_2D = J * W * Sigma_3D * W^T * J^T`
where J is the Jacobian of perspective projection (captures how depth scaling distorts the shape) and W is the view rotation. Ignore third row and column.

**6. Prepare for rasterization** (per Gaussian, per frame) — *Linear algebra*
Invert the 2x2 covariance to get the "conic" form (used for fast per-pixel evaluation).
Compute the screen radius from eigenvalues (3-sigma rule: covers 99.7% of the Gaussian).

**7. Sort by depth** (per frame) — *Painter's algorithm*
Sort all visible Gaussians front-to-back by depth so alpha compositing layers them correctly.

**8. Rasterize** (per frame, per pixel) — *Paper Eq. 4 + volume rendering (Eq. 3)*
For each pixel, walk through sorted Gaussians and blend their contributions. A simple implementation iterates over all splats per pixel and skips those outside the radius (`dx² + dy² > radius²`). For speed, the paper uses **tiling**: divide the screen into tiles (e.g. 16×16), assign each splat to the tiles it overlaps, then per pixel only iterate splats in that tile's list.
Start with T = 1.0 (fully transparent) and r, g, b = 0.
For each Gaussian:
- Compute d = (pixel - gaussian_center) in screen space
- Evaluate the Gaussian falloff: `alpha = opacity * exp(-0.5 * d^T * conic * d)` (Eq. 4).
  This is one exp() per pixel-Gaussian pair. Alpha depends only on distance and shape, not color.
- Compute `weight = alpha * T` (once), then apply to all three channels:
  `r += weight * gaussian_r`, `g += weight * gaussian_g`, `b += weight * gaussian_b`
- Update transmittance: `T *= (1 - alpha)`. Each layer absorbs some light.
  The weights across all Gaussians plus the background always sum to exactly 1.0, so no
  normalization is needed.
- Stop early when T < 0.0001 (pixel is fully opaque, remaining Gaussians can't contribute)
- After all Gaussians: `r += T * bg_r`, `g += T * bg_g`, `b += T * bg_b`.
  T is whatever light wasn't absorbed — the background gets that remainder.

**9. Output** (per frame) — Write image as PPM, convert to PNG for viewing.
