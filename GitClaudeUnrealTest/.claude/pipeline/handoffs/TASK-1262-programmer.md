# TASK-1262 — [AURA-SETUP-DOCS-D-2] gameplay-programmer handoff (2026-09-13)

**Status: ready-for-qa (gate `TASK-1263`; host `TASK-1242` re-measures the hash pair). ⛔ Not committed, not compiled, no git write operations (git was used READ-ONLY: `rev-parse`, `diff`, `status`). The vault twin is outside the repo and is never staged.**

Ruling R10 + `qa/TASK-1249-report.md` WARN-1. Law: `VER-§7` cl. 1 + cl. 3 (as amended 2026-09-13) · R2 · `SC-§68` · `SC-§100`.

## Acceptance (1) — FIRST, the identical-hash line (`SC-§68`)
```
sha256sum Docs/setupdirections.md  C:\GitProjects\GitHub\MyObsidianVault\JonWesOBVault\GitClaudeUnrealsetupdirections.md
ae48dc8ef6cb769efebcb77677933a5f068c47edd01035ee557748ded731e425  Docs/setupdirections.md
ae48dc8ef6cb769efebcb77677933a5f068c47edd01035ee557748ded731e425  .../JonWesOBVault/GitClaudeUnrealsetupdirections.md
cmp -> IDENTICAL
```
Both **73,427 bytes · 1,179 lines** (was 69,979 / 1,126 at `208edbec…80f58`). Twin re-measured BEFORE the edit: both copies `208edbec1ba664a0e8e36c9b81786c0726286b8bb2bf97acef11086be8880f58`, 69,979 B — the twin started clean. 0 CR bytes, no BOM, LF; tail bytes `…65 2e 0a` (`surface.\n`, same EOF shape as before — the WARN-2 blank line was NOT hand-restored). Write once (repo file), then `cp` to the vault path.

## Acceptance (2) — the wildcard lines + the counts, quoted
D.1 fenced `jsonc` block = lines **1011–1148** (opening fence `1011:```jsonc`, closing `1148`).
```
sed -n '1011,1148p' Docs/setupdirections.md | grep -c 'unreal_inspector__\*'  -> 0
sed -n '1011,1148p' Docs/setupdirections.md | grep -c 'unreal_editor__\*'     -> 0
grep -c '"mcp__unreal_inspector__' Docs/setupdirections.md                    -> 49
grep -c '"mcp__unreal_editor__'    Docs/setupdirections.md                    -> 32
```
Whole-file hits of either wildcard token are exactly three PROSE sites, none inside the block: `:895` (§11.5's new ⛔ NEVER-wholesale bullet — the token is the subject of the ban), `:900` (§11.5's pre-existing editor ban), `:976` (the Appendix B namespace reference QA-1238 already ruled).

## Acceptance (3) — the 49 names ↔ `handoffs/TASK-1254-buildmaster.md`
The 49 were EXTRACTED by regex from the handoff's fenced block under `## The 49 granted …` (never typed); the 13 from the `## The 13 EXCLUDED` block. Asserted in-script before the write: `len == 49` · `len == 13` · handoff block already sorted · disjoint · no wildcard in the list. Asserted after the write over the D.1 block: the 49 `"mcp__unreal_inspector__<name>"` entries `== granted` (same strings, same order) → **`missing = []`**; intersection with the 13 → **`forbidden = []`**. Whole-file alternation grep of the 13 excluded names → **0**. `recompile_unreal_project` appears ONCE in the file, at `:896`, as prose inside the §11.5 ban (not a grant). Comment-stripped `json.loads` of the block succeeds: `permissions.allow` = 108 entries (27 pre-existing + 49 + 32), `enabledMcpjsonServers = ["unreal-mcp","blender","unreal_inspector","unreal_editor"]` unchanged.

## Acceptance (4) — `grep -n -i 'read-only'` BEFORE → AFTER, in full
BEFORE (8 hits): `242` `334` `883` `893` `1021` `1026` `1051` `1053` `1120`.
AFTER (4 hits):
```
242:| **qa-reviewer** | … | Read-only + report Write + Slack |     ← QA's POSTURE (agent table), not the server — left
334:in CONVENTIONS before the task is issued; QA is read-only; …   ← QA's POSTURE (spec fence), not the server — left
1025:      // Read-only probes (zero risk, highest frequency)         ← PowerShell probe rules, not the server — left
1030:      // Read-only git ONLY — enumerated deliberately. …          ← git rules, not the server — left
```
The four inspector sites are gone: `:883` (→ `:883–885`), `:893` (→ `:895–897`), `:1051/:1053` (→ `:1055–1062`), `:1120` (→ `:1173`). `grep -n -i 'wholesale'` AFTER → `885`, `895`, `1173` — every hit is now the phrase **never wholesale / NEVER allowed wholesale**; none describes the grant as wholesale.

## The four sites — before → after (one line each)
- **§11.4 step 3 (`:883`)** — *`unreal_inspector` is **read-only**;* → *`unreal_inspector` is MOSTLY read tools — the census found 13 that are not (engine lifecycle incl. a recompile path, generation, plan bookkeeping), so it is granted by NAME, 49 tools, never wholesale (`VER-§7` cl. 3, R10);* (re-wrapped to the file's ~92-col style; `unreal_editor`'s sentence unchanged).
- **§11.5 first bullet (`:893`)** — *`mcp__unreal_inspector__*` may be allowed **wholesale** — it is read-only.* → *`mcp__unreal_inspector__*` is ⛔ **NEVER allowed wholesale** (R10, 2026-09-13 — the server carries `recompile_unreal_project` and engine-lifecycle tools); the 49 read names are enumerated in Appendix D.1.*
- **D.1 comment (`:1051`–`:1057`) + entry (`:1058`)** — *the read-only / enumerated split … unreal_inspector = read-only → allowed WHOLESALE … `"mcp__unreal_inspector__*"`* → *both servers ENUMERATED (VER-§7 cl. 1 + cl. 3, R10): unreal_inspector = its 49 read tools (census §2 minus §2a), as TASK-1254 granted them …; unreal_editor = the 32 PIE / verify / screenshot / input names, as TASK-1222 granted them …; ⛔ Never a wildcard of either.* + the 49 entries, one per line, sorted, same 6-space indent, directly above the 32 editor entries.
- **D.4 bullet (`:1120`)** — *`unreal_inspector` is **read-only** and allowed wholesale,* → *`unreal_inspector` is allowed only by its **49 enumerated read tools** (never wholesale — the census found 13 lifecycle / generation / plan names inside it, excluded per R10; the list is in D.1),* — `unreal_editor`'s description in the same bullet unchanged.

## Acceptance (5) — hunk list (`SC-§102`: git run from the git root `C:/GitProjects/GitHub/GitClaudeUnrealTesting`, HEAD = `9f2c990`)
**MY hunks only** (the pre-edit file was reconstructed in the scratchpad by reversing the four replacements — its sha256 = `208edbec…80f58` / 69,979 B, i.e. exactly the TASK-1243 state — and `git diff --no-index -U0` run against the working file):
```
@@ -883    +883,3    @@   §11.4 step 3 — the read-only sentence (1 → 3 lines)
@@ -893    +895,3    @@   §11.5 first bullet (1 → 3 lines)
@@ -1051,8 +1055,57 @@   D.1 — 7-line comment + wildcard entry → 8-line comment + 49 names (8 → 57 lines)
@@ -1120   +1173     @@   D.4 — the "Local MCP servers" bullet (1 → 1 line)
```
64 insertions / 11 deletions, net **+53** (1,126 → 1,179 — reconciles). Four hunks, all inside §11.4–§11.5 and Appendix D.1 / D.4. ⛔ No other chapter, appendix, table or list row.

**CUMULATIVE vs HEAD `9f2c990`** (TASK-1243's still-uncommitted three hunks + mine, which share the D.1 / D.4 regions and so merge):
```
git diff --stat 9f2c990 -- GitClaudeUnrealTest/Docs/setupdirections.md
 GitClaudeUnrealTest/Docs/setupdirections.md | 104 ++++++++++++++++++++++++++--   (99 insertions, 5 deletions)
git diff -U0 9f2c990 -- … | grep '^@@'
@@ -883  +883,3   @@   (1262)
@@ -893  +895,3   @@   (1262)
@@ -1049 +1053,91 @@   (1243's D.1 hunk merged with 1262's D.1 hunk — adjacent lines)
@@ -1052 +1146    @@   (1243: enabledMcpjsonServers)
@@ -1079 +1173    @@   (1243's D.4 bullet, re-edited by 1262 — same line)
hunk count 5 · 'No newline' markers 0
```
📌 **For the host (`TASK-1242`), re QA-1249 WARN-2:** the 5 deletions are exactly the five changed lines above (883 · 893 · 1049 · 1052 · 1079) and there is NO tail hunk and no `\ No newline at end of file` marker — so TASK-1243's `44/3` was the true reading and the "dropped EOF blank line" inference does not hold against `git diff`; the file at HEAD already ended `surface.\n`. WARN-2 dissolves on measurement. Nothing was restored by hand.

## Acceptance (6) — no secrets
Case-insensitive grep of MY added lines for `service_role|api[_ ]key|token=|sbp_|eyJ|hf_[A-Za-z]|ghp_|sk-[A-Za-z0-9]|Bearer |access_token` → **4 hits, ALL the same false positive** as QA-1249 §(7): `sk-[A-Za-z0-9]` matching the `SK-1` inside `TASK-1254` / `TASK-1222` on the four comment lines that cite the handoffs. Case-sensitive → **0**. The additions are tool-name strings, prose, and two handoff-path cites.

## What QA (`TASK-1263`) should scrutinize
- The 49 in D.1 vs the `TASK-1254` fenced block — `grep -o '"mcp__unreal_inspector__[A-Za-z_]*"'` over `1011–1148`, sorted, vs the handoff block: my in-script assertion says equal content AND order; measure it.
- That the 32 editor entries are untouched — they are the same strings at the same relative order, now at `1112–1143` (were `1059–1090`). QA-1249's table applies with a +53 offset.
- `:242` and `:334` are QA-posture lines and are named as such above; `:1025` / `:1030` are PowerShell / git rule comments. If QA reads any of the four as describing the inspector SERVER, say so and I will re-word — I read none of them that way.
- The D.1 comment carries the literals `49` and `32` (QA-1249 NIT-1's shape); the spec's own replacement text carries both, so they are kept.

## Fences honoured
No git add / commit / push · no compile · no editor / MCP engine call · no `Saved/` write · no `.mcp.json` / `settings.local.json` / agent-file edit · Chapter 11 edited ONLY at §11.4 step 3 and §11.5's first bullet · `:334` untouched · only the `TASK-1262` `- status:` line edited on the board (header-anchored exact match, matched once) · the vault twin written by `cp` of the finished repo file.

## Note for build-master (TASK-1242)
Commit `Docs/setupdirections.md` ONLY, by pathspec, after `TASK-1263` PASS; re-measure both hashes immediately before — expected `ae48dc8ef6cb769efebcb77677933a5f068c47edd01035ee557748ded731e425` / 73,427 B on both. A mismatch means a copy drifted after this handoff — route back, do not copy over. Git's `core.autocrlf` warning ("LF will be replaced by CRLF") is the pre-existing repo setting; the working file is LF.
