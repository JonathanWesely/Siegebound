"""extract_frames.py — the footage-review frame extractor (FR-§2).

Turns gameplay videos dropped in <git-root>/testvideo/ into things Claude can
actually read: probe metadata, timestamp-labeled contact sheets (coarse pass),
full-resolution single frames (fine pass), crops, and promoted evidence PNGs.

LAWS (FR-§ batch in .claude/pipeline/CONVENTIONS.md):
  - Runtime: system Python (3.14), stdlib + subprocess->ffmpeg ONLY. Pillow is
    OPTIONAL: sheets degrade to an unlabeled ffmpeg tile grid without it; the
    `crop` subcommand requires it (exit 4). No venv, no numpy, and
    Tools/ArtPipeline/.venv is never touched by this tool.
  - Every subprocess call uses list argv; shell=True is BANNED. This is what
    makes Xbox Game Bar filenames (double spaces, parentheses) a non-issue.
  - Video filenames are accepted VERBATIM: never parse tokens out of them,
    never assume a TASK-### exists. Default video = newest mtime in testvideo/.
  - The frame cache testvideo/.frames/<slug>-<hash8>/ is DISPOSABLE and
    gitignored (the whole testvideo/ dir is ignored at the root .gitignore).
  - Promoted evidence goes to .claude/pipeline/playtest-evidence/<YYYY-MM-DD>/
    named VID-###[-t<MM>m<SS>s]-<symptom>.png (FR-§1) — the only lane of this
    pipeline that ever enters git.

EXIT CODES:
  0  ok
  2  ffmpeg/ffprobe not found (message names the winget install command)
  3  video missing or undecodable
  4  Pillow required for this subcommand but not installed
  5  bad timestamp (out of range / unparseable)
  64 usage error
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import re
import shutil
import subprocess
import sys
from datetime import date
from pathlib import Path

# --- paths (derived, never hardcoded to a drive) ---------------------------
GIT_ROOT = Path(__file__).resolve().parents[3]
VIDEO_DIR = GIT_ROOT / "testvideo"
CACHE_ROOT = VIDEO_DIR / ".frames"
EVIDENCE_ROOT = GIT_ROOT / "GitClaudeUnrealTest" / ".claude" / "pipeline" / "playtest-evidence"

VIDEO_EXTS = (".mp4", ".mkv", ".mov")
WINGET_HINT = "winget install --id Gyan.FFmpeg -e   (then open a fresh terminal, or rerun --check to auto-locate)"

# --- exe discovery ----------------------------------------------------------

def find_exe(name: str) -> str | None:
    """PATH first; then the winget Links shim; then the Gyan.FFmpeg package dir.

    The extra probes matter because a shell opened BEFORE the winget install
    does not see the new PATH — the tool self-locates instead of failing.
    """
    hit = shutil.which(name)
    if hit:
        return hit
    localapp = os.environ.get("LOCALAPPDATA", "")
    if localapp:
        link = Path(localapp) / "Microsoft" / "WinGet" / "Links" / f"{name}.exe"
        if link.exists():
            return str(link)
        pkgs = Path(localapp) / "Microsoft" / "WinGet" / "Packages"
        if pkgs.exists():
            for cand in sorted(pkgs.glob(f"Gyan.FFmpeg*/**/bin/{name}.exe")):
                return str(cand)
    return None


def require_exes() -> tuple[str, str]:
    ffmpeg, ffprobe = find_exe("ffmpeg"), find_exe("ffprobe")
    if not ffmpeg or not ffprobe:
        print(f"ERROR: ffmpeg/ffprobe not found. Install with:\n  {WINGET_HINT}", file=sys.stderr)
        sys.exit(2)
    return ffmpeg, ffprobe


def run(argv: list[str]) -> subprocess.CompletedProcess:
    # list argv only — shell=True is banned (FR-§2).
    return subprocess.run(argv, capture_output=True, text=True)


# --- video selection / cache ------------------------------------------------

def resolve_video(arg: str | None) -> Path:
    if arg:
        p = Path(arg)
        if not p.is_absolute():
            cand = VIDEO_DIR / arg
            p = cand if cand.exists() else p
        if not p.exists():
            print(f"ERROR: video not found: {p}", file=sys.stderr)
            sys.exit(3)
        return p
    if not VIDEO_DIR.exists():
        print(f"ERROR: video folder missing: {VIDEO_DIR}", file=sys.stderr)
        sys.exit(3)
    vids = [p for p in VIDEO_DIR.iterdir() if p.suffix.lower() in VIDEO_EXTS]
    if not vids:
        print(f"ERROR: no videos ({'/'.join(VIDEO_EXTS)}) in {VIDEO_DIR}", file=sys.stderr)
        sys.exit(3)
    return max(vids, key=lambda p: p.stat().st_mtime)


def cache_dir_for(video: Path) -> Path:
    slug = re.sub(r"[^a-z0-9]+", "-", video.stem.lower()).strip("-")[:60] or "video"
    h8 = hashlib.sha1(video.name.encode("utf-8")).hexdigest()[:8]
    d = CACHE_ROOT / f"{slug}-{h8}"
    d.mkdir(parents=True, exist_ok=True)
    return d


# --- timestamp helpers ------------------------------------------------------

def parse_ts(text: str) -> float:
    """Accept '93.5', '1:33.5', '01:02:03'."""
    parts = text.strip().split(":")
    try:
        nums = [float(p) for p in parts]
    except ValueError:
        print(f"ERROR: unparseable timestamp '{text}'", file=sys.stderr)
        sys.exit(5)
    t = 0.0
    for n in nums:
        t = t * 60 + n
    if t < 0 or any(n < 0 for n in nums):
        print(f"ERROR: negative timestamp '{text}'", file=sys.stderr)
        sys.exit(5)
    return t


def fmt_ts(t: float) -> str:
    m, s = divmod(t, 60.0)
    return f"{int(m):02d}:{s:04.1f}"


# --- probe ------------------------------------------------------------------

def do_probe(video: Path, quiet: bool = False) -> dict:
    _, ffprobe = require_exes()
    cp = run([ffprobe, "-v", "error", "-print_format", "json",
              "-show_format", "-show_streams", str(video)])
    if cp.returncode != 0 or not cp.stdout.strip():
        print(f"ERROR: ffprobe could not decode {video.name}:\n{cp.stderr.strip()}", file=sys.stderr)
        sys.exit(3)
    data = json.loads(cp.stdout)
    vstream = next((s for s in data.get("streams", []) if s.get("codec_type") == "video"), None)
    if vstream is None:
        print(f"ERROR: no video stream in {video.name}", file=sys.stderr)
        sys.exit(3)

    def rate(expr: str) -> float:
        try:
            num, den = expr.split("/")
            return float(num) / float(den) if float(den) else 0.0
        except (ValueError, AttributeError):
            return 0.0

    duration = float(data.get("format", {}).get("duration", 0.0))
    avg, real = rate(vstream.get("avg_frame_rate", "0/1")), rate(vstream.get("r_frame_rate", "0/1"))
    summary = {
        "video": video.name,
        "duration_s": round(duration, 2),
        "width": vstream.get("width"),
        "height": vstream.get("height"),
        "codec": vstream.get("codec_name"),
        "avg_fps": round(avg, 3),
        "vfr": abs(avg - real) > 0.01,
        "size_mb": round(video.stat().st_size / 1e6, 1),
    }
    cache = cache_dir_for(video)
    (cache / "probe.json").write_text(json.dumps({"summary": summary, "raw": data}, indent=2), encoding="utf-8")
    if not quiet:
        print(f"{video.name}\n  duration {summary['duration_s']} s · {summary['width']}x{summary['height']}"
              f" · {summary['codec']} · avg {summary['avg_fps']} fps · {summary['size_mb']} MB")
        if summary["vfr"]:
            print("  WARNING: variable frame rate capture (Game Bar) — sheet timestamps are ±interval/2")
        print(f"  cache: {cache}")
    return summary


# --- autocrop ---------------------------------------------------------------

def detect_crop(ffmpeg: str, video: Path, duration: float, width: int, height: int) -> str | None:
    """Game Bar captures often park the game window in a corner of a larger
    black canvas. Sample cropdetect at 3 points; return 'crop=w:h:x:y' when the
    content box is meaningfully smaller than the frame, else None."""
    votes: dict[str, int] = {}
    for frac in (0.15, 0.5, 0.85):
        t = max(0.0, min(duration * frac, max(duration - 1.0, 0.0)))
        cp = run([ffmpeg, "-v", "info", "-ss", f"{t:.2f}", "-t", "1",
                  "-i", str(video), "-vf", "cropdetect=24:8:0", "-f", "null", "-"])
        hits = re.findall(r"crop=(\d+:\d+:\d+:\d+)", cp.stderr)
        if hits:
            votes[hits[-1]] = votes.get(hits[-1], 0) + 1
    if not votes:
        return None
    best = max(votes, key=lambda k: votes[k])
    w, h, _, _ = (int(v) for v in best.split(":"))
    if w <= 0 or h <= 0 or (w >= width - 32 and h >= height - 32):
        return None  # content fills the frame — no crop worth applying
    return f"crop={best}"


# --- sheets (coarse pass) ---------------------------------------------------

def do_sheets(video: Path, interval: float | None, grid: str, tile_width: int,
              autocrop: bool = True) -> None:
    ffmpeg, _ = require_exes()
    info = do_probe(video, quiet=True)
    duration = info["duration_s"]
    if duration <= 0:
        print("ERROR: zero-duration video", file=sys.stderr)
        sys.exit(3)
    if interval is None:
        interval = min(max(duration / 48.0, 0.5), 10.0)
    m = re.fullmatch(r"(\d+)x(\d+)", grid)
    if not m:
        print("ERROR: --grid must look like 4x3", file=sys.stderr)
        sys.exit(64)
    cols, rows = int(m.group(1)), int(m.group(2))
    per_sheet = cols * rows

    cache = cache_dir_for(video)
    thumbs = cache / "thumbs"
    if thumbs.exists():
        shutil.rmtree(thumbs)
    thumbs.mkdir(parents=True)
    for stale in cache.glob("sheet_*.png"):  # sheets carry crop state only in the manifest — never serve stale ones
        stale.unlink()
    crop = detect_crop(ffmpeg, video, duration, info["width"], info["height"]) if autocrop else None
    vf = f"fps=1/{interval}," + (f"{crop}," if crop else "") + f"scale={tile_width}:-2"
    cp = run([ffmpeg, "-v", "error", "-i", str(video), "-vf", vf,
              str(thumbs / "thumb_%04d.png")])
    if cp.returncode != 0:
        print(f"ERROR: ffmpeg thumb extraction failed:\n{cp.stderr.strip()}", file=sys.stderr)
        sys.exit(3)
    thumb_files = sorted(thumbs.glob("thumb_*.png"))
    if not thumb_files:
        print("ERROR: no thumbnails produced", file=sys.stderr)
        sys.exit(3)

    # thumb n (1-based) ~ (n-1)*interval, ±interval/2 under VFR — stated in the manifest.
    stamps = [(i) * interval for i in range(len(thumb_files))]
    sheets: list[dict] = []
    n_sheets = math.ceil(len(thumb_files) / per_sheet)

    try:
        from PIL import Image, ImageDraw, ImageFont  # optional (FR-§2)
        have_pil = True
    except ImportError:
        have_pil = False

    if have_pil:
        try:
            font = ImageFont.truetype("arial.ttf", 22)
        except OSError:
            font = ImageFont.load_default()
        label_h = 30
        with Image.open(thumb_files[0]) as first:
            tw, th = first.size
        for s in range(n_sheets):
            batch = thumb_files[s * per_sheet:(s + 1) * per_sheet]
            sheet_img = Image.new("RGB", (cols * tw, rows * (th + label_h)), (12, 12, 12))
            draw = ImageDraw.Draw(sheet_img)
            tiles = []
            for i, tf in enumerate(batch):
                r, c = divmod(i, cols)
                x, y = c * tw, r * (th + label_h)
                with Image.open(tf) as im:
                    sheet_img.paste(im, (x, y))
                t = stamps[s * per_sheet + i]
                draw.text((x + 6, y + th + 4), fmt_ts(t), fill=(255, 255, 80), font=font)
                tiles.append({"tile": i + 1, "t": round(t, 2), "label": fmt_ts(t)})
            out = cache / f"sheet_{s + 1:02d}.png"
            sheet_img.save(out)
            sheets.append({"sheet": out.name, "tiles": tiles})
    else:
        # unlabeled fallback — the manifest below is the only timestamp source.
        cp = run([ffmpeg, "-v", "error", "-framerate", "1",
                  "-i", str(thumbs / "thumb_%04d.png"),
                  "-vf", f"tile={cols}x{rows}", str(cache / "sheet_%02d.png")])
        if cp.returncode != 0:
            print(f"ERROR: ffmpeg tile fallback failed:\n{cp.stderr.strip()}", file=sys.stderr)
            sys.exit(3)
        for s in range(n_sheets):
            tiles = [{"tile": i + 1, "t": round(stamps[s * per_sheet + i], 2),
                      "label": fmt_ts(stamps[s * per_sheet + i])}
                     for i in range(len(thumb_files[s * per_sheet:(s + 1) * per_sheet]))]
            sheets.append({"sheet": f"sheet_{s + 1:02d}.png", "tiles": tiles})

    manifest = {
        "video": video.name, "duration_s": duration, "interval_s": round(interval, 3),
        "grid": f"{cols}x{rows}", "labeled": have_pil, "autocrop": crop or "none",
        "timestamp_accuracy": "±interval/2 (VFR capture)" if info["vfr"] else "±interval/2",
        "tile_order": "row-major, top-left first", "sheets": sheets,
    }
    (cache / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(f"{len(sheets)} sheet(s) ({cols}x{rows}, interval {interval:.2f} s, "
          f"{'labeled' if have_pil else 'UNLABELED — use manifest.json'}, "
          f"autocrop {crop or 'none'}) in {cache}")
    for sh in sheets:
        print(f"  {cache / sh['sheet']}")


# --- frames (fine pass) -----------------------------------------------------

def do_frames(video: Path, at: str, run_n: int, autocrop: bool = True) -> None:
    ffmpeg, _ = require_exes()
    info = do_probe(video, quiet=True)
    crop = detect_crop(ffmpeg, video, info["duration_s"], info["width"], info["height"]) if autocrop else None
    crop_args = ["-vf", crop] if crop else []
    cache = cache_dir_for(video)
    frames_dir = cache / "frames"
    frames_dir.mkdir(exist_ok=True)
    outs: list[Path] = []
    for token in at.split(","):
        t = parse_ts(token)
        if t > info["duration_s"]:
            print(f"ERROR: timestamp {token} > duration {info['duration_s']} s", file=sys.stderr)
            sys.exit(5)
        # crop state is part of the name — a pre-autocrop cached frame must never
        # be mistaken for (or overwritten ambiguously by) a cropped one.
        tag = f"f{t:08.2f}s".replace(".", "_") + ("-c" if crop else "")
        if run_n > 1:
            pattern = frames_dir / f"{tag}-r%02d.png"
            cp = run([ffmpeg, "-v", "error", "-ss", f"{t:.3f}", "-i", str(video),
                      *crop_args, "-frames:v", str(run_n), "-fps_mode", "passthrough", str(pattern)])
            got = sorted(frames_dir.glob(f"{tag}-r*.png"))
        else:
            out = frames_dir / f"{tag}.png"
            cp = run([ffmpeg, "-v", "error", "-ss", f"{t:.3f}", "-i", str(video),
                      *crop_args, "-frames:v", "1", "-update", "1", str(out)])
            got = [out] if out.exists() else []
        if cp.returncode != 0 or not got:
            print(f"ERROR: frame extraction failed at {token}:\n{cp.stderr.strip()}", file=sys.stderr)
            sys.exit(3)
        outs.extend(got)
    for p in outs:
        print(p)


# --- crop -------------------------------------------------------------------

def do_crop(frame: str, box: str, scale: int) -> None:
    try:
        from PIL import Image
    except ImportError:
        print("ERROR: Pillow is required for crop (pip install pillow)", file=sys.stderr)
        sys.exit(4)
    src = Path(frame)
    if not src.exists():
        print(f"ERROR: frame not found: {src}", file=sys.stderr)
        sys.exit(3)
    m = re.fullmatch(r"(\d+),(\d+),(\d+),(\d+)", box)
    if not m:
        print("ERROR: --box must be x,y,w,h (integers)", file=sys.stderr)
        sys.exit(64)
    x, y, w, h = map(int, m.groups())
    with Image.open(src) as im:
        region = im.crop((x, y, x + w, y + h))
        if scale > 1:
            region = region.resize((w * scale, h * scale), Image.NEAREST)  # crisp UI pixels
        out = src.with_name(f"{src.stem}-crop-x{x}y{y}.png")
        try:
            out.resolve().relative_to(EVIDENCE_ROOT.resolve())
            print("ERROR: crops are working files — they may not land in playtest-evidence/ (use promote)", file=sys.stderr)
            sys.exit(64)
        except ValueError:
            pass  # outside the evidence tree — the normal case
        region.save(out)
    print(out)


# --- promote ----------------------------------------------------------------

def do_promote(frame: str, vid_id: str, symptom: str, t: str | None) -> None:
    src = Path(frame)
    if not src.exists():
        print(f"ERROR: frame not found: {src}", file=sys.stderr)
        sys.exit(3)
    if not re.fullmatch(r"VID-\d{3}", vid_id):
        print("ERROR: --id must look like VID-007 (FR-§1)", file=sys.stderr)
        sys.exit(64)
    slug = re.sub(r"[^a-z0-9]+", "-", symptom.lower()).strip("-")
    if not slug:
        print("ERROR: --symptom produced an empty slug", file=sys.stderr)
        sys.exit(64)
    token = ""
    if t:
        secs = parse_ts(t)
        mm, ss = divmod(int(round(secs)), 60)
        token = f"-t{mm:02d}m{ss:02d}s"
    dest_dir = EVIDENCE_ROOT / date.today().isoformat()
    dest_dir.mkdir(parents=True, exist_ok=True)
    dest = dest_dir / f"{vid_id}{token}-{slug}.png"
    shutil.copyfile(src, dest)
    rel = dest.relative_to(GIT_ROOT / "GitClaudeUnrealTest")
    print(str(rel).replace("\\", "/"))


# --- check ------------------------------------------------------------------

def do_check() -> None:
    ffmpeg, ffprobe = find_exe("ffmpeg"), find_exe("ffprobe")
    print(f"ffmpeg : {ffmpeg or 'MISSING'}")
    print(f"ffprobe: {ffprobe or 'MISSING'}")
    if not ffmpeg or not ffprobe:
        print(f"Install: {WINGET_HINT}", file=sys.stderr)
        sys.exit(2)
    ver = run([ffmpeg, "-version"]).stdout.splitlines()
    print(f"version: {ver[0] if ver else '?'}")
    try:
        import PIL  # noqa: F401
        from PIL import __version__ as pv
        print(f"Pillow : {pv} (labeled sheets + crop available)")
    except ImportError:
        print("Pillow : absent (sheets will be UNLABELED; crop unavailable) — informational")
    if VIDEO_DIR.exists():
        vids = [p for p in VIDEO_DIR.iterdir() if p.suffix.lower() in VIDEO_EXTS]
        if vids:
            newest = max(vids, key=lambda p: p.stat().st_mtime)
            print(f"newest : {newest.name}")
            do_probe(newest)
        else:
            print(f"newest : (no videos in {VIDEO_DIR})")
    else:
        print(f"folder : {VIDEO_DIR} MISSING")
    print("check  : OK")


# --- main -------------------------------------------------------------------

def main(argv: list[str] | None = None) -> None:
    ap = argparse.ArgumentParser(
        prog="extract_frames.py",
        description="Footage-review frame extractor (FR-§2). See module docstring for laws + exit codes.")
    ap.add_argument("--check", action="store_true", help="health probe: ffmpeg present, newest video decodable")
    sub = ap.add_subparsers(dest="cmd")

    p = sub.add_parser("probe", help="ffprobe summary + probe.json")
    p.add_argument("--video")

    p = sub.add_parser("sheets", help="coarse pass: timestamp-labeled contact sheets")
    p.add_argument("--video")
    p.add_argument("--interval", type=float, default=None, help="seconds between tiles (default duration/48, clamped 0.5..10)")
    p.add_argument("--grid", default="4x3")
    p.add_argument("--tile-width", type=int, default=480)
    p.add_argument("--no-autocrop", action="store_true", help="skip black-border cropdetect (Game Bar corner captures)")

    p = sub.add_parser("frames", help="fine pass: full-res frames at timestamps")
    p.add_argument("--video")
    p.add_argument("--at", required=True, help="comma list: 93.5 or 1:33.5")
    p.add_argument("--run", type=int, default=1, help="N consecutive decoded frames from each t (flicker instrument)")
    p.add_argument("--no-autocrop", action="store_true", help="skip black-border cropdetect")

    p = sub.add_parser("crop", help="crop+zoom a frame (Pillow required)")
    p.add_argument("frame")
    p.add_argument("--box", required=True, help="x,y,w,h")
    p.add_argument("--scale", type=int, default=1)

    p = sub.add_parser("promote", help="copy a frame into playtest-evidence/ with the FR-§1 name")
    p.add_argument("frame")
    p.add_argument("--id", required=True, help="VID-###")
    p.add_argument("--symptom", required=True, help="kebab-case symptom slug")
    p.add_argument("--t", default=None, help="timestamp for the -tMMmSSs token (e.g. 1:32)")

    try:
        args = ap.parse_args(argv)
    except SystemExit as e:
        sys.exit(64 if e.code not in (0, None) else 0)

    if args.check:
        do_check()
        return
    if args.cmd == "probe":
        do_probe(resolve_video(args.video))
    elif args.cmd == "sheets":
        do_sheets(resolve_video(args.video), args.interval, args.grid, args.tile_width,
                  autocrop=not args.no_autocrop)
    elif args.cmd == "frames":
        do_frames(resolve_video(args.video), args.at, args.run, autocrop=not args.no_autocrop)
    elif args.cmd == "crop":
        do_crop(args.frame, args.box, args.scale)
    elif args.cmd == "promote":
        do_promote(args.frame, getattr(args, "id"), args.symptom, args.t)
    else:
        ap.print_help()
        sys.exit(64)


if __name__ == "__main__":
    main()
