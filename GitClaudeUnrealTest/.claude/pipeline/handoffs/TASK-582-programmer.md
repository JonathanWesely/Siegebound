# TASK-582 — `Tools/reimport_meshes.py` encoding fix — PROGRAMMER HANDOFF

**Status:** ready-for-qa · **Gate:** `qa/TASK-584.md` · **Import leg it unblocks:** TASK-566 re-run · **Commit:** TASK-570
**Files touched:** `Tools/reimport_meshes.py` (2 lines, both argument-only additions). Nothing else, anywhere.

---

## 1. The fix — before/after, verbatim

**Spec'd site, `:162` (`_load_manifest()`):**

```python
# BEFORE
        with open(MANIFEST_PATH, "r") as fh:
# AFTER
        with open(MANIFEST_PATH, "r", encoding="utf-8") as fh:
```

**Sweep-found site, `:148` (`_resolve_card_ids()`):**

```python
# BEFORE
        with open(sidecar, "r") as fh:
# AFTER
        with open(sidecar, "r", encoding="utf-8") as fh:
```

Both are the one-argument fix and nothing else: no reformat, no refactor of `_load_manifest()`, `except Exception` fallback untouched (a failed load still returns `{}` and logs via `_err`, exactly as before). `:148` is taken under spec (2) *"Fix any other TEXT-READ that lacks one — same class, same crash"* and the `names` line *"plus any text-READ the sweep finds"*.

**Declared omission (`SC-§15`):** I deliberately added **no explanatory comment** at either site. Spec (1) says *nothing else on that line* and *no refactor of `_load_manifest()`*; a comment is neither, but it is also not the one argument I was sent for. If QA wants the rationale in-file, it is a one-line follow-up — I am flagging the choice rather than making it silently.

---

## 2. The sweep — `open(` in `Tools/reimport_meshes.py`

Search commands and **raw counts**:

```
grep -n  "open(" Tools/reimport_meshes.py     ->  2 hits
grep -c  "open(" Tools/reimport_meshes.py     ->  2      (raw count)
grep -nE "io\.open|codecs\.open|\.read_text|\.write_text|Path\(|pathlib|\bfile\(|\.readlines|\.readline|\.read\(|\.write\(|json\.load|json\.dump|encoding=|shutil|os\.remove|os\.rename" Tools/reimport_meshes.py   ->  2 hits (both non-open, below)
```

| # | Line | Call | Mode | Encoding before | Class | Action |
|---|------|------|------|-----------------|-------|--------|
| 1 | `:148` | `open(sidecar, "r")` | **`r`** (text READ) | ❌ none | text read — same crash class | ✅ **FIXED** → `encoding="utf-8"` |
| 2 | `:162` | `open(MANIFEST_PATH, "r")` | **`r`** (text READ) | ❌ none | text read — the spec'd site | ✅ **FIXED** → `encoding="utf-8"` |

**Non-`open` I/O idioms found (2), both benign:**

| Line | Call | Verdict |
|------|------|---------|
| `:163` | `json.load(fh)` | Consumes the **already-decoded** stream from `:162`. Inherits the encoding; fixed transitively. No change. |
| `:534` | `json.dumps(results)` | Builds a **str for `unreal.log`**, not a file write. Default `ensure_ascii=True` escapes non-ASCII to `\uXXXX`, so `SUMMARY_JSON` stays ASCII-safe regardless. No change. |

### ⭐ The write-mode result — reported as a result

> **There are ZERO write-mode `open(` calls in this file.** Raw count of `open(` is **2**, both `"r"`.

Spec (2)'s stop condition — *"if you find a write without an encoding, that is a FINDING — name it and stop"* — **does not arise.** No write mode was touched, considered, or needed. Every package this script writes goes through `unreal.EditorAssetLibrary.save_loaded_asset` (the engine owns those bytes), never through Python file I/O. Per `SC-§22`, the named site `:162` was a **lower bound**: the sweep found one more read, and that is the whole delta.

---

## 3. ⚠️ FINDING — the ASCII-only premise is already false, and the recorded trigger is the wrong character

I verified the actual bytes rather than trusting the line. Two corrections to the record, both material:

**(a) `pipeline_manifest.json` is NOT pure ASCII and has not been for a long time.**
It carries **251 non-ASCII bytes / 85 characters**: em-dash `U+2014` ×81, `±` `U+00B1` ×2, `§` `U+00A7` ×2. First occurrence at byte offset 790. **Every one lives in a prose field** — `_doc/*`, `_note`, `_verified`, `_dims_source`, `_tuned`, `_variant`, `_ucx_source`. **None in a key. None in `ucx.boxes`.** The collision numbers were never exposed.

**(b) `⛔` would NOT have crashed cp1252. `⭐` would.**
cp1252 has exactly **five** undefined byte positions: `0x81, 0x8D, 0x8F, 0x90, 0x9D`. A UTF-8 sequence crashes the old read **only** if it contains one of those bytes.

| Char | UTF-8 | Old-path verdict |
|------|-------|------------------|
| `⛔` U+26D4 | `e2 9b 94` | **survives** → silent mojibake |
| `⭐` U+2B50 | `e2 ad 90` | 💥 **CRASHES** (`0x90`) |
| `🔍` U+1F50D | `f0 9f 94 8d` | 💥 **CRASHES** (`0x8D`) |
| `✅ ⚠️ 📌 ⚙️ 🚨 ⚖️ 📋 ⇒ — " § ± × • … ▶ 🔒` | — | all **survive** → silent mojibake |

Of 20 characters from this batch's working vocabulary, **2 crash and 18 mojibake silently.** So the standing constraint was real but mis-stated: the danger was never "non-ASCII", it was a **five-byte trapdoor** that `⭐` — one of the most-used glyphs in these very specs — falls straight through, while the `⛔` on record does not.

**TASK-555's rewrite-to-ASCII was still the correct call**, and its both-codecs verification is exactly why the manifest is clean today: I confirmed **0 cp1252-undefined bytes currently present**, so the pre-fix read path was in fact safe — by luck of which glyphs were used, not by the ASCII discipline anyone thought was protecting it. The real historical cost was **silent mojibake** on every commandlet run to date (`—` decoding as `â€"`), harmless only because this script reads `category` / `team_region` / `ucx.boxes` and never the prose fields.

---

## 4. ✅ THE MANDATED ANSWER — may the manifest stop being ASCII-only?

**YES — and the guarantee is broader than this task assumed.** The spec framed the win as *"this file can now read UTF-8"*. I swept every reader of `pipeline_manifest.json` (`SC-§22` — search the claim):

| Reader | Encoding state |
|--------|----------------|
| `Tools/reimport_meshes.py:162` | ❌ was the **only** non-explicit reader → ✅ **fixed here** |
| `Tools/ArtPipeline/refine_trellis_glb.py:213` | ✅ already `open(path, "r", encoding="utf-8")` |
| `Tools/ArtPipeline/rescale_refined_fbx.py:634` | ✅ already `Path(...).read_text(encoding="utf-8")` |
| `Tools/ArtPipeline/build_warroom_props.py` | ✅ comment reference only — **zero file I/O** (verified: 0 hits for `open(`/`read_text`/`json.load`); it declares itself manifest-free at `:15`) |
| `Source/GitClaudeUnrealTest/Siegebound/Castle.cpp:64` | ✅ comment reference only — no runtime read |

⇒ **With this landed, every reader of `pipeline_manifest.json` is encoding-explicit UTF-8. The ASCII-only rule is retired, not merely narrowed to one script.**

⛔ **But do not exercise it yet.** Per spec (3), the hold stands until **TASK-566's re-run confirms the import leg**. This task buys the safety; it does not spend it. Concretely: **keep the manifest ASCII until TASK-566 is green**, then non-ASCII is free.
⚠️ And if anyone reverts or forks a reader, the trapdoor character to fear is **`⭐`**, not `⛔`.

---

## 5. Verification performed (and what I did NOT do)

- ✅ **AST parse** of the edited file — clean (`ast.parse`, which does not execute the `import unreal`, and writes no `__pycache__` into `Tools/`).
- ✅ Byte-level analysis of the manifest under **both** cp1252 and utf-8 — both still decode **and** `json.loads` clean, so the pre-existing both-codecs property TASK-555 established is preserved.
- ✅ Empirical cp1252 undefined-set derivation (all 256 bytes probed) rather than quoting it from memory.
- ⛔ **No compile, no editor launch, no MCP, no PIE, no Git** (not even `git diff` — the before/after above comes from the pre-edit read, so the constraint is respected literally).
- ⛔ **`Tools/ArtPipeline/rescale_refined_fbx.py` NOT run and NOT touched** (non-idempotent; 27× castle).
- ⛔ **`pipeline_manifest.json` NOT modified** — content unchanged; this task changed only how it is *read*.
- ⛔ Stayed inside `Tools/`. No `Source/`, no `Content/`, no `.csv`, no `Tests/`. Zero overlap with TASK-560 or TASK-583.

## 6. 📌 M8 declaration

**Nothing replicated, nothing emitted.** `reimport_meshes.py` is a UE-Python **editor commandlet script**, not a game-module source file — it is not in the build, has no `UCLASS`/`UPROPERTY`/RPC surface, contributes no replicated state, and **cannot affect the compile**. Zero prompt characters; `ZoneA` untouched.

## 7. What QA should scrutinise

1. That `:148` is accepted as in-scope — it is a **text READ**, explicitly admitted by spec (2) and the `names` line, but it is the one site beyond the named line and deserves the ruling.
2. **The `⛔`-vs-`⭐` correction in §3(b)** — it contradicts the hazard as stated in the TASK-582 spec and in TASK-555's note. My claim is byte-level and reproducible; please re-derive rather than take it. If QA concurs, CONVENTIONS' ASCII note should record the **five-byte trapdoor**, not "non-ASCII".
3. **The §4 verdict** that the rule is retired *globally* — it rests on the reader sweep being complete. I searched `.py/.cpp/.h/.cs` for `pipeline_manifest`; a reader that builds the path by string-concatenation would evade that grep.
4. My **declared omission** of an in-file comment (§1).
5. `:148`'s caller `_resolve_card_ids()` has **no `try/except`** — a decode error there propagates out of `main()` and kills the batch before card #1, whereas `_load_manifest()` degrades to `{}`. That asymmetry is **pre-existing and unchanged**; I did not add a guard because that is a refactor I was not sent for. Flagging it as a candidate follow-up, not fixing it.
6. **BOM nuance, reported not acted on:** if `reimport_cards.txt` were ever saved as UTF-8-**with-BOM**, `encoding="utf-8"` yields a leading `﻿` on the first CardID (→ benign SKIP + warn). `utf-8-sig` would absorb it. I did **not** use `utf-8-sig`: it is a widening beyond "add an encoding", and the pre-fix behaviour was equally broken (BOM decoded as `ï»¿`), so this is **not a regression**. QA's call. Current sidecar is confirmed **0 non-ASCII bytes**.
