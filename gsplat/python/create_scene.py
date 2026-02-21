"""Generate a complex .gsplat scene with varied Gaussians."""
import struct
import math
import random

RECORD_FMT = "<3f3ff4f3f"
random.seed(42)


def axis_angle_to_quat(ax, ay, az, angle):
    """Axis-angle -> quaternion (qx, qy, qz, qw)."""
    s = math.sin(angle / 2)
    return (ax * s, ay * s, az * s, math.cos(angle / 2))


def write_gsplat(path, gaussians):
    with open(path, "wb") as f:
        f.write(struct.pack("<I", len(gaussians)))
        for g in gaussians:
            f.write(struct.pack(RECORD_FMT, *g))
    print(f"Wrote {len(gaussians)} Gaussians to {path}")


def make_scene():
    gs = []

    # --- Large soft background blobs (deep, big, semi-transparent) ---
    bg_configs = [
        (-6, -4, -25, 4.0, (0.15, 0.10, 0.30)),  # purple
        ( 6,  3, -30, 5.0, (0.05, 0.15, 0.25)),  # dark blue
        ( 0, -2, -20, 3.5, (0.20, 0.08, 0.10)),  # dark red
        (-3,  5, -28, 4.5, (0.10, 0.20, 0.15)),  # dark green
    ]
    for x, y, z, sc, col in bg_configs:
        gs.append((x, y, z, sc, sc, sc, 0.6, 0, 0, 0, 1, *col))

    # --- Ring of colored splats at mid-depth ---
    n_ring = 12
    ring_r = 5.0
    ring_z = -12.0
    for i in range(n_ring):
        angle = 2 * math.pi * i / n_ring
        x = ring_r * math.cos(angle)
        y = ring_r * math.sin(angle)
        hue = i / n_ring
        r = max(0, min(1, abs(hue * 6 - 3) - 1))
        g = max(0, min(1, 2 - abs(hue * 6 - 2)))
        b = max(0, min(1, 2 - abs(hue * 6 - 4)))
        qx, qy, qz, qw = axis_angle_to_quat(0, 0, 1, angle)
        gs.append((x, y, ring_z, 0.8, 0.3, 0.8, 0.9, qx, qy, qz, qw, r, g, b))

    # --- Bright foreground cluster (close, small, vivid) ---
    cluster_configs = [
        (-1.5, -1.0, -6, 0.4, (1.0, 0.2, 0.1)),   # red
        (-0.5,  0.5, -5, 0.3, (1.0, 0.8, 0.1)),   # yellow
        ( 0.5, -0.5, -5, 0.35, (0.1, 1.0, 0.3)),  # green
        ( 1.5,  0.2, -6, 0.4, (0.2, 0.4, 1.0)),   # blue
        ( 0.0,  1.0, -4, 0.25, (1.0, 1.0, 1.0)),  # white
        ( 0.0, -1.5, -7, 0.5, (1.0, 0.4, 0.7)),   # pink
    ]
    for x, y, z, sc, col in cluster_configs:
        gs.append((x, y, z, sc, sc, sc, 1.0, 0, 0, 0, 1, *col))

    # --- Elongated streaks (non-uniform scale + rotation) ---
    streak_configs = [
        (-4, 2, -14, (2.0, 0.2, 0.2), 0.7, (0.9, 0.6, 0.1), (0, 0, 1, 0.3)),
        ( 4, -2, -15, (0.2, 2.0, 0.2), 0.7, (0.1, 0.7, 0.9), (0, 0, 1, -0.5)),
        ( 0, 4, -13, (1.5, 0.15, 0.15), 0.8, (0.8, 0.2, 0.8), (0, 0, 1, 0.8)),
        ( 0, -4, -16, (0.2, 0.2, 1.5), 0.5, (0.3, 0.9, 0.3), (1, 0, 0, 0.4)),
    ]
    for x, y, z, sc, op, col, (ax, ay, az, ang) in streak_configs:
        qx, qy, qz, qw = axis_angle_to_quat(ax, ay, az, ang)
        gs.append((x, y, z, sc[0], sc[1], sc[2], op, qx, qy, qz, qw, *col))

    # --- Scattered small dots for texture ---
    for _ in range(20):
        x = random.uniform(-8, 8)
        y = random.uniform(-6, 6)
        z = random.uniform(-20, -8)
        sc = random.uniform(0.1, 0.4)
        op = random.uniform(0.4, 1.0)
        r = random.uniform(0.2, 1.0)
        g = random.uniform(0.2, 1.0)
        b = random.uniform(0.2, 1.0)
        gs.append((x, y, z, sc, sc, sc, op, 0, 0, 0, 1, r, g, b))

    return gs


if __name__ == "__main__":
    scene = make_scene()
    write_gsplat("../scene.gsplat", scene)
