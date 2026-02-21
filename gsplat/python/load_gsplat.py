"""
Load a .gsplat file into an array of Gaussians.
Uses only Python stdlib.

.gsplat layout:
  [4 bytes]      N (uint32 LE)
  [N x 56 bytes] per Gaussian: pos(3f) scale(3f) opacity(1f) rot(4f) color(3f)
"""
import struct
from dataclasses import dataclass
from typing import List

RECORD_FMT = "<3f3ff4f3f"  # 14 floats = 56 bytes
RECORD_SIZE = struct.calcsize(RECORD_FMT)


@dataclass
class Gaussian:
    pos: tuple    # (x, y, z)
    scale: tuple  # (sx, sy, sz)
    opacity: float
    rot: tuple    # (qx, qy, qz, qw)
    color: tuple  # (r, g, b) pre-baked, [0,1]


def load_gsplat(path: str) -> List[Gaussian]:
    gaussians = []
    with open(path, "rb") as f:
        n = struct.unpack("<I", f.read(4))[0]
        for _ in range(n):
            vals = struct.unpack(RECORD_FMT, f.read(RECORD_SIZE))
            gaussians.append(Gaussian(
                pos=vals[0:3],
                scale=vals[3:6],
                opacity=vals[6],
                rot=vals[7:11],
                color=vals[11:14],
            ))
    return gaussians


if __name__ == "__main__":
    gs = load_gsplat("../dummy.gsplat")
    print(f"Loaded {len(gs)} Gaussians from .gsplat")
    for i, g in enumerate(gs):
        print(f"  [{i}] pos={g.pos} scale={g.scale} opacity={g.opacity:.2f} "
              f"rot={g.rot} color=({g.color[0]:.3f}, {g.color[1]:.3f}, {g.color[2]:.3f})")
