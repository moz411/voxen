#!/usr/bin/env python3
"""Compile Open Brush Dots control points to a compact, little-endian GPU payload.

Format v1: magic 8 bytes 'VXDOT001', uint32 count, uint32 stride (32),
then count records: 3f position, 4f RGBA, f diameter.
Source points are recentered once, around the global AABB center, then scaled.
The result is *input* for a Vulkan instance buffer; it does not render itself.
"""
import argparse
import json
import math
import struct
from pathlib import Path

DOTS_GUID = "6a1cf9f9-032c-45ec-9b1d-a6680bee30f7"
MAGIC = b"VXDOT001"
RECORD = struct.Struct("<3f4ff")


def compile_dots(data, world_scale):
    strokes = data["strokes"]
    positions = [p["position"] for s in strokes for p in s["points"]]
    if not positions:
        raise ValueError("Empty sketch")
    if not math.isfinite(world_scale) or world_scale <= 0:
        raise ValueError("world_scale must be finite and positive")
    lo = [min(p[i] for p in positions) for i in range(3)]
    hi = [max(p[i] for p in positions) for i in range(3)]
    center = [(a+b)*0.5 for a,b in zip(lo,hi)]
    records = []
    for stroke in strokes:
        if stroke["brush_guid"].lower() != DOTS_GUID:
            continue
        size = float(stroke["size"])
        scale = float(stroke.get("extras", {}).get("scale", 1.0))
        color = [float(c) for c in stroke["color"]]
        if not all(math.isfinite(v) for v in (size,scale,*color)):
            raise ValueError("Non-finite stroke size, scale or color")
        for point in stroke["points"]:
            pos = [float(x) for x in point["position"]]
            pressure = float(point["pressure"])
            diameter = size * scale * world_scale * max(0., min(1., pressure))
            if not all(math.isfinite(v) for v in (*pos,diameter)):
                raise ValueError("Non-finite control point")
            if diameter <= 0:
                continue
            records.append(RECORD.pack(*[(p-c)*world_scale for p,c in zip(pos,center)],
                                       *color,diameter))
    return struct.pack("<8sII", MAGIC, len(records), RECORD.size) + b"".join(records), center


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("json", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--world-scale", type=float, default=0.025,
                        help="meters per Open Brush unit (default: 0.025)")
    args = parser.parse_args()
    payload, center = compile_dots(json.loads(args.json.read_text()), args.world_scale)
    args.output.write_bytes(payload)
    print(f"Dots instances={(len(payload)-16)//RECORD.size}, bytes={len(payload)}, "
          f"world center={center}, scale={args.world_scale}")


if __name__ == "__main__":
    main()
