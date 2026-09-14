# TASK-1266 — [AURA-EXPOSURE-CENSUS] — build-master handoff (2026-09-14)

**Verdict line: `VALUE` = 2 expected + 2 BEYOND the expected set → STOP (🚧), named in 🚨 Blockers, not classified away.**

Read-only census of every credential-shaped hit an on-demand file reader can reach on the project tree, `.gitignore` NOT honoured (`rg --no-ignore --hidden`). No compile, no editor, no engine, no git write, no edit to any file but this one. ⛔ No matched value appears in this file — `VALUE` rows carry path, pattern, match length only.

Law: R15 · `ACC-§11` (2026-09-13 amendment, read on disk, uncommitted) · `PKG-§12` · `SC-§101` · `SC-§102` · `SC-§105`.

---

## 0. The answer first

| # | File (project-root relative) | Pattern | Match length | Class | Justification |
|---|---|---|---|---|---|
| 1 | `Config/SiegeCloudDev.ini` (L14) | (6) `^AnonKey=\S`, also (1) as a 3-segment JWT | 208 chars (segments 36 · 127 · 43) | **VALUE** — EXPECTED | The ruled config home (`ACC-§11`). The only 3-segment-JWT-shaped string on the tree besides its twin. |
| 2 | `Saved/Temp/Win64/GitClaudeUnrealTest/Config/SiegeCloudDev.ini` (L3) | (6), (1) | 208 chars | **VALUE** — EXPECTED TWIN | Staging copy (`handoffs/TASK-1258-programmer.md` L43). Both key LINES hash-identical to the real file's (§3b). |
| 3 | `Saved/Cooked/Windows/ue.projectstore` (L15) | ⚠️ NOT one of the eight — found by a widened (8): `"password"\s*:` (JSON-quoted key) | 32 chars (13 upper, 19 digit, no lower/punct) | **VALUE** — BEYOND EXPECTED → STOP | A real credential: the local Zen server's `hostauth` block (`type: password`, username 2 chars, `islocalhost: true`, port 8558). The same 32-char string occurs in exactly one other place on this machine: `%LOCALAPPDATA%/UnrealEngine/Common/Zen/Install/security-config.json` — i.e. it is the engine-generated per-machine Zen auth secret, written by the 2026-09-09 23:37 ship cook. Not in `Intermediate/`, not in `packagedZIPofGame/` loose files, 0 `projectstore` entries in the shipped zip. Gitignored via `.gitignore:111` (`Saved/`). Not the project's secret — but a credential-shaped VALUE on the tree that any in-editor file reader can open, so it is named, not classified away (R15 / `SC-§101`). |
| 4 | `Saved/Temp/Win64/Engine/Plugins/MetaHuman/MetaHumanSDK/Config/DefaultMetaHumanSDK.ini` (L8) | (8) key `ClientCredentialsSecret` | 43 chars (14 lower, 17 upper, 12 digit) | **VALUE-shaped** — BEYOND EXPECTED → named | Epic-shipped plugin constant: the `ClientCredentialsSecret` LINE is byte-identical (sha256 of the line) to the UE 5.8 install's own `Engine/Plugins/MetaHuman/MetaHumanSDK/Config/DefaultMetaHumanSDK.ini` (the whole files differ only by staging strip: 18 vs 19 lines, 1287 vs 1299 B). Present in every UE 5.8 install and in Epic's public source; a staging copy under `Saved/Temp` (gitignored `.gitignore:111`). Named for the record; recommendation below. |

Everything else on the tree (all eight patterns, both roots) is `PLACEHOLDER` / `PROSE` / `TEST` / base64-image `NOISE` — per-file table in §2.

**Recommendation (a claim for the manager to rule, `SC-§101`, not a classification):** rows 3–4 are engine-owned artefacts under `Saved/` (Zen local-server auth; an Epic-shipped OAuth client constant) — neither is a Siegebound secret, neither ships, neither is tracked. If the manager rules them out of scope, the census closes at exactly the expected pair; if not, both are one `del`/re-stage away. Row 3 argues for a NEW pattern (9) `"(password|passwd|secret|token)"\s*:` in every future census — the row's (8) cannot see a JSON-quoted key (§5, defect 3).

---

## 1. Per-pattern totals (`rg --no-ignore --hidden -c` = matching LINES per file; the `-o` pass counts individual matches where stated)

Project root `GitClaudeUnrealTest/` with the row's exclusions (`.git/`, `Intermediate/`, `DerivedDataCache/`, `Binaries/`, `Saved/Logs/`, `Saved/Crashes/` — anchored at the root, so `Saved/Temp/`, `Saved/Config/`, `Saved/.Aura/`, `Saved/Cooked/`, `Saved/StagedBuilds/`, `.claude/`, `Config/`, `Tools/` (incl. `.venv`), `Docs/`, `Plugins/`, `Source/`, `Content/` are all IN):

| # | Pattern | Hit files | Hit lines | Classes present |
|---|---|---|---|---|
| 1 | `eyJ[A-Za-z0-9_-]{20,}` | 273 | 275 (≈2,404 individual matches) | VALUE ×2 (expected pair) · NOISE ×270 · PROSE ×1 |
| 2 | `sb_secret_` | 3 | 3 | PROSE ×3 |
| 3 | `sb_publishable_` | 5 | 5 | PROSE ×5 (one of them a comment INSIDE the real ini, L21 — §4 lead) |
| 4 | `service_role` | 23 | 43 | PROSE ×22 · PLACEHOLDER ×1 |
| 5 | `hf_[A-Za-z0-9]{20,}` | 0 | 0 | — (no HF token on the tree; `HF_TOKEN` is env-only as ruled) |
| 6 | `^AnonKey=\S` | 3 | 3 | VALUE ×2 (expected pair) · PLACEHOLDER ×1 |
| 7 | `^\s*;?\s*DbPassword=\S` | 1 | 1 | PLACEHOLDER ×1 (`Config/SiegeCloudDev.ini.example` L16 — §4 finding) |
| 8 | `(?i)(password\|passwd\|secret\|token)\s*[:=]\s*\S{8,}` | 129 | 502 | PROSE (third-party `.venv` source) ×81 files/426 lines · TEST ×12 · PROSE (code/law/handoff) ×24 · PLACEHOLDER ×1 · index-copy artefacts ×10 · VALUE-shaped ×1 (row 4 above) |

Strict 3-segment JWT probe `eyJ[A-Za-z0-9_-]{20,}\.[A-Za-z0-9_-]{20,}\.[A-Za-z0-9_-]{20,}` over the same tree: **exactly 2 files** — `Config/SiegeCloudDev.ini` and its `Saved/Temp` twin, 1 line each. Nothing else on the tree is a JWT.

`../packagedZIPofGame/` (git root, loose files only — zips are binary and not searched): **0 files / 0 lines on all eight patterns.**

`Saved/.Aura/indexed_files_aura/` (the Aura index's file copies): 0 files containing `eyJ`; the only `SiegeCloudDev`-named index copy is `…_SiegeCloudDev_ini_example.json` (the placeholder template) — the real ini is NOT in the index, consistent with `TASK-1265`'s index-only exclusion; the R15 finding (`TASK-1259`) is about on-demand READS, which this census cannot fence.

---

## 2. Per-hit-file table

Classes: `VALUE` (a real credential) · `PLACEHOLDER` (`.example` / `<your-key>` shapes) · `PROSE` (law/handoff/doc/code text naming the pattern — code identifiers and expressions are marked `PROSE (code)`) · `TEST` (the suite's scratch buffers) · **`NOISE`** (⚠️ a fifth class this census had to add: single-line base64 PNG captures and PEM certificate bodies where `eyJ` occurs by chance — calling them `PROSE` would be a misstatement; the manager may fold them into whichever class the law prefers).

### 2.1 Pattern (1) `eyJ[A-Za-z0-9_-]{20,}` — 273 files

| File | Lines / matches | Max match len | Class | Evidence |
|---|---|---|---|---|
| `Config/SiegeCloudDev.ini` | 1 / 2 | 127 | **VALUE** (expected) | The AnonKey's header + payload segments (a JWT's payload also begins `eyJ`). |
| `Saved/Temp/Win64/GitClaudeUnrealTest/Config/SiegeCloudDev.ini` | 1 / 2 | 127 | **VALUE** (expected twin) | Same two segments; line hash identical (§3b). |
| `.claude/pipeline/handoffs/TASK-310-gallery.html` | 1 / 3 | 134 | NOISE | 3.6 MB HTML holding 80 `data:image/…;base64,` URIs; strict 3-segment probe = 0. |
| `Tools/ArtPipeline/Cache/_certs/win-ca-bundle.pem` | 1 / 1 | <60 | NOISE | PEM public CA certificate bodies. |
| `Tools/ArtPipeline/Cache/_certs/win-ca-bundle.pem.bak-20260801` | 1 / 1 | <60 | NOISE | Same, backup. |
| `Tools/ArtPipeline/.venv/Lib/site-packages/certifi/cacert.pem` | 1 / 1 | <60 | NOISE | Same (certifi bundle). |
| `Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/_eval_results.py` | 1 / 1 | <60 | PROSE (code) | Library source. |
| 266 files under `Saved/verifycaps/` (108) · `Saved/grounddetail/` (56) · `Saved/treesurvey/` (49) · `Saved/artdiag/` (24) · `Saved/FieldShip/` (15) · `Saved/qa/` (8) · `Saved/mcpcaptures/` (6) | 1 line each / ≈2,390 matches | 298 | NOISE | Measured: **266/266 are single-line files and 266/266 begin with the PNG base64 prefix** — MCP screenshot captures dumped as base64 text; `eyJ` occurs by chance inside image data (match-length histogram: 1,053 hits of 20–39 chars, tapering to 1 of 280–299; a JWT segment is a fixed 36 for the header). Strict 3-segment probe = 0 in all of them. Full path list: Appendix A. |

### 2.2 Pattern (2) `sb_secret_` — 3 files, PROSE ×3

| File:line | Match len (`sb_secret_\S*`) | Class | Evidence |
|---|---|---|---|
| `.claude/pipeline/TASKBOARD.md:3008` | 11 | PROSE | The TASK-1266 row itself (prefix + backtick). |
| `.claude/pipeline/qa/TASK-1250-report.md:48` | inside a 45-char `sb_publishable_…` run | PROSE | The run holds a backtick and 8 other punctuation chars; 45 chars cannot hold two ~26-char key bodies. |
| `.claude/pipeline/handoffs/TASK-1244-programmer.md:72` | inside a 31-char run | PROSE | Same argument (backtick + 3 punctuation). |

### 2.3 Pattern (3) `sb_publishable_` — 5 files, PROSE ×5

| File:line | Match len (`sb_publishable_\S*`) | Class | Evidence |
|---|---|---|---|
| **`Config/SiegeCloudDev.ini:21`** | 19 = prefix + 4 (3 dots + 1 paren) | PROSE (comment) | A comment naming the key FORMAT with an ellipsis. ⚠️ §4 lead: the amended `ACC-§11` allows only "comment lines that name NO credential of any kind" — this names a format, not a key; manager's call. |
| `.claude/pipeline/TASKBOARD.md:3008` | 16 (prefix + backtick) | PROSE | The 1266 row. |
| `.claude/pipeline/qa/TASK-1250-report.md:48` | 45 (10 lower · 2 upper · 4 digit · 2 `_` · 1 backtick · 3 dash · 8 other) | PROSE | Markdown prose; a real key has no backticks/punctuation. |
| `.claude/pipeline/handoffs/TASK-641-buildmaster.md:22` | 19 (prefix + 3 dots + backtick) | PROSE | — |
| `.claude/pipeline/handoffs/TASK-1244-programmer.md:72` | 31 (backtick + 3 other) | PROSE | — |

### 2.4 Pattern (4) `service_role` — 23 files / 43 lines, PROSE ×22 · PLACEHOLDER ×1

No file below contains any `eyJ` string (pattern 1 = 0 in each) ⇒ **no service-role KEY exists anywhere on the tree**; every hit is the word in law/QA/handoff text or the `.example`'s forbidding comment.

`Config/SiegeCloudDev.ini.example` (1, PLACEHOLDER — a comment forbidding it) · `Docs/setupdirections.md` (3) · `Docs/GDD.md` (1) · `.claude/pipeline/CONVENTIONS.md` (2) · `.claude/pipeline/TASKBOARD.md` (13) · `qa/TASK-1263-report.md` (1) · `qa/TASK-1249-report.md` (1) · `qa/TASK-1238-report.md` (1) · `qa/TASK-654.md` (2) · `qa/TASK-648.md` (2) · `qa/TASK-640.md` (3) · `handoffs/TASK-1262-programmer.md` (1) · `handoffs/TASK-1243-programmer.md` (1) · `handoffs/TASK-1228-programmer.md` (1) · `handoffs/TASK-928-buildmaster.md` (1) · `handoffs/TASK-653-programmer.md` (1) · `handoffs/TASK-650-buildmaster.md` (1) · `handoffs/TASK-647-programmer.md` (2) · `handoffs/TASK-646-programmer.md` (1) · `handoffs/TASK-645-programmer.md` (1) · `handoffs/TASK-643-programmer.md` (1) · `handoffs/TASK-641-buildmaster.md` (1) · `handoffs/TASK-639-programmer.md` (1) — all PROSE.

### 2.5 Pattern (5) `hf_[A-Za-z0-9]{20,}` — 0 files.

### 2.6 Pattern (6) `^AnonKey=\S` — 3 files

| File | Value length | Class |
|---|---|---|
| `Config/SiegeCloudDev.ini` (L14, line length 216 = `AnonKey=` + 208) | 208 | **VALUE** (expected) |
| `Saved/Temp/Win64/GitClaudeUnrealTest/Config/SiegeCloudDev.ini` (L3) | 208 | **VALUE** (expected twin) |
| `Config/SiegeCloudDev.ini.example` | 25 | PLACEHOLDER |

### 2.7 Pattern (7) `^\s*;?\s*DbPassword=\S` — 1 file

| File:line | Match len | Class | Evidence |
|---|---|---|---|
| `Config/SiegeCloudDev.ini.example:16` | 22 (= `; DbPassword=` + a 9-char value: punctuation-first, 8 lowercase, 1 other) | PLACEHOLDER | A `<…>`-shaped template. ⚠️ §4 finding: the amended `ACC-§11` QA criterion (`rg --no-ignore -c '^\s*;?\s*DbPassword='` = 0 on BOTH files) FAILS here (= 1) — the tracked template still carries the retired line. |

### 2.8 Pattern (8) broad — 129 files / 502 lines

**(a) `Tools/ArtPipeline/.venv/Lib/site-packages/**` — 81 files / 426 lines — PROSE (code).** Third-party Python library source (`huggingface_hub`, `anyio`, …: `token=`/`secret=` parameters, docstrings, constants). Pattern (5) = 0 and the strict JWT probe = 0 across the venv ⇒ no token value lives there. Longest values ≥ 32 chars all sit in `huggingface_hub` source (`_oidc.py`, `_sandbox.py`, `_login.py`, `_webhooks_server.py`, `constants.py`, `utils/_auth.py`, `utils/_xet.py`, `_eval_results.py`) and `anyio` — library code. Full path list: Appendix B.

**(b) Everything outside `.venv` — 48 files / 76 lines:**

| File (: line) | Key(s) | Value len | Class | Evidence |
|---|---|---|---|---|
| `Config/SiegeCloudDev.ini.example:16` | `DbPassword` | 9 | PLACEHOLDER | As §2.7. |
| `Docs/setupdirections.md:511` | `token` | 14 (identifier-shaped) | PROSE | Setup prose. |
| `Tools/VideoReview/extract_frames.py:400` | `token` | 23 (8 punctuation, 4 digits) | PROSE (code) | An expression, not a literal. |
| `Tools/ArtPipeline/trellis_generate.py` :951 :960 :990 :991 | `token` ×4 | 10 · 11 · 8 · 26 | PROSE (code) | Placeholder-shaped strings and an `os.environ`-style call (26 chars with `()` and dots) — `HF_TOKEN` stays env-only; pattern (5) = 0. |
| `Tools/ArtPipeline/concept_generate.py` :972 :1024 :1098 :1120 | `token` ×4 | 26 · 11 · 15 · 15 | PROSE (code) | Same shapes. |
| `Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/include/llama.h:1507` | `token` | 30 | PROSE (code) | LLM text-token API comment. |
| `Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSubsystem.cpp:1774` | `Token` | 27 (identifier) | PROSE (code) | — |
| `Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSpike.cpp` :3594 :4371 | `Token` · `MsPerToken` | 27 · 19 (identifiers) | PROSE (code) | — |
| `.claude/pipeline/TASKBOARD.md` :34060 :34436 | `hf_token` · `token` | 11 · 9 (quoted) | PROSE | Board text. |
| `.claude/pipeline/handoffs/TASK-086.md:20` | `hf_token` | 22 | PROSE | Pattern (5) = 0 ⇒ not a token. |
| `.claude/pipeline/handoffs/TASK-082.md` :40 :74 | `hf_token` ×2 | 8 · 8 | PROSE | — |
| `.claude/pipeline/qa/TASK-082-report.md:65` | `token` | 8 | PROSE | — |
| `.claude/pipeline/qa/TASK-184-qa.md:21` | `token` | 8 | PROSE | — |
| `.claude/pipeline/qa/TASK-654.md:24` | `CloudRefreshToken` | 15 | PROSE | Names the field. |
| `.claude/pipeline/qa/TASK-1250-report.md:48` | `DbPassword` | 9 (placeholder-shaped) | PROSE | Quotes the `.example`'s placeholder. |
| `.claude/pipeline/qa/TASK-1263-report.md:69` | `token` | 48 (21 lower · 6 upper · 2 digit · 6 dash · 10 other) | PROSE | Identical fingerprint in 1262 / 1249 — the same sentence quoted thrice; 10 punctuation chars ⇒ prose. |
| `.claude/pipeline/qa/TASK-1249-report.md:87` | `token` | 48 (same fingerprint) | PROSE | — |
| `.claude/pipeline/handoffs/TASK-1262-programmer.md:71` | `token` | 48 (same fingerprint) | PROSE | — |
| `.claude/pipeline/handoffs/TASK-1243-programmer.md:92` | `token` | 31 (placeholder-shaped, 8 punctuation) | PROSE | — |
| `.claude/pipeline/handoffs/TASK-1228-programmer.md:53` | `token` | 13 | PROSE | — |
| `.claude/pipeline/handoffs/TASK-644-programmer.md:62` | `token` | 10 | PROSE | — |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCloudTest.cpp` :296 :607 :848 :879 | `DbPassword` · `LinkRefreshToken` ×2 · `RotatedRefreshToken` | 41 · 52 · 52 · 60 | TEST | The row names this file; the 41-char "password" is lowercase words with 5 dashes and a paren — a scratch string. |
| `Source/…/Tests/SiegeControlsHelpTest.cpp` :1285 :1296 | `RealToken` · `BadToken` | 74 · 77 (6 parens) | TEST | Expressions. |
| `Source/…/Tests/SiegeCardHandKeyLabelTest.cpp:211` | `ExpectedAssetToken` | 34 | TEST | — |
| `Source/…/Tests/SiegeAccountTest.cpp` :208 :209 | `Password` · `OtherPassword` | 13 · 13 | TEST | — |
| `Source/…/Tests/SiegeWarMapTest.cpp:3649` · `SiegeLadderClimbTest.cpp` :981 :1890 · `SiegeInvisibilityTest.cpp` :1917 :3548 · `SiegeHeroCameraTest.cpp:517` · `SiegeGhostPawnTest.cpp` :402 :419 · `SiegeFogClampTest.cpp:957` · `SiegeClimbableTowerTest.cpp` :761 :908 :1396 · `SiegeAcquisitionFunnelTest.cpp` :1275 :1367 | `Token` (a UMG/text-token identifier) | 12–27, all identifier-shaped | TEST | — |
| `Source/GitClaudeUnrealTest/Siegebound/AccountMenuWidget.cpp` :82 :261–264 :584 :1429 :1514 | `CloudEnterPassword` · `bIsPassword` ×4 · `Password` · `StoredRefreshToken` · `token` | 11 · 8–9 · 17 · 31 · 10 | PROSE (code) | Property assignments / identifiers; the file is the ruled token-custody seam (`ACC-§11` R1) and holds no literal. |
| `Source/…/SiegeAccountSubsystem.cpp` :268 :382 | `CloudRefreshToken` ×2 | 13 · 10 (identifiers) | PROSE (code) | — |
| `Source/…/SiegeCloudClient.cpp:393` | `AccessToken` | 15 (identifier) | PROSE (code) | — |
| `Source/…/SiegeControlsHelpWidget.cpp:1721` | `Token` | 30 (identifier) | PROSE (code) | — |
| `Saved/.Aura/indexed_files_aura/qq-…_SiegeCloudTest_cpp.json` · `…_SiegeAccountTest_cpp.json` · `…_SiegeCardHandKeyLabelTest_cpp.json` · `…_SiegeHeroCameraTest_cpp.json` · `…_SiegeGhostPawnTest_cpp.json` · `…_SiegeAssistantGrammarTest_cpp.json` · `…_AccountMenuWidget_cpp.json` · `…_SiegeAccountSubsystem_cpp.json` · `…_SiegeCloudClient_cpp.json` | as their sources | JSON-escaped, lengths inflated | TEST / PROSE (code) — index copies | Aura's index holds a JSON copy of each source above; classes follow the source. |
| `Saved/.Aura/indexed_files_aura/qq-…_DefaultEngine_ini.json:1` | `SecurityToken` | 289 (artefact) | PROSE (artefact) | Measured: the real `Config/DefaultEngine.ini` has two `SecurityToken` lines, both with **value length 0** (one is the comment "intentionally left blank"); in the JSON copy the literal `\r\n` escape glues the following lines onto the empty value (`SecurityToken=\r\n` present, count 1). Not a value. |
| **`Saved/Temp/Win64/Engine/Plugins/MetaHuman/MetaHumanSDK/Config/DefaultMetaHumanSDK.ini:8`** | `ClientCredentialsSecret` | 43 | **VALUE-shaped — §0 row 4** | Engine-shipped constant, line-identical to the UE 5.8 install's copy. |

**(c) Found by a WIDENED (8) only — `"(password|passwd|secret|token)"\s*:` (JSON-quoted key) — 1 file:**

| File:line | Key | Value len | Class |
|---|---|---|---|
| **`Saved/Cooked/Windows/ue.projectstore:15`** | `"password"` (inside `zenserver.hostauth`) | 32 | **VALUE — §0 row 3** |

---

## 3. The two specific checks

### (a) `Config/SiegeCloudDev.ini`

| Measurement | Expected | Measured |
|---|---|---|
| `rg --no-ignore -c '^\s*;?\s*DbPassword=\S' Config/SiegeCloudDev.ini` | **0** | **0** ✅ (no password; `PKG-§12`'s 2026-09-09 reading re-confirmed under the amended `ACC-§11`) |
| `rg --no-ignore -c '^(ProjectUrl\|AnonKey)=' Config/SiegeCloudDev.ini` | **2** | **2** ✅ |
| line count (`wc -l`, and `awk END{NR}`) | Aura said 22 | **22** ✅ (1,705 B, mtime 2026-08-23 12:04 — `PKG-§12`'s 1,705 B) |
| ⚠️ the AMENDED `ACC-§11` QA criterion `rg --no-ignore -c '^\s*;?\s*DbPassword='` (any, even empty) | 0 on BOTH files | **1 on the real file** (L15: an EMPTY `; DbPassword=` template line, comment) · **1 on `Config/SiegeCloudDev.ini.example`** (L16, placeholder) — §4 findings 1–2 |

Line-shape fingerprint of the real file (no text): L1–L7 comments (55–79 chars) · L8 `[SiegeCloud]` · L9–L12 comments · **L13 `ProjectUrl=` (line 53 chars, quoted per the 2026-08-23 rider)** · **L14 `AnonKey=` (line 216 chars)** · L15 comment = the empty `; DbPassword=` line · L16–L22 comments (L21 names the `sb_publishable_` format with an ellipsis). No CRLF. Exactly two keys, one section — `PKG-§12`'s void conditions untouched.

### (b) The `Saved/Temp` twin — `Saved/Temp/Win64/GitClaudeUnrealTest/Config/SiegeCloudDev.ini`

- **PRESENT.** 290 B, mtime 2026-08-23 12:04 (same minute as the real file). Gitignored via `.gitignore:111` (`Saved/`).
- **NOT byte-identical:** sha256 real `23bf18535c2103bf7d5937392a026c064e7998cd74c86c5a8fac5b99b1bc9563` (1,705 B) vs twin `347ec09140f4d3f310653ee20deff15825d7646438bb784774433f2ca6211348` (290 B).
- **Value-identical:** the twin is the engine's staging strip — 3 lines (BOM + `[SiegeCloud]`, `ProjectUrl=`, `AnonKey=`), every comment removed. Hash of each key LINE (CR-stripped) real vs twin: `ProjectUrl` line **IDENTICAL** (sha256 prefix `78641ab7fd218478`), `AnonKey` line **IDENTICAL** (`e5e7589bf6b15769`). `rg -c '^(ProjectUrl|AnonKey)='` on the twin = 2; `DbPassword=` = 0.
- **Verdict:** a second, ownerless copy of the SAME credential pair, stripped of comments — a second exposure exactly as the row feared, not stale (same values), and not needed by anything after the stage that wrote it. Lead (`SC-§101`): `Saved/Temp/Win64/**` is engine scratch, regenerated by the next stage — deletable by whoever owns `Saved/` hygiene; manager rules.
- Basename census (`rg --files -g 'SiegeCloudDev*'`, no ignore): exactly 4 — the real file, the `.example`, the twin, and the Aura index copy of the `.example` (`Saved/.Aura/indexed_files_aura/qq-56e91638-…_SiegeCloudDev_ini_example.json`). No copy under `Saved/StagedBuilds/` or `Saved/Cooked/`.

---

## 4. Findings and leads for the manager (none classified away; none acted on — read-only row)

1. **`Config/SiegeCloudDev.ini` L15 is an EMPTY `; DbPassword=` line** — the amended `ACC-§11` says "no `; DbPassword=` even empty (a template for a value is an invitation to fill it)" and its QA criterion counts it (`= 1`, expected 0). The file is Jonathan's, untracked — his edit (or a task with his consent), not an agent's.
2. **`Config/SiegeCloudDev.ini.example` L16 carries `; DbPassword=<placeholder>`** — tracked; fails the same criterion (`= 1`). Programmer task: retire the line (and, per lead 3, review the format-naming comments).
3. **The real ini's comment block names key formats** (L21: `sb_publishable_…`; other comments mention `service_role` on the `.example` side) — the amended rule allows only comments that "name NO credential of any kind". Naming a FORMAT is arguably not naming a credential; the manager rules whether the comment block must go.
4. **Zen `hostauth` password in `Saved/Cooked/Windows/ue.projectstore`** (§0 row 3) — engine-generated, per-machine, localhost:8558, also held in `%LOCALAPPDATA%/UnrealEngine/Common/Zen/Install/security-config.json`. Written by every cook. Not fenceable by `INDEX_IGNORE` from an on-demand reader; a `Saved/Cooked` line in `Docs/AuraIndexIgnore.txt` (if absent) would at least keep it out of the INDEX.
5. **`DefaultMetaHumanSDK.ini` staging copy** (§0 row 4) — Epic-shipped constant; the engine install itself carries the same line, so the tree holds nothing the machine did not already hold.
6. **Pattern gap:** the row's (8) misses JSON-quoted keys (`"password":`). Row 3 was found only by the widened probe. Recommend a ninth standing pattern.
7. **266 base64-PNG capture dumps under `Saved/`** (§2.1) are not credentials but are 100+ MB of reachable text noise for any future census; a `Saved/**/*.txt` purge or an `INDEX_IGNORE` line is a hygiene lead, not a finding.

---

## 5. Instrument record (`SC-§105` — what went wrong and how the recipe compensates)

1. **The first run reported 0 files / 0 lines on ALL EIGHT patterns — a false-clean census.** Cause: in this environment `rg` is a shell FUNCTION (it delegates to `claude.exe`'s bundled ripgrep 14.1.1), not a PATH binary, so a child `bash census.sh` printed `rg: command not found` to stderr and `wc -l` counted an empty file. Only reading the `.err` files exposed it; pattern (6) on a file KNOWN to hold an `AnonKey=` line was the tell. Fix: `export -f rg` before the child shell (or run from the parent shell). **A census that reports zero must first prove its instrument can see a known positive.**
2. `rg -c` counts matching LINES, not matches — the 266 single-line PNG dumps each count as 1 line but hold ~9 `eyJ` matches apiece; both numbers are given above.
3. The row's pattern (8) cannot match a JSON-quoted key (`"password": "…"`) — the quote sits between the word and the colon. Widened probe added (§2.8c).
4. ⚠️ **Self-report:** while fingerprinting `ue.projectstore`, one Python probe printed the nested `hostauth` object as a dict (the value was expected one level up), so the 32-char Zen password appeared ONCE in a tool-output line of this session's transcript. It was not copied anywhere — not into this file, the Slack posts, or the report — and every later probe matched it from a temp file that was deleted afterwards. The credential is the local Zen server's; rotating it is `del %LOCALAPPDATA%\UnrealEngine\Common\Zen\Install\security-config.json` + a Zen restart (it regenerates), if the manager wants the transcript exposure closed.

---

## 6. The exact recipe (re-runnable; `SC-§105`)

From the PROJECT root `C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest` (git root is ONE LEVEL UP, `SC-§102`), Git Bash, ripgrep 14.1.1:

```bash
export -f rg   # rg is a shell function here (delegates to claude.exe's ripgrep) — a child bash will not find it otherwise
EX=(-g '!.git/**' -g '!Intermediate/**' -g '!DerivedDataCache/**' -g '!Binaries/**' -g '!Saved/Logs/**' -g '!Saved/Crashes/**')
# (1)..(8) — output is path:count (matching LINES) per hit file; a pattern with no hit prints nothing and exits 1
rg --no-ignore --hidden -c "${EX[@]}" -e 'eyJ[A-Za-z0-9_-]{20,}' .
rg --no-ignore --hidden -c "${EX[@]}" -e 'sb_secret_' .
rg --no-ignore --hidden -c "${EX[@]}" -e 'sb_publishable_' .
rg --no-ignore --hidden -c "${EX[@]}" -e 'service_role' .
rg --no-ignore --hidden -c "${EX[@]}" -e 'hf_[A-Za-z0-9]{20,}' .
rg --no-ignore --hidden -c "${EX[@]}" -e '^AnonKey=\S' .
rg --no-ignore --hidden -c "${EX[@]}" -e '^\s*;?\s*DbPassword=\S' .
rg --no-ignore --hidden -c "${EX[@]}" -e '(?i)(password|passwd|secret|token)\s*[:=]\s*\S{8,}' .
# the same eight from the git root's packagedZIPofGame/ (loose files; zips are binary and not searched)
( cd ../packagedZIPofGame && for p in 'eyJ[A-Za-z0-9_-]{20,}' 'sb_secret_' 'sb_publishable_' 'service_role' 'hf_[A-Za-z0-9]{20,}' '^AnonKey=\S' '^\s*;?\s*DbPassword=\S' '(?i)(password|passwd|secret|token)\s*[:=]\s*\S{8,}'; do rg --no-ignore --hidden -c "${EX[@]}" -e "$p" . ; done )
# strict JWT shape (three base64url segments)
rg --no-ignore --hidden -c "${EX[@]}" -e 'eyJ[A-Za-z0-9_-]{20,}\.[A-Za-z0-9_-]{20,}\.[A-Za-z0-9_-]{20,}' .
# widened (8) for JSON-quoted keys — the gap that found ue.projectstore
rg --no-ignore --hidden -c "${EX[@]}" -e '(?i)"(password|passwd|secret|token)"\s*:\s*"[^"]{8,}"' .
# check (a)
rg --no-ignore -c '^\s*;?\s*DbPassword=\S' Config/SiegeCloudDev.ini || echo 0
rg --no-ignore -c '^\s*;?\s*DbPassword='   Config/SiegeCloudDev.ini || echo 0     # the amended ACC-§11 criterion (any, even empty)
rg --no-ignore -c '^(ProjectUrl|AnonKey)=' Config/SiegeCloudDev.ini
wc -l < Config/SiegeCloudDev.ini
# check (b)
sha256sum Config/SiegeCloudDev.ini Saved/Temp/Win64/GitClaudeUnrealTest/Config/SiegeCloudDev.ini
for k in ProjectUrl AnonKey; do grep "^$k=" Config/SiegeCloudDev.ini | tr -d '\r' | sha256sum; grep "^$k=" Saved/Temp/Win64/GitClaudeUnrealTest/Config/SiegeCloudDev.ini | tr -d '\r' | sha256sum; done
rg --no-ignore --hidden --files . -g 'SiegeCloudDev*'
# match LENGTHS without printing a value (per hit: path<TAB>len)
rg --no-ignore --hidden -o -n "${EX[@]}" -e '^AnonKey=\S+' . | awk '{ if (match($0,/^[^:]+:[0-9]+:/)) { p=substr($0,1,RLENGTH-1); m=substr($0,RLENGTH+1); print p "\t" length(m)-8 } }'
```

Every `-o` pass in this census went through the same `awk` length reducer; no raw match line was ever printed by the recipe.

---

## 7. `git status --porcelain` (git root) — read-only row

BEFORE:
```
 M GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md
 M GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md
 M GitClaudeUnrealTest/.claude/pipeline/qa/AURA-PHASE0.md
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1230-buildmaster.md
```
AFTER (measured after this file was written):
```
 M GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md
 M GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md
 M GitClaudeUnrealTest/.claude/pipeline/qa/AURA-PHASE0.md
 M GitClaudeUnrealTest/Docs/AuraIndexIgnore.txt
 M GitClaudeUnrealTest/Docs/setupdirections.md
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1230-buildmaster.md
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1266-buildmaster.md
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1267-programmer.md
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1268-report.md
```
Delta vs BEFORE = five lines. **Mine: exactly one** — `?? …/handoffs/TASK-1266-buildmaster.md` (this deliverable, the `??` line `TASK-1269`'s pre-flight expects). **Not mine: four** — ` M Docs/AuraIndexIgnore.txt`, ` M Docs/setupdirections.md`, `?? handoffs/TASK-1267-programmer.md`, `?? qa/TASK-1268-report.md` — the concurrent `TASK-1267` / `TASK-1268` lanes landing while this census ran (both are files those rows own; all four are in `TASK-1269`'s expected pre-flight set). This row changed no tracked file and wrote nothing under the project except this handoff (all scratch lived in the session scratchpad); `Config/SiegeCloudDev.ini` never appeared in `git status`.

## 8. Status request

Row status text requested (the board is not mine to edit this pass): **`🚧 STOP — census complete, VALUE = 2 expected (AnonKey + Saved/Temp twin) + 2 beyond (Zen hostauth password in Saved/Cooked/Windows/ue.projectstore; Epic-shipped MetaHuman ClientCredentialsSecret in Saved/Temp) — manager rules before TASK-1269 commits; handoffs/TASK-1266-buildmaster.md`**.

---

## Appendix A — pattern (1) NOISE: the 266 single-line base64-PNG capture dumps under `Saved/` (path:matching-lines)

```
Saved/FieldShip/pie_editor_screen.txt:1
Saved/FieldShip/pie_lane_0.txt:1
Saved/FieldShip/pie_lane_3.txt:1
Saved/FieldShip/sim_hero_0.txt:1
Saved/FieldShip/sim_hero_1.txt:1
Saved/FieldShip/sim_hero_2.txt:1
Saved/FieldShip/sim_hero_3.txt:1
Saved/FieldShip/sim_hero_4.txt:1
Saved/FieldShip/sim_hero_5.txt:1
Saved/FieldShip/sim_lane_0.txt:1
Saved/FieldShip/sim_lane_1.txt:1
Saved/FieldShip/sim_lane_2.txt:1
Saved/FieldShip/sim_lane_3.txt:1
Saved/FieldShip/sim_lane_4.txt:1
Saved/FieldShip/sim_lane_5.txt:1
Saved/artdiag/p3_before_a.txt:1
Saved/artdiag/p3_before_b.txt:1
Saved/artdiag/p3_cam1200.txt:1
Saved/artdiag/p3_cam3600.txt:1
Saved/artdiag/p3_decalctrl_after.txt:1
Saved/artdiag/p3_out_after_a.txt:1
Saved/artdiag/p3_out_after_b.txt:1
Saved/artdiag/p3_out_before_a.txt:1
Saved/artdiag/p3_out_before_b.txt:1
Saved/artdiag/p3_out_before_c.txt:1
Saved/artdiag/p3_out_before_d.txt:1
Saved/artdiag/p3_plate_out.txt:1
Saved/artdiag/p3_probe.txt:1
Saved/artdiag/p3_yaw0.txt:1
Saved/artdiag/p3_yaw270.txt:1
Saved/artdiag/p3_yaw90.txt:1
Saved/artdiag/sk_footman.txt:1
Saved/artdiag/sk_knight.txt:1
Saved/artdiag/sk_miner.txt:1
Saved/artdiag/sm_knight.txt:1
Saved/artdiag/sm_militiamob.txt:1
Saved/artdiag/tex_cleric_orm.txt:1
Saved/artdiag/tex_footman_d.txt:1
Saved/artdiag/tex_longbowman_orm.txt:1
Saved/grounddetail/after-hero-0.txt:1
Saved/grounddetail/after-hero-1.txt:1
Saved/grounddetail/after-hero-2.txt:1
Saved/grounddetail/after-lane-0.txt:1
Saved/grounddetail/after-lane-1.txt:1
Saved/grounddetail/after-lane-2.txt:1
Saved/grounddetail/after-mid-0.txt:1
Saved/grounddetail/after-mid-1.txt:1
Saved/grounddetail/aim-grass1-3m.txt:1
Saved/grounddetail/aim-grass1-8m.txt:1
Saved/grounddetail/aim-hill02-30m.txt:1
Saved/grounddetail/aim-plant15-3m.txt:1
Saved/grounddetail/aim-rock1-30m.txt:1
Saved/grounddetail/aim-slab31-30m.txt:1
Saved/grounddetail/aim-tree9-30m.txt:1
Saved/grounddetail/before-gamecam-0.txt:1
Saved/grounddetail/before-gamecam-1.txt:1
Saved/grounddetail/before-hero-0.txt:1
Saved/grounddetail/before-hero-1.txt:1
Saved/grounddetail/before-hero-2.txt:1
Saved/grounddetail/before-lane-0.txt:1
Saved/grounddetail/before-lane-1.txt:1
Saved/grounddetail/before-lane-2.txt:1
Saved/grounddetail/before-lane-3.txt:1
Saved/grounddetail/probe-mid120-0.txt:1
Saved/grounddetail/probe-mid1400-0.txt:1
Saved/grounddetail/probe-mid260-0.txt:1
Saved/grounddetail/probe-mid260-1.txt:1
Saved/grounddetail/probe-side-0.txt:1
Saved/grounddetail/sim-hero-0.txt:1
Saved/grounddetail/sim-hero-1.txt:1
Saved/grounddetail/sim-hero-2.txt:1
Saved/grounddetail/sim-lane-0.txt:1
Saved/grounddetail/sim-lane-1.txt:1
Saved/grounddetail/sim-lane-2.txt:1
Saved/grounddetail/sim-mid-0.txt:1
Saved/grounddetail/sim-mid-1.txt:1
Saved/grounddetail/sim-null.txt:1
Saved/grounddetail/thumb-sm_plant_6.txt:1
Saved/grounddetail/world-editorimage.txt:1
Saved/grounddetail/world-heroaim-viewport.txt:1
Saved/grounddetail/world-nopie-lane.txt:1
Saved/grounddetail/world-nopie-mid260.txt:1
Saved/grounddetail/world-piefresh-heroeye.txt:1
Saved/grounddetail/world-piefresh-mid260.txt:1
Saved/grounddetail/world-piefresh-screen-0.txt:1
Saved/grounddetail/world-piefresh-screen-1.txt:1
Saved/grounddetail/world-piefresh-screen-2.txt:1
Saved/grounddetail/x1-control-mid260.txt:1
Saved/grounddetail/x1-cull0-mid260.txt:1
Saved/grounddetail/x2-hillmat-high1400.txt:1
Saved/grounddetail/x2-hillmat-mid260.txt:1
Saved/grounddetail/x3a-rocks-hillmat-lane.txt:1
Saved/grounddetail/x3b-grass-cull0-hill-mid260.txt:1
Saved/grounddetail/x3c-grass-cull0-only-mid260.txt:1
Saved/grounddetail/x3d-grass-restored-mid260.txt:1
Saved/mcpcaptures/battlecry.b64.txt:1
Saved/mcpcaptures/battlecry_long.b64.txt:1
Saved/mcpcaptures/battlecry_small.b64.txt:1
Saved/mcpcaptures/chainzap_0.b64.txt:1
Saved/mcpcaptures/chainzap_1.b64.txt:1
Saved/mcpcaptures/lightning_0.b64.txt:1
Saved/qa/frame_idle.txt:1
Saved/qa/frame_swing_1.txt:1
Saved/qa/frame_swing_3.txt:1
Saved/qa/frame_swing_5.txt:1
Saved/qa/frame_swing_8.txt:1
Saved/qa/shake_0_b.txt:1
Saved/qa/shake_1_b.txt:1
Saved/qa/victory_screen.txt:1
Saved/treesurvey/branch_norway_spruce_01.txt:1
Saved/treesurvey/branch_norway_spruce_dead_01.txt:1
Saved/treesurvey/branch_norway_spruce_hanging_01.txt:1
Saved/treesurvey/branch_norway_spruce_top_01.txt:1
Saved/treesurvey/d1_pie_after_0.txt:1
Saved/treesurvey/d1_pie_after_1.txt:1
Saved/treesurvey/d1_pie_after_2.txt:1
Saved/treesurvey/d1_pie_after_3.txt:1
Saved/treesurvey/d1_pie_after_4.txt:1
Saved/treesurvey/d1_pie_after_5.txt:1
Saved/treesurvey/d1_pie_before_0.txt:1
Saved/treesurvey/d1_pie_before_1.txt:1
Saved/treesurvey/d1_pie_before_2.txt:1
Saved/treesurvey/d1_pie_before_3.txt:1
Saved/treesurvey/d1_pie_before_4.txt:1
Saved/treesurvey/d1_pie_before_5.txt:1
Saved/treesurvey/sm-mobile_tree_1.txt:1
Saved/treesurvey/sm-mobile_tree_11.txt:1
Saved/treesurvey/sm-mobile_tree_7.txt:1
Saved/treesurvey/sm-mobile_tree_8.txt:1
Saved/treesurvey/sm_highpoly_tree_11.txt:1
Saved/treesurvey/sm_highpoly_tree_8.txt:1
Saved/treesurvey/t1085_hero_0.txt:1
Saved/treesurvey/t1085_hero_1.txt:1
Saved/treesurvey/t1085_hero_2.txt:1
Saved/treesurvey/t1085_hero_3.txt:1
Saved/treesurvey/t1085_hero_4.txt:1
Saved/treesurvey/t1085_lane_0.txt:1
Saved/treesurvey/t1085_lane_1.txt:1
Saved/treesurvey/t1085_lane_2.txt:1
Saved/treesurvey/t1085_lane_3.txt:1
Saved/treesurvey/t1085_lane_4.txt:1
Saved/treesurvey/t1099_ann_0.txt:1
Saved/treesurvey/t1099_auto_0.txt:1
Saved/treesurvey/t1099_auto_1.txt:1
Saved/treesurvey/t1099_auto_2.txt:1
Saved/treesurvey/t1099_auto_3.txt:1
Saved/treesurvey/t1099_auto_4.txt:1
Saved/treesurvey/t1099_flod0_0.txt:1
Saved/treesurvey/t1099_flod0_1.txt:1
Saved/treesurvey/t1099_flod0_2.txt:1
Saved/treesurvey/t1099_flod0_3.txt:1
Saved/treesurvey/t1099_flod0_4.txt:1
Saved/treesurvey/t1099_fol0_0.txt:1
Saved/treesurvey/t1099_fol1_0.txt:1
Saved/treesurvey/t1099_fol4_0.txt:1
Saved/treesurvey/t1099_r0_0.txt:1
Saved/treesurvey/tex_t_norway_spruce_foliage_01_ca.txt:1
Saved/treesurvey/tree_japanese_cypress_01_b.txt:1
Saved/verifycaps/cand_a.txt:1
Saved/verifycaps/cand_b.txt:1
Saved/verifycaps/cand_c.txt:1
Saved/verifycaps/cand_d.txt:1
Saved/verifycaps/cap_archer_feet.txt:1
Saved/verifycaps/cap_archer_threequarter.txt:1
Saved/verifycaps/cap_archer_tight.txt:1
Saved/verifycaps/cap_archer_wide.txt:1
Saved/verifycaps/cap_cavalry_feet.txt:1
Saved/verifycaps/cap_cavalry_gait.txt:1
Saved/verifycaps/cap_cavalry_threequarter.txt:1
Saved/verifycaps/cap_cavalry_tight.txt:1
Saved/verifycaps/cap_cavalry_wide.txt:1
Saved/verifycaps/cap_cleric_feet.txt:1
Saved/verifycaps/cap_cleric_threequarter.txt:1
Saved/verifycaps/cap_cleric_tight.txt:1
Saved/verifycaps/cap_cleric_wide.txt:1
Saved/verifycaps/cap_footman_feet.txt:1
Saved/verifycaps/cap_footman_threequarter.txt:1
Saved/verifycaps/cap_footman_tight.txt:1
Saved/verifycaps/cap_footman_wide.txt:1
Saved/verifycaps/cap_knight_feet.txt:1
Saved/verifycaps/cap_knight_threequarter.txt:1
Saved/verifycaps/cap_knight_tight.txt:1
Saved/verifycaps/cap_knight_wide.txt:1
Saved/verifycaps/cap_longbowman_feet.txt:1
Saved/verifycaps/cap_longbowman_threequarter.txt:1
Saved/verifycaps/cap_longbowman_tight.txt:1
Saved/verifycaps/cap_longbowman_wide.txt:1
Saved/verifycaps/cap_militiamob_feet.txt:1
Saved/verifycaps/cap_militiamob_threequarter.txt:1
Saved/verifycaps/cap_militiamob_tight.txt:1
Saved/verifycaps/cap_militiamob_wide.txt:1
Saved/verifycaps/cap_miner_feet.txt:1
Saved/verifycaps/cap_miner_threequarter.txt:1
Saved/verifycaps/cap_miner_tight.txt:1
Saved/verifycaps/cap_miner_wide.txt:1
Saved/verifycaps/cap_ogre_feet.txt:1
Saved/verifycaps/cap_ogre_threequarter.txt:1
Saved/verifycaps/cap_ogre_tight.txt:1
Saved/verifycaps/cap_ogre_wide.txt:1
Saved/verifycaps/cap_pikeman_feet.txt:1
Saved/verifycaps/cap_pikeman_rank.txt:1
Saved/verifycaps/cap_pikeman_threequarter.txt:1
Saved/verifycaps/cap_pikeman_tight.txt:1
Saved/verifycaps/cap_pikeman_wide.txt:1
Saved/verifycaps/cap_sapper_feet.txt:1
Saved/verifycaps/cap_sapper_threequarter.txt:1
Saved/verifycaps/cap_sapper_tight.txt:1
Saved/verifycaps/cap_sapper_wide.txt:1
Saved/verifycaps/cav_side_a135.txt:1
Saved/verifycaps/cav_side_b160.txt:1
Saved/verifycaps/probe_archer.txt:1
Saved/verifycaps/probe_knight.txt:1
Saved/verifycaps/probe_ogre.txt:1
Saved/verifycaps/probe_sapper.txt:1
Saved/verifycaps/scout_ground.txt:1
Saved/verifycaps/scout_iso.txt:1
Saved/verifycaps/scout_top.txt:1
Saved/verifycaps/sweep_archer_0.txt:1
Saved/verifycaps/sweep_archer_180.txt:1
Saved/verifycaps/sweep_archer_270.txt:1
Saved/verifycaps/sweep_archer_90.txt:1
Saved/verifycaps/sweep_cavalry_0.txt:1
Saved/verifycaps/sweep_cavalry_180.txt:1
Saved/verifycaps/sweep_cavalry_270.txt:1
Saved/verifycaps/sweep_cavalry_90.txt:1
Saved/verifycaps/sweep_cleric_0.txt:1
Saved/verifycaps/sweep_cleric_180.txt:1
Saved/verifycaps/sweep_cleric_270.txt:1
Saved/verifycaps/sweep_cleric_90.txt:1
Saved/verifycaps/sweep_footman_0.txt:1
Saved/verifycaps/sweep_footman_180.txt:1
Saved/verifycaps/sweep_footman_270.txt:1
Saved/verifycaps/sweep_footman_90.txt:1
Saved/verifycaps/sweep_knight_0.txt:1
Saved/verifycaps/sweep_knight_180.txt:1
Saved/verifycaps/sweep_knight_270.txt:1
Saved/verifycaps/sweep_knight_90.txt:1
Saved/verifycaps/sweep_longbowman_0.txt:1
Saved/verifycaps/sweep_longbowman_180.txt:1
Saved/verifycaps/sweep_longbowman_270.txt:1
Saved/verifycaps/sweep_longbowman_90.txt:1
Saved/verifycaps/sweep_militiamob_0.txt:1
Saved/verifycaps/sweep_militiamob_180.txt:1
Saved/verifycaps/sweep_militiamob_270.txt:1
Saved/verifycaps/sweep_militiamob_90.txt:1
Saved/verifycaps/sweep_miner_0.txt:1
Saved/verifycaps/sweep_miner_180.txt:1
Saved/verifycaps/sweep_miner_270.txt:1
Saved/verifycaps/sweep_miner_90.txt:1
Saved/verifycaps/sweep_ogre_0.txt:1
Saved/verifycaps/sweep_ogre_180.txt:1
Saved/verifycaps/sweep_ogre_270.txt:1
Saved/verifycaps/sweep_ogre_90.txt:1
Saved/verifycaps/sweep_pikeman_0.txt:1
Saved/verifycaps/sweep_pikeman_180.txt:1
Saved/verifycaps/sweep_pikeman_270.txt:1
Saved/verifycaps/sweep_pikeman_90.txt:1
Saved/verifycaps/sweep_sapper_0.txt:1
Saved/verifycaps/sweep_sapper_180.txt:1
Saved/verifycaps/sweep_sapper_270.txt:1
Saved/verifycaps/sweep_sapper_90.txt:1
Saved/verifycaps/t2_opposite.txt:1
Saved/verifycaps/t2_sunfront.txt:1
Saved/verifycaps/test_3q.txt:1
Saved/verifycaps/test_negx.txt:1
Saved/verifycaps/test_posx.txt:1
```

## Appendix B — pattern (8) PROSE (code): the 81 third-party files under `Tools/ArtPipeline/.venv/` (path:matching-lines)

```
Tools/ArtPipeline/.venv/Lib/site-packages/PIL/PdfParser.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/PIL/PpmImagePlugin.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/anyio/_backends/_trio.py:5
Tools/ArtPipeline/.venv/Lib/site-packages/anyio/_core/_eventloop.py:2
Tools/ArtPipeline/.venv/Lib/site-packages/anyio/from_thread.py:11
Tools/ArtPipeline/.venv/Lib/site-packages/anyio/lowlevel.py:4
Tools/ArtPipeline/.venv/Lib/site-packages/anyio/pytest_plugin.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/filelock/_soft_rw/_sync.py:3
Tools/ArtPipeline/.venv/Lib/site-packages/fsspec/fuse.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/fsspec/implementations/jupyter.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/fsspec/implementations/smb.py:2
Tools/ArtPipeline/.venv/Lib/site-packages/fsspec/implementations/webhdfs.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/fsspec/json.py:2
Tools/ArtPipeline/.venv/Lib/site-packages/fsspec/spec.py:3
Tools/ArtPipeline/.venv/Lib/site-packages/gradio_client/client.py:6
Tools/ArtPipeline/.venv/Lib/site-packages/gradio_client/templates/discord_chat.py:3
Tools/ArtPipeline/.venv/Lib/site-packages/h11/_abnf.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/httpcore/_async/http_proxy.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/httpcore/_async/socks_proxy.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/httpcore/_models.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/httpcore/_sync/http_proxy.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/httpcore/_sync/socks_proxy.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/httpx/_auth.py:4
Tools/ArtPipeline/.venv/Lib/site-packages/httpx/_client.py:3
Tools/ArtPipeline/.venv/Lib/site-packages/httpx/_main.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/httpx/_urlparse.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub-1.22.0.dist-info/METADATA:1
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/_dataset_viewer.py:2
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/_eval_results.py:2
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/_inference_endpoints.py:10
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/_login.py:10
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/_oauth.py:4
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/_oidc.py:3
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/_sandbox.py:19
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/_upload_pipeline.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/_webhooks_server.py:5
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/cli/_cp.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/cli/auth.py:3
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/cli/buckets.py:7
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/cli/cache.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/cli/collections.py:8
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/cli/datasets.py:6
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/cli/discussions.py:10
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/cli/download.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/cli/inference_endpoints.py:10
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/cli/jobs.py:19
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/cli/models.py:3
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/cli/papers.py:4
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/cli/repo_files.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/cli/repos.py:12
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/cli/sandbox.py:14
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/cli/skills.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/cli/spaces.py:22
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/cli/upload.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/cli/upload_large_folder.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/cli/webhooks.py:9
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/constants.py:2
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/errors.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/hf_api.py:52
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/hf_file_system.py:5
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/hub_mixin.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/inference/_client.py:15
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/inference/_generated/_async_client.py:15
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/inference/_generated/types/text_generation.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/inference/_providers/_common.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/repocard.py:2
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/repocard_data.py:2
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/utils/_auth.py:13
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/utils/_git_credential.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/utils/_headers.py:3
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/utils/_runtime.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/huggingface_hub/utils/_xet.py:2
Tools/ArtPipeline/.venv/Lib/site-packages/numpy/_core/_ufunc_config.py:2
Tools/ArtPipeline/.venv/Lib/site-packages/numpy/_core/arrayprint.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/packaging/_parser.py:3
Tools/ArtPipeline/.venv/Lib/site-packages/packaging/_tokenizer.py:2
Tools/ArtPipeline/.venv/Lib/site-packages/packaging/licenses/__init__.py:1
Tools/ArtPipeline/.venv/Lib/site-packages/tqdm/contrib/discord.py:2
Tools/ArtPipeline/.venv/Lib/site-packages/tqdm/contrib/slack.py:2
Tools/ArtPipeline/.venv/Lib/site-packages/tqdm/contrib/telegram.py:2
Tools/ArtPipeline/.venv/Lib/site-packages/yaml/parser.py:41
```
