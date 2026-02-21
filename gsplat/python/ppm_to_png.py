"""Convert a PPM (P6) image to PNG using only the standard library."""
import struct
import sys
import zlib


def read_ppm(path):
    with open(path, "rb") as f:
        magic = f.readline().strip()
        assert magic == b"P6", f"Expected P6, got {magic}"
        dims = f.readline().strip()
        while dims.startswith(b"#"):
            dims = f.readline().strip()
        w, h = map(int, dims.split())
        maxval = int(f.readline().strip())
        data = f.read()
    assert len(data) == w * h * 3
    return w, h, maxval, data


def write_png(path, w, h, rgb_data):
    """Write a minimal PNG file (8-bit RGB, no compression tricks)."""

    def chunk(chunk_type, data):
        c = chunk_type + data
        return struct.pack(">I", len(data)) + c + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)

    ihdr_data = struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)

    raw = bytearray()
    stride = w * 3
    for y in range(h):
        raw.append(0)  # filter byte: None
        raw.extend(rgb_data[y * stride:(y + 1) * stride])

    compressed = zlib.compress(bytes(raw))

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", ihdr_data))
        f.write(chunk(b"IDAT", compressed))
        f.write(chunk(b"IEND", b""))


def main():
    src = sys.argv[1] if len(sys.argv) > 1 else "../output.ppm"
    dst = src.rsplit(".", 1)[0] + ".png"
    if len(sys.argv) > 2:
        dst = sys.argv[2]

    w, h, _, data = read_ppm(src)
    write_png(dst, w, h, data)
    print(f"{src} ({w}x{h}) -> {dst}")


if __name__ == "__main__":
    main()
