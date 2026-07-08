# Tools/ArtPipeline — TRELLIS.2 → Blender → UE5 art pipeline

Automated generate → refine → import pipeline that turns a single concept image
into a game-ready textured mesh (law: `.claude/pipeline/CONVENTIONS.md`
"Textured mesh law (TRELLIS.2 art pipeline)"). Pilot scope: **Footman, Archer,
Castle**; the remaining 16 blockouts reuse this pipeline at M7.

```
Inbox/<AssetName>.png                (Jonathan drops the concept)
   │  Stage 1  trellis_generate.py   (HF Space microsoft/TRELLIS.2, gradio_client)
   ▼
Cache/<AssetName>/trellis_raw.glb  + state.json + api_schema.json
   │  Stage 2  refine_trellis_glb.py (headless Blender — TASK-083)
   ▼
Content/RawAssets/<AssetName>.fbx  + RawAssets/Textures/<AssetName>/*.png
   │  Stage 3  Unreal editor import  (art-director via Unreal MCP, serialized)
   ▼
/Game/Meshes/SM_<AssetName>  (same-path overwrite — soft references survive)
```

## One-time setup

### 1. Python environment (uv-managed, pinned 3.12)

```powershell
cd Tools\ArtPipeline
uv sync
```

`uv` downloads its own managed Python 3.12 (`gradio_client` is not validated on
3.14 — the system Python is never touched) and installs the locked deps.

### 2. HF_TOKEN (generation only — `--check` needs no token)

`HF_TOKEN` is **ENV-ONLY** (binding ruling): it lives in your environment
variables and nowhere else — never in a file, never on a command line, never in
chat, and the tooling never echoes or logs it.

1. Create a token at https://huggingface.co/settings/tokens (read scope is enough).
2. Windows Settings → System → About → Advanced system settings →
   Environment Variables → **User variables** → New → name `HF_TOKEN`,
   value = your token. (The GUI keeps it off command lines and shell history.)
3. Open a **new** terminal — running shells do not see new variables.

If the token is unset, `trellis_generate.py` exits with code 2 and prints these
setup steps. Agents only ever check that the variable *exists*.

**Quota:** free ZeroGPU ≈ 5 GPU-min/day ≈ 1–2 assets/day. Quota-exceeded
messages are surfaced verbatim (they contain the reset time) with exit code 3 —
an expected pause, not a failure. HF PRO (~$9/mo, 40 GPU-min/day) is
recommended before the 16-mesh M7 batch.

## Stage commands

All from `Tools\ArtPipeline`.

### Stage 1 — Generate (TRELLIS.2 on Hugging Face)

```powershell
uv run trellis_generate.py Footman            # reads Inbox/Footman.png
uv run trellis_generate.py Footman --seed 7   # reroll a bad generation
uv run trellis_generate.py --check            # tokenless smoke test (no GPU, no quota)
```

Writes `Cache/Footman/trellis_raw.glb` + `state.json` (seed, params, concept
sha256) + `api_schema.json` (live endpoint snapshot). The whole
preprocess → generate → extract sequence runs atomically in one session —
never split it across runs. Exit codes: 0 success · 1 failure · 2 HF_TOKEN
unset · 3 quota exhausted · 4 Space API drift · 5 concept image missing ·
64 CLI usage error.

### Stage 2 — Refine (headless Blender; script lands in TASK-083)

```powershell
& "C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --python refine_trellis_glb.py -- --asset Footman
```

Cleanup → remesh/decimate to the `pipeline_manifest.json` tri budget → Smart-UV
(`UVMap`) → Cycles CPU bake D/N/ORM → two-slot split (`TeamRegion` /
`<AssetName>PBR`) → UCX (buildings) → FBX + texture PNGs + previews +
`refine_report.json`. Heavy Blender work always runs headless like this — the
live Blender MCP bridge has a 30 s socket cap and is for quick inspection only.
(Exact flags: see the header of `refine_trellis_glb.py` once TASK-083 lands.)

### Stage 3 — Import (Unreal editor, art-director, serialized)

Not a shell command: the art-director imports via Unreal MCP — textures to
`/Game/Textures/T_<AssetName>_D|_N|_ORM`, `MI_<AssetName>_PBR` from
`/Game/Materials/M_AssetPBR`, and the FBX **overwriting** `/Game/Meshes/SM_<AssetName>`
at the same path (never delete+recreate). Nanite OFF. See TASK-086/087 specs.

## Concept image guidance (for Inbox/ drops)

TRELLIS.2 generates best from a clean product-shot-style reference:

- **One single subject** — no scenes, props, companions, or text.
- **Plain or transparent background** (flat white/grey is fine; the preprocess
  step removes it, and clutter confuses the cutout).
- **¾ view** (front-three-quarter) shows the most geometry; straight-on front
  or side views lose depth.
- **No ground shadow / no contact shadow** — shadows get baked into geometry.
- Subject fills most of the frame; square ~1024×1024 PNG recommended
  (the script warns on non-square or <512 px inputs).
- Neutral, even lighting; avoid dramatic rim light or colored washes — surface
  color becomes the texture.
- Named exactly `<AssetName>.png` (e.g. `Footman.png`, `Archer.png`, `Castle.png`).

## Fallback procedures

### Space API drift (`--check` or a run exits 4)

The Space's endpoints changed. The `api_schema.json` snapshot shows what the
Space exposes now — file it with the QA report so the client can be updated.
Until then, use the manual fallback below.

### Manual browser fallback (quota stuck, API drift, or Space down)

The pipeline resumes at Stage 2 from a hand-delivered GLB:

1. Jonathan opens https://huggingface.co/spaces/microsoft/TRELLIS.2 in a
   browser (logged in), uploads the concept image, generates, and downloads
   the extracted GLB (use ~500k decimation target / 2048 texture size if the
   UI asks).
2. Drop the file at `Cache/<AssetName>/trellis_raw.glb` (create the folder if
   needed).
3. Run Stage 2 as normal — it only needs the GLB; a missing `state.json` just
   means provenance is "manual browser run" (note it in the task handoff).

### Bad generation (mesh is mangled/wrong)

Reroll Stage 1 with a different `--seed` (quota permitting) before touching
Stage 2. If bakes misbehave later, Stage 2 has a `"native"` mode that keeps
TRELLIS's own mesh/UVs/textures (see TASK-083).

## Directory layout

| Path | What | Git |
|---|---|---|
| `Inbox/<AssetName>.png` | Concept drops (human) | ignored (TASK-084) |
| `Cache/<AssetName>/` | GLB, state.json, api_schema.json, previews, refine_report.json | ignored (TASK-084) |
| `pyproject.toml`, `uv.lock`, `.python-version` | Pinned 3.12 env | committed |
| `trellis_generate.py` | Stage 1 CLI | committed (code — full QA gate) |
| `refine_trellis_glb.py`, `pipeline_manifest.json` | Stage 2 (TASK-083) | committed (code — full QA gate) |

Accepted concepts are additionally committed at
`Content/RawAssets/Concepts/<AssetName>.png` during Stage 2/3 (see CONVENTIONS).
