"""Offline evaluation of the M_AncientGround node graph (stock-node math, 1:1).

Renders the exact expression tree authored in /Game/Materials/M_AncientGround so the
shape can be eyeballed against M_CaptureZone before anything ships. Pure stdlib PNG.
"""
import math, struct, zlib

# ---- M_AncientGround parameter defaults (as authored) ----
RING_COUNT = 2.5
RING_SHARP = 6.0
SPOKE_COUNT = 6.0
SPOKE_SHARP = 3.0
SPOKE_DEPTH = 0.55
DISC_MIN, DISC_MAX = 0.93, 1.03   # SmoothStep constMin/constMax
FILL_OP, RING_OP = 0.10, 0.55
FILL_GLOW, RING_GLOW = 0.60, 2.40
PULSE_AMOUNT = 0.35
GROUND_COLOR = (0.10, 0.85, 0.55)

# ---- M_CaptureZone (donor, for the differentiation check) ----
CZ_BAND = (0.80, 0.97)
CZ_FILL_OP, CZ_RING_OP = 0.08, 0.45
CZ_FILL_GLOW, CZ_RING_GLOW = 0.9, 1.8
CZ_BLUE = (0.05, 0.30, 1.00)

GROUND = (0.075, 0.070, 0.055)   # dark battlefield ground, linear


def smoothstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


def lerp(a, b, t):
    return a + (b - a) * t


def ancient(u, v, pulse):
    """Emissive(rgb), Opacity -- exactly the authored graph."""
    cx, cy = (u - 0.5) * 2.0, (v - 0.5) * 2.0          # Subtract 0.5 -> Multiply 2
    r = math.hypot(cx, cy)                              # Length
    ring = abs(math.sin(2 * math.pi * r * RING_COUNT)) ** RING_SHARP   # Sine/Abs/Power
    ang = math.atan2(cy, cx)                            # Arctangent2
    spoke = abs(math.sin(2 * math.pi * (ang * 0.1591549431) * SPOKE_COUNT)) ** SPOKE_SHARP
    spoke_factor = lerp(1.0 - SPOKE_DEPTH, 1.0, spoke)  # OneMinus + Lerp
    rune_raw = ring * spoke_factor
    disc = 1.0 - smoothstep(DISC_MIN, DISC_MAX, r)      # SmoothStep + OneMinus
    ring_mask = rune_raw * disc
    # fill term is NOT disc-masked: the faint wash covers the full square quad so the
    # honest mechanic footprint (840x840 box, corners included) is marked.
    op = min(max(lerp(FILL_OP, RING_OP, ring_mask) * pulse, 0.0), 1.0)  # Saturate
    glow = lerp(FILL_GLOW, RING_GLOW, ring_mask) * pulse
    return tuple(c * glow for c in GROUND_COLOR), op


def capture_zone(u, v):
    cx, cy = (u - 0.5) * 2.0, (v - 0.5) * 2.0
    d = max(abs(cx), abs(cy))                            # Chebyshev (Abs/Mask/Mask/Max)
    border = smoothstep(CZ_BAND[0], CZ_BAND[1], d)
    op = lerp(CZ_FILL_OP, CZ_RING_OP, border)
    glow = lerp(CZ_FILL_GLOW, CZ_RING_GLOW, border)
    return tuple(c * glow for c in CZ_BLUE), op


def composite(base, emissive, opacity):
    # deferred decal, BLEND_Translucent, BaseColor unconnected (black):
    # albedo lerps toward black by opacity; emissive is added scaled by opacity.
    return tuple(base[i] * (1.0 - opacity) + emissive[i] * opacity for i in range(3))


def encode(v):
    return min(255, max(0, int(round((v ** (1 / 2.2)) * 255))))


SS = 2          # supersample
PANEL = 400
W = H = PANEL * 2

rows = []
for py in range(H):
    row = bytearray([0])
    for px in range(W):
        acc = [0.0, 0.0, 0.0]
        for sy in range(SS):
            for sx in range(SS):
                gx = px + (sx + 0.5) / SS
                gy = py + (sy + 0.5) / SS
                panel = (1 if gx >= PANEL else 0) + (2 if gy >= PANEL else 0)
                u = (gx % PANEL) / PANEL
                v = (gy % PANEL) / PANEL
                col = GROUND
                if panel == 0:                       # top-left: pulse peak
                    e, o = ancient(u, v, 1.0 + PULSE_AMOUNT)
                    col = composite(col, e, o)
                elif panel == 1:                     # top-right: pulse trough
                    e, o = ancient(u, v, 1.0 - PULSE_AMOUNT)
                    col = composite(col, e, o)
                elif panel == 2:                     # bottom-left: M_CaptureZone donor
                    e, o = capture_zone(u, v)
                    col = composite(col, e, o)
                else:                                # bottom-right: overlapped, offset
                    e, o = capture_zone(u, v)
                    col = composite(col, e, o)
                    ou, ov = u - 0.28, v - 0.28      # ancient ground offset, drawn on top
                    if 0.0 <= ou <= 1.0 and 0.0 <= ov <= 1.0:
                        e2, o2 = ancient(ou, ov, 1.0)
                        col = composite(col, e2, o2)
                for i in range(3):
                    acc[i] += col[i]
        n = SS * SS
        # panel separator
        if abs(px - PANEL) <= 1 or abs(py - PANEL) <= 1:
            row += bytes([90, 90, 90])
        else:
            row += bytes([encode(acc[0] / n), encode(acc[1] / n), encode(acc[2] / n)])
    rows.append(bytes(row))

raw = b"".join(rows)


def chunk(tag, data):
    return (struct.pack(">I", len(data)) + tag + data
            + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))


png = (b"\x89PNG\r\n\x1a\n"
       + chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 2, 0, 0, 0))
       + chunk(b"IDAT", zlib.compress(raw, 9))
       + chunk(b"IEND", b""))

out = (r"C:\Users\wesel\AppData\Local\Temp\claude"
       r"\C--GitProjects-GitHub-GitClaudeUnrealTesting-GitClaudeUnrealTest"
       r"\5f2dd256-656a-4481-9e2f-fce95384eeeb\scratchpad\M_AncientGround_preview.png")
with open(out, "wb") as f:
    f.write(png)
print("wrote", out)
