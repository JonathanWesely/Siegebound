# Vendored llama.cpp — provenance and reproduction

Vendored by **TASK-409** (LLM-ASSISTANT batch, Wave 0) on **2026-08-02**.

## Upstream

| | |
|---|---|
| Project | [ggml-org/llama.cpp](https://github.com/ggml-org/llama.cpp) |
| Licence | MIT — see `LICENSE` (verbatim upstream copy) |
| Release tag | **`b10235`** |
| Commit SHA | **`221f0f6356efe2260023208365705ec5d5a7c8f5`** |
| Published | 2026-08-02T21:02:39Z |
| Binary artifact | `llama-b10235-bin-win-vulkan-x64.zip` |
| Artifact SHA-256 | `f3ab5195a620247e29b10e0eac5efa88beaa6098c9a45743beddc24cec60420f` |

Binaries are the **official upstream release build** — not a local build. This
machine has no Vulkan SDK, and using the published artifact is both reproducible
and avoids a bespoke shader-compilation toolchain.

## Build configuration

| | |
|---|---|
| Backend | **Vulkan + CPU. NEVER CUDA.** |
| Platform | Win64, x64 |
| Configuration | Release |
| CRT | **`/MD`** (dynamic) — verified: every vendored DLL imports `MSVCP140.dll` / `VCRUNTIME140.dll`, matching UE's own `/MD`. |
| Toolset (import libs) | MSVC **19.38.33145** (VS2022 **14.38.33130**), satisfying UE 5.8's 14.38+ requirement |

**Why Vulkan, not CUDA:** one binary covers NVIDIA / AMD / Intel, which is
Jonathan's no-vendor-lock-in ruling made concrete. CUDA would add 1 GB+ of DLLs
and be NVIDIA-only. Proven on this machine — a single `ggml-vulkan.dll` drove
**both** an Intel Arc 140T iGPU and an NVIDIA RTX 5070 Laptop GPU.

## ⚠️ The C API only

Only `llama.h` + `ggml.h` and their transitive headers are vendored, and only
`llama.dll` / `ggml.dll` / `ggml-base.dll` are linked.

**llama.cpp's `common/` helpers are NOT vendored and NOT linked.** They are C++
with STL in their signatures and are exactly where `/MD` vs `/MT`,
`_ITERATOR_DEBUG_LEVEL` and MSVC-toolset-mismatch link failures come from. The
cost — roughly 60 lines of sampler setup — is code we want to own anyway,
because the sampler chain is unusual (grammar-first, near-greedy).

Verified rather than assumed: **`llama.dll`'s import table does not reference
`llama-common.dll`**, so the C API is cleanly separable.

Deliberately excluded from `bin/Win64/`: `llama-common.dll` (the forbidden
`common/`), `mtmd.dll` (multimodal), `ggml-rpc.dll` (RPC backend — the
sidecar/network shape was rejected), all `*-impl.dll`, and all `.exe`.

## Vendored contents

- `include/` — 7 headers, a verified-complete include closure:
  `llama.h`, `ggml.h`, `ggml-alloc.h`, `ggml-backend.h`, `ggml-cpu.h`,
  `ggml-opt.h`, `gguf.h`
- `lib/Win64/` — `llama.lib`, `ggml.lib`, `ggml-base.lib` (generated, see below)
- `bin/Win64/` — 19 DLLs ≈ 72 MB:
  - linked: `llama.dll`, `ggml.dll`, `ggml-base.dll`
  - runtime-loaded by ggml: `ggml-vulkan.dll` + 14 × `ggml-cpu-<arch>.dll`
  - dependency: `libomp140.x86_64.dll`

**Why all 14 CPU variants:** `ggml-base` picks the best match for the host CPU at
registration time (observed selecting `ggml-cpu-alderlake.dll` on an Intel Core
Ultra 9 285H). Shipping only the baseline would make the spike's CPU-fallback
latency pessimistic versus what actually ships — and CPU fallback is a hard
requirement, not a nicety.

`vulkan-1.dll` is intentionally **not** vendored: it is the Vulkan loader, supplied
by the GPU driver / OS. If absent, the Vulkan backend simply fails to register and
ggml falls back to CPU — the intended graceful degradation.

## Reproducing the import libraries

Upstream ships **no `.lib`** in the Windows release zip, so the import libraries
are generated from the DLL export tables. All exports are functions (no
`GGML_API extern` data declarations exist in the vendored headers), so no symbol
needs a `DATA` tag.

```bat
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" -vcvars_ver=14.38

rem 1. dump the export table
dumpbin /exports llama.dll > exp-llama.txt

rem 2. turn it into a .def:  "LIBRARY <dll>" + "EXPORTS" + one symbol per line
rem 3. build the import library
lib /def:llama.def /machine:x64 /out:llama.lib
```

Export counts at this tag: `llama.dll` 258 · `ggml.dll` 16 · `ggml-base.dll` 676.

Note the symbol split, which is why **three** import libs are needed rather than
one: `ggml.dll` owns the backend *registry* (`ggml_backend_reg_count`,
`ggml_backend_dev_get`, `ggml_backend_load_all`, …) while `ggml-base.dll` owns
the rest of the ggml API (`ggml_backend_reg_name`, `ggml_backend_dev_name`, …).

## Verification performed at vendoring time

A standalone MSVC harness compiled against `include/`, linked against the three
generated import libs, and ran — proving the headers are self-contained, the
import libs resolve, and no `common/` symbol is required. Real output:

```
ggml_vulkan: Found 2 Vulkan devices:
ggml_vulkan: 0 = Intel(R) Arc(TM) 140T GPU (32GB) (Intel Corporation) | uma: 1 | fp16: 1 | ...
ggml_vulkan: 1 = NVIDIA GeForce RTX 5070 Laptop GPU (NVIDIA) | uma: 0 | fp16: 1 | ...
load_backend: loaded Vulkan backend from ...\bin\Win64\ggml-vulkan.dll
load_backend: loaded CPU backend from ...\bin\Win64\ggml-cpu-alderlake.dll

=== llama_print_system_info() ===
CPU : SSE3 = 1 | SSSE3 = 1 | AVX = 1 | AVX_VNNI = 1 | AVX2 = 1 | F16C = 1 | FMA = 1 |
      BMI2 = 1 | LLAMAFILE = 1 | OPENMP = 1 | REPACK = 1 |

=== registered backends: 2 ===   [0] Vulkan   [1] CPU
=== devices: 3 ===
  [0] Vulkan0  type=iGPU  | Intel(R) Arc(TM) 140T GPU (32GB)
  [1] Vulkan1  type=GPU   | NVIDIA GeForce RTX 5070 Laptop GPU
  [2] CPU      type=CPU   | Intel(R) Core(TM) Ultra 9 285H

LINKPROOF_VERDICT: PASS (backends=2 devices=3)
```

⚠️ This is the **standalone** proof. The equivalent in-engine proof —
`Siege.Llama.Info` under a UBT build — was deferred at TASK-409 because the game
module was not quiet (the quiet-module law). See
`.claude/pipeline/handoffs/TASK-409-buildmaster.md`.
