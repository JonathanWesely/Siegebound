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

## 2. Language model weights — ✅ FILLED (TASK-413, 2026-08-02)

> **Licence cleared for commercial use.** Verified by TASK-413 against **the exact
> quant repo's own model card**, not the family's — see the warning below for why
> that distinction matters.
>
> ⚠️ **The MODEL SELECTION is still provisional even though the LICENCE is not.**
> This is the model the spike *measures*; **Jonathan rules on the shipping model
> at TASK-415.** If that ruling picks a different model, **this section must be
> re-verified from scratch** — a licence recorded here is valid only for the exact
> repo id + filename named in it.

No model weights are committed to this repository. The GGUF is fetched per
machine by `Tools/fetch_llm_model.py` into `<ProjectRoot>/Models/`, which is
gitignored wholesale. This section covers weights **redistributed in a packaged
build**, which is a Wave-2 concern.

| Field | Value |
|---|---|
| Repo id | **`Qwen/Qwen3-4B-GGUF`** |
| Exact filename | **`Qwen3-4B-Q4_K_M.gguf`** |
| Quantisation | **Q4_K_M** |
| Size | 2,497,280,256 bytes (2.497 GB) |
| sha256 (HF LFS oid) | `7485fe6f11af29433bc51cab58009521f205840f5b4ae3a32fa7f92e8534fdf5` |
| Upstream base model | `Qwen/Qwen3-4B` |
| Publisher | **Qwen themselves** — this is the official GGUF repo, *not* a third-party re-upload |
| Gated | **No** (`gated: false`) — no manual licence acceptance required |
| Licence | **Apache-2.0** |
| Licence line, verbatim | `license: apache-2.0` |
| Licence link, verbatim | `license_link: https://huggingface.co/Qwen/Qwen3-4B-GGUF/blob/main/LICENSE` |

**Verbatim front-matter of `Qwen/Qwen3-4B-GGUF`'s own `README.md` model card:**

```yaml
---
license: apache-2.0
license_link: https://huggingface.co/Qwen/Qwen3-4B-GGUF/blob/main/LICENSE
pipeline_tag: text-generation
base_model: Qwen/Qwen3-4B
---
```

**✅ Commercial-use statement:** Apache-2.0 is a permissive licence that **permits
commercial use, redistribution and distribution in a packaged game**, subject to
its own conditions: retain the copyright and licence notice, include a copy of
the licence, and state significant changes. The `…-GGUF` repo carries **its own
verbatim copy of the Apache License 2.0** (11,544 bytes) at
`https://huggingface.co/Qwen/Qwen3-4B-GGUF/blob/main/LICENSE`, so the quant
re-host and the base model agree — **there is no licence delta between them.**
**No non-commercial, research-only or field-of-use restriction applies.**

⚠️ **Wave-2 duty when the GGUF is actually staged into a build:** Apache-2.0 §4
obliges us to ship the licence copy and the notice alongside the redistributed
weights. Recording the licence here is **not** the same as satisfying §4 at
package time.

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

All rows below were re-probed live against Hugging Face by TASK-413 (2026-08-02);
the `Q4_K_M` column is the *real* filename in that repo, never a guessed one.

| Model | Repo carrying a real Q4_K_M | Licence | Notes |
|---|---|---|---|
| **Qwen3-4B** | **`Qwen/Qwen3-4B-GGUF`** → `Qwen3-4B-Q4_K_M.gguf` | **apache-2.0** | ✅ **THE SPIKE MODEL** — official Qwen GGUF, ungated, dense transformer |
| Qwen3.5-4B | `unsloth/…`, `lmstudio-community/…`, `bartowski/…` | apache-2.0 | ⚠️ **no official Qwen GGUF**; hybrid linear-attention + multimodal (see below) |
| Phi-4-mini | `unsloth/Phi-4-mini-instruct-GGUF` | MIT | |
| SmolLM3-3B | `ggml-org/SmolLM3-3B-GGUF`, `unsloth/SmolLM3-3B-GGUF` | apache-2.0 | |
| Granite 4.x | `ibm-granite/granite-4.0-h-tiny-GGUF`, `…-micro-GGUF` | apache-2.0 | |
| ~~Gemma~~ | `google/*-GGUF` | ⚠️ **`license: gemma`, `gated: manual`** | ⛔ **NOT Apache-2.0** as previously recorded, and gated behind a manual acceptance Jonathan must click |

⚠️ **The Gemma row is a correction.** This file previously listed "Gemma 4 E4B
(Apache 2.0, one clause to confirm)". That was wrong on both counts: Google's GGUF
repos carry the **custom Gemma licence**, not Apache-2.0, and they are
**manually gated**. Treat Gemma as un-cleared until someone re-reads that card.

### ⚠️ Why the spike does not use Qwen3.5-4B, despite it being the originally named default

`Qwen/Qwen3.5-4B` is `Qwen3_5ForConditionalGeneration` — a **multimodal** model
with a `vision_config`, and a **hybrid** attention stack: of its 32 layers only
**8 are `full_attention`; the other 24 are `linear_attention`** (Gated DeltaNet,
`full_attention_interval: 4`). Linear-attention layers carry a rolling recurrent
state instead of a per-token KV cache, so the **prefix-KV-reuse** behaviour that
CONVENTIONS §8 is written around — and that spike bar #3 measures — does not
apply to three quarters of the model. Measuring §8's prompt layout on it would
attribute an architectural property to the prompt design.

`Qwen/Qwen3-4B` is `Qwen3ForCausalLM`: **36 uniform full-attention layers**, GQA
32Q/8KV, no `layer_types` key, no vision tower, standard KV cache. That is the
architecture §8 was designed against.

⚠️ Also recorded so it is not re-researched: the originally pinned filename
`Qwen3.5-4B-Instruct-Q4_K_M.gguf` **cannot exist** — Qwen publishes no `-Instruct`
variant of Qwen3.5-4B, and no official GGUF for it at all.

Also cleared and already relevant: llama.cpp (MIT). Whisper / whisper.cpp (MIT)
and Silero VAD (MIT) are **not** v1 scope — there is no voice in v1 — and are
listed only so a later voice pass does not re-research them.
