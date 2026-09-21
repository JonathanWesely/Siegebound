# TASK-1379 — GRANT-SURFACE-VS-R10-CENSUS

**Agent:** build-master · **Date:** 2026-09-21 · **Marker:** `TASK-1379-GRANT-SURFACE-VS-R10-CENSUS`
**Law:** `SC-§39` · `SC-§50` · `SC-§101` · `SC-§137` · `SC-§138`
**Mode:** READ-ONLY census. **NO GRANT SURFACE WAS MODIFIED BY THIS ROW.**

---

## VERDICT — do the live surfaces agree with ruling R10?

# YES.

**No live grant surface carries a wholesale / wildcard `mcp__unreal_inspector__*` or
`mcp__unreal_editor__*` grant.** The wildcard pattern was grepped across **10 files** and
returned **0** matches. Every Aura grant that exists is **enumerated by name, one entry per
tool**, and the counts land exactly where R10 says they should: **49** inspector read names,
**0** of the 13 census-§2a mutating names, anywhere.

The five struck prescriptions in the Integration Plan (`§4` ×2, `§6` items 8 and 12, and the
*"give `qa-reviewer` **only** `mcp__unreal_inspector__*`"* sentence) were **never followed by
any live file.** The documentation was wrong for a period; the machine never was.

⚠️ This is a **yes with three named findings** (§6 below) — the wildcard question is clean, but
the census found things adjacent to it that are not this row's to repair.

---

## 1. The instrument — BOTH CONTROL ARMS (`SC-§137`)

A matcher never shown saying *"no"* cannot be trusted when it says *"no wildcards"*.

### ARM 1 — POSITIVE, plain name. Pattern: `mcp__unreal_inspector__get_asset_meta`
A name I can see with my own eyes on line 61 of `settings.local.json`.

```
.claude/settings.local.json:1
.claude/settings.json:0
.mcp.json:0
.claude/agents/art-director.md:0
.claude/agents/build-master.md:0
.claude/agents/footage-analyst.md:0
.claude/agents/gameplay-programmer.md:0
.claude/agents/manager.md:0
.claude/agents/playtest-verifier.md:1
.claude/agents/qa-reviewer.md:1
TOTAL OCCURRENCES: 3
```
**FIRED.** ✅

### ARM 1b — POSITIVE, **wildcard-shaped**. Pattern: `mcp__[A-Za-z0-9_-]+__\*`
This is the arm that matters: it proves the *star-matching* regex — the exact matcher whose
silence carries the verdict — is capable of firing at all.

```
.claude/agents/gameplay-programmer.md:18:mcp__unreal-mcp__*
TOTAL: 1
```
**FIRED on a known-present wildcard-shaped string.** ✅
(That hit is prose, not a grant — dissected in §6c.)

### ARM 2 — NEGATIVE, fabricated names. Must return zero.
```
pattern: mcp__unreal_inspector__get_holodeck_manifold   -> exit=1, TOTAL: 0
pattern: mcp__unreal_quantum__\*                        -> exit=1, TOTAL: 0
```
**SILENT.** ✅

**The instrument says both words.** Its "no wildcards" below is therefore earned, not blind.

---

## 2. Surface (a) — `.claude/settings.local.json`

**Wildcard `mcp__unreal_inspector__*` / `mcp__unreal_editor__*`: NO.**
`grep -- 'mcp__unreal_inspector__\*'` → **exit 1, 0 matches**. Same for `unreal_editor`.
`grep -oE 'mcp__[A-Za-z0-9_-]*[^",[:space:]]*\*'` (any starred `mcp__` token, any server) → **0 matches in this file**.

**What is actually there — `permissions.allow`, quoted structure:**

| lines | content | count |
|---|---|---|
| 4–7 | four `Bash(grep/awk …)` board-parsing entries | 4 |
| 8 | `"mcp__claude_ai_Slack__slack_read_channel"` | 1 |
| 9–48 | PowerShell/Bash entries (Get-Process, git read verbs, Build.bat, RunUAT.bat, UnrealEditor-Cmd, Tools/ python), `"Agent"`, `"mcp__unreal-mcp__call_tool"` | — |
| **49–97** | **`mcp__unreal_inspector__<tool>` — 49 entries, ONE TOOL PER LINE** | **49** |
| **98–129** | **`mcp__unreal_editor__<tool>` — 32 entries, ONE TOOL PER LINE** | **32** |

Verbatim, the first and last of each enumerated block:
```
49:      "mcp__unreal_inspector__execute_unreal_python_readonly",
97:      "mcp__unreal_inspector__search_geometry_scripts",
98:      "mcp__unreal_editor__attach_pie_frames",
129:      "mcp__unreal_editor__wait_pie_seconds"
```
Counted mechanically: `grep -c '"mcp__unreal_inspector__'` = **49** · `grep -c '"mcp__unreal_editor__'` = **32**.
**49 is exactly the number R10 prescribes** (*"the ENUMERATED 49 read names (census §2 minus §2a)"*,
Integration Plan §6 item 8). The surface agrees with the ruling to the digit.

**Bare server-level grant** (a `"mcp__unreal_inspector"` entry with no tool suffix would grant the
whole server just as effectively as a star): grepped `"mcp__(unreal_inspector|unreal_editor|unreal-mcp|blender)"`
as a complete quoted token → **exit 1, 0 matches**. **NO.**

**`SC-§39` — absence from `allow` is not presence in `deny`, and I measured the right one:**
`grep '"deny"\|"ask"\|"defaultMode"'` across both settings files → **exit 1, 0 matches.**
**There is no `deny` block and no `ask` block anywhere.** So the 13 mutating inspector tools are
**ABSENT from `allow`** — they are **NOT refused**. Under interactive permissions that means a
call to one would *prompt*, not fail. Recorded as a measurement, not a complaint.

**Named dangerous tool:** `recompile_unreal_project` → **0 occurrences in this file.**
Mutating-verb sweep over the inspector prefix (`write_ create_ delete_ set_ launch_ shutdown_
generate_ compile_`) → **`<none>` for every verb.** The only `execute_*` present is
`execute_unreal_python_readonly`.

**File state:** 🚨 **UNTRACKED** — and gitignored at `GitClaudeUnrealTest/.gitignore:16`
(`git check-ignore -v` confirms). Not dirty, because it is not in the index at all.
See finding §6a: an untracked grant surface is a *different* risk class, and not one this row repairs.

---

## 3. Surface (b) — all SEVEN agent `tools:` lines under `.claude/agents/`

Censused all seven, not just the two Aura consumers.

| # | agent file | `tools:` line | inspector names | editor names | wildcard? |
|---|---|---|---|---|---|
| 1 | `art-director.md` | **ABSENT** | — | — | **NO** (see §6b) |
| 2 | `build-master.md` | **ABSENT** | — | — | **NO** (see §6b) |
| 3 | `footage-analyst.md` | line 4, 9 names | 0 | 0 | **NO** |
| 4 | `gameplay-programmer.md` | **ABSENT** | — | — | **NO** (see §6b/§6c) |
| 5 | `manager.md` | line 4, 9 names | 0 | 0 | **NO** |
| 6 | `playtest-verifier.md` | line 4, enumerated | **49** | **32** | **NO** |
| 7 | `qa-reviewer.md` | line 4, enumerated | **49** | **0** | **NO** |

Case-insensitive re-sweep `^\s*tools\s*:` across the directory returns the **same four** files —
the three absences are real, not a casing or indentation artefact.

**`qa-reviewer.md` line 4** — the sentence the Integration Plan once endangered. Verbatim head and tail:
```
tools: Read, Grep, Glob, Write, Edit, mcp__unreal_inspector__execute_unreal_python_readonly, …
      …, mcp__unreal_inspector__search_geometry_scripts, mcp__claude_ai_Slack__slack_send_message,
      mcp__claude_ai_Slack__slack_read_channel, mcp__claude_ai_Slack__slack_read_thread,
      mcp__claude_ai_Slack__slack_search_channels
```
**49 unique inspector names, 0 editor names, no star.** The struck plan sentence — *"Give
`qa-reviewer` **only** `mcp__unreal_inspector__*`"* — **was not followed here.** The live file
enumerates, and it correctly withholds the whole `unreal_editor` server (which bundles C++
authoring and Live Coding compile), matching Integration Plan line 213's reasoning.

**`playtest-verifier.md` line 4** — **49 inspector + 32 editor names, no star.** Its body prose at
lines 12–16 additionally names the forbidden set out loud:
```
⛔ NEVER call `unreal_inspector`'s engine-lifecycle tools (`launch_unreal_project`,
`recompile_unreal_project`, `shutdown_headless`, `cancel_operation`) or its generation /
plan tools — none of the 13 census-§2a names is on your `tools:` line and none is ever
called; … even if such a name were granted, calling it is a failed task (`VER-§7` cl. 4).
```
This is the **only** occurrence of `recompile_unreal_project` on any surface, and it is a
**prohibition**, not a grant. The file's own claim (*"none of the 13 … is on your `tools:` line"*)
is **independently confirmed by my count**, not taken on its word.

---

## 4. Surface (c) — `.mcp.json`

**Wildcard: NO — and structurally it cannot carry one.** `.mcp.json` declares *servers*, not
*tool grants*; it has no allow-list. Full contents censused: four servers — `unreal-mcp`
(http, `127.0.0.1:8000/mcp`), `blender` (stdio bridge), `unreal_inspector` (stdio,
Aura `unreal_inspector.py`), `unreal_editor` (stdio, Aura `unreal_editor.py`).
**Zero `mcp__` tokens of any kind, starred or otherwise, appear in the file.**

`settings.local.json` `enabledMcpjsonServers` lists all four by key — that enables the *servers*,
which is the prerequisite for the enumerated grants above and is not itself a tool grant.

**Bonus surface censused, not on the row:** `.claude/settings.json` (tracked, clean) — **hooks
only, NO `permissions` block at all.** Worth stating so nobody assumes a second allow-list is
hiding there: there is none.

---

## 5. File state per surface (`SC-§39` — tracked/dirty is its own measurement)

Measured from the git root **one level up** (`SC-§102`): `C:/GitProjects/GitHub/GitClaudeUnrealTesting`, HEAD `a9880b9`.

| surface | tracked | dirty vs HEAD |
|---|---|---|
| `.claude/settings.local.json` | 🚨 **UNTRACKED** (gitignored, `.gitignore:16`) | n/a — not in index |
| `.claude/settings.json` | TRACKED | **CLEAN** |
| `.mcp.json` | TRACKED | **CLEAN** |
| `.claude/agents/art-director.md` | TRACKED | **CLEAN** |
| `.claude/agents/build-master.md` | TRACKED | **CLEAN** |
| `.claude/agents/footage-analyst.md` | TRACKED | **CLEAN** |
| `.claude/agents/gameplay-programmer.md` | TRACKED | **CLEAN** |
| `.claude/agents/manager.md` | TRACKED | **CLEAN** |
| `.claude/agents/playtest-verifier.md` | TRACKED | **CLEAN** |
| `.claude/agents/qa-reviewer.md` | TRACKED | **CLEAN** |

`git diff HEAD --numstat` over the whole surface set → **empty**.
`git status --porcelain` over the whole surface set → **empty**.
Nine of ten surfaces are tracked and byte-identical to `a9880b9`; the tenth is deliberately ignored.

---

## 6. FINDINGS — named and left (`SC-§50`, `SC-§100`). **NOT REPAIRED.**

Each goes to the **manager**, who routes to 🧑 Jonathan. A grant change is his, never an agent's.

### 6a. `settings.local.json` — the only grant surface with no git history
It is **untracked and gitignored**, so the 81 enumerated Aura grants that make the verdict above
true have **no diffable history and cannot regress visibly**. A future edit — by a person, a
tool, or `/permissions` — would leave **no trace any reviewer could see**, and the *next* census
would have nothing to compare against. This row's YES is therefore **true at this instant and
carries no guarantee about the next one.** Not a defect this row repairs; a property worth a ruling.

### 6b. 🚨 Three of seven agents have **NO `tools:` line at all**
`art-director.md`, `build-master.md`, `gameplay-programmer.md`. An absent `tools:` line is not a
narrow grant — it means **inherit the full session tool set**, which is *wider* than the wildcard
R10 refuted, not narrower. It is not a literal `mcp__unreal_inspector__*` string, so it does not
make the §2 answer NO — but **the dispatch's own warning landed**: *"a wildcard on an agent
nobody was thinking about is exactly the one nobody has read."* These three are that shape.

**Honest mitigation, stated so the finding isn't inflated:** the `settings.local.json` allow-list
still applies on top, and it enumerates only the 49 read names — so a mutating inspector tool
reached this way would **prompt**, not auto-run. **Honest limit of that mitigation:** §2 measured
that there is **no `deny` block**, so "prompt" is the whole of the protection, and in a headless
or auto-approved run a prompt protects nothing. Whether the three should enumerate is 🧑 **his call.**

### 6c. The one wildcard-shaped string in the whole census is **documentation, not a grant**
`.claude/agents/gameplay-programmer.md:18`, quoted whole:
```
- Blueprint work goes through the Unreal MCP tools (`mcp__unreal-mcp__*`) against the running editor
```
Three reasons it is not a live wildcard grant, each checked: it is **body prose**, not a
frontmatter `tools:` line (that file has none); it names **`unreal-mcp`**, the local HTTP bridge —
**not** either Aura server R10 concerns; and it is inside a backticked phrase describing a toolset
to a reader. It is nonetheless the string that **proved my matcher can see a star at all**, which
is the only reason this census's "no" is worth anything. Flagged for the record, not for repair.

---

## 7. ⛔ WHAT THIS CENSUS CANNOT SEE

**This census reads tool NAMES on grant surfaces. It CANNOT see inside a granted tool's step
vocabulary** — `mcp__unreal_editor__run_verification_sequence` is granted (`settings.local.json`
line 110, and on `playtest-verifier.md`'s `tools:` line) as **one enumerated name**, and whatever
step verbs it accepts are **invisible to every grep in this report**. That lane is `TASK-1368`'s
(*"a granted tool with an unenumerated step vocabulary is a wildcard wearing an enumerated name"*),
and it is restart-blocked. **My NO does not cover it.** `SC-§39`: *unenumerated* and *refuted* are
not the same word — I measured the second, not the first.

Also out of scope by construction: user-scope settings outside the repo (`~/.claude/`), any
session-only `/permissions` approvals (which are not persisted to any file and therefore cannot
be censused from disk at all), and runtime state of the two Aura servers — both disconnected at
my instant, **which is irrelevant to this row: I read files, not a live server.**

---

## 8. Declarations

- 🚨 **NO GRANT SURFACE WAS MODIFIED BY THIS ROW.** Not `.claude/settings.local.json`, not
  `.mcp.json`, not any of the seven agent files — **not one character.** Shell was read-only
  throughout (`grep`, `sed -n`, `cat`, `git status`, `git diff --numstat`, `git ls-files`,
  `git check-ignore`).
- **NO commit, NO push** on this row — artefacts ride `TASK-1380`. `TASK-1374` was told this
  handoff may land inside its commit window and to name-and-hold it.
- **NO compile, NO suite, NO PIE, NO MCP call, NO editor lifecycle action.** The editor process
  was neither started, stopped, nor queried by this row.
- Files written by this row: **this handoff** + **`TASKBOARD.md` `TASK-1379` `status:` line only.**

**Artefact:** `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\.claude\pipeline\handoffs\TASK-1379-buildmaster.md`
