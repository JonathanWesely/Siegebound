# Third-Party Notices

Third-party software and data distributed with **Siegebound**.

Created by TASK-409 (LLM-ASSISTANT batch). **Missing notices are a shipping
defect, not a documentation nit** — see CONVENTIONS "In-match LLM command
assistant (v1, text-only) — 2026-08-02" §7.

---

## 1. llama.cpp

| | |
|---|---|
| Component | `llama.cpp` / `ggml` |
| Upstream | https://github.com/ggml-org/llama.cpp |
| Version | release tag **`b10235`**, commit **`221f0f6356efe2260023208365705ec5d5a7c8f5`** |
| Vendored at | `Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/` |
| Distributed as | 19 Win64 DLLs (`llama.dll`, `ggml*.dll`, `ggml-vulkan.dll`, `ggml-cpu-*.dll`, `libomp140.x86_64.dll`) |
| Licence | **MIT** |

Only the C API is used (`llama.h` + `ggml.h`). llama.cpp's `common/` helpers are
neither vendored nor linked. Build provenance and reproduction steps:
`Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/VERSION.md`.

### Licence text (verbatim)

```
MIT License

Copyright (c) 2023-2026 The ggml authors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

### A note on `libomp140.x86_64.dll`

Redistributed unmodified as shipped in the upstream llama.cpp Windows release.
It is the LLVM/Intel OpenMP runtime (Apache 2.0 with LLVM exceptions) and
`ggml-base.dll` has a load-time dependency on it. ⚠️ **At content lock, confirm
whether this file requires its own notice entry** — it is a redistributable from
the MSVC/LLVM toolchain rather than part of llama.cpp's own source.

---

## 2. Language model weights — ⛔ PLACEHOLDER, NOT YET FILLED

> **THIS SECTION IS INTENTIONALLY INCOMPLETE AND IS A SHIPPING BLOCKER UNTIL FILLED.**
> **Owner: TASK-413**, which verifies the model card of **the exact GGUF quant**
> actually shipped and pastes its licence line here verbatim.

No model weights are committed to this repository. The GGUF is fetched per
machine by `Tools/fetch_llm_model.py` into `<ProjectRoot>/Models/`, which is
gitignored wholesale. This section covers weights **redistributed in a packaged
build**, which is a Wave-2 concern.

| Field | Value |
|---|---|
| Repo id | *(TASK-413)* |
| Exact filename | *(TASK-413)* |
| Quantisation | *(TASK-413)* |
| Licence | *(TASK-413)* |
| Licence line, verbatim | *(TASK-413)* |

### ⚠️ Verify the quant's card, not the family's

A permissive base does **not** guarantee a permissive quant re-upload. Quantisers
re-host under their own terms, so a `…-GGUF` repo carries a *different* card from
the original publisher's. **"It's Apache because the family is" is not
verification.**

### ⛔ Banned models — never ship these

Every purpose-built small **function-calling** model surveyed is non-commercial
(CC-BY-NC or research-only). Shipping one is a licensing breach. Named because
they are exactly what a naive search recommends:

- **xLAM-2**
- **Hammer 2.1**
- **Arch-Function**

### ✅ Permissive bases cleared for evaluation

- **Qwen3.5-4B (Apache 2.0)** — the spike default
- Gemma 4 E4B (Apache 2.0, one clause to confirm)
- Phi-4-mini (MIT)
- SmolLM3-3B
- Granite 4.x

Also cleared and already relevant: llama.cpp (MIT). Whisper / whisper.cpp (MIT)
and Silero VAD (MIT) are **not** v1 scope — there is no voice in v1 — and are
listed only so a later voice pass does not re-research them.
