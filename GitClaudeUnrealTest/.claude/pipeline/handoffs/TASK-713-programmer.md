# TASK-713 — [NAME-2] STRIKE THE INERT `bUseLoggingInShipping` KEY, KEEP A SIGNPOST — handoff (gameplay-programmer)

**Date:** 2026-08-30 · **Status:** ready-for-integration (rides TASK-700's commit of the same file)
**Law:** `PKG-§9a-3` (authorizing) · `PKG-§9a-1` (the measurement that killed the key) · `PKG-§5` (Jonathan's claimed line)
**Fences honoured:** `Config/DefaultEngine.ini` **only** · zero other ini · zero Source · zero `.uasset` · no compile · no editor · no MCP · no Git · no board edits beyond this task's own status row.

## 1. Files touched (complete list)

| File | Change |
|---|---|
| `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Config\DefaultEngine.ini` | 29 lines removed (TASK-698's 28-line explanatory block + the `bUseLoggingInShipping=True` key), 1 signpost comment line added in their place |

Nothing else. **Net: −28 lines, −1784 bytes** (18,795 → 17,011 B; CRLF count 396 → 368).

## 2. BEFORE / AFTER

### BEFORE — `[/Script/Engine.Engine]`, lines 257–285 (29 lines)

A 28-line comment block headed `; TASK-698 (CONVENTIONS law PKG-9a route 1, 2026-08-30) - SHIPPING LOGGING.` — stating route 1's intent and accepted cost, then the `!! MEASURED FINDING` sub-block (TargetRules.cs:1610-1611, no `[ConfigFile]`, UEBuildTarget.cs:6452-6458, the `Shared`-environment BuildException at UEBuildTarget.cs:1514), then the closing rationale *"Kept - not deleted - because TASK-698's spec mandates it… Removing the line is safe; removing this comment is not."* — followed by the operative line:

```ini
bUseLoggingInShipping=True
```

### AFTER — one line, in the same place, verbatim

```ini
; bUseLoggingInShipping is NOT an ini key - it is a UBT TargetRules property with no ini binding, unusable on this installed engine (CONVENTIONS PKG-9a-1, measured by TASK-698; key struck by TASK-713). Shipping builds here are log-silent by design; a silent log is NOT a clean run. Do not re-add it.
```

299 chars, **pure ASCII** (verified: 0 bytes > 127 on this line).

**Why this exact wording** — it carries all five things `PKG-§9a-3` asks a signpost to carry, in the order the next author needs them: **(a)** the key's own name, so a `grep bUseLoggingInShipping` over the project **still lands here** and finds the answer where they were already looking; **(b)** the mechanism (UBT `TargetRules` property, **no ini binding**), so "then I'll put it in the right ini" is closed off too; **(c)** the machine constraint (installed engine), so the `.Target.cs` detour is closed off — `PKG-§9a-1` bans chasing it; **(d)** the law cite for the full measurement; **(e)** the ban that now carries the weight — *a silent log is NOT a clean run* — plus an explicit **"do not re-add it"**, because the failure mode this task exists to prevent is a future author re-adding the key in good faith.

### Trimming the surrounding block — why none of it survives (per the fence)

The fence said keep 698's block *only insofar as it stays true after the strike*. **None of it does, as written**: every sentence in it is framed as *"read this before relying on the line below"* / *"removing the line is safe; removing this comment is not"* — i.e. it is a warning **attached to a key**. With the key gone, that block would describe a setting that no longer exists, which is a second, quieter version of the same stale-symbol trap. The durable content (engine source line numbers, the shared-environment BuildException, the route-2 ruling) **is not lost** — it is in `CONVENTIONS PKG-§9a-1`/`§9a-3` and in `handoffs/TASK-698-programmer.md` §2, and the signpost points at the first by name.

## 3. ⛔ JONATHAN'S `bAllowHighDPIInGameMode` — BYTE-IDENTITY PROOF

**Instrument:** SHA-256 over the raw UTF-8 byte range from the first byte of `; JONATHAN'S SETTING` to EOF (the 7-line provenance comment + the setting line + trailing newline), measured **before** the edit and **after** it.

```
BEFORE   HIGHDPI REGION BYTES : 544
BEFORE   HIGHDPI REGION SHA256: A74AC72E51F831BF63BB64237121179ADCF825A1A7CF02BC5C5E55073063D1D0

AFTER    HIGHDPI REGION BYTES : 544
AFTER    HIGHDPI REGION SHA256: A74AC72E51F831BF63BB64237121179ADCF825A1A7CF02BC5C5E55073063D1D0
```

**Identical, byte for byte** — the setting **and** its dated provenance comment.

**Second, independent proof — `git diff` shows the setting as an UNCHANGED CONTEXT LINE** (no `-`/`+` pair). The whole working-tree diff of this file against `HEAD` is **9 insertions, 0 deletions**:

```diff
@@ -254,6 +254,8 @@ AppliedDefaultGraphicsPerformance=Scalable
 +ActiveClassRedirects=(OldClassName="TP_ThirdPersonCharacter",NewClassName="GitClaudeUnrealTestCharacter")

+; bUseLoggingInShipping is NOT an ini key - it is a UBT TargetRules property with no ini binding, unusable on this installed engine (CONVENTIONS PKG-9a-1, measured by TASK-698; key struck by TASK-713). Shipping builds here are log-silent by design; a silent log is NOT a clean run. Do not re-add it.
+
 [/Script/AndroidFileServerEditor.AndroidFileServerRuntimeSettings]
@@ -355,5 +357,12 @@ MaxSimultaneousTileGenerationJobsCount=8
 [/Script/Engine.UserInterfaceSettings]
+; JONATHAN'S SETTING - CLAIMED, DATED, AND IT STAYS (2026-08-30, CONVENTIONS
+; PKG-5). His verbatim reason: "I set that because it fixed a bug with screen
+; recording, so leave it as it is."
+; Recorded by TASK-698 because three commits in a row excluded this line as
+; unexplained foreign dirt: an unexplained config line is indistinguishable from
+; an accident, so the provenance IS the fix. NO future audit may strip it.
+; TASK-698 added this comment ONLY - the setting itself was not moved or changed.
 bAllowHighDPIInGameMode=True
```

⚖️ **Read this diff correctly:** `0 deletions` is not a claim that nothing was struck — it is the **net** of the uncommitted TASK-698 block being added and then removed while both were still un-staged, so `HEAD` never saw either. The 9 insertions are **7 lines of TASK-698's provenance comment + 1 signpost line + 1 blank**. The `bAllowHighDPIInGameMode=True` line appearing **without a leading `+` or `-`** is git's own statement that it is untouched relative to `HEAD`.

⛔ **The remote-exec block** at the top of the file (`[/Script/PythonScriptPlugin.PythonScriptPluginSettings]` → `bRemoteExecution=True`) is likewise untouched — it does not appear in the diff at all.

## 4. Other verifications run

| Check | Result |
|---|---|
| Residual `bUseLoggingInShipping` **as a key** | **0** — the name now occurs exactly **1×** in the file, inside the signpost comment. That single occurrence is deliberate: it is what makes the grep land here. |
| Line endings | **CRLF throughout, preserved** — CRLF count == LF count both before (396/396) and after (368/368); no LF-only line introduced, no EOL churn for the committer to explain |
| ASCII purity | Signpost is 0/299 non-ASCII. The file's only non-ASCII lines are **3**, at lines 4, 262, 322 (two em dashes + one `§`), and each is **byte-identical to the `HEAD` version** — pre-existing, not mine |
| Scope | `git diff --stat` names exactly one file: `GitClaudeUnrealTest/Config/DefaultEngine.ini` |

## 5. ⚠️ IN-FLIGHT COOK — THIS EDIT IS A FUNCTIONAL NO-OP

**TASK-699's Shipping cook may be reading this file right now. Striking the key changes NO build behaviour, in this cook or any other** — that is the entire finding of `PKG-§9a-1`: the property has **no ini binding**, so nothing in the engine or UBT ever read the line. Removing an unread line cannot alter a build. Concretely: **if the cook re-reads `DefaultEngine.ini` mid-run it sees one fewer comment-adjacent line and behaves identically; if it already read it, likewise.** The Shipping artifact is log-silent **either way**, before and after this task, and `PKG-§9a-2`'s route-2 (pixel) evidence path is unaffected. ⛔ **No cook needs restarting on account of TASK-713.**

⚠️ **TASK-712** owns `Tools/Packaging/ship.ps1` in parallel — **not touched here**, no overlap: this task's entire footprint is one ini file.

## 6. ⭐ FOR THE RECORD (spec item 4) — TASK-698 DID EXACTLY THE RIGHT THING

It complied with the letter of a spec it had been given, **then measured the mechanism at the engine source and reported that the spec was wrong** — loudly, in the handoff, in the ini itself, and one step *before* the cook that would have inherited the false belief. **The finding is the deliverable; the key was just the receipt.** This task is the receipt being filed, not a correction of 698. The measurement it bought is now law (`PKG-§9a-1`), it is what redirected TASK-699 to route 2, and it is what `PKG-§9a-2` repaired the `A6` gate against.

## 7. Deviations from spec (SC-§15)

1. **The signpost is ONE physical line of 299 characters, against this file's house wrap of ~78.** The spec and `PKG-§9a-3` both say *"one line"* / *"it costs one line"*, so I took the literal reading over the file's wrapping convention. **Flagged, not hidden** — if QA or the manager prefers it wrapped to ~78 across 3–4 `;` lines to match TASK-027/217/349/530 house style, that is a cosmetic one-edit change and I have no objection; **the content would be unchanged.** I chose literal-one-line because the fence was explicit and repeated.
2. **The signpost adds two clauses beyond the law's example text:** `"measured by TASK-698; key struck by TASK-713"` (task provenance, so the next reader can find the full measurement without knowing which law to open) and `"Do not re-add it."` (the imperative, because the specific failure mode is a good-faith re-add). Both are within *"naming the trap and citing the law"*; neither changes the ruling.
3. **TASK-698's 28-line block was removed in full, not partially folded** — reasoning in §2 above. This is the fence's *"trim or fold it so nothing left behind describes a key that no longer exists"* resolved in the direction of **trim**, because on inspection **every** sentence in it was predicated on the key's existence.

## 8. What QA / the committer should scrutinise

- **The byte-identity proof in §3** — both instruments (region hash + the diff's context line). This is the one thing in this task that must not be wrong.
- **The diff is 9 insertions / 0 deletions, and that is expected** — §3 explains why a strike shows no `-` lines here. ⛔ Do not read `0 deletions` as "the key was never removed": the key's absence is confirmed independently by the residual-occurrence count in §4 (1 occurrence, inside a comment).
- **Commit routing:** TASK-700 carries this same file. Per the board, **this should land in TASK-700's commit if 700 has not committed yet** — one commit, not two. If 700 has already committed, this rides its own follow-up. Either way, `PKG-§5` asks that Jonathan's `bAllowHighDPIInGameMode` line be **named in the commit message with its reason**, since it travels in the same file.
- **The comment is now the only carrier of this knowledge in the ini.** A future in-editor Project Settings re-save can strip ini comments (TASK-698 flagged the same risk for the provenance block) — worth a glance after any editor session that touches project settings.
