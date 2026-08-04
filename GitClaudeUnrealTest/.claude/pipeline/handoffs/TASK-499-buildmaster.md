# TASK-499 — [FT-F0] The `/Models/` gitignore negations were DEAD. Struck, not repaired.

- **Agent:** build-master
- **Date:** 2026-08-03
- **Files touched:** `GitClaudeUnrealTest/.gitignore` (this task is its **single owner** — manager ruling 11)
- **Commit:** `<filled in by the follow-up commit — read back from git log, never predicted>`
- **Compile:** ⛔ **none run.** This task owns no compile gate (two exist in this batch and neither is mine). No editor, no MCP, no model touched.
- **Push:** ⛔ **none.** `main` is ahead of origin; the push is Jonathan's.

---

## 1. THE DEFECT, RE-DERIVED FROM THE ARTIFACT (not from the spec that relayed it)

Per the RELAYED-DIAGNOSIS LAW I re-ran the diagnosis rather than accept it. It holds, exactly as boarded:

```
$ git check-ignore -v Models/README.md
GitClaudeUnrealTest/.gitignore:132:/Models/	Models/README.md
```

The rule that matched is the **directory** rule at line 132, not the negation at line 43. Git does not descend into an
ignored directory, so it never sees a `!` for a path inside one ⇒ **`!Models/.gitkeep` and `!Models/README.md`
(lines 42–43) were dead rules.** Corroboration that they were dead in practice, not just in theory:

```
$ git ls-files Models/          # tracked files under Models/
(empty)

$ ls Models/
Qwen3-4B-Q4_K_M.gguf            # 2,497,280,256 B — the weight file, and the ONLY thing in there
```

Neither negated file has ever existed on disk or in the index. **Anything placed in `Models/` intending to be
tracked was being swallowed in silence** — which is the live consequence, independent of whether training happens.

⚠️ **Contributing cause worth naming:** the block sat 13 lines below `Tools/ArtPipeline/Inbox/*` + `!.../.gitkeep`,
which is the `dir/*` form and **does** support negation. The author copied a working pattern into a directory whose
rule was the `/dir/` form. The two forms look interchangeable and are not.

## 2. THE DECISION: **STRIKE THE DEAD RULES.** Not repair the mechanism.

Both options were live. I struck, on three grounds:

1. **Repairing the mechanism means making `Models/` traversable** (`Models/*` + a live `!`). That creates a
   **working re-include surface inside the one directory that holds the 2.5 GB GGUF** — and, after a fine-tune,
   the merged and re-quantised artifacts. CONVENTIONS "THE FINE-TUNE RUNG" §9 already ruled **NO `.gitignore`
   carve-out**, on the grounds that *the rule's stated reason is PERMANENCE, not size*. A working negation is a
   carve-out mechanism sitting one line away from the weights, and the realistic failure mode is a future agent
   adding `!Models/Siegebound-Assistant-*.gguf` **because the negation pattern is right there and looks sanctioned.**
2. **Repairing buys nothing.** The two negated files are the only conceivable beneficiaries and neither is wanted:
   - `.gitkeep` is **unnecessary** — `Tools/fetch_llm_model.py` creates the directory itself
     (`dest_dir.mkdir(parents=True, exist_ok=True)`, lines 193 and 293), so a fresh clone needs no placeholder.
   - `README.md` is **pinned elsewhere by law** — FINE-TUNE RUNG §5 and §9 put the shipping manifest at
     `Docs/ModelManifest.md`, `Docs/`, never `Models/`.
3. **A rule that looks like it works and does not is the defect** (spec item 1). Leaving it "just in case" is the
   thing being fixed.

I also replaced the top block's `Models/*` with `/Models/`. **This changes no matching behaviour** — the bottom
`/Models/` already ignored the directory wholesale — but it removes the **false affordance**: `Models/*` is the form
that *does* support negations, and leaving it there next to a struck negation invites the same mistake again.

### 📌 THE LAW-ADJACENT FACT, RECORDED IN THE FILE ITSELF (spec item 4)

> **MODEL DOCUMENTATION CANNOT LIVE IN `Models/`. IT LIVES IN `Docs/`** — the shipping manifest is pinned at
> **`Docs/ModelManifest.md`** (FINE-TUNE RUNG §5 and §9).

Written into `.gitignore` as a comment beside the rule, because that is where the next person to reach for
`Models/README.md` will actually be standing. It is already law in CONVENTIONS §9; I did **not** edit CONVENTIONS —
see §6 below.

## 3. ⛔ ACCEPTANCE — PASTED `git check-ignore -v` OUTPUT, POST-FIX (§7: acceptance, not assertion)

### ⚠️ READ THE EXIT CODES CORRECTLY — I nearly filed this backwards myself

Two `git check-ignore` behaviours will make a reader mis-score the evidence below, so they are stated before it:

- **`-v` exits 0 when a pattern MATCHED — including a NEGATION pattern.** Exit 0 does **not** mean "ignored".
  **The `!` prefix on the printed rule is the discriminator**, not the exit code.
- **Tracked paths print NOTHING by default.** The vendored `.dll`/`.lib` are already tracked, so a bare
  `git check-ignore -v <dll>` returns empty/exit 1, which reads exactly like "no rule matched". **`--no-index` is
  required** to see the rule. My first pass mislabeled both of these; the raw exit codes are re-derived below.

### (a) one `.gguf` under `Models/` — **MUST report the ignore line**

```
$ git check-ignore -v Models/Qwen3-4B-Q4_K_M.gguf
GitClaudeUnrealTest/.gitignore:151:/Models/	Models/Qwen3-4B-Q4_K_M.gguf     exit=0   IGNORED ✅

$ git check-ignore -v Models/Siegebound-Assistant-Qwen3-4B-r16-Q4_K_M.gguf   # the future artifact, §5's name
GitClaudeUnrealTest/.gitignore:151:/Models/	Models/Siegebound-Assistant-Qwen3-4B-r16-Q4_K_M.gguf   exit=0   IGNORED ✅

$ git check-ignore -v Models/adapter/adapter_model.safetensors               # the 66 MB adapter, §9's case
GitClaudeUnrealTest/.gitignore:151:/Models/	Models/adapter/adapter_model.safetensors               exit=0   IGNORED ✅
```

Same rule, same effect as before the edit — only the line number moved (132 → 151) because comments were added.

### (b) one vendored `.dll` — **MUST still be re-includable (negation intact)**

```
$ git check-ignore -v --no-index Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/bin/Win64/ggml-vulkan.dll
GitClaudeUnrealTest/.gitignore:142:!Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/bin/**/*.dll	Plugins/.../ggml-vulkan.dll
exit=0   → matched rule is a NEGATION (`!`) ⇒ NOT ignored ✅
```

### (c) one vendored `.lib` — same

```
$ git check-ignore -v --no-index Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/lib/Win64/llama.lib
GitClaudeUnrealTest/.gitignore:141:!Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/lib/**/*.lib	Plugins/.../llama.lib
exit=0   → matched rule is a NEGATION (`!`) ⇒ NOT ignored ✅
```

Control, proving the global `*.dll` rule was not weakened — an ordinary `.dll` outside the vendored tree:

```
$ git check-ignore -v --no-index Binaries/Win64/some-random.dll
GitClaudeUnrealTest/.gitignore:102:Binaries/	Binaries/Win64/some-random.dll     exit=0   IGNORED ✅
```

### (d) the path intended to become trackable — **`Docs/ModelManifest.md`**

```
$ git check-ignore -v Docs/ModelManifest.md
(no output)   exit=1   → no rule matches ⇒ TRACKABLE ✅
```

⚖️ **Note what this says about my chosen option:** under the strike, **nothing under `Models/` becomes trackable —
that is the point.** The path that must be trackable is the one law already pinned, and it is, in `Docs/`.

### (e) ⛔ BEHAVIOURAL PROOF — the strongest form, because it tests `git add`, not a matcher

```
$ git add -n Models/Qwen3-4B-Q4_K_M.gguf        # DRY RUN — index untouched
The following paths are ignored by one of your .gitignore files:
GitClaudeUnrealTest/Models
hint: Use -f if you really want to add them.
exit=1   → git REFUSES the 2.5 GB weight ✅

$ git ls-files -o -i --exclude-standard Models/  # ignored-untracked listing
Models/Qwen3-4B-Q4_K_M.gguf                      → present, i.e. seen and excluded ✅

$ git status --porcelain -uall Models/
(empty)                                          → nothing under Models/ is addable ✅

$ git ls-files --error-unmatch <the vendored dll> <the vendored lib>
Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/bin/Win64/ggml-vulkan.dll
Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/lib/Win64/llama.lib
                                                 → both really are TRACKED, negations work in practice ✅
```

### (f) 📌 PER-KIND LFS (§25) — `git check-attr filter`

```
.gitignore                                            : filter: unspecified   ✅ text, plain git — correct kind
Plugins/.../bin/Win64/ggml-vulkan.dll                 : filter: lfs           ✅
Plugins/.../lib/Win64/llama.lib                       : filter: lfs           ✅
Content/Maps/L_Arena.umap                             : filter: lfs           ✅
Docs/ModelManifest.md                                 : filter: unspecified   ✅
```

The only file this task commits is `.gitignore`, and `unspecified` is the correct filter for it — a text rules file
through the `lfs` filter would be the defect.

## 4. BYTE-LEVEL VERIFICATION (GIT HAZARD LAW (a) — `git diff` is blind to line-ending damage)

`core.autocrlf=true` here, so a CRLF→LF round-trip would be invisible to `git diff`. Both edits were **targeted
string edits, not a parse/re-serialise round-trip**, and the result was checked byte-level, not by diff:

| | bytes | CRLF lines | bare LF | sha256 |
|---|---|---|---|---|
| before | 4,557 | 132 | **0** | `3215f40e445751c966e50b41ad36e71b81aa4925b640e54aad523bcfbd97e719` |
| after  | 6,134 | 151 | **0** | `c008d1faaac062fb6ccf5bac6cd37a4a34343e77e353b5ca75b1091cdfce85c5` |

**Bare-LF count is 0 in both** ⇒ no line-ending corruption. +19 lines = 17 comment lines added, 3 rule lines removed,
1 rule line changed (`Models/*` → `/Models/`), matching the diff exactly.

Grep proof that no live negation survives (the only `!Models` string left is inside a `#` comment, which is not a rule):

```
$ grep -n '!Models' .gitignore
44:# `!Models/.gitkeep` and `!Models/README.md`. Git does not descend into an

$ grep -n '^\*\.gguf\|^/Models/\|^Models/' .gitignore
40:*.gguf     41:/Models/     150:*.gguf     151:/Models/
```

## 5. 🔒 UNTOUCHED-ASSET VERIFICATION — `L_Arena`, BY SHA256 (not mtime)

```
sha256  b3dbc5d9ae484a7bd02cafad52b4681da68b011477479b65ee7781ae459f8268   Content/Maps/L_Arena.umap
size    535,522 bytes
```

Matches the expected `B3DBC5D9AE484A7B…` / 535,522 B. **Untouched.** No editor was opened, so it could not have been.

## 6. ⛔ WHAT I DELIBERATELY DID **NOT** COMMIT — inverse-filter result

`git status --porcelain --untracked-files=all` re-read immediately before staging showed **four** entries. Only one
is mine:

| path | in my commit? | why |
|---|---|---|
| `GitClaudeUnrealTest/.gitignore` | ✅ yes | the task |
| `.claude/pipeline/handoffs/TASK-499-buildmaster.md` | ✅ yes | this file |
| `.claude/pipeline/TASKBOARD.md` | ⛔ **NO** | carries **603 uncommitted insertions** — the manager's whole FINE-TUNE decomposition. A `TASK-499:` commit must not claim authorship of it. |
| `.claude/pipeline/CONVENTIONS.md` | ⛔ **NO** | carries **180 uncommitted insertions** — the FINE-TUNE RUNG law. Not my file; `.gitignore` is what TASK-499 owns. |
| `.claude/pipeline/handoffs/TASK-470-buildmaster.md` (untracked) | ⛔ **NO** | another task's handoff. |

⚠️ **Consequence the orchestrator must not lose:** I set TASK-499's board status in the working tree, but **that
edit is NOT committed** — it rides along whenever the manager's decomposition is committed. Staging the board to
capture a one-line status change would have swept 603 lines of someone else's in-flight authorship into this commit.
Explicit pathspec only; no `-a`, no `add -A`, no bare commit. The index was empty before staging and contained
exactly the two intended paths after.

## 7. FOLLOW-UPS FOR THE MANAGER (reported, not actioned)

1. ⚠️ **§7's acceptance standard mandates pasting `git check-ignore -v`, and that output is easy to read exactly
   backwards** — exit 0 means *a pattern matched*, including a negation, and **tracked paths print nothing without
   `--no-index`**. A future agent pasting a bare empty result for a vendored `.dll` would be pasting evidence that
   looks like "no rule protects this" when the truth is the opposite. **Worth one clause in §7: the `!` prefix is
   the discriminator, and `--no-index` is required for tracked paths.** I hit this in this task.
2. 📌 **The `dir/*` vs `/dir/` distinction is the root cause here and it is not written down anywhere.** The repo now
   contains one working example of each (`Tools/ArtPipeline/Inbox/*` + negation; `/Models/` + no negation possible).
   One line of law would prevent the next copy-paste.
3. **`Docs/ModelManifest.md` does not exist yet** — verified trackable, but empty. It is Stage F's deliverable
   (`F-2`); flagged only so nobody assumes TASK-499 created it. **I did not create it** — creating a manifest with
   no artifact to describe would be fabricating provenance.
