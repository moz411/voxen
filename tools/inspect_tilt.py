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
        # Do not parse stroke records until all extension masks are implemented:
        # guessing offsets would silently corrupt subsequent brush GUIDs.
        print("Native stroke geometry decoding: pending")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("tilt", help="Open Brush .tilt archive")
    args = parser.parse_args()
    inspect(args.tilt)
