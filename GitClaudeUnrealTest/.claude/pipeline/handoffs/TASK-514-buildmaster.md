# TASK-514 — [KBD-BUILD] handoff (build-master, 2026-08-04)

**✅ GATE GREEN. Code commit `97bd887` on `main`. ⛔ NOT PUSHED.**

| | |
|---|---|
| Compile | ✅ `Result: Succeeded` — **0 errors / 0 warnings**, **19.64 s** |
| Suite | ✅ full `Siegebound` filter — **40 performed, 40 `Result={Success}`, 0 `Result={Fail}`** |
| Code commit | **`97bd887`** — 6 files, +3125 / −1 |
| Docs commit | see §7 (separately labelled, **no integration claim**) |
| `L_Arena` | 🔒 **UNCHANGED**, SHA256 identical before *and* after |
| Push | ⛔ **NONE.** `main` = **2** ahead of `origin/main` |
| Code authored by this gate | ⛔ **NONE** ⇒ no `SC-§27` diff-scoped verdict is owed |

⛔ **MY LIMIT, PLAINLY: this gate proves the code COMPILES and that 40 automation tests PASS on a QWERTY host. It does not prove the game is playable on Dvorak.** That is TASK-515 and it is Jonathan's alone — `-nullrhi` automation has no OS keyboard. ⚖️ **Machine evidence for the mechanism; a human for the outcome.**

---

## 1. ⭐ THE GIT PRE-FLIGHT — RUN WITH THE INSTRUMENT, AND IT CONTRADICTED THE BOARD

The board's pre-flight was written by the manager **with no git tool and no shell**, and it said so, and it demanded re-verification. **I re-ran it. The board's snapshot was stale, and the stale half was the part that would have made me do extra work.**

```
$ git status --porcelain          # repo root = C:/GitProjects/GitHub/GitClaudeUnrealTesting
 M GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md
 M GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md
 M GitClaudeUnrealTest/Docs/GDD.md
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp
?? …/handoffs/TASK-509-programmer.md  ?? …/TASK-510-  ?? …/TASK-511-  ?? …/TASK-512-
?? …/qa/TASK-513-keyboard-layout.md
?? …/Siegebound/SiegeKeyboardLayoutStatics.{h,cpp}
?? …/Siegebound/SiegeKeyboardLayoutSubsystem.{h,cpp}
?? …/Siegebound/Tests/SiegeKeyboardLayoutTest.cpp

$ git rev-list --left-right --count origin/main...main   →   0	0
$ git log -1   →   ddc6115  "first iteration of SiegeBoundAICommander"
$ git diff --cached --name-only   →   (empty — the index was NOT pre-staged)
```

⭐ **EVERY ENTRY IS THIS BATCH'S OWN. There was no foreign dirt to carry.**

| board precondition | reality at build time | disposition |
|---|---|---|
| (1) uncommitted Stage-A `DumpAssistantPrompt` (TASK-479) + TASK-505's `SiegeAssistantConsoleWidget.{h,cpp}` | ⛔ **ABSENT from the tree** | **DISCHARGED** — nothing to hold back |
| (2) TASK-514 vs TASK-481 index contention | `origin/main...main` = `0 0`; TASK-481 not resuming | **DISCHARGED** |
| (3) TASK-489 · 490 · 473 · 501 un-dispatched | all four still `backlog` (verified on the board) | **HOLDS** |

⚠️ **WHY, AND IT IS THE REUSABLE LESSON: Jonathan committed and pushed since the board was written** (`ddc6115`, and `origin` is level with `main`). This is the `SC-§9` correction firing live — *a "this file is dirty" premise is a claim about the working tree, and it decays faster than anything else a spec can assert*. ⛔ **Had I trusted the board I would have gone hunting for two lanes' files that do not exist, and I would have written a `SC-§27b` dilution note that was simply false.**

⚠️ **The index was NOT auto-staged this time** — memory records it as hostile on this repo, so I checked rather than assumed. I staged by explicit path anyway (GIT HAZARD LAW (d)); the law does not stop being right when the hazard happens to be absent.

**Also confirmed before building:** the Unreal Editor was **not running** (`tasklist` → no `UnrealEditor.exe`), so there was no Live Coding mutex and ⛔ **no question of asking Jonathan to close anything.** Nothing was force-killed and no close was driven.

**Secret scan (spec item 7):** grepped all six source files for `hf_…` / `HF_TOKEN` / `api_key` / `secret` / `password` / private-key headers / `Bearer …` ⇒ ⛔ **ZERO hits, benign or otherwise.** LFS posture: **no binary enters this commit** — six text files, `+3125/−1`; no `.gguf` and no `Models/` path anywhere in the tree or the commit.

---

## 2. COMPILE

```
Build.bat GitClaudeUnrealTestEditor Win64 Development -project=…/GitClaudeUnrealTest.uproject -waitmutex
```

```
[Adaptive Build] Excluded from GitClaudeUnrealTest unity file:
    HeroCharacter.cpp, SiegeKeyboardLayoutStatics.cpp,
    SiegeKeyboardLayoutSubsystem.cpp, SiegeKeyboardLayoutTest.cpp
[1/16] … [16/16] WriteMetadata GitClaudeUnrealTestEditor.target
Result: Succeeded
Total execution time: 19.64 seconds
```

⛔ **PARSED FROM THE LOG, NOT FROM THE EXIT CODE.** `$LASTEXITCODE` was `0` — which is worth exactly nothing on this project (Build.bat returns 0 on a FAILED build), so the verdict is the grepped `Result:` line. **Errors 0 · Warnings 0**, by regex over `error C####` / `warning C####` / `error LNK` / `warning LNK` / `C4819`. Both DLLs linked (`UnrealEditor-GitClaudeUnrealTest.dll`, `UnrealEditor-SiegeLlama.dll`).

📌 **vs baseline TASK-507** (`Result: Succeeded`, 0/0, 13.44 s): **same verdict, same counts, +6.2 s.** The delta is explained on the log's own face — **UBT's adaptive-unity pass pulled all four touched TUs out of the unity blob and compiled them standalone.** That is the expected cost of new/modified files, not a regression.

### The four watch-list items, each discharged

1. ⭐ **THE TOP RISK DID NOT MATERIALISE. UHT accepted `TMap<TObjectPtr<const UInputMappingContext>, TObjectPtr<UInputMappingContext>>` as a `UPROPERTY` — the repo's first `const` `TObjectPtr` map key.** ⛔ **And this is a stronger result than "it compiled": the UBT log shows UHT invoked with `-WarningsAsErrors`, so the header parser had no way to pass it quietly.** ⛔ **No `const` was dropped, no fallback to a non-reflected map was needed** — `KBD-§1` remains enforced by the type system rather than by care, which is the whole point of that key type.
2. ✅ **The repo's first `TAutoConsoleVariable`** (`siege.Input.LayoutPollEnabled`) compiled clean inside its `#if PLATFORM_WINDOWS` fence.
3. ✅ **`Windows/WindowsHWrapper.h`** inside the platform fence compiled clean — no `NOUSER`/`NOVIRTUALKEYCODES` fallout, no macro collision.
4. ✅ **NO `Build.cs` CHANGE was needed and none was made** (`git diff --stat` on `GitClaudeUnrealTest.Build.cs` is empty). The spec said stop and report if I reached for one; I never did.

✅ **Zero `C4819` and zero mojibake**, so the UTF-8 literals (U+2192 in a `TEXT()` literal, emoji in log strings) are fine — `/utf-8` is in effect as QA predicted.

⛔ **DIAGNOSTIC ATTRIBUTION: not applicable — there were no diagnostics to attribute.** ⚠️ Worth recording anyway: **the `SiegeLlama` plugin DID compile and link as part of this target** (`Module.SiegeLlama.cpp`, `UnrealEditor-SiegeLlama.dll`), exactly as the manager's flagged-but-unenacted caution predicted. **It was clean, so the caution did not bite — but the caution is CORRECT and the QUIET-MODULE LAW's "plugin is parallel-safe" wording is still wrong.** That amendment remains open for Jonathan/orchestrator.

---

## 3. ⭐ THE AUTOMATION SUITE — 40/40

```
UnrealEditor-Cmd.exe <project> -ExecCmds="Automation RunTests Siegebound"
  -unattended -nopause -nullrhi -testexit="Automation Test Queue Empty"
  -ini:Engine:[/Script/EngineSettings.GameMapsSettings]:EditorStartupMap=/Engine/Maps/Entry.Entry
```

Run from **PowerShell** (via `Start-Process -ArgumentList`, so the leading-slash object path survived verbatim — ⛔ Git Bash/MSYS would have mangled it). 22.27 s wall clock.

```
LogAutomationCommandLine: Display: ...Automation Test Queue Empty 40 tests performed.
LogExit: Display: **** TestExit: Automation Test Queue Empty ****
```

**Tallied by regex over `Result={…}`: 40 × `{Success}`, 0 × `{Fail}`. No other result kind appeared.**

### 3a. This batch's 7 — ⛔ BY NAME, INDIVIDUALLY

| # | test | result |
|---|---|---|
| 1 | `Siegebound.Input.ScanCodeTable` | ✅ **Success** |
| 2 | `Siegebound.Input.DvorakTranslation` | ✅ **Success** |
| 3 | `Siegebound.Input.QwertyIsPassThrough` | ✅ **Success** ⚠️ *(1 captured warning — expected, see 3c)* |
| 4 | `Siegebound.Input.DegenerateProbesAreRefused` | ✅ **Success** |
| 5 | `Siegebound.Input.RetargetPreservesModifiers` | ✅ **Success** |
| 6 | `Siegebound.Input.NonIdentityDoesNotCompound` | ✅ **Success** |
| 7 | `Siegebound.Input.LiveResolverMatchesRuntime` | ✅ **Success** ⭐ *(asserted, did not fall back to logging — see 3c)* |

### 3b. ⭐ THE REGRESSION CHECK — AND THE WIDENING PAID FOR ITSELF

Distinct test paths, counted from the log:

| suite | distinct tests | result |
|---|---|---|
| `Siegebound.Assistant.*` | **26** | ✅ all pass |
| `Siegebound.Settings.*` | **7** | ✅ all pass |
| `Siegebound.Input.*` | **7** *(new)* | ✅ all pass |
| **total** | **40** | ✅ **40/40** |

⭐ **26 + 7 = the 33 pre-existing tests, ALL STILL GREEN.** ⛔ **This is the concrete vindication of TASK-470's lesson: a `Siegebound.Input` filter would have run 7 of 40 and told me nothing about the other 33** — and `HeroCharacter.cpp` is exactly the kind of shared file whose edit could have moved `Settings` or `Assistant`. **The widening cost ~0.3 s and bought a real regression result.**

### 3c. ⚠️ BOTH PREDICTED YELLOWS — EXACTLY AS QA CALLED THEM. NEITHER IS A FAILURE AND ⛔ NEITHER WAS "FIXED".

**(i) `QwertyIsPassThrough` — WARN-3, confirmed to the letter.** Exactly one captured warning in the whole run:

> `LogAutomationController: Warning: LogSiegeInputLayout: [SiegeInputLayout] ⛔ AUTOMATION OVERRIDE: the translation was set by SetTranslationMapForAutomationTests (0 entr(y/ies)) and this instance will no longer probe the OS. This line must never appear in a shipped session.`

The test still returned `Result={Success}` — QA's reading of `bElevateLogWarningsToErrors = false` with no ini override is **empirically confirmed**. ⛔ **I did not touch the test and did not apply WARN-3's one-word `Warning`→`Log` change**: no other reason opened that file, and a build gate silencing a test's own diagnostic is precisely the move the spec forbids.

**(ii) `LiveResolverMatchesRuntime` — ⭐ it ASSERTED rather than logged, which is the better of the two correct outcomes.** The build machine is QWERTY, so the design's non-QWERTY log-instead-of-fail fallback was never reached:

> `LIVE RESOLVER OK: a US-QWERTY W press ('W', 'W') resolves to W, as expected.`
> `LIVE RESOLVER OK: what the QWERTY-W position sends on Dvorak (VK_OEM_COMMA, ',') resolves to Comma, as expected.`
> `LIVE RESOLVER SUMMARY: 2 of 2 probes matched the US-QWERTY expectation on this host.`

⭐ **That second line is the one worth reading twice: the engine's live resolver independently confirms that the character Dvorak's W-position emits resolves to `Comma`** — the exact substitution the whole feature is built on, verified against `FInputKeyManager` rather than against the batch's own fixture.

### 3d. 🔒 `L_Arena` — hashed, not mtime'd

| | SHA256 | bytes |
|---|---|---|
| before | `B3DBC5D9AE484A7BD02CAFAD52B4681DA68B011477479B65EE7781AE459F8268` | 535,522 |
| after | `B3DBC5D9AE484A7BD02CAFAD52B4681DA68B011477479B65EE7781AE459F8268` | 535,522 |

✅ **IDENTICAL, and matches the board's recorded baseline.** ⭐ **Stronger still: the `-ini:` override worked so completely that NO world was loaded at all** — zero `LoadMap` and zero `Bringing World` lines in a 355 KB log. The never-save map was never opened, not merely never written.

---

## 4. ⛔ WHAT THIS GREEN DOES **NOT** MEAN

- ⛔ **It does not mean the game works on Dvorak.** Every one of the 7 passes on a **QWERTY** host by design, through the `FSiegeKeyResolver` seam. **TASK-515 is the gate and no agent can substitute for it.**
- ⛔ **`RefreshKeyboardLayout` — the probe → handle-compare → in-place retarget → rebuild → no-op-broadcast path — WAS NEVER EXECUTED** (QA WARN-4). Every subsystem test enters through the automation latch, which returns before the probe. **"7/7 green" must never be read as "the mid-session `Win+Space` path is proven."** It has **zero** machine evidence and TASK-515's `LogSiegeInputLayout` lines are the only evidence that will ever exist for it.
- ⛔ **`check(IsInGameThread())` was not exercised** — the tests never call `Initialize`, so no probe, no timer and no Slate hook ran during automation. Deliberate.
- ⚠️ **WARN-1 is live and unfixed by design:** launching **already on QWERTY** and then switching **to Dvorak** mid-match will not take effect until the next possession/respawn. It never falls below pre-feature behaviour and it is a property of the approved architecture, not a defect. **TASK-515 must run check 5 from BOTH start layouts.**

---

## 5. ⛔ THE TREE I ACTUALLY BUILT (`SC-§27b`)

⭐ **Unlike TASK-447's, this green is NOT diluted.** The board expected me to compile a tree carrying two lanes' held uncommitted code and to disclaim them. **That tree no longer exists** — the pre-flight found the working tree clean of everything except this batch. ⇒ **The compile and the 40/40 describe THIS BATCH'S SIX FILES plus the committed baseline `ddc6115`, and nothing else.** Nothing was held back, because there was nothing to hold.

---

## 6. FOLLOW-UPS FOR THE MANAGER (reported, not enacted — I author no code and no law)

⛔ **None of these blocked the commit. I fixed none of them, because nothing gave me a legitimate reason to open those files.**

| # | item | source |
|---|---|---|
| 1 | ⭐ **QUIET-MODULE LAW amendment** — *"`Plugins/SiegeLlama/` is parallel-safe against a game-module compile gate"* is **wrong about the build command**. `Build.bat <Target>` compiles every enabled plugin; I watched `SiegeLlama` build and link inside this gate. **Now witnessed, not just reasoned.** | manager flagged, I confirmed |
| 2 | **WARN-2** — the `SwizzleAxis` half of CLAIM 4 is **vacuous** (`YXZ` is the class default, so a default-constructed stand-in — the TASK-445 failure this test exists to catch — passes). 2-line fix: `ZYX` at `:354` and `:1161`. ⚠️ **This is the batch's highest-value test and half of it currently cannot fail.** | QA WARN-2 |
| 3 | **WARN-3** — one-word `Warning`→`Log` in the test-only setter, to stop a green run reporting a warning forever. | QA WARN-3 |
| 4 | **WARN-5** — one-line `Source == Target` guard in `RetargetContextKeys`, to make `KBD-§1` structurally unbreakable. ⛔ Its ride-along condition (*"if TASK-514 authors a compile fix in this file"*) **never fired** — no fix was needed. | QA WARN-5 |
| 5 | **NIT-6 / WARN-6 — CONVENTIONS edits, manager-owned:** `KBD-§8`'s worked example `"W → Comma, S → O, D → E"` contradicts its own A..Z ordering sentence (**the code is right, the example is wrong**), and `KBD-§11`'s non-Latin bullet should read *"synthetic, non-action-bindable keys"* rather than *"identity-equivalent"*. | QA NIT-6, WARN-6 |
| 6 | **TASK-474 is still backlog** — `Tools/run_automation_tests.ps1` does not exist, so I hand-assembled the invocation and the mandatory `-ini:` override again. ⚠️ **That override is one forgotten flag away from opening the never-save `L_Arena`.** | observed |

---

## 7. THE COMMITS

**A — code, `97bd887`** *(the integration claim)*
```
Source/GitClaudeUnrealTest/Siegebound/SiegeKeyboardLayoutStatics.h        (new)
Source/GitClaudeUnrealTest/Siegebound/SiegeKeyboardLayoutStatics.cpp      (new)
Source/GitClaudeUnrealTest/Siegebound/SiegeKeyboardLayoutSubsystem.h      (new)
Source/GitClaudeUnrealTest/Siegebound/SiegeKeyboardLayoutSubsystem.cpp    (new)
Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeKeyboardLayoutTest.cpp   (new)
Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp                   (modified, +36/−1)
```
⛔ **Staged by six explicit paths — never `git add -A`, never `git add .`, never a directory pathspec.** `HeroCharacter.cpp`'s full diff was read before staging and is **exactly** TASK-512's edit: two annotated includes plus the `NotifyControllerChanged` resolve. Nothing foreign rode in.

**B — docs** *(separately labelled; ⛔ **carries NO integration claim**)*
`TASKBOARD.md` · `CONVENTIONS.md` (+216, the `KBD-§0..§11` law) · `Docs/GDD.md` (+1, the §3.1 as-built amendment) · handoffs `TASK-509/510/511/512-programmer.md` · `qa/TASK-513-keyboard-layout.md` · this file.

⛔ **NO PUSH.** `main` is **2** ahead of `origin/main` — **both commits are this batch's**, since the tree started level at `0 0`. The push is Jonathan's standing call.

---

## 8. M8 DECLARATION — verbatim

> **adds no replicated property, no new replicated class, no new relevancy tier.**

✅ **And the reason is structural, not merely observed:** `USiegeKeyboardLayoutSubsystem` is a `UGameInstanceSubsystem` — **one per client process, client-local by construction.** Host and joining client each probe **their own** OS layout, which is the only correct behaviour, and the duplicated `UInputMappingContext` lives in the transient package where the network never sees it.

---

## 9. ⭐ NEXT — TASK-515 IS JONATHAN'S AND IT IS THE REAL GATE

Everything a machine can prove about this feature is now proven. **What remains cannot be delegated:** PIE on `L_Arena`, the five checks in TASK-515's spec, and in particular

- ⭐ **check 2's second half — mouse-look not inverted, strafe not swizzled — BY HAND.** That is the TASK-445 regression and **it reads correct in every property table**; only hands catch it. *(`RetargetPreservesModifiers` passed, which is real evidence the modifiers survive the retarget — but it tests the statics layer, not the assembled in-game feel.)*
- ⭐ **check 5 from BOTH start layouts**, because WARN-1 says only one direction works before a respawn.

---
*build-master, TASK-514. Compile + suite verdicts are parsed machine output; the pre-flight is the instrument's own words. ⛔ No code was authored at this gate.*
