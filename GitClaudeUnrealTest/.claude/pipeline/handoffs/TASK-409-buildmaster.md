# TASK-409 — `SiegeLlama` plugin scaffold + vendored llama.cpp (C API only)

**Agent:** build-master · **Date:** 2026-08-02 · **Status:** `ready-for-qa` (gate = TASK-412)
**Lane posture:** developed on `main`. **NOT COMMITTED, NOT PUSHED** — TASK-414 owns the commit.
**`L_Arena` never opened. No editor asset touched. No gameplay code written.**

**M8 DECLARATION DUTY:** adds no replicated property, no new replicated class, no new relevancy tier.

---

## 0. Pre-flight

| Check | Result |
|---|---|
| `git status --porcelain .gitignore` | **clean** ✅ |
| `git status --porcelain GitClaudeUnrealTest.uproject` | **clean** ✅ |

Both shipped single-owner files were clean before editing, so no merge with parked work occurred.

### ⚠️ (0) BASELINE `Build.bat` — **NOT RUN. DELIBERATELY DEFERRED.**

The spec's step (0) asks for a baseline build on "the untouched tree". **The tree was not untouched
and a build was explicitly forbidden by the dispatch**, so running it would have produced a
misleading result rather than a baseline:

- **25 files under `Source/GitClaudeUnrealTest/` were already dirty** on arrival, including
  4 untracked new `SiegeAssistant*.{h,cpp}` pairs and a `Tests/` directory from TASK-416/417/418.
- The dispatch stated two game-module agents were actively fixing compile errors, and instructed:
  *"do not run a UBT build of the game module — a game-module gate is pending and the quiet-module
  law binds."*
- **Under the QUIET-MODULE LAW this is exactly the contaminated-gate scenario that failed TASK-401**
  an hour earlier. A build now would compile another batch's half-written files and attribute
  foreign diagnostics to this lane.

**This is not a skipped step; it is the law being applied.** Ruling 11 exists to stop a build-master
attributing a foreign red to this batch — deferring is the same protection, applied one step earlier.

**➡️ ORCHESTRATOR ACTION REQUIRED:** the baseline + in-engine compile must run once the module is
quiet. Exact command, unchanged from CLAUDE.md:

```
"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" GitClaudeUnrealTestEditor Win64 Development -project="C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/GitClaudeUnrealTest.uproject" -waitmutex
```

Then in-editor: `Siege.Llama.Info`. ⚠️ **Apply the exit-code law — `Build.bat` returns exit 0 on
failure. Judge on log text; a missing verdict line is a failure.**

---

## 1. Upstream vendored — exact version

| | |
|---|---|
| Project | [ggml-org/llama.cpp](https://github.com/ggml-org/llama.cpp) (**MIT**) |
| **Release tag** | **`b10235`** |
| **Commit SHA** | **`221f0f6356efe2260023208365705ec5d5a7c8f5`** |
| Published | 2026-08-02T21:02:39Z |
| Artifact | `llama-b10235-bin-win-vulkan-x64.zip` |
| Artifact SHA-256 | `f3ab5195a620247e29b10e0eac5efa88beaa6098c9a45743beddc24cec60420f` |

Official upstream release binaries, not a local build — this machine has **no Vulkan SDK**, and the
published artifact is both reproducible and avoids a bespoke shader-compilation toolchain.

### Build flags

| | |
|---|---|
| Backend | **Vulkan + CPU. NOT CUDA.** |
| Platform / config | Win64 x64, **Release** |
| CRT | **`/MD`** — *verified, not assumed*: every vendored DLL imports `MSVCP140.dll` / `VCRUNTIME140.dll`, matching UE |
| Toolset (import libs) | MSVC **19.38.33145** = VS2022 **14.38.33130** (UE 5.8 wants 14.38+) |
| Defines | `GGML_SHARED=1`, `GGML_BACKEND_SHARED=1`, `LLAMA_SHARED=1` |

### ✅ The C API only — enforced *and verified*

`llama.h` + `ggml.h` + transitive headers. **`common/` is neither vendored nor linked.**

**Verified rather than trusted: `llama.dll`'s import table does not reference `llama-common.dll`.**
The C API is cleanly separable, so the `/MD`-vs-`/MT` + `_ITERATOR_DEBUG_LEVEL` + toolset-mismatch
failure class is structurally avoided, not merely hoped away.

Deliberately excluded from `bin/Win64/`: `llama-common.dll` (the forbidden `common/`), `mtmd.dll`,
`ggml-rpc.dll` (RPC — the sidecar/network shape was rejected), all `*-impl.dll`, all `.exe`.

---

## 2. ⚠️ Link proof — **standalone PASS; in-engine DEFERRED**

`Siege.Llama.Info` is written and registered, but its in-engine output requires the UBT build above.
**I did not fake it.** Instead I built the strongest proof available without UBT: a standalone MSVC
harness compiled against `include/`, linked against the three generated import libs, and **run**.

It exercises the same calls, in the same form, as `Siege.Llama.Info`. Real output:

```
ggml_vulkan: Found 2 Vulkan devices:
ggml_vulkan: 0 = Intel(R) Arc(TM) 140T GPU (32GB) (Intel Corporation) | uma: 1 | fp16: 1 | bf16: 0 | warp size: 32 | matrix cores: KHR_coopmat
ggml_vulkan: 1 = NVIDIA GeForce RTX 5070 Laptop GPU (NVIDIA) | uma: 0 | fp16: 1 | bf16: 1 | warp size: 32 | matrix cores: NV_coopmat2
load_backend: loaded Vulkan backend from ...\LlamaCpp\bin\Win64\ggml-vulkan.dll
load_backend: loaded CPU backend from ...\LlamaCpp\bin\Win64\ggml-cpu-alderlake.dll

=== llama_print_system_info() ===
CPU : SSE3 = 1 | SSSE3 = 1 | AVX = 1 | AVX_VNNI = 1 | AVX2 = 1 | F16C = 1 | FMA = 1 | BMI2 = 1 | LLAMAFILE = 1 | OPENMP = 1 | REPACK = 1 |

=== registered backends: 2 ===
  [0] Vulkan
  [1] CPU

=== devices: 3 ===
  [0] Vulkan0    type=iGPU  mem=47796.7/37020.8 MiB  | Intel(R) Arc(TM) 140T GPU (32GB)
  [1] Vulkan1    type=GPU   mem=7123.0/7891.0 MiB    | NVIDIA GeForce RTX 5070 Laptop GPU
  [2] CPU        type=CPU   mem=40564.1/64948.8 MiB  | Intel(R) Core(TM) Ultra 9 285H

LINKPROOF_VERDICT: PASS (backends=2 devices=3)
```

**This empirically settles the no-vendor-lock-in ruling.** One `ggml-vulkan.dll` drove **both** an
Intel Arc iGPU **and** an NVIDIA RTX 5070 — and the RTX 5070's **7891 MiB is precisely the modal
8 GB card** the design targets. CPU fallback registered independently.

**What it proves:** headers are self-contained · import libs resolve every symbol we call · no
`common/` symbol is required · Vulkan+CPU both register from the vendored tree.
**What it does NOT prove:** UBT/UHT integration, the `.uplugin` wiring, module startup, or the
`UDeveloperSettings` class. **Those still need the build.**

### 🐞 A real bug this caught — and it would have been a guaranteed red build

A second compile check over the exact llama/ggml API surface used by the plugin caught an error in
my own `SiegeLlamaInfo.cpp`:

`ggml_backend_dev_type` is **both a type and a function** (`ggml-backend.h:182`). In C++ the function
name **hides** the type, so the elaborated `enum` keyword is mandatory — which is why upstream's own
declaration reads `GGML_API enum ggml_backend_dev_type ggml_backend_dev_type(...)`.

Verified with teeth: the un-fixed form fails hard with `C4430 / C2146 / C3646 / C2059 / C2065 /
C2050`. **Fixed, re-compiled clean at `/W4` with zero warnings.** Flagged here because it is exactly
the class of defect the deferred gate exists to catch, and it is now pre-empted.

---

## 3. `git check-ignore -v` — **acceptance, pasted, not asserted**

```
$ git check-ignore -v Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/bin/Win64/llama.dll
GitClaudeUnrealTest/.gitignore:113:!Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/bin/**/*.dll	Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/bin/Win64/llama.dll

$ git check-ignore -v Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/lib/Win64/llama.lib
GitClaudeUnrealTest/.gitignore:112:!Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/lib/**/*.lib	Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/lib/Win64/llama.lib

$ git check-ignore -v Models/Qwen3.5-4B-Instruct-Q4_K_M.gguf
GitClaudeUnrealTest/.gitignore:117:/Models/	Models/Qwen3.5-4B-Instruct-Q4_K_M.gguf
```

Both vendored paths report the **negation** line; the `.gguf` reports the **ignore** line. Confirmed
with the plain form (exit 1 = not ignored, exit 0 = ignored):

```
exit=1  .../bin/Win64/llama.dll          <- tracked
exit=1  .../lib/Win64/llama.lib          <- tracked
exit=1  .../bin/Win64/ggml-vulkan.dll    <- tracked
exit=0  Models/x.gguf                    <- ignored
```

**Decisive end-to-end check — `git add --dry-run Plugins/` stages exactly 40 files:**
19 `.dll` + 3 `.lib` + 7 vendored headers + 11 source/docs. Nothing swallowed, nothing extra.

The block was appended **after** the global `*.dll`/`*.lib` rules (last matching pattern wins), and
the parent dirs are not themselves ignored — only `Plugins/*/Binaries/` and `Plugins/*/Intermediate/`
are, while the vendored tree lives under `Source/`. Added verbatim from CONVENTIONS §7:

```gitignore
!Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/lib/**/*.lib
!Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/bin/**/*.dll
*.gguf
/Models/
```

---

## 4. Vendored file list

`Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/` — **29 files, ~72 MB**

| File | Bytes |
|---|---:|
| `LICENSE` (MIT, verbatim) | 1,078 |
| `VERSION.md` | — |
| `LlamaCpp.Build.cs` | 3,787 |
| **include/** (7) | |
| `llama.h` | 82,792 |
| `ggml.h` | 111,883 |
| `ggml-backend.h` | 25,306 |
| `ggml-opt.h` | 14,040 |
| `gguf.h` | 11,355 |
| `ggml-cpu.h` | 7,753 |
| `ggml-alloc.h` | 3,753 |
| **lib/Win64/** (3, generated) | |
| `ggml-base.lib` | 151,568 |
| `llama.lib` | 66,400 |
| `ggml.lib` | 5,288 |
| **bin/Win64/** (19) | |
| `ggml-vulkan.dll` | 52,332,032 |
| `llama.dll` | 2,795,008 |
| `ggml-base.dll` | 772,096 |
| `libomp140.x86_64.dll` | 661,856 |
| `ggml.dll` | 86,016 |
| 14 × `ggml-cpu-<arch>.dll` | 17,300,992 total |

CPU variants: `alderlake · cannonlake · cascadelake · cooperlake · haswell · icelake · ivybridge ·
piledriver · sandybridge · sapphirerapids · skylakex · sse42 · x64 · zen4`.

**Include closure verified complete** — every `#include "…"` across the 7 headers resolves inside
`include/`.

**Why 3 import libs, not 1:** the symbols split across DLLs — `ggml.dll` owns the backend *registry*
(`ggml_backend_reg_count`, `ggml_backend_dev_get`, `ggml_backend_load_all`, 16 exports) while
`ggml-base.dll` owns the rest of ggml (676 exports) and `llama.dll` the llama API (258 exports).
Upstream ships **no `.lib`**, so all three were generated via `dumpbin /exports` → `.def` →
`lib /def:` (recipe in `VERSION.md`). All exports are functions — no `DATA` tags needed.

**Why all 14 CPU variants:** `ggml-base` selects the best match for the host at registration time
(observed picking `alderlake` on a Core Ultra 9 285H). Shipping only the baseline would make the
spike's **CPU-fallback latency pessimistic versus what actually ships** — and CPU fallback is
Jonathan's hard requirement, so the measurement must be honest.

`vulkan-1.dll` is intentionally **not** vendored — it is the driver/OS-supplied Vulkan loader. If
absent, the Vulkan backend simply fails to register and ggml falls back to CPU: the intended
graceful degradation.

---

## 5. Files created / modified

**No file under `Source/GitClaudeUnrealTest/` was created, modified, or opened for writing.**
Verified: nothing in that tree has an mtime later than this task's first write. The 25 dirty files
there are **foreign** (TASK-416/417/418 lanes) and pre-dated this task. **The exemption holds.**

**New — plugin (exclusively owned):**
- `Plugins/SiegeLlama/SiegeLlama.uplugin` — Runtime, `LoadingPhase: Default`, Win64 allow-list
- `Plugins/SiegeLlama/Source/SiegeLlama/SiegeLlama.Build.cs`
- `Public/SiegeLlamaLog.h` — `LogSiegeLlama` declaration ⚠️ see flag (a)
- `Public/SiegeLlamaModule.h` · `Private/SiegeLlamaModule.cpp` — DLL lifetime, degradation posture
- `Public/SiegeLlamaSettings.h` · `Private/SiegeLlamaSettings.cpp` — the model override chain
- `Private/SiegeLlamaInfo.cpp` — `Siege.Llama.Info`
- `Source/ThirdParty/LlamaCpp/**` — vendored library (above)

**New — elsewhere:** `Docs/ThirdPartyNotices.md` · `Tools/fetch_llm_model.py`
**Modified (both were clean):** `.gitignore` (+16 lines at end) · `GitClaudeUnrealTest.uproject`
(one `{"Name": "SiegeLlama", "Enabled": true}` entry; JSON re-parsed OK)
**Created empty + gitignored:** `Models/`

### The DLL trap, handled

`PublicAdditionalLibraries` (3 `.lib`) · `PublicDelayLoadDLLs` (**only** the 3 we import from) ·
`RuntimeDependencies` → `$(BinaryOutputDir)` for **all 19** DLLs — the ggml backends are opened by
ggml itself *by filename* at runtime, so they are never link-time imports and would otherwise vanish
from a packaged build.

`StartupModule` does an explicit `FPlatformProcess::GetDllHandle` per DLL in dependency order
(`libomp140` → `ggml-base` → `ggml` → `llama`), inside `PushDllDirectory`/`PopDllDirectory`, with
`FreeDllHandle` in reverse at shutdown. It requires the **full set present** before loading anything,
so a partial vendored tree fails cleanly instead of half-loading. **A missing/unloadable DLL logs
once on `LogSiegeLlama` at Warning and leaves the module inert — never a crash, never a blocked
editor launch.** Search order: plugin vendored dir (editor) → `FPlatformProcess::BaseDir()` (packaged).

⚠️ **Because the DLLs are delay-loaded, calling any `llama_*`/`ggml_*` function while
`IsLlamaAvailable()` is false raises a structured exception rather than returning an error. Every
call site must gate on it.** Written into the header and `LlamaCpp.Build.cs` so TASK-410/423 cannot
miss it.

Backend registration is **deferred** to `EnsureBackendsLoaded()` rather than done at startup —
enumerating Vulkan devices creates driver objects and costs editor-startup time we should not pay
until something actually wants inference.

### `Tools/fetch_llm_model.py` — smoke tests run (`Tools/**/*.py` is CODE ⇒ QA-gated)

Stdlib-only (no `huggingface_hub`, no pip), resumable via HTTP Range into `<file>.part`,
sha256-verified against HF's LFS oid, `--check` health probe, non-zero exit on failure.

```
$ python Tools/fetch_llm_model.py --check
HF_TOKEN      : SET
writable      : YES
huggingface.co: reachable (HTTP 200)
local models  : 0
CHECK_VERDICT: PASS                                    EXIT=0
```

Ran **bare** — Norton's HF exclusions held, no `SSL_CERT_FILE` hack. Failure paths verified:
missing args → `FETCH_VERDICT: FAIL` **exit 2**; unknown repo → `FETCH_VERDICT: FAIL` **exit 1**.
Metadata/checksum plumbing validated without a 2.5 GB download: a real LFS file returned size
548,105,171 and a 64-char sha256; a non-LFS file correctly returned `sha256: None`.

🔒 **`HF_TOKEN` is env-only and never printed** — `--check` reports presence only (`SET`), never the
value. The token is additionally **stripped on any cross-host redirect**, so HF's CDN redirect cannot
carry the bearer token off `huggingface.co`/`hf.co`. No secret appears in any file, log or report.

---

## 🚩 FLAGGED — need a ruling; none is blocking on its own

**(a) `LogSiegeLlama` could not live where §5 pins it — cross-task link contract.**
CONVENTIONS §5 pins the category to `SiegeLlamaSubsystem.h/.cpp`, but that file is **TASK-423's
exclusively owned file** (ruling 6) and does not exist yet, while this module and `Siege.Llama.Info`
need the category now. Declared in `Public/SiegeLlamaLog.h`, **defined once** in
`SiegeLlamaModule.cpp`.
⚠️ **TASK-423 must `#include "SiegeLlamaLog.h"` and must NOT re-declare/re-define it — a second
`DEFINE_LOG_CATEGORY` is a duplicate-symbol LINK error, which surfaces late and confusingly.**
Noted in the header itself so it cannot be missed.

**(b) Two module deps beyond the spec's "`Core`, `CoreUObject`, `Engine` only".**
Added `Projects` (for `IPluginManager`, which the spec itself mandates for locating the plugin base
dir) and `DeveloperSettings` (for `UDeveloperSettings`, the base of `USiegeLlamaSettings`). **Neither
is `HTTP` nor `Sockets`** — the actual prohibition, whose rationale is the firewall prompt from a
sidecar `llama-server.exe`. **That decision stays closed: this plugin opens no socket and makes no
network call.** If QA rules against `DeveloperSettings`, the fallback is a `GConfig` read with **no
API change**, because all resolution routes through `USiegeLlamaSettings::ResolveModelPath()`.

**(c) ⚠️⚠️ `.gitattributes` HAS NO LFS RULE FOR `*.dll`/`*.lib` — ~72 MB WOULD ENTER GIT AS RAW
BLOBS, PERMANENTLY. Decide BEFORE TASK-414 commits.**
The repo **does** use LFS, but `.gitattributes` (at the **repo root**, one level above the project
dir) tracks only `*.uasset, *.umap, *.fbx, *.png, *.jpg, *.wav, *.mp4`. `ggml-vulkan.dll` alone is
52 MB. **`.gitattributes` is NOT in TASK-409's owned-file list and is outside the project directory,
so I did not touch it.** This is cheap to fix now and expensive later — undoing it means rewriting
history. Recommend a manager ruling + a task to add `*.dll`/`*.lib` LFS tracking **before** the
commit. ⚠️ **Note the ordering trap: files committed before the pattern is added stay raw blobs
unless re-added.**

**(d) Three dispatch instructions were superseded by CONVENTIONS, which I followed as the law.**
1. Dispatch said model dir `Plugins/SiegeLlama/Models/*.gguf`; **CONVENTIONS §5/§7 pins
   `<ProjectRoot>/Models/` ignored wholesale.** Used CONVENTIONS.
2. Dispatch asked for a tracked `Models/README.md`; **impossible** under the pinned `/Models/` rule
   — git will not descend into an ignored directory to honour a `!` inside it (the file's own
   documented rule). Rather than weaken the pinned block, the setup documentation lives in
   `Tools/fetch_llm_model.py`'s header + `--help` and in `USiegeLlamaSettings`' doc comment. **Say
   the word and I'll add `/Models/*` + `!/Models/README.md` instead** — one-line change, but it
   deviates from a character-for-character pin, so I did not take it unilaterally.
3. Dispatch asked for the **model** staged via `RuntimeDependencies(..., NonUFS)`; **CONVENTIONS §7
   and flagged item (b) explicitly defer GGUF staging to Wave 2** ("do not bolt a shipping path in
   early"). `RuntimeDependencies` **is** wired for the **DLLs**, which is what spec (3) requires.

**(e) `libomp140.x86_64.dll` may need its own licence entry.** It is the LLVM/Intel OpenMP runtime
(Apache 2.0 with LLVM exceptions) redistributed unmodified in the upstream zip, and `ggml-base.dll`
has a load-time dependency on it — so it is **not** covered by llama.cpp's MIT text. Noted in
`Docs/ThirdPartyNotices.md` for confirmation at content lock.

**(f) `Docs/ThirdPartyNotices.md` §2 is an intentional PLACEHOLDER and is a shipping blocker until
filled.** TASK-413 owns it: repo id, exact filename, quant, and the licence line **verbatim** — from
the **quant's** card, never the family's. Banned models (xLAM-2, Hammer 2.1, Arch-Function) and the
cleared permissive list are recorded there.

**(g) Cosmetic upstream quirk, no action:** the Intel Arc iGPU reports free (47,796 MiB) > total
(37,020 MiB) via UMA. A driver/upstream reporting artefact — do not "fix" it in our code.

---

## Downstream notes for TASK-410 (spike harness)

- Include `SiegeLlamaLog.h` for `LogSiegeLlama`. Do **not** define the category.
- **Gate every llama/ggml call on `FSiegeLlamaModule::GetPtr()->IsLlamaAvailable()`** (delay-load).
- Call `EnsureBackendsLoaded()` before any inference; backends are not registered at startup.
- Resolve the model with `USiegeLlamaSettings::ResolveModelPath()` — `-siegellm.model=<path>` →
  settings → `<ProjectDir>/Models/<DefaultModelFileName>`. **Swapping quants needs no rebuild.**
- The path is **not guaranteed to exist** — a missing model must degrade, never hard-fail.
- `DefaultModelFileName` currently reads `Qwen3.5-4B-Instruct-Q4_K_M.gguf` and is **provisional**
  until TASK-413 verifies the exact repo/file/quant.
- `use_mmap = true` is load-bearing for the 8 GB claim (weights page-cache-backed, not resident).
- You own **only** `Private/SiegeLlamaSpike.cpp`. Everything above is TASK-409's.
