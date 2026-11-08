#!/usr/bin/env python3
"""Inspect native Open Brush .tilt archives without converting brush geometry.

Reference: Open Brush Assets/Scripts/Save/SketchWriter.cs (SKETCH_SENTINEL,
GetStrokes, WriteMemory). This intentionally does not claim to render brushes.
"""
import argparse
import io
import json
import struct
import zipfile


def read(stream, fmt):
    size = struct.calcsize("<" + fmt)
    chunk = stream.read(size)
    if len(chunk) != size:
        raise ValueError("Truncated data.sketch")
    return struct.unpack("<" + fmt, chunk)


def inspect(path):
    with zipfile.ZipFile(path) as archive:
        names = archive.namelist()
        print("Archive entries:", ", ".join(names))
        metadata_name = next((n for n in names if n.lower().endswith("metadata.json")), None)
        if metadata_name:
            metadata = json.loads(archive.read(metadata_name))
            print("Metadata keys:", ", ".join(sorted(metadata)))
            for key in ("BrushIndex", "Brushes", "SceneTransformInRoomSpace"):
                if key in metadata:
                    value = metadata[key]
                    print(f"{key}:", json.dumps(value, ensure_ascii=False)[:1000])
        sketch_name = next((n for n in names if n.lower().endswith("data.sketch")), None)
        if sketch_name is None:
            raise ValueError("Missing data.sketch")
        stream = io.BytesIO(archive.read(sketch_name))
        sentinel, version, reserved, extra = read(stream, "IiII")
        if sentinel != 0xC576A5CD or version not in (5, 6):
            raise ValueError(f"Unsupported sketch sentinel/version: {sentinel:08x}/{version}")
        if extra > len(stream.getbuffer()) - stream.tell():
            raise ValueError("Invalid header extension length")
        stream.seek(extra, io.SEEK_CUR)
        (count,) = read(stream, "i")
        if not 0 <= count <= 2_000_000:
            raise ValueError("Invalid stroke count")
        print(f"Sketch version={version}, strokes={count}, bytes={len(stream.getbuffer())}")
        brush_index = metadata.get("BrushIndex", []) if metadata_name else []
        strokes = []
        total_points = 0
        brush_counts = {}
        for stroke_id in range(count):
            (brush_id,) = read(stream, "i")
            if not 0 <= brush_id < len(brush_index):
                raise ValueError(f"Stroke {stroke_id}: invalid brush index {brush_id}")
            color = read(stream, "4f")
            (size, stroke_mask, point_mask) = read(stream, "fII")
            if stroke_mask & ~0x1001F:
                raise ValueError(f"Stroke {stroke_id}: unsupported stroke mask {stroke_mask:#x}")
            if point_mask & ~3:
                raise ValueError(f"Stroke {stroke_id}: unsupported control point mask {point_mask:#x}")
            extras = {}
            for bit in (1, 2, 4, 8, 16, 0x10000):
                if not stroke_mask & bit:
                    continue
                if bit == 1:
                    extras["flags"] = read(stream, "I")[0]
                elif bit == 2:
                    extras["scale"] = read(stream, "f")[0]
                elif bit == 4:
                    extras["group"] = read(stream, "I")[0]
                elif bit == 8:
                    extras["seed"] = read(stream, "i")[0]
                elif bit == 16:
                    extras["layer"] = read(stream, "I")[0]
                else:
                    (byte_length,) = read(stream, "I")
                    if byte_length > len(stream.getbuffer()) - stream.tell():
                        raise ValueError("Invalid per-point color extension length")
                    extras["point_color_extension_bytes"] = byte_length
                    stream.seek(byte_length, io.SEEK_CUR)
            (point_count,) = read(stream, "i")
            if not 0 <= point_count <= 2_000_000 or total_points + point_count > 10_000_000:
                raise ValueError(f"Stroke {stroke_id}: invalid control point count")
            points = []
            for _ in range(point_count):
                position = read(stream, "3f")
                orientation = read(stream, "4f")
                pressure = read(stream, "f")[0] if point_mask & 1 else 1.0
                timestamp = read(stream, "I")[0] if point_mask & 2 else 0
                points.append({"position": position, "orientation": orientation,
                               "pressure": pressure, "timestamp_ms": timestamp})
            guid = brush_index[brush_id]
            brush_counts[guid] = brush_counts.get(guid, 0) + 1
            total_points += point_count
            strokes.append({"brush_guid": guid, "color": color, "size": size,
                            "extras": extras, "points": points})
        print(f"Decoded strokes={len(strokes)}, control points={total_points}, remaining bytes={len(stream.getbuffer()) - stream.tell()}")
        print("Brush usage:")
        for guid, amount in sorted(brush_counts.items(), key=lambda item: -item[1]):
            print(f"  {amount:5d}  {guid}")
        return {"strokes": strokes, "brush_usage": brush_counts}



if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("tilt", help="Open Brush .tilt archive")
    parser.add_argument("--json", help="Write decoded strokes as JSON")
    args = parser.parse_args()
    result = inspect(args.tilt)
    if args.json:
        with open(args.json, 'w', encoding='utf-8') as output:
            json.dump(result, output, separators=(',', ':'))
        print('Wrote', args.json)
