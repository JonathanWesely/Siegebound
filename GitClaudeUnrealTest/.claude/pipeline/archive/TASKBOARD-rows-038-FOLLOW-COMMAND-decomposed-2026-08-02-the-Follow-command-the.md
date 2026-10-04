<!-- ARCHIVED from .claude/pipeline/TASKBOARD.md on 2026-10-04 by Tools/archive_board.py. Every row below was in a terminal state when moved; bytes are unchanged and this comment is the only addition. Law: TASKBOARD.md '## Archive'. -->
#### TASK-400 — [FC-QA1] QA gate covering TASK-395 · 396 · 397 · 398 (qa-reviewer)
- assignee: qa-reviewer
- status: **done** (2026-08-02 — VERDICT **PASS ×4, 0 BLOCKERS** · 3 WARN · 6 NIT. Nothing went back to a programmer.)
- report: `qa/TASK-400-report.md` (2026-08-02) — all 11 named criteria met; **10 flagged decisions RULED, all upheld**. Anti-repath composition verified structurally sound (3 latches, 2 drivers, no unit driven by both). ⚠️ **TASK-401 inherits 4 named PIE watch items**, chiefly **WARN-1**: a follower crossing the 150 uu idle ring at a *moving* anchor produces a ~0.5–0.75 s stop-go hitch — not the mill (every request produces motion) and the 150 uu test is CONVENTIONS §4 law, so it was not failed, but it is the likeliest "stutter" report at the gate. **If it reads badly the fix is HYSTERESIS, NOT lowering `FollowRepathTolerance` — that value is the mill floor.** WARN-2: a stranded Defend miner re-queries pathfinding at 4 Hz behind a one-shot warning, so the retry storm is invisible in the log. TASK-401 also owns the castle interior anchor's live nav-projection check (correctly NOT claimed by TASK-398, which had no editor; the probe is built — `MinerUnit.cpp:722-729` logs the resolved point once per miner, expect Blue −25000,0,0 / Red +25000,0,0).
- blocked-by: TASK-395 · TASK-396 · TASK-397 · TASK-398
- parallel-safe: no (gates TASK-401)
- spec: >
    Pre-compile review of the whole Follow + Miner code lane. **NAMED CRITERIA — cite file:line for each:**
    **(1) THE PINNED REGISTRY (CONVENTIONS §7) matches character-for-character**, access levels included. A "improved" signature is an
    automatic FAIL. **(2) A FOLLOWING UNIT CAN NEVER ATTACK** — trace `UpdateStateFollow` and prove it never reaches `AcquireTarget` /
    `AcquireEnemyNearPoint` / `EnterAttack`, and that the seal is per-BODY (a released unit becomes a normal attacker again).
    **(3) THE MINER CAN NEVER ATTACK UNDER ANY OF THE FIVE COMMANDS** — cite the guard points by file:line, and **independently verify the
    `Profile == None` legacy-body fall-through** (`SummonedUnit.cpp:1197-1211`): if TASK-397 chose approach (A), prove the miner cannot reach
    `Goal = FindNearestEnemyCastle()`; if (B), prove the timer seal is genuinely intact. **This is the single highest-risk item in the batch.**
    **(4) ANTI-REPATH IS IMPLEMENTED, NOT DESCRIBED** — `FollowRepathTolerance` actually gates the `EnterAdvanceToLocation` call. An
    unconditional per-tick re-path at a moving anchor FAILS (the TASK-280/282 mill).
    **(5) THE ANCHOR IS RESOLVED LIVE AND NEVER CACHED**, and the hero-death branch idles rather than marching or fighting (ruling 8).
    **(6) THE DISPATCH HOIST IS FOLLOW-ONLY** — Hold/Ambush dispatch, the stance gate, `UpdateStateSiege`, `UpdateStateSupport`, the legacy
    body and every bot/Red path are byte-identical. Diff-reason them; do not take the handoff's word.
    **(7) `GetFirstPlayerController()` APPEARS NOWHERE** in the changed gameplay code (M8 TEAM LAW).
    **(8) THE M8 DECLARATION DUTY IS DISCHARGED** — each handoff states "no new replicated property / no new replicated class ⇒ no tier to
    declare". **A handoff that is silent on this FAILS**, even though the conclusion is trivially true.
    **(9) MINER BOOKKEEPING UNTOUCHED** — tenure latches, `AddMinerIncome`/`RemoveMinerIncome` pairing, `TryRegisterArrivedMiner`,
    `NotifyMineDepleted`, `FreezeAI`, `EndPlay(Destroyed)`; and `FindBestMineFor` is unmodified (the bot shares it).
    **(10) THE HOUSE COMPILE TRAPS** — literal `*/` in doc comments · non-literal `FString::Printf` formats (TCheckedFormatString / C7595) ·
    shadowing of inherited reflected members (C4457/4458/4459) · the complete-type include law (a method call or `Cast<>` on a
    forward-declared return type with no `#include`). **QA MUST scan for the last two — they are invisible to a casual read and have cost
    build loops before.**
    **(11) FLAGGED-DECISION HYGIENE** — the miner spawn-default flip line exists and is documented; the Cleric heals while following; the
    Cleric is not granted zone orders.
    Report `qa/TASK-400.md`. Verdict + path in ⚙️ Dev & QA.
- names: >
    Reviews `UnitCommand.h`, `SiegePlayerController.{h,cpp}`, `SummonedUnit.{h,cpp}`, `MinerUnit.{h,cpp}`, `GoldNode.{h,cpp}`,
    `Castle.{h,cpp}`. Report `qa/TASK-400.md`. Law: CONVENTIONS "FOLLOW command … (2026-08-02)" + "Networked 1v1 (M8)" + "C++".

#### TASK-401 — [FC-6] Compile GREEN + machine PIE verification of the Follow/Miner lane (build-master)
- assignee: build-master
- status: ✅ **done — COMPILE + LINK GREEN** (2026-08-02 RUN #3 @ 15:02, `Result: Succeeded`). **PIE checks (a)–(i) + the live nav-projection are formally DELEGATED to TASK-402** — desktop LOCKED, real input impossible, exactly as this spec directs. Handoff: `handoffs/TASK-401-buildmaster.md`
- blocked-by: TASK-400 **PASS** · TASK-399 — ⛔ *(RUN #2's hard blocker on TASK-419/420 is CLEARED: TASK-417 fixed forward, so the module went green without the Follow lane having to wait)*
- parallel-safe: no
- result: >
    ## ✅ RUN #3 (2026-08-02 15:02) — **GREEN. THE LANE COMPILES AND LINKS.**
    ```
    Result: Succeeded
    Total execution time: 5.78 seconds
    ```
    **Zero errors, zero warnings, zero unresolved externals** — a grep of the whole log for
    `error|warning|unresolved|LNK` returns exactly one line: `Result: Succeeded`.
    ✅ **THE LINK WAS REACHED FOR THE FIRST TIME ⇒ QA's WARN-2 LINK SURFACE IS NOW PROVEN BY MACHINE.**
    Runs #1/#2 both aborted at compile, so the link had NEVER executed. Run #3 ran
    `[2/4] Link UnrealEditor-GitClaudeUnrealTest.lib` + `[3/4] Link UnrealEditor-GitClaudeUnrealTest.dll`,
    **both succeeded with zero unresolved externals** — the only real proof that every §7 pinned symbol resolves across
    TASK-395↔396↔397↔398↔**379** (incl. TASK-379's `GetPermanentDamageBonusPerStack`/`GetMaxPermanentDamageStacks`,
    the access-specifier risk TASK-380 raised). **Static proof is not a link. This is a link.**
    **Binary: `UnrealEditor-GitClaudeUnrealTest.dll` 2026-08-02 15:02:23, 3,281,920 B** (was 2,907,136 B @ 08-01 18:23) ⇒ it now
    **postdates the lane source (12:23:51)**, so the shipped binary FINALLY CONTAINS THE LANE. The "no binary" PIE blocker is GONE.
    **Lane compile proof is NOT stale:** run #3 ran only 4 actions (1 compile + 2 links + metadata); UBT legitimately reused the lane
    objects (every lane `.obj` ~14:01 postdates its source ≤12:23, unchanged since). **The link consumed all of them**, so the green
    link covers the whole lane, not just the one recompiled file.
    ✅ **CAUSE OF RUNS #1/#2, CLOSED:** both were **100% FOREIGN** diagnostics (TASK-416/417), **never this lane** —
    TASK-395/396/397/398 emitted **zero** diagnostics across all three runs and **no QA loop was ever consumed**. Run #1 = a **race**
    (live agent writing mid-build ⇒ produced the QUIET-MODULE LAW). Run #2 = **structural**: module genuinely quiet, but foreign
    `ready-for-qa` code had never compiled. 🚨 **RUN #2's FINDING IS CONFIRMED BY ITS OWN RESOLUTION — A QUIET MODULE IS NOT A GREEN
    MODULE**; `ready-for-qa` = "finished writing", NOT "known to build". **Proposed amendment:** *a compile gate requires that every
    other lane's code in the module has already passed its own compile gate, or is absent.*
    ✅ **FIX VERIFIED AGAINST THE ARTIFACT, NOT THE RELAY** (RELAYED-DIAGNOSIS LAW): `FJsonObject::Values` is keyed by
    **`UE::FSharedString`** (interned), not `FString` — **exactly why `TPair<FString, TSharedPtr<FJsonValue>>&` CONVERTED rather than
    BOUND.** TASK-417 routed all 7 key uses through two helpers that never name the concrete type (`JsonKeyView`/`JsonKeyString`, using
    only `operator*` and `Len()`), so it compiles under either `UE_JSONOBJECT_LEGACY_STRING_KEYS`. **I touched NO foreign file in any
    run** — mtimes identical before/after each build.
    ✅ **PLUGIN HELD OUT AGAIN AND RESTORED.** `SiegeLlama` backed up (sha `63058f3c…`), set `"Enabled": false`, built with **zero**
    `SiegeLlama`/`llama` occurrences in the log (fully excluded), then **restored byte-identically** (same sha, 859 B, `"Enabled": true`).
    ⚠️ **TASK-412/420 TRAP:** the `.uplugin` has **`"EnabledByDefault": true`** ⇒ **deleting the `.uproject` entry does NOT disable it**;
    it must be explicitly `false`. **The plugin remains UNPROVEN through UBT/UHT — its gate is still TASK-412/420.**
    ✅ **BONUS (NOT a PIE claim): headless `-run=pythonscript` CDO probe — `Success - 0 error(s)`**, proving the freshly-linked module
    loads and initialises at runtime (which a link alone does not prove). All 5 lane classes **FOUND**;
    **`FollowRepathTolerance = 250.0` — machine-confirmed UNTUNED**; `FollowFormationRadius = 900.0`;
    **`Castle.InteriorAnchorRelativeLocation = (0,0,0)` read from the BUILT CDO** (upgrades the anchor argument from header-read to
    binary-read ⇒ Blue −25000,0,0 / Red +25000,0,0 — still the VALUE, not its reachability); **`bFollowOnSpawn` IS reflected and
    editable** (python name `follow_on_spawn`, default `False`) ⇒ Jonathan's no-compile flip genuinely exists.
    ❌ **PIE NOT RUN, ZERO ITEMS CLAIMED — (a)–(i) ALL left OPEN for TASK-402.** One absolute blocker remains: the **desktop is LOCKED**
    (`LogonUI` RUNNING, `GetForegroundWindow()`=0), so **real key presses are impossible** and check (a) requires one by shipped law.
    **The castle interior anchor's LIVE nav-projection is STILL OWED and I do NOT claim it — it is Jonathan's at TASK-402.**
    A resolved coordinate is not a reachable one.
    **`L_Arena` NEVER saved — 535,522 B / 7/29 03:53:38, verified unchanged.** No `Content/` asset dirtied by the probe.
    **No code changed, no value tuned. No commit, no push, nothing staged by me.** HEAD `5fa10eb`.
    ⚠️ **CARRY-FORWARD TO TASK-403/414 — `.gitattributes` LFS ORDERING TRAP:** Jonathan ruled `*.dll`/`*.lib` get LFS rules at the
    **repo root**; the pattern MUST land **BEFORE** those files are first committed or they stay **raw blobs in history permanently**
    (only a history rewrite would undo it). **`Plugins/SiegeLlama/` is ~72 MB, untracked — the trap is LIVE and TASK-414 springs it.**
    Order: (1) land patterns → (2) `git check-attr filter -- <path>` → (3) only then `git add`.

    ## ⛔ RUN #2 (2026-08-02 ~14:50) — **QUIET WAS NOT ENOUGH. A QUIET MODULE IS NOT A GREEN MODULE.**
    I enforced the QUIET-MODULE LAW's pre-flight and **the module was genuinely quiet** (416/417/418/409 all
    `ready-for-qa` with handoffs written; 410/411/423 `backlog`; newest source write 14:15:32 vs build 14:49 = 33 min idle).
    **It failed anyway**, on **4 NEW diagnostics in the same foreign file** — verbatim:
    `SiegeAssistantCommand.cpp` **(72,18) · (96,18) · (116,19) `error C2039: 'Equals': is not a member of 'UE::TSharedString<TCHAR>'`**
    and **(125,16) `error C2664` cannot convert `FJsonObjectSharedStringStorage::FStringType` → `const FString&`**.
    ```
    Result: Failed (OtherCompilationError)
    Total execution time: 7.40 seconds
    ```
    🚨 **THE STRUCTURAL FINDING (proposed law amendment):** "quiet" = *every C++ task finished (handoff written)* stops the
    **race**, but does **NOT** make foreign code **compile**. **`ready-for-qa` means "finished writing", NOT "known to build".**
    TASK-416/417/418 are finished, quiet, and **RED**, and they share the one UBT module. ⇒ **TASK-401 CANNOT reach
    `Result: Succeeded` until the LLM-ASSISTANT lane compiles.** Re-running achieves nothing.
    🚨 **ORDERING INVERSION NEEDING A DECISION:** the board makes TASK-401/TASK-420 mutually exclusive but leaves the ORDER open.
    It is now **forced: TASK-419 → TASK-420 green → then re-run TASK-401.** The **older, `qa-passed`, ready-to-commit Follow lane
    is now gated behind the newer, pre-QA LLM lane.** Alt route (b): the **LLM batch's own owner** parks its own files — legitimate
    only if done by that batch, never by me.
    ✅ **VINDICATES the RELAYED-DIAGNOSIS entry #3 — the compiler named the type.** The C4172 diagnosis was **CORRECT**:
    the key really is **`UE::TSharedString<TCHAR>`**, not `FString`, which is exactly why naming
    `TPair<FString, TSharedPtr<FJsonValue>>&` **converted rather than bound**. The `auto&` fix correctly removed the temporary
    **and thereby exposed the true key type at every use site** — a latent dangling-pointer bug traded for 4 loud compile errors
    (a strict improvement, still red). Owner **TASK-417**, routed to **TASK-419** as EARLY INFORMATION. **Not TASK-401 findings.**
    ✅ **PLUGIN HAZARD HANDLED AND NOT IMPLICATED.** `SiegeLlama` **was enabled**; I backed the `.uproject` up (sha `63058f3c…`),
    set `"Enabled": false`, built (**zero** `SiegeLlama`/`llama` occurrences in the log ⇒ fully excluded), then **RESTORED
    byte-identically** (same sha `63058f3c…`, 859 B, `"Enabled": true`; `git diff` shows only TASK-409's original 4-line insert).
    ⚠️ **TRAP FOR TASK-412/420:** the `.uplugin` has **`"EnabledByDefault": true`**, so **deleting the `.uproject` entry does NOT
    disable it** — it must be explicitly `"Enabled": false`. The plugin remains **unproven through UBT/UHT**; its gate is TASK-412/420.
    ✅ **LANE COMPILE PROOF HOLDS, NOT STALE.** Run #2 ran only 8 actions (5 compiles, all `SiegeAssistant*`); UBT legitimately
    skipped the lane TUs because **every lane `.obj` (~14:01) postdates its source (≤12:23) and no lane source changed since.**
    ❌ **LINK STILL NEVER REACHED** in either run ⇒ WARN-2's link surface **remains unproven by machine**.
    ✅ **I touched NO foreign file** — `SiegeAssistantCommand.cpp` 14:14:56 · `Snapshot.cpp` 14:14:34 · `Vocabulary.cpp` 14:15:32,
    **identical before and after my build.** The fix was 4 small edits and visible to me; the law says a contaminated gate is
    **RE-RUN, not re-litigated**, and fixing another batch's file is a single-owner violation stacked on a serialization violation.
    ❌ **PIE NOT RUN, zero items claimed** — two independent blockers: the shipped
    `UnrealEditor-GitClaudeUnrealTest.dll` is **2026-08-01 18:23:28**, a full day OLDER than the lane's newest source
    (2026-08-02 12:23:51) ⇒ **no binary contains the lane**; and the **desktop is LOCKED** (`LogonUI` RUNNING,
    `GetForegroundWindow()`=0). **The castle interior anchor's LIVE nav-projection is STILL OWED and I do NOT claim it —
    left to Jonathan's TASK-402**, exactly as instructed. `L_Arena` re-verified **535,522 B / 7/29 03:53:38 — UNCHANGED**.
    **No code changed, no value tuned** (`FollowRepathTolerance` still 250.f). No commit, no push, nothing staged.
    ⚠️ **CARRY-FORWARD TO TASK-403/414 — `.gitattributes` LFS ORDERING TRAP:** Jonathan ruled `*.dll`/`*.lib` get LFS rules at the
    **repo root**; the pattern MUST land **BEFORE** those files are first committed or they stay **raw blobs in history permanently**.
    **`Plugins/SiegeLlama/` is ~72 MB, untracked — the trap is LIVE and TASK-414 is what springs it.**

    ## RUN #1 (2026-08-02 ~14:03) — the original contamination that produced the QUIET-MODULE LAW
    **`Result: Failed (OtherCompilationError)`** — and **all 3 diagnostics are FOREIGN to this lane**:
    `SiegeAssistantCommand.cpp(58) C4172` + `SiegeAssistantSnapshot.cpp(338) C2228/C2737`. Those files are **untracked** and belong to the
    **Assistant/Llama batch (TASK-416 · TASK-417)**, whose own gate **TASK-420 has not run**. A parallel agent was **actively writing them
    during the build** (`SiegeAssistantSnapshot.cpp` 33,245→33,799 B at 14:02:36; marker `▶ NOW TASK-417`), so they were **left untouched** —
    not parked, not renamed. They share the **one UBT module** with this lane, which is how they broke a gate they have nothing to do with.
    ⇒ **TASK-395 · 396 · 397 · 398 STAY `qa-passed`. NOT set `qa-failed`. NO QA loop consumed** — they emitted **zero** diagnostics.
    **THE LANE COMPILED CLEAN:** all 7 lane TUs + 4 unity blobs, no diagnostic; a grep of the log for every lane file **and** every §7 pinned
    symbol ∩ `error|warning|unresolved` returns **zero rows** ⇒ **no registry mismatch, §7 held.**
    ⚠️ **LINK NOT REACHED** (aborted at compile) ⇒ WARN-2's link surface still unproven. Its **static** half was re-run and **PASSES**:
    `SummonedUnit.h` = exactly 3 specifiers (`public:` **121** · `protected:` **574** · `private:` **862**), TASK-379's getters at **:557**/**:572**,
    both inside the original `public:` block.
    **numstat (QA ruling 5): TASK-398's six owned files match its claimed table 6/6 exactly.** Honest limit: HEAD predates all four tasks, so
    numstat cannot attribute per-task lines on shared base files — so the **decisive content check** was run instead: every miner-related
    added/removed line in `SummonedUnit.*`/`SiegePlayerController.*`/`UnitCommand.h` is a **COMMENT**; **zero miner executable code**. Ruling 5 stands.
    **PIE: NOT RUN, zero items claimed — (a)–(i) ALL left OPEN for TASK-402.** Three independent blockers: no binary contains the lane (compile
    red); the **workstation is LOCKED** (idle 3d15:28, `GetForegroundWindow()`=0, capture black) so the **real key presses are impossible**; and
    MCP has **no function-invoke tool**, so `SummonTestUnit` has no route. **Castle interior anchor:** the *resolved value* is proven statically
    (`TransformPosition(ZeroVector)` ≡ actor location ⇒ Blue −25000,0,0 / Red +25000,0,0; yaw-180 moot at this default), but the **live
    nav-projection is still OWED** — the probe fires only on a real **E** press.
    **`L_Arena` NEVER saved — 535,522 B / 7/29/2026 3:53:38 AM, verified unchanged.** Editor deliberately **left CLOSED** (a stuck Slate
    "Save Content" modal forced process termination; blind-clicking risked hitting "Save Selected" next to "Don't Save"). No commit, no push,
    nothing staged by me. **No code changed, no value tuned** — `FollowRepathTolerance` untouched at 250.f.
    🚨 **FOLLOW-UP FOR THE MANAGER — PIPELINE SERIALIZATION DEFECT:** `Source/GitClaudeUnrealTest/` is **one UBT module**, so any in-flight
    `.cpp` breaks every other lane's compile gate. **File-disjointness is not build-disjointness** — the M8 PARALLEL LAW guards edit conflicts
    but not compile-gate contamination. **TASK-401 and TASK-420 must be mutually exclusive**, and no programmer task in that module may be
    `▶ NOW` while either runs.
- spec: >
    **Compile first (hard gate, must be GREEN).** Compile failure ⇒ append the errors to `qa/TASK-400.md` and route back to
    gameplay-programmer (**counts as a QA loop**; max 3, then escalate). **NO COMMIT IN THIS TASK** — TASK-403 owns Git.
    **MACHINE-VERIFIABLE PIE CHECKS (single-player / host; units are server-only in M8 P1, so this is host-side only by design):**
    (a) `IA_CmdFollow` resolves and C is bound — **and the resolve is verified from a REAL KEY PRESS, never a scripted invoke** (the shipped
    law: `FEditorScriptExecutionGuard` forces local callspace, so python remote-exec cannot validate an input/RPC path); if the desktop is
    locked and real input is impossible, **say so plainly and leave the item OPEN for TASK-402** rather than claiming it.
    (b) `SummonTestUnit Footman false` ⇒ the unit **walks to the hero and does not march on Castle_Red**; move the hero and confirm the unit
    follows without visible per-tick re-path stutter.
    (c) The same unit parked next to an enemy for 30 s **never enters Attack** (log/state readback).
    (d) `SummonTestUnit Ogre false` ⇒ **still auto-marches** at the enemy castle (the Siege exclusion).
    (e) A Red/bot unit's behavior is unchanged (bot still fields waves and attacks).
    (f) A miner played with no command **mines and reaches +1 gold/s** (the ruling-7 default).
    (g) Press **T** ⇒ every follower switches to the attack march; play a new unit ⇒ it spawns FOLLOWING again (flag (a)).
    (h) Kill the hero ⇒ followers **idle in place**, and resume following the respawned pawn 5 s later.
    (i) Play Again ×3 — no leaked group, no leaked `DefaultFollowGroupId`, Message Log clean.
    **Report every item as PASS / FAIL / NOT-MACHINE-VERIFIABLE with evidence. Do not report a machine readback as proof of anything visual.**
    **`L_Arena` is NEVER saved.** Handoff `handoffs/TASK-401-buildmaster.md`. Post the compile result + the check table in 🔧 Build & Git.
- names: >
    Build: `"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" GitClaudeUnrealTestEditor Win64 Development
    -project="C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/GitClaudeUnrealTest.uproject" -waitmutex`.
    Exec cheats: `SummonTestUnit`, `AddTestGold`, `ApplyTestDamage`. Law: the hard gate + the real-input verification law (CONVENTIONS
    "Networked 1v1 (M8)" → the RPC/remote-exec clause).

