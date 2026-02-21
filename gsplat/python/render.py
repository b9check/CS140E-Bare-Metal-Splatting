"""
Gaussian Splatting Renderer — Steps 1-9 from Steps.md.
Uses numpy for matrix math, load_gsplat.py for input. No other dependencies.
"""
import math
import sys
import numpy as np
from dataclasses import dataclass
from typing import List
from load_gsplat import load_gsplat, Gaussian


# ---------------------------------------------------------------------------
# Data types
# ---------------------------------------------------------------------------

@dataclass
class Camera:
    view_matrix: np.ndarray      # 4x4, world-to-camera (row-vector convention)
    proj_matrix: np.ndarray      # 4x4, full projection (view * perspective)
    tan_fovx: float
    tan_fovy: float
    width: int
    height: int


@dataclass
class Splat:
    """Preprocessed Gaussian ready for rasterization."""
    px: float
    py: float
    depth: float
    conic: np.ndarray   # (3,) — inverse 2x2 cov packed as (a, b, c)
    opacity: float
    color: np.ndarray   # (3,) — RGB
    radius: float


# ---------------------------------------------------------------------------
# Step 2: Build 3D covariance — Paper Eq. 6
# ---------------------------------------------------------------------------

def quat_to_rotation(q):
    """Quaternion (qx, qy, qz, qw) -> 3x3 rotation matrix."""
    x, y, z, w = q
    return np.array([
        [1 - 2*(y*y + z*z),   2*(x*y - w*z),       2*(x*z + w*y)],
        [2*(x*y + w*z),       1 - 2*(x*x + z*z),   2*(y*z - w*x)],
        [2*(x*z - w*y),       2*(y*z + w*x),       1 - 2*(x*x + y*y)],
    ], dtype=np.float32)


def compute_cov3d(scale, rot):
    """Sigma = R @ S @ S^T @ R^T  (returns 3x3 matrix)."""
    S = np.diag(np.array(scale, dtype=np.float32))
    R = quat_to_rotation(rot)
    M = R @ S
    return M @ M.T


# ---------------------------------------------------------------------------
# Step 3: Transform to camera space
# ---------------------------------------------------------------------------

def transform_to_camera(pos, view_matrix):
    """Returns camera-space position (4,). Depth is result[2]."""
    p = np.array([pos[0], pos[1], pos[2], 1.0], dtype=np.float32)
    return p @ view_matrix


# ---------------------------------------------------------------------------
# Step 4: Project to screen
# ---------------------------------------------------------------------------

def ndc_to_pixel(ndc, size):
    return ((ndc + 1.0) * size - 1.0) * 0.5


def project_to_screen(pos, proj_matrix, W, H):
    """World position -> (pixel_x, pixel_y) via full projection matrix."""
    p = np.array([pos[0], pos[1], pos[2], 1.0], dtype=np.float32)
    p_hom = p @ proj_matrix
    p_w = 1.0 / (p_hom[3] + 1e-7)
    ndc_x = p_hom[0] * p_w
    ndc_y = p_hom[1] * p_w
    return ndc_to_pixel(ndc_x, W), ndc_to_pixel(ndc_y, H)


# ---------------------------------------------------------------------------
# Step 5: Project covariance to 2D (EWA splatting) — Paper Eq. 5
# ---------------------------------------------------------------------------

def compute_cov2d(pos, cov3d, view_matrix, tan_fovx, tan_fovy, W, H):
    """Returns 2D covariance as (sigma_xx, sigma_xy, sigma_yy)."""
    t = transform_to_camera(pos, view_matrix)

    limx = 1.3 * tan_fovx
    limy = 1.3 * tan_fovy
    txtz = t[0] / t[2]
    tytz = t[1] / t[2]
    t[0] = min(limx, max(-limx, txtz)) * t[2]
    t[1] = min(limy, max(-limy, tytz)) * t[2]

    focal_x = W / (2.0 * tan_fovx)
    focal_y = H / (2.0 * tan_fovy)

    J = np.array([
        [focal_x / t[2], 0.0,            -(focal_x * t[0]) / (t[2] * t[2])],
        [0.0,            focal_y / t[2],  -(focal_y * t[1]) / (t[2] * t[2])],
        [0.0,            0.0,             0.0],
    ], dtype=np.float32)

    W_rot = view_matrix[:3, :3].copy()
    T = J @ W_rot

    cov = T @ cov3d @ T.T
    return np.array([cov[0, 0], cov[0, 1], cov[1, 1]], dtype=np.float32)


# ---------------------------------------------------------------------------
# Step 6: Prepare conic and radius
# ---------------------------------------------------------------------------

def prepare_conic_and_radius(cov2d):
    """Invert 2x2 covariance (with blur), compute screen radius."""
    a = cov2d[0] + 0.3
    b = cov2d[1]
    c = cov2d[2] + 0.3

    det = a * c - b * b
    if det <= 0:
        return None, 0

    det_inv = 1.0 / det
    conic = np.array([c * det_inv, -b * det_inv, a * det_inv], dtype=np.float32)

    mid = 0.5 * (a + c)
    lambda_max = mid + math.sqrt(max(0.1, mid * mid - det))
    radius = math.ceil(3.0 * math.sqrt(max(lambda_max, 0.0)))

    return conic, radius


# ---------------------------------------------------------------------------
# Steps 2-6: Preprocess all Gaussians
# ---------------------------------------------------------------------------

def preprocess(gaussians: List[Gaussian], cam: Camera) -> List[Splat]:
    """Run Steps 2-6 on each Gaussian. Returns list of visible Splats."""
    splats = []
    for g in gaussians:
        # Step 3: transform to camera space, cull if behind camera
        p_cam = transform_to_camera(g.pos, cam.view_matrix)
        depth = p_cam[2]
        if depth < 0.2:
            continue

        # Step 4: project to screen
        px, py = project_to_screen(g.pos, cam.proj_matrix, cam.width, cam.height)

        # Step 2: build 3D covariance
        cov3d = compute_cov3d(g.scale, g.rot)

        # Step 5: project covariance to 2D
        cov2d = compute_cov2d(g.pos, cov3d, cam.view_matrix,
                              cam.tan_fovx, cam.tan_fovy, cam.width, cam.height)

        # Step 6: invert covariance, compute radius
        conic, radius = prepare_conic_and_radius(cov2d)
        if conic is None or radius <= 0:
            continue

        splats.append(Splat(
            px=px, py=py, depth=depth,
            conic=conic, opacity=g.opacity,
            color=np.array(g.color, dtype=np.float32),
            radius=radius,
        ))

    return splats


# ---------------------------------------------------------------------------
# Step 7: Sort by depth
# ---------------------------------------------------------------------------

def sort_by_depth(splats: List[Splat]) -> List[Splat]:
    """Sort splats front-to-back (smallest depth first)."""
    return sorted(splats, key=lambda s: s.depth)


# ---------------------------------------------------------------------------
# Step 8: Rasterize — Paper Eq. 4 + volume rendering
# ---------------------------------------------------------------------------

def rasterize(splats: List[Splat], W: int, H: int, background: np.ndarray) -> np.ndarray:
    """Per-pixel alpha compositing. Returns (H, W, 3) float image."""
    image = np.zeros((H, W, 3), dtype=np.float32)

    for y in range(H):
        for x in range(W):
            T = 1.0
            r, g, b = 0.0, 0.0, 0.0

            for s in splats:
                dx = s.px - x
                dy = s.py - y

                if dx * dx + dy * dy > s.radius * s.radius:
                    continue

                power = -0.5 * (s.conic[0] * dx * dx + s.conic[2] * dy * dy) - s.conic[1] * dx * dy
                if power > 0.0:
                    continue

                alpha = min(0.99, s.opacity * math.exp(power))
                if alpha < 1.0 / 255.0:
                    continue

                weight = alpha * T
                r += weight * s.color[0]
                g += weight * s.color[1]
                b += weight * s.color[2]

                T *= (1.0 - alpha)
                if T < 0.0001:
                    break

            image[y, x, 0] = r + T * background[0]
            image[y, x, 1] = g + T * background[1]
            image[y, x, 2] = b + T * background[2]

    return image


# ---------------------------------------------------------------------------
# Step 9: Write PPM
# ---------------------------------------------------------------------------

def write_ppm(path: str, image: np.ndarray):
    """Write (H, W, 3) float image as binary PPM."""
    H, W, _ = image.shape
    with open(path, "wb") as f:
        f.write(f"P6\n{W} {H}\n255\n".encode())
        clamped = np.clip(image * 255, 0, 255).astype(np.uint8)
        f.write(clamped.tobytes())


# ---------------------------------------------------------------------------
# Camera setup (matches 3dgs-warp-scratch/render.py scene)
# ---------------------------------------------------------------------------

def setup_camera(width=256, height=256, fovx=45.0, fovy=45.0, znear=0.01, zfar=100.0):
    R = np.array([[1, 0, 0], [0, 1, 0], [0, 0, -1]], dtype=np.float32)
    t = np.array([0, 0, 5], dtype=np.float32)

    # View matrix (row-vector convention, matching Warp)
    Rt = np.zeros((4, 4), dtype=np.float32)
    Rt[:3, :3] = R.T
    Rt[:3, 3] = t
    Rt[3, 3] = 1.0
    view_matrix = Rt.copy()

    # World-to-camera (transposed form for row-vector projection)
    w2c = np.eye(4, dtype=np.float32)
    w2c[:3, :3] = R
    w2c[:3, 3] = t
    w2c = w2c.T

    # Perspective projection matrix
    tan_fovx = math.tan(fovx * 0.5)
    tan_fovy = math.tan(fovy * 0.5)
    top = tan_fovy * znear
    right = tan_fovx * znear
    P = np.zeros((4, 4), dtype=np.float32)
    P[0, 0] = 2.0 * znear / (2.0 * right)
    P[1, 1] = 2.0 * znear / (2.0 * top)
    P[3, 2] = 1.0
    P[2, 2] = zfar / (zfar - znear)
    P[2, 3] = -(zfar * znear) / (zfar - znear)

    proj_matrix = w2c @ P.T

    return Camera(
        view_matrix=view_matrix,
        proj_matrix=proj_matrix,
        tan_fovx=tan_fovx,
        tan_fovy=tan_fovy,
        width=width,
        height=height,
    )


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    input_path = sys.argv[1] if len(sys.argv) > 1 else "../scene.gsplat"
    output_base = input_path.rsplit(".", 1)[0]

    # Step 1: Load
    gaussians = load_gsplat(input_path)
    print(f"Loaded {len(gaussians)} Gaussians from {input_path}")

    cam = setup_camera(width=256, height=256)
    background = np.array([0.0, 0.0, 0.0], dtype=np.float32)

    # Steps 2-6: Preprocess
    splats = preprocess(gaussians, cam)
    print(f"Preprocessed: {len(splats)} visible splats")

    # Step 7: Sort
    splats = sort_by_depth(splats)

    # Step 8: Rasterize
    print(f"Rasterizing {cam.width}x{cam.height} with {len(splats)} splats...")
    image = rasterize(splats, cam.width, cam.height, background)

    # Step 9: Output
    ppm_path = output_base + ".ppm"
    write_ppm(ppm_path, image)
    print(f"Saved {ppm_path}")


if __name__ == "__main__":
    main()
