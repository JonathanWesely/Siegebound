# QA Report — TASK-1238
subject: TASK-1228 — [AURA-SETUP-DOCS] `Docs/setupdirections.md` + the vault copy
Verdict: PASS
Blockers: 0 · Warns: 1 · Nits: 2
Reviewer: qa-reviewer · 2026-09-13 · gate over `handoffs/TASK-1228-programmer.md`

## What I read
- `.claude/pipeline/TASKBOARD.md` — section header + standing constraints (2322–2339), TASK-1228 row (2502–2511), TASK-1238 row (2608–2616).
- `C:\Users\wesel\.claude\plans\look-into-a-new-moonlit-kernighan.md` — pre-flight (line 7), "Corrections to the doc" table (9–18), Part B item 12 (59).
- `Docs/Aura AI for Unreal — Integration Plan.md` — §3 cautions (85–96), §4 Verification checklist rows 12–16 (178–182), §7 open questions (282–290).
- `Docs/setupdirections.md` — ALL 1086 lines (two pages: 1–858, 859–1086).
- `C:\GitProjects\GitHub\MyObsidianVault\JonWesOBVault\GitClaudeUnrealsetupdirections.md` — ALL 1086 lines (same two pages).
- `handoffs/TASK-1228-programmer.md` — whole file.
- Glob for the three Chapter 11 file references.

## Acceptance, item by item

### (1) The two copies are identical — ACCEPTED-AS-DECLARED (SC-§71b), consistent on Read
- Declared by the handoff: `51d112392e25dcb5a9ea6ea0ac6ed7556c3ad22e1a9b41e79fbf12330ed9a876` for BOTH files, 67,374 bytes each, `cmp` byte-identical, 0 CR bytes. I hold no hashing tool; this pair is recorded as declared and is re-measured by the host row `TASK-1241` (SC-§68).
- What I could and did verify by Read: both files report 1086 lines; every line I compared — THE LIST row 17 (line 40), Chapter 11 §11.1–§11.7 (782–920), Appendix A rows 12–16 (939–943), the Appendix B Aura block (960–975), and the surrounding text of Chapters 1–10 and Appendices A–D — is word-identical at the same line number in both copies. Read cannot see line endings, trailing whitespace, or encoding, so this is consistent with, not proof of, byte identity.
- The vault path used is verbatim the `names:` path (handoff line 8/14). The vault copy is outside the repo; nothing about it is stageable from here.

### (2) Chapter / row numbering — PASS
- `## Chapter 10 — Supabase` at line 710; `---` at 780; `## Chapter 11 — Aura` at 782; `---` at 922; `## Appendix A` at 924. Chapter 11 follows Chapter 10 with the same `---`/blank separator pattern every other chapter uses.
- THE LIST: row 16 (Supabase, line 39) → row 17 (Aura, line 40), `Ch.` column = `11`. No other row renumbered (rows 1–16 unchanged, verified by Read).
- Appendix A: row 11 (line 938) → rows 12–16 (939–943). Header `| # | Probe | Expect |` + separator `|---|-------|--------|` unchanged.
- Handoff's declared numbers (last chapter 10→11, LIST 16→17, Appendix A 11→16, header line 782, Appendix A 924, Appendix B 945) all match what I read.

### (3) Appendix A rows 12–16 character-exact vs the source table (SC-§38a) — PASS
Source doc lines 178–182 vs `Docs/setupdirections.md` lines 939–943, compared cell by cell including the backtick spans, the `🟢`, the em-dashes and the trailing `|`:
- 12: `| 12 | Aura toolbar → 🟢 → "Tell me about this project" | Project summary that names `Siegebound` classes, not the template |` — identical.
- 13: `| 13 | Claude Code `/mcp` | `unreal_inspector`, `unreal_editor`, `unreal-mcp`, `blender` all connected |` — identical.
- 14: `| 14 | `playtest-verifier` on a known-good task | `Verdict: VERIFIED`, a video/screenshot path that exists, quoted widget values |` — identical.
- 15: `| 15 | Same on a deliberately broken branch | `VERIFY-FAILED` with the failing observation — a verifier that cannot fail is not a gate |` — identical.
- 16: `| 16 | Sandbox on → Aura edits a widget → Reject | Real `Content/` file unchanged (`git status` clean) |` — identical.
The vault copy carries the same five lines at 939–943.

### (4) No other chapter edited — PASS (text-level; I cannot run `git diff`)
- Every occurrence of `Aura` / `aura_sync` / `41200` / `playtest-verifier` in the repo copy (Grep, 61 hits) falls inside exactly the four declared hunk ranges: line 40 (LIST row 17), 782–920 (Chapter 11), 939–943 (Appendix A rows), 960–975 (Appendix B block). Zero hits anywhere else.
- Read-verified unchanged and internally coherent: the preamble + secrets law (1–16), THE LIST rows 1–16 + the AV note (18–63), Chapters 1–10 in full (67–778), Appendix A rows 1–11 (926–938), Appendix B's seven pre-existing bullets (947–959), Appendix C (977–988), Appendix D.1–D.5 (992–1085). No Aura reference, no renumbering, no rewording detected in any of them; the `🧪` at line 694 is the pre-existing status-emoji legend in Chapter 9, not an addition.
- The Appendix B block is one new parent bullet with six sub-bullets appended after the last existing bullet (the Terminal-plugin item), as declared.

### (5) No secrets — PASS
Case-insensitive Grep over the whole file for `service_role`, `api key`/`api_key`, `token=`, `sbp_`, `eyJ`, `hf_…`, `ghp_`, `sk-…`, `Bearer `, `?token`, `access_token=`: only pre-existing hits (LIST row 11 Meshy "API key" location, Chapter 7 `MESHY_TOKEN`/`MESHY_API_KEY` names, Chapter 10's `service_role` law) plus one new sentence at 792 — "there is no API key to store anywhere". The Chapter 11 additions contain vendor domain `tryaura.dev`, engine-relative install paths under `C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Marketplace/Aura/…`, a local port number, and a `$150` price. None is a token, key, or tokenised URL.

### (6) Corrections table applied over the source doc — PASS
| Plan correction | Where applied | Verified |
|---|---|---|
| `INDEX_IGNORE.txt` lives under `Saved/.Aura/`, not project root | §11.3 line 832 + table row 844 | yes — no "project root" wording anywhere in Ch. 11 |
| No skills mirror | §11.3 lines 850–851: "There is no skills mirror: `.claude/skills/` does not exist in this project, so that step from Aura's docs is dropped." | yes |
| No "🧪 Dev & QA" thread wording | Ch. 11 names no thread at all; §11.6 references only the role, the report path, and the verdict set | yes — sole `🧪` in the file is the pre-existing Ch. 9 emoji legend |
| Measured one-click fact (`~/.claude.json` / `mcpServers`, `~/.claude/mcp.json` never created) | §11.4 step 2, lines 858–862 | yes — states Aura's doc is wrong and what was measured |
| Never `mcp__unreal_editor__*` wholesale | §11.5 line 896 (⛔ Never …); `mcp__unreal_inspector__*` wholesale at 893 | yes |
| Tool names census-only, never guessed | §11.4 step 4 (886–889) + Appendix B sub-bullet (972–973) | yes |
Appendix B coverage vs the source: §3 item 8 (Fab SKU) and §7's Enhanced Input, Sandbox second `.uproject` entry, credit per verification, real tool names, assistant-snapshot observability are all present. §7's "where the one-click wrote the config" and "does enabling the plugin modify `.uproject`" are correctly NOT listed as unverified — both are now measured and stated in §11.4 and §11.1 step 4.

### (7) Chapter 11 file references exist on disk — PASS
Glob confirms all three: `Tools/aura_sync.ps1`, `Docs/AuraIndexIgnore.txt`, `Docs/AuraProjectMemory.md` (wave-1 siblings; untracked at the handoff's instant per its note).

### The eight spec topics of Chapter 11 — all present
Engine-level 5.8 install + enable (§11.1 steps 3–4) · `Aura.exe` tray + port 41200 (§11.1 step 5) · Set up Unreal MCP with `:8000` untouched (§11.2.1) · Filesystem Sandbox, 5.8-only, `Intermediate/Sandboxes/AuraSandbox`, C++ not covered (§11.2.2) · `Saved/.Aura` regeneration from the two `Docs/` canonicals via `Tools/aura_sync.ps1` as a session-start step (§11.3) · the two stdio servers with measured paths + project `.mcp.json` house rule (§11.4) · allow-list law (§11.5) · `playtest-verifier` + `verified` gate paragraph with `VERIFIED | VERIFY-FAILED | UNOBSERVABLE`, after-compile, one-at-a-time, announce-when-present, advisory-until-three-matches (§11.6) · training-toggle privacy note (§11.1 step 2 + LIST row 17) · §11.7 Verify → Appendix A rows 12–16.

## Findings
- [WARN] `Docs/setupdirections.md:865` + `:891` vs `:1052` / `:1079` — §11.4 step 3 and §11.5 tell the reader to add `unreal_inspector`/`unreal_editor` to `enabledMcpjsonServers` and to merge `mcp__unreal_inspector__*` into `permissions.allow` "(Appendix D)", but Appendix D.1's snippet still reads `"enabledMcpjsonServers": ["unreal-mcp", "blender"]` and D.4 still lists only those two local servers. This is NOT a defect of TASK-1228 — its spec forbids touching any other chapter, and the programmer correctly left Appendix D alone — but the doc is now internally out of step. Suggested fix: a follow-up row (after `TASK-1216-B`'s census lands the real tool names) that updates D.1/D.4 in both copies and re-measures the hash pair. Not blocking.
- [NIT] `Docs/setupdirections.md:972` — the Appendix B sub-bullet titles the unknown as "The real `mcp__unreal_editor__*` tool names". It is a namespace reference, not a grant, and it mirrors the source doc §7's own wording; it is not a violation of the "never wholesale" law. No change requested.
- [NIT] `Docs/setupdirections.md:917` — §11.7 says "🟢 + Siegebound classes named", a project-specific name in a guide that otherwise generalizes. Appendix A row 12 (spliced verbatim from the source) carries the same name, so it is consistent; leave as is.

## Limits on this verdict's provenance
- I cannot hash (SC-§71b): the sha256 pair and the 67,374-byte size are quoted as the programmer declared them. `TASK-1241` re-measures.
- I cannot run `git diff`: "four pure-insertion hunks" is confirmed at text level (every Aura reference confined to the four ranges; all other sections read unchanged), not at diff level. The host's `git diff --stat` should show `Docs/setupdirections.md | 164 +` and nothing else in that file.
- `unreal_inspector` was not used — this task has no engine or asset claim to inspect.

## Notes for build-master (TASK-1241)
- Commit `Docs/setupdirections.md` ONLY; the vault twin is outside the repo and must never be staged.
- Re-measure both hashes before committing; expected `51d112392e25dcb5a9ea6ea0ac6ed7556c3ad22e1a9b41e79fbf12330ed9a876` / 67,374 bytes on both. A mismatch means one copy drifted after the handoff — do not "fix" by copying; route back.
- The three files Chapter 11 references (`Tools/aura_sync.ps1`, `Docs/AuraIndexIgnore.txt`, `Docs/AuraProjectMemory.md`) are sibling rows' deliverables and ride their own gates/commits, not this one.
