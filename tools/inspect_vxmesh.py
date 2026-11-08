#!/usr/bin/env python3
"""Inspect VXMSH001 mesh scale, degenerate and giant triangles without dependencies."""
import argparse
import collections
import math
import pathlib
import struct

def inspect(path):
    blob = pathlib.Path(path).read_bytes()
    if len(blob) < 16 or blob[:8] != b"VXMSH001":
        raise ValueError("not a VXMSH001 file")
    version, count = struct.unpack_from("<II", blob, 8)
    if version != 1 or count > 100000:
        raise ValueError("unsupported version or mesh count")
    pos = 16
    def read(fmt):
        nonlocal pos
        size = struct.calcsize(fmt)
        if pos + size > len(blob):
            raise ValueError("truncated file")
        result = struct.unpack_from(fmt, blob, pos)
        pos += size
        return result
    records = []
    for i in range(count):
        guid = read("36s")[0].split(b"\0", 1)[0].decode("ascii", "replace")
        n, ni = read("<II")
        if n > 10_000_000 or ni > 30_000_000:
            raise ValueError("excessive geometry")
        bounds = read("<6f")
        positions = read("<" + str(3 * n) + "f")
        pos += 4 * (3*n + 4*n + 2*n)  # normals, colors, uv0
        if pos > len(blob):
            raise ValueError("truncated attributes")
        indices = read("<" + str(ni) + "I")
        if any(j >= n for j in indices):
            raise ValueError("invalid index")
        edges = []
        degenerate = 0
        longest = 0.0
        for j in range(0, ni - 2, 3):
            ids = indices[j:j+3]
            vertices = [positions[k*3:k*3+3] for k in ids]
            lens = [math.dist(vertices[a], vertices[b]) for a,b in ((0,1),(1,2),(2,0))]
            m = max(lens)
            longest = max(longest, m)
            if m < 1e-7 or min(lens) < 1e-7:
                degenerate += 1
        diagonal = math.dist(bounds[:3], bounds[3:])
        records.append(dict(stroke=i, guid=guid, vertices=n, triangles=ni//3,
                            diagonal=diagonal, max_triangle_edge=longest,
                            degenerate=degenerate, bounds=tuple(round(x,3) for x in bounds)))
    if pos != len(blob):
        raise ValueError("trailing bytes")
    print("meshes:", len(records), "vertices:", sum(x["vertices"] for x in records),
          "triangles:", sum(x["triangles"] for x in records))
    print("brush GUIDs:", dict(collections.Counter(x["guid"] for x in records)))
    print("largest meshes by world-space diagonal (meters):")
    for x in sorted(records, key=lambda x:x["diagonal"], reverse=True)[:12]:
        print(x)
    print("largest individual triangle edges (meters):")
    for x in sorted(records, key=lambda x:x["max_triangle_edge"], reverse=True)[:12]:
        print(x)
    print("degenerate triangles:", sum(x["degenerate"] for x in records))
if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("scene")
    args = parser.parse_args()
    inspect(args.scene)
