#!/usr/bin/env python3
"""Normalize RuntimeInspector Fab listing images.

The Fab media gallery accepts JPEG or PNG images, with 2D image files required
to be at least 1920x1080 and less than the configured byte limit. This script
keeps the full screenshot visible by fitting it inside the target canvas and
letterboxing with the RuntimeInspector dark panel color.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError as exc:
    raise SystemExit("Pillow is required: python -m pip install Pillow") from exc


IMAGE_EXTENSIONS = {".png", ".jpg", ".jpeg"}
DEFAULT_BACKGROUND = (8, 15, 22)
DEFAULT_FAB_FILES = [
    "cover.png",
    "screenshot_01_actor_panel.png",
    "screenshot_02_changes_workflow.png",
    "screenshot_03_settings.png",
    "screenshot_04_tools.png",
]
OPTIONAL_FAB_FILES = [
    "screenshot_05_remote_session.png",
    "screenshot_06_promote_or_audit.png",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Normalize Fab media images.")
    parser.add_argument("--input", required=True, type=Path, help="Input image directory.")
    parser.add_argument("--output", required=True, type=Path, help="Output media directory.")
    parser.add_argument("--width", type=int, default=1920, help="Target canvas width.")
    parser.add_argument("--height", type=int, default=1080, help="Target canvas height.")
    parser.add_argument("--max-bytes", type=int, default=3 * 1024 * 1024, help="Maximum file size in bytes.")
    parser.add_argument("--manifest", default="fab_media_manifest.json", help="Manifest filename in output directory.")
    parser.add_argument("--files", nargs="*", default=None, help="Specific filenames to normalize.")
    parser.add_argument("--include-optional", action="store_true", help="Also normalize optional remote/promote screenshots.")
    parser.add_argument("--all-images", action="store_true", help="Normalize every image in the input directory.")
    return parser.parse_args()


def iter_images(input_dir: Path, filenames: list[str] | None, include_optional: bool, all_images: bool) -> list[Path]:
    if all_images:
        return sorted(
            path
            for path in input_dir.iterdir()
            if path.is_file() and path.suffix.lower() in IMAGE_EXTENSIONS
        )

    selected = filenames if filenames else [*DEFAULT_FAB_FILES, *(OPTIONAL_FAB_FILES if include_optional else [])]
    missing: list[str] = []
    images: list[Path] = []
    for filename in selected:
        path = input_dir / filename
        if path.is_file() and path.suffix.lower() in IMAGE_EXTENSIONS:
            images.append(path)
        else:
            missing.append(filename)

    if missing:
        joined = ", ".join(missing)
        raise FileNotFoundError(f"Missing expected Fab media file(s): {joined}")

    return images


def fit_to_canvas(source: Image.Image, width: int, height: int) -> Image.Image:
    source = source.convert("RGB")
    scale = min(width / source.width, height / source.height)
    fit_size = (
        max(1, int(round(source.width * scale))),
        max(1, int(round(source.height * scale))),
    )
    resized = source.resize(fit_size, Image.Resampling.LANCZOS)
    canvas = Image.new("RGB", (width, height), DEFAULT_BACKGROUND)
    offset = ((width - fit_size[0]) // 2, (height - fit_size[1]) // 2)
    canvas.paste(resized, offset)
    return canvas


def remove_existing_variants(output_dir: Path, stem: str) -> None:
    for suffix in IMAGE_EXTENSIONS:
        candidate = output_dir / f"{stem}{suffix}"
        if candidate.exists():
            candidate.unlink()


def save_png(canvas: Image.Image, path: Path) -> None:
    canvas.save(path, format="PNG", optimize=True, compress_level=9)


def save_jpeg_under_limit(canvas: Image.Image, path: Path, max_bytes: int) -> tuple[int, int]:
    for quality in range(92, 69, -4):
        canvas.save(path, format="JPEG", quality=quality, optimize=True, progressive=True)
        size = path.stat().st_size
        if size < max_bytes:
            return quality, size

    canvas.save(path, format="JPEG", quality=68, optimize=True, progressive=True)
    return 68, path.stat().st_size


def normalize_image(source_path: Path, output_dir: Path, width: int, height: int, max_bytes: int) -> dict[str, object]:
    remove_existing_variants(output_dir, source_path.stem)

    with Image.open(source_path) as source:
        original_size = (source.width, source.height)
        canvas = fit_to_canvas(source, width, height)

    png_path = output_dir / f"{source_path.stem}.png"
    save_png(canvas, png_path)
    png_size = png_path.stat().st_size

    output_path = png_path
    output_format = "PNG"
    quality = None
    output_size = png_size

    if png_size >= max_bytes:
        png_path.unlink()
        output_path = output_dir / f"{source_path.stem}.jpg"
        output_format = "JPEG"
        quality, output_size = save_jpeg_under_limit(canvas, output_path, max_bytes)

    passed = canvas.width == width and canvas.height == height and output_size < max_bytes
    return {
        "source": str(source_path),
        "filename": output_path.name,
        "width": canvas.width,
        "height": canvas.height,
        "bytes": output_size,
        "format": output_format,
        "jpegQuality": quality,
        "originalWidth": original_size[0],
        "originalHeight": original_size[1],
        "pass": passed,
    }


def main() -> int:
    args = parse_args()
    input_dir = args.input.resolve()
    output_dir = args.output.resolve()

    if not input_dir.is_dir():
        print(f"Input directory not found: {input_dir}", file=sys.stderr)
        return 2

    output_dir.mkdir(parents=True, exist_ok=True)
    try:
        images = iter_images(input_dir, args.files, args.include_optional, args.all_images)
    except FileNotFoundError as exc:
        print(str(exc), file=sys.stderr)
        return 3
    if not images:
        print(f"No images found under: {input_dir}", file=sys.stderr)
        return 3

    records = [
        normalize_image(path, output_dir, args.width, args.height, args.max_bytes)
        for path in images
    ]

    manifest = {
        "targetWidth": args.width,
        "targetHeight": args.height,
        "maxBytes": args.max_bytes,
        "images": records,
        "allPassed": all(bool(record["pass"]) for record in records),
    }
    manifest_path = output_dir / args.manifest
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")

    for record in records:
        status = "PASS" if record["pass"] else "FAIL"
        print(
            f"{status} {record['filename']} {record['width']}x{record['height']} "
            f"{record['bytes']} bytes {record['format']}"
        )

    print(f"Manifest: {manifest_path}")
    return 0 if manifest["allPassed"] else 4


if __name__ == "__main__":
    raise SystemExit(main())
