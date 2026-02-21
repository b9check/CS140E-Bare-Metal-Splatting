"""
Convert a 3DGS PLY file to the minimal .gsplat binary format.

.gsplat layout:
  [4 bytes]      N (uint32 LE)
  [N x 56 bytes] per Gaussian: pos(3f) scale(3f) opacity(1f) rot(4f) color(3f)

Color is baked from SH DC term: color = 0.282 * sh_dc + 0.5
"""
import struct
import sys
from load_ply import load_ply

SH_C0 = 0.28209479177387814


def ply_to_gsplat(ply_path: str, gsplat_path: str):
    gaussians = load_ply(ply_path)
    n = len(gaussians)

    with open(gsplat_path, "wb") as f:
        f.write(struct.pack("<I", n))

        for g in gaussians:
            # Bake color from SH DC term (first 3 values of sh)
            r = max(0.0, min(1.0, SH_C0 * g.sh[0] + 0.5))
            gr = max(0.0, min(1.0, SH_C0 * g.sh[1] + 0.5))
            b = max(0.0, min(1.0, SH_C0 * g.sh[2] + 0.5))

            f.write(struct.pack("<3f", *g.pos))
            f.write(struct.pack("<3f", *g.scale))
            f.write(struct.pack("<f", g.opacity))
            f.write(struct.pack("<4f", *g.rot))
            f.write(struct.pack("<3f", r, gr, b))

    print(f"Converted {n} Gaussians: {ply_path} -> {gsplat_path}")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} input.ply output.gsplat")
        sys.exit(1)
    ply_to_gsplat(sys.argv[1], sys.argv[2])
