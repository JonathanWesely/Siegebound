# TASK-401 — [FC-6] Compile + machine PIE verification of the Follow/Miner lane — build-master handoff

**Agent:** build-master · **Runs:** #1 ~14:03 RED (foreign) · #2 ~14:50 RED (foreign) · **#3 15:02 — ✅ GREEN**
**Status on exit:** `done` for the **compile + link** half · **PIE items formally DELEGATED to TASK-402** (locked desktop, per the spec's own clause)
**NO COMMIT. NO PUSH.** (TASK-403 owns Git; it is gated on TASK-402.)
**`L_Arena` NEVER saved** — re-verified after every run: **535,522 B · 2026-07-29 03:53:38 — UNCHANGED.**

---

## 0. HEADLINE — ✅ **THE LANE IS GREEN AND THE LINK RESOLVED**

```
Result: Succeeded
Total execution time: 5.78 seconds
```

**Zero errors. Zero warnings. Zero unresolved externals.** A grep of the entire log for
`error|warning|unresolved|LNK` returns exactly one line: `Result: Succeeded`.

> ### ✅ **THE LINK WAS REACHED FOR THE FIRST TIME — QA's WARN-2 LINK SURFACE IS NOW PROVEN BY MACHINE.**
> Runs #1 and #2 both aborted at the compile stage, so the link had **never** executed. Run #3 ran it:
> ```
> [2/4] Link [x64] UnrealEditor-GitClaudeUnrealTest.lib
> [3/4] Link [x64] UnrealEditor-GitClaudeUnrealTest.dll
> [4/4] WriteMetadata GitClaudeUnrealTestEditor.target
> ```
> **Both link actions succeeded with zero unresolved externals**, which is the only real proof that every §7
> pinned symbol resolves across the TASK-395 ↔ 396 ↔ 397 ↔ 398 ↔ 379 boundaries — including TASK-379's
> `GetPermanentDamageBonusPerStack()` / `GetMaxPermanentDamageStacks()`, the access-specifier risk TASK-380
> raised and QA re-checked statically. **Static proof is not a link. This is a link.**

**Output binary:** `Binaries/Win64/UnrealEditor-GitClaudeUnrealTest.dll` — **2026-08-02 15:02:23, 3,281,920 B**
(was 2,907,136 B dated 2026-08-01 18:23). It now **postdates the lane's newest source** (2026-08-02 12:23:51),
so **the shipped binary finally contains the Follow/Miner lane.** That removes the "no binary" blocker
permanently; only the locked desktop remains.

---

## 1. WHY THE FIRST TWO RUNS WERE RED (closed — recorded, not re-litigated)

Neither was this lane's fault; **TASK-395 · 396 · 397 · 398 never emitted a single diagnostic and no QA loop
was ever consumed.**

| Run | Verdict | Diagnostics | Owner | Cause |
|---|---|---|---|---|
| #1 | `Failed` | 3 (`C4172`, `C2228`, `C2737`) | TASK-416/417 | **Race** — a live agent was writing those files mid-build ⇒ produced the QUIET-MODULE LAW |
| #2 | `Failed` | 4 (`C2039` ×3, `C2664`) | TASK-417 | **Structural** — module was genuinely quiet, but `ready-for-qa` foreign code had never compiled |
| #3 | **`Succeeded`** | **0** | — | TASK-417's type-level fix landed |

**Run #2's finding stands as law-worthy and is now confirmed by its own resolution: A QUIET MODULE IS NOT A
GREEN MODULE.** Quiet stops the *race*; it does not make foreign code *compile*. `ready-for-qa` means
"finished writing", **not** "known to build". The gate only went green once TASK-417's code actually compiled
— exactly as predicted. **Recommended amendment (manager's call):**
> *A compile gate requires that every other lane's code in the module has already passed its own compile gate,
> or is absent. A lane whose code is `ready-for-qa` but has never been built is a BUILD BLOCKER for every
> other lane in that module.*

### The fix, verified against the artifact (not the relay) — and it vindicates the whole chain

Per the RELAYED-DIAGNOSIS LAW I re-derived it from the source before building. `FJsonObject::Values` is keyed
by **`UE::FSharedString`** (interned — many objects parsed from one document share one copy of each key), not
`FString`. **That is precisely why naming `TPair<FString, TSharedPtr<FJsonValue>>&` CONVERTED rather than
BOUND**, and why removing the conversion exposed four previously-hidden sites at once. TASK-417 routed all
seven key uses through two helpers that deliberately **never name the concrete type**:
```cpp
FStringView JsonKeyView  (const FJsonObject::FStringType& Key) { return FStringView(*Key, Key.Len()); }
FString     JsonKeyString(const FJsonObject::FStringType& Key) { return FString(*Key); }
```
Both use only `operator*` and `Len()`, which `UE::FSharedString` **and** `FString` provide, so the file
compiles under either setting of `UE_JSONOBJECT_LEGACY_STRING_KEYS`. All three loops retain `const auto&`.
**Confirmed on disk before the build; confirmed by the compiler after it.**

**I never touched a foreign file across all three runs** — mtimes were identical before and after every build.
A contaminated gate is re-run, not re-litigated.

---

## 2. THE PLUGIN — HELD OUT, RESTORED, AND STILL UNPROVEN

`SiegeLlama` **was enabled** (TASK-409's uncommitted 4-line `.uproject` insert). A UBT editor build also
builds enabled plugins, so I held it out again so the Follow gate is judged on the Follow lane alone.

| Step | Evidence |
|---|---|
| Backup before edit | sha256 `63058f3c…`, 859 B |
| Disabled | `"Enabled": true` → **`false`** |
| Build | **zero** occurrences of `SiegeLlama`/`llama` in the entire log ⇒ fully excluded |
| **Restored** | sha256 **`63058f3c…` — byte-identical**, 859 B, `"Enabled": true`; `git diff` shows only TASK-409's original 4-line insert |

⚠️ **TRAP FOR TASK-412/420, worth carrying:** `SiegeLlama.uplugin` sets **`"EnabledByDefault": true`**. For a
project plugin that means **deleting the `.uproject` entry does NOT disable it** — it must be explicitly
`"Enabled": false`. Anyone isolating a build from this plugin must set the flag, not remove the block.

**⇒ The plugin is still UNPROVEN through UBT/UHT. Its `.uplugin` wiring and `UDeveloperSettings` class remain
untested and it keeps its own gate at TASK-412/420.** Also confirmed: the game module has **zero code
dependency** on it — all 9 `llama`/`SiegeLlama` hits in `Source/GitClaudeUnrealTest/` are comments.

---

## 3. WHAT RUN #3 ACTUALLY BUILT — and why the lane's compile proof is not stale

Run #3 executed **4 actions**: one compile (`SiegeAssistantCommand.cpp`) + two links + metadata. The lane TUs
were **not recompiled — legitimately.** Verified mechanically: every lane `.obj` (~14:01) postdates its source
(≤ 12:23) and **no lane source has changed since 12:23**.

| Lane TU | source | `.obj` | valid |
|---|---|---|---|
| `MinerUnit.cpp` | 12:23:31 | 14:01:08 | ✅ |
| `SummonedUnit.cpp` | 12:23:14 | 14:01:15 | ✅ |
| `Castle.cpp` | 12:10:49 | 14:01:08 | ✅ |
| `GoldNode.cpp` | 12:11:22 | 14:01:07 | ✅ |
| `SiegePlayerController.cpp` | 11:57:03 | 14:01:13 | ✅ |
| `DeckBuilderWidget.cpp` | 11:50:10 | 14:01:08 | ✅ |
| `SiegeCheatManager.cpp` | 11:51:07 | 14:01:10 | ✅ |

Run #1's compile result therefore stands: **all 7 lane TUs + 4 unity blobs clean, zero diagnostics**, and a
grep of every lane file **and** every §7 pinned symbol ∩ `error|warning|unresolved` returns **zero rows**.
**Critically, the link consumed all of those objects** — so the green link covers the lane, not just the one
file that recompiled.

---

## 4. BONUS MACHINE EVIDENCE — headless CDO probe (NOT a PIE claim)

Since real input is impossible, I took the strongest evidence available without it: a **headless
`-run=pythonscript` commandlet** (no Slate, no map load, no modal, nothing dirtied). It is **not** a PIE run
and I claim none of (a)–(i) from it. Result: **`Success - 0 error(s)`** — the freshly-linked module loads and
initialises cleanly at runtime, which a link alone does not prove.

```
CLASS MinerUnit              = FOUND
CLASS SummonedUnit           = FOUND
CLASS Castle                 = FOUND
CLASS GoldNode               = FOUND
CLASS SiegePlayerController  = FOUND
PROP  SiegePlayerController.follow_repath_tolerance      = 250.0
PROP  SiegePlayerController.follow_formation_radius      = 900.0
PROP  Castle.interior_anchor_relative_location           = (0.0, 0.0, 0.0)
PROP  MinerUnit.follow_on_spawn                          = False
```

What this establishes:
- **All five lane classes are registered** — UHT processed the new/changed `UCLASS`es and they resolve at runtime.
- **`FollowRepathTolerance` is 250.0 — machine-confirmed UNTUNED.** I promised not to touch it and the CDO proves it.
- **`Castle.InteriorAnchorRelativeLocation` is genuinely `(0,0,0)` in the CDO**, which upgrades my static anchor
  argument from "read from the header" to "read from the built binary". The resolved anchor is therefore
  Blue `(−25000, 0, 0)` / Red `(+25000, 0, 0)`. **This is still only the VALUE, not its reachability.**
- **`bFollowOnSpawn` is reflected and editable** (Python name `follow_on_spawn`, `EditDefaultsOnly`, default
  `False`) ⇒ **Jonathan's no-compile flip on `BP_Unit_Miner` genuinely exists.** *(A first probe reported it
  "missing" — that was MY naming error: UE Python strips the `b` prefix on bools. Not a defect. Recorded so
  nobody re-derives it.)*

---

## 5. PIE VERIFICATION — **NOT RUN. ZERO ITEMS CLAIMED.**

**One blocker remains, and it is absolute: THE WORKSTATION IS LOCKED.** `LogonUI` is **running** and
`GetForegroundWindow()` returns **0** (session 2 `console`/`wesel`, Active). **Real key presses are
impossible**, and board check (a) requires a real press by shipped law — `FEditorScriptExecutionGuard` forces
local callspace, so a scripted invoke cannot validate an input path. MCP exposes no function-invoke tool, so
`SummonTestUnit` / `AddTestGold` / `ApplyTestDamage` have no route either.

**Board checks (a)–(i): ALL left OPEN for TASK-402**, exactly as the spec instructs for a locked desktop.
*(The "no binary" blocker from runs #1–2 is now GONE — the binary is green and current. Only input is missing.)*

### The castle interior anchor's LIVE nav-projection — **STILL OWED. I do NOT claim it.**

Stated plainly, as instructed: **the desktop is locked, I cannot press keys, the probe at
`MinerUnit.cpp:722-729` fires only on a real Defend (E) order, and therefore this stays Jonathan's TASK-402
duty.** I have now proven the anchor's *value* twice — statically from source and from the built CDO — but
**a resolved coordinate is not a reachable one.** What TASK-402 must read back:

```
AMinerUnit '<name>': DEFEND — hiding inside '<castle>'; interior anchor resolves to <point> (castle at <loc>) (TASK-398).
```
plus **no** `MoveToLocation toward … failed` Warning. If it does not project, the fix is the
`InteriorAnchorRelativeLocation` **tunable** (`Castle.h:373`), **not code**.

### The four watch items

| # | Item | Status |
|---|---|---|
| **WARN-1** | 150 uu idle-ring stop-go at a moving anchor | **NOT OBSERVED — no PIE.** Untouched and **untuned**; `FollowRepathTolerance` **machine-confirmed 250.0**. If it reads badly the fix is **HYSTERESIS** — the 150 uu idle ring and the 250 uu re-path band are different mechanisms, only the first is implicated, and below ~180 uu the real mill returns. **Reported, not tuned.** |
| **WARN-2** | stranded Defend miner re-queries at 4 Hz behind a one-shot warning | **NOT OBSERVED — no PIE.** (Its *link-surface* half is now **PROVEN GREEN** — §0.) |
| 3 | reinforcement miners inherit the stance (miner played after **E** hides until **T**) | Expected, QA-ruled acceptable. **NOT filed.** |
| 4 | hero death freezes the following army for the respawn | Expected. **NOT filed.** |

**I changed no code and tuned no value.** Nothing in QA's "do not tidy" list was touched: `LeaveMining` still
does not clear `bHasIssuedPointGoal`; the `bStillWalking` term stands; `FollowRepathTolerance` /
`FollowFormationRadius` stay `public`; the enrollment ordinal stays monotonic.

---

## 6. ⚠️ CARRY-FORWARD FOR THE COMMIT (TASK-403 / TASK-414) — RECORDED, **NOT ACTED ON**

> ### 🚨 **JONATHAN'S RULING: add LFS rules for `*.dll` and `*.lib` to the REPO-ROOT `.gitattributes`.**
> ### ⚠️ **ORDERING TRAP — THE PATTERN MUST BE ADDED *BEFORE* THOSE FILES ARE FIRST COMMITTED.**
> If any `.dll`/`.lib` is committed first it is stored as a **raw blob in history PERMANENTLY** — adding the
> rule afterwards does **not** retroactively convert it, and only a history rewrite would.
> **`Plugins/SiegeLlama/` is ~72 MB of vendored llama.cpp and is currently UNTRACKED**, so the trap is **live**
> and **TASK-414 is the task that springs it.**
>
> **Required order: (1) land the `.gitattributes` patterns → (2) verify with `git check-attr filter -- <path>`
> → (3) only then `git add` any binary.**

## 7. FOLLOW-UPS FOR THE MANAGER (I report; I do not file)

1. **Propose the QUIET-MODULE amendment in §1** — a gate needs a *green* module, not just a quiet one.
2. **The ordering inversion is now resolved in practice** (TASK-417 fixed forward rather than the Follow lane
   waiting on TASK-420), but the general rule should still be written down: whichever lane's code is red in a
   shared module gates every other lane's compile gate, regardless of task age or QA status.
3. **WARN-3 / NIT-1 doc staleness still present** (`SummonedUnit.h:363-371` describes a deleted function;
   `MinerUnit.h:146-148`/`:339` cite pre-TASK-396 line numbers). Doc-only, zero compile effect — fold into the
   next touch of those files, as QA directed.

## 8. M8 DECLARATION DUTY

**No new replicated property, no new replicated class, no new tier.** This task ships **no code at all** — it
is a build/verification gate. Nothing to declare.

## 9. GIT

**Nothing staged by me. Nothing committed. Nothing pushed.** Working tree left exactly as found. The only file
I wrote outside `.claude/pipeline/` was `GitClaudeUnrealTest.uproject`, **restored byte-identically**
(sha256 `63058f3c…`). No `Content/` asset changed — the headless probe dirtied nothing.
`IA_CmdFollow.uasset` remains staged **by the editor's Git provider from TASK-399** (the known TASK-378
auto-stage hazard); I did not add it and did not unstage it. **TASK-403 must re-check `git status --porcelain`
before committing.** HEAD unchanged at `5fa10eb`.

## 10. WHAT TASK-401 STILL OWES

**Nothing that a locked machine can produce.** The compile + link gate is **DISCHARGED GREEN**. The remaining
rows — (a)–(i) and the live nav-projection — require real input and are **formally delegated to TASK-402**,
which is what the board spec itself directs for a locked desktop. **The Follow/Miner lane is now ready to
reach Jonathan.**
