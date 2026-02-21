"""
Step 1: Load PLY file → array of Gaussians.
Uses only Python stdlib.
"""
import struct
from dataclasses import dataclass
from typing import List


@dataclass
class Gaussian:
    pos: tuple  # (x, y, z)
    scale: tuple  # (sx, sy, sz)
    opacity: float
    rot: tuple  # quaternion (qx, qy, qz, qw)
    sh: tuple  # 16 bands × 3 channels, flat: (f_dc_0,1,2, f_rest_0..44)


def load_ply(path: str) -> List[Gaussian]:
    """
    Parse a binary PLY file (3DGS format) and return a list of Gaussian objects.
    Expects: x,y,z, scale_0,1,2, opacity, rot_x,y,z,w, red,green,blue, f_dc_0,1,2, f_rest_0..44
    """
    gaussians = []

    with open(path, "rb") as f:
        # Parse header
        line = f.readline().decode("ascii").strip()
        if line != "ply":
            raise ValueError("Not a PLY file")

        n_vertices = 0
        props = []  # (name, type_char, size)
        type_sizes = {"float": 4, "uchar": 1, "uint8": 1}

        while True:
            line = f.readline().decode("ascii").strip()
            if line.startswith("element vertex "):
                n_vertices = int(line.split()[-1])
            elif line.startswith("property "):
                parts = line.split()
                dtype = parts[1]
                name = parts[2]
                size = type_sizes.get(dtype, 4)
                props.append((name, dtype, size))
            elif line == "end_header":
                break

        # Build struct format (little-endian for binary_little_endian PLY)
        fmt = "<"
        prop_names = [p[0] for p in props]
        for _, dtype, size in props:
            if dtype == "float":
                fmt += "f"
            elif dtype in ("uchar", "uint8"):
                fmt += "B"
            else:
                raise ValueError(f"Unsupported property type: {dtype}")

        vertex_size = struct.calcsize(fmt)

        for _ in range(n_vertices):
            data = f.read(vertex_size)
            if len(data) < vertex_size:
                break
            values = struct.unpack(fmt, data)

            # Map by property name
            d = dict(zip(prop_names, values))

            pos = (d["x"], d["y"], d["z"])
            scale = (d["scale_0"], d["scale_1"], d["scale_2"])
            opacity = d["opacity"]
            rot = (d["rot_x"], d["rot_y"], d["rot_z"], d["rot_w"])

            sh_dc = (d["f_dc_0"], d["f_dc_1"], d["f_dc_2"])
            sh_rest = tuple(d[f"f_rest_{i}"] for i in range(45))
            sh = sh_dc + sh_rest

            gaussians.append(Gaussian(pos=pos, scale=scale, opacity=opacity, rot=rot, sh=sh))

    return gaussians


if __name__ == "__main__":
    gs = load_ply("../dummy.ply")
    print(f"Loaded {len(gs)} Gaussians")
    for i, g in enumerate(gs):
        print(f"  [{i}] pos={g.pos} scale={g.scale} opacity={g.opacity}")
