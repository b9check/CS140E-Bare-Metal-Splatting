# my-splat

Minimal 3D Gaussian splatting renderer, targeting bare-metal Raspberry Pi.
Based on "3D Gaussian Splatting for Real-Time Radiance Field Rendering" (Kerbl et al., SIGGRAPH 2023).

## Directory Structure

```
my-splat/
  *.gsplat, *.ply       Input scene data (generated or converted)
  c/                     C implementation (bare-metal target)
  python/                Python reference implementation
  Steps.md              Pipeline walkthrough (equations + intuition)
```

## Pipeline Overview

1. **Load** — Read `.gsplat` binary: N Gaussians, each with position, scale, rotation, opacity, color.
2. **Preprocess** (per Gaussian, per frame) — For each Gaussian:
   - Transform to camera space, cull if behind camera
   - Project center to screen pixel coordinates
   - Build 3D covariance from scale + rotation, project to 2D
   - Invert 2D covariance to get conic + screen radius; discard degenerate splats
3. **Sort** — Order visible splats front-to-back by depth.
4. **Rasterize** — Per-pixel alpha compositing through sorted splats.
5. **Output** — Write image (PPM or framebuffer).

## .gsplat - Custom File Format I made

```
[4 bytes]      N (uint32, little-endian)
[N x 56 bytes] per Gaussian: pos(3f) scale(3f) opacity(1f) rot(4f) color(3f)
```

All values are little-endian float32. Color is pre-baked from SH DC term.
Quaternion ordering is (x, y, z, w). Total file size: `4 + N * 56` bytes.

## C Implementation (`c/`)

Designed for bare-metal — no libc dependencies in the math/preprocessing code.
Static buffers, no malloc in the hot path.

| File | Purpose |
|------|---------|
| `config.h` | System parameters (screen size, FOV, depth range, buffer limits) |
| `matrix-helpers.h/.c` | Generic matrix/vector types and operations |
| `preprocess-helpers.h/.c` | Splatting-specific types (Gaussian, Splat) and pipeline steps 2-6 |
| `main-helpers.h` | Umbrella header pulling everything together |
| `Preprocess.c` | Orchestration: loads `.gsplat`, runs preprocessing loop |
| `main.c` | Entry point: load once, preprocess per frame, sort + rasterize (TODO) |

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
# Generate a test scene
cd python
python create_scene.py

# Render it
python render.py ../complex.gsplat

# Convert from PLY
python ply_to_gsplat.py ../input.ply ../output.gsplat
```
