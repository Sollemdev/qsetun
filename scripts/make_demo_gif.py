#!/usr/bin/env python3
"""
Converts LilyGO T-Display recording to optimized GitHub demo GIF.
Auto-rotates 90 deg clockwise, crops to breadboard + display, and applies adaptive palette.
"""

import av
from PIL import Image
from pathlib import Path

VIDEO_PATH = Path(r"C:\Users\PC1000\Downloads\VID_20260918_090902.mp4")
OUTPUT_GIF = Path(r"D:\Noosphere\qsetun\docs\demo_esp32_cardiac.gif")
OUTPUT_GIF.parent.mkdir(parents=True, exist_ok=True)

def generate_gif(start_sec=1.5, duration_sec=5.0, fps=10, target_width=420):
    print(f"[+] Opening video: {VIDEO_PATH}")
    container = av.open(str(VIDEO_PATH))
    stream = container.streams.video[0]
    
    src_fps = float(stream.average_rate)
    frame_step = max(1, int(round(src_fps / fps)))
    start_frame = int(start_sec * src_fps)
    end_frame = int((start_sec + duration_sec) * src_fps)

    print(f"[+] Extracting frames from {start_sec}s to {start_sec + duration_sec}s (step={frame_step})...")

    frames = []
    for idx, frame in enumerate(container.decode(video=0)):
        if idx < start_frame:
            continue
        if idx > end_frame:
            break
        if (idx - start_frame) % frame_step != 0:
            continue

        # Convert to PIL and rotate 90 deg clockwise
        img = frame.to_image().rotate(-90, expand=True)
        w, h = img.size

        # Crop to breadboard focus: include LEDs, display, buzzer
        # Y from 0.18 to 0.65
        crop_box = (0, int(h * 0.18), w, int(h * 0.65))
        cropped = img.crop(crop_box)

        # Scale down to target_width
        aspect = cropped.height / cropped.width
        target_height = int(target_width * aspect)
        resized = cropped.resize((target_width, target_height), Image.Resampling.LANCZOS)

        # Quantize to 128-color adaptive palette with dithering for compact GIF
        paletted = resized.quantize(colors=128, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.FLOYDSTEINBERG)
        frames.append(paletted)

    print(f"[+] Captured {len(frames)} frames. Encoding GIF...")
    duration_ms = int(1000 / fps)
    frames[0].save(
        str(OUTPUT_GIF),
        save_all=True,
        append_images=frames[1:],
        duration=duration_ms,
        loop=0,
        optimize=True
    )

    size_mb = OUTPUT_GIF.stat().st_size / (1024 * 1024)
    print(f"[+] GIF successfully created: {OUTPUT_GIF}")
    print(f"    Dimensions: {target_width}x{target_height} | Duration: {len(frames)*duration_ms/1000:.1f}s | Size: {size_mb:.2f} MB")

if __name__ == "__main__":
    generate_gif()
