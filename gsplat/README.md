# gsplat

Minimal 3D Gaussian splatting renderer, targeting bare-metal Raspberry Pi.
Based on "3D Gaussian Splatting for Real-Time Radiance Field Rendering" (Kerbl et al., SIGGRAPH 2023).

## Directory Structure

```
gsplat/
  *.gsplat, *.ply       Input scene data (generated or converted)
  Steps.md              Pipeline walkthrough (equations + intuition)
  c/                     C implementation (bare-metal target)
  python/                Python reference implementation
```

## Input Format (`.gsplat`)

Our renderer uses a custom binary format (`.gsplat`) — a stripped-down version of the standard PLY that's trivial to parse on bare metal.

```
[4 bytes]      N (uint32, little-endian)
[N x 56 bytes] per Gaussian: pos(3f) scale(3f) opacity(1f) rot(4f) color(3f)
```

All values are little-endian float32. Color is pre-baked from SH DC term (`0.282 * sh_dc + 0.5`).
Quaternion ordering is (x, y, z, w). Total file size: `4 + N * 56` bytes.

To get a `.gsplat` file, either:
- **Create a synthetic scene** with `create_scene.py`, or
- **Convert from PLY** (the format available from trained 3DGS models online) with `ply_to_gsplat.py`. This bakes the spherical harmonics down to a single RGB color and drops all other SH coefficients.

## Pipeline Overview

See [Steps.md](Steps.md) for a detailed walkthrough of the rendering pipeline with equations.

1. **Load** — On bare metal: `.gsplat` is embedded as a C array at build time (`scene-data.h`). Main casts directly to `Gaussian*`; no file I/O.
2. **Build 3D covariance** (once, per Gaussian) — From scale + rotation, before the frame loop.
3. **Preprocess** (per frame) — For each Gaussian:
   - Transform to camera space, cull if behind camera
   - Project center to screen pixel coordinates
   - Project cov to 2D, invert to conic + screen radius; discard degenerate splats
4. **Sort** — Order visible splats front-to-back by depth.
5. **Rasterize** — Per-pixel alpha compositing through sorted splats.
6. **Output** — Write image as PPM, then convert to PNG for viewing.

## Data Types

### Gaussian (input)
Raw scene data. 56 bytes, matches `.gsplat` layout exactly.
- `pos[3]` — world position (x, y, z)
- `scale[3]` — ellipsoid radii (sx, sy, sz)
- `opacity` — alpha
- `rot[4]` — quaternion (qx, qy, qz, qw)
- `color[3]` — pre-baked RGB [0,1]

### Splat (output of preprocessing, per-frame)
Ready for rasterization.
- `px, py` — screen pixel coordinates
- `depth` — camera-space z (for sorting)
- `conic[3]` — inverse 2D covariance packed as (a, b, c)
- `opacity` — from Gaussian
- `color[3]` — from Gaussian
- `radius` — bounding radius in pixels (3-sigma); 0 = invalid/culled

## C Implementation (`c/`)

Designed for bare-metal — no libc dependencies in the math/preprocessing code.
Static buffers, no malloc in the hot path. NOTE: I have not yet implemented sorting or rasterizing, just the pre-processing steps. For the final version, we probably want at least 2 kernels (1 for sorting, 1 for rasterizing). We could make more if we have time so the whole thing runs on the GPU. I think priority at the start should be getting rasterizing to work on the GPU.

| File | Purpose |
|------|---------|
| `config.h` | System parameters (screen size, FOV, depth range, buffer limits) |
| `matrix-helpers.h/.c` | Generic matrix/vector types and operations |
| `preprocess-helpers.h/.c` | Splatting-specific types (Gaussian, Splat) and pipeline steps 2-6 |
| `main-helpers.h` | Umbrella header pulling everything together |
| `Preprocess.c` | Preprocess (steps 2-6): camera transform, project, cov2d, conic |
| `main.c` | Entry point: reads count + casts embedded `scene-data.h` to Gaussians, then preprocess + sort + rasterize |

## Python Reference (`python/`)

NumPy-based reference renderer. Useful for validation and generating test scenes.

| File | Purpose |
|------|---------|
| `render.py` | Full pipeline: load, preprocess, sort, rasterize, output PPM |
| `create_scene.py` | Generate synthetic `.gsplat` test scenes |
| `load_gsplat.py` | Parse `.gsplat` binary format |
| `load_ply.py` | Parse 3DGS PLY files |
| `ply_to_gsplat.py` | Convert PLY to `.gsplat` (bakes SH to RGB) |
| `ppm_to_png.py` | Convert PPM output to PNG |

### Quick Start

```bash
cd python

# Option A: generate a synthetic scene (writes ../scene.gsplat)
python create_scene.py

# Option B: convert an existing PLY from a trained 3DGS model
python ply_to_gsplat.py ../model.ply ../scene.gsplat

# Render (default input: ../scene.gsplat)
python render.py

# Convert output to PNG for viewing
python ppm_to_png.py ../scene.ppm
```
