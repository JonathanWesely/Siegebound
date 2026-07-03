# TASK-019 Handoff — WBP_CastleHealthBar audit-and-finish (post-incident)

- author: gameplay-programmer
- date: 2026-07-03
- status: complete — all 6 PIE acceptance checks PASS; asset saved; donor untouched by me; no Git, no TASKBOARD/CONVENTIONS edits, no Variant_* asset edited; editor left running, PIE stopped

## Verdict up front

**The dead orphan session had actually FINISHED the widget and saved it at 14:41:05 — the on-disk asset was correct the entire time since then.** Proof: my end-of-session save of the verified, freshly-compiled, PIE-green state is **byte-identical (SHA256 `5E897F65...52C0D3`) to the 14:41 file**. What the orphan never did was verify, document, or hand off — and its own broken intermediate state poisoned a long-running PIE session whose log spam became the incident evidence. This session's work = full audit, root-cause reconstruction of the spam, warnings-as-errors recompile, complete PIE acceptance battery, donor verdict, and this paperwork.

## Root cause of the "Accessed None ... Bar" spam (reconstructed from the session log)

All times local (log UTC−7). The editor has not restarted all day, so the log covers the whole incident.

1. **14:32:51** — orphan duplicates UI_LifeBar → WBP_CastleHealthBar (broken intermediate wiring state).
2. **14:38:02** — orphan starts PIE; log shows `[PlayLevel] Compiling WBP_CastleHealthBar before play` — the class snapshot compiled at that instant left the generated class's `Bar` property unbound at runtime → `Accessed None trying to read (real) property Bar ... SetPercent` on EVERY castle HP broadcast in that PIE world.
3. **14:38→14:41** — orphan fixes the designer state in-editor and recompiles. **The already-running PIE keeps its stale widget instances** (UMG recompile does not rebind live PIE widgets), so the spam continues in that session — through the 017-driver cadence errors at 14:46-14:47 and a PlayAgain double-error at 14:48:23 — until the PIE stops at **14:51**.
4. **14:41:05** — Save All writes the FIXED state to disk (plus donor + map — see donor section).
5. **14:54:46** — orphan starts a new PIE (log: `Compiling UI_LifeBar before play` — the donor was dirty in memory). This PIE boots with the fixed class: **zero Accessed None from 14:54 onward**, including both castle seed broadcasts. The orphan was killed ~15:05 without reporting this; that PIE was left running and I found it still up at session start (stopped it before my clean run).
6. The TASK-017 A4 forensics ("spams per broadcast in every PIE run since") was accurate for what it observed but was stale by 14:54.

The exact micro-defect of step 2's compiled class is not recoverable post-mortem (candidates: GetBar written against the pre-reparent CombatLifeBar scope, or an event/reparent ordering that generated the property without the tree bind). What is certain and evidenced: the 14:41 designer state compiles clean with warnings-as-errors, binds `Bar` at runtime, and passes every acceptance check.

## Audit findings (current asset state — verified item by item)

| Spec item | State found | Verdict |
|---|---|---|
| Parent class | `/Script/GitClaudeUnrealTest.CastleHealthBarWidget` | already reparented, correct |
| Template logic stripped | EventGraph = 7 nodes, ONLY the OnHPChanged chain; donor's SetLifePercentage/SetBarColor events gone; zero BP member variables; zero local functions; saved asset contains NO Variant_Combat references at all | done, correct |
| OnHPChanged implementation | `EventOnHPChanged(CurrentHP, MaxHP)` → Branch on `MaxHP > 0.0` (literal via MakeLiteralFloat node) → `Bar.SetPercent(CurrentHP / MaxHP)`; all pins verified via node readback (division operand order A=CurrentHP, B=MaxHP correct) | exactly per spec |
| Bar visuals kept | Tree intact from donor: `Overlay [root] → Border_0 → Bar (ProgressBar)`; Bar attached (Slot = Border_0.BorderSlot_0) | kept |
| Numeric text block | Donor layout has NO text block (binary-scan of both assets: no TextBlock export) | bar-only, per spec's conditional |
| Compile | `compile_blueprint` with warnings_as_errors=true → clean | pass |

**How `Bar` resolves (for QA):** `UCombatLifeBar` (template C++) declares NO `Bar` property — in the donor, `Bar` is a widget-tree ProgressBar left at UWidget's C++ default `bIsVariable=true` (neither asset serializes a bIsVariable override), so the UMG compiler generates the class property and the runtime name-bind assigns it. Reparenting to UCastleHealthBarWidget preserves that mechanism — no BindWidget C++ property needed. Live-verified: castle delegate invocation list shows `HandleCastleHPChanged` (the C++ forwarder from TASK-018 — the widget BP correctly does NOT touch InitForCastle), `ObservedCastle` correct per castle, `Bar` bound to the instance's cloned `WidgetTree_0.Bar`.

## Changes made by this session

- Recompiled the Blueprint (warnings-as-errors, clean).
- Saved `/Game/UI/WBP_CastleHealthBar` (explicit single-asset save) — **resulting file byte-identical to the pre-existing 14:41 file**, i.e. zero functional delta from the orphan's saved state.
- Nothing else saved. Mtimes after my save: WBP_CastleHealthBar 15:48:45 (mine); UI_LifeBar 14:41:05, L_Arena 14:52:58, BP_HeroCharacter 13:29:42 (all untouched by me).

## Donor check — /Game/Variant_Combat/UI/UI_LifeBar

**Verdict: merely resaved, NOT functionally altered.** Evidence:

- MCP readback: parent = `/Script/GitClaudeUnrealTest.CombatLifeBar` (correct); EventGraph = exactly the two template BIE implementations (`SetLifePercentage → Bar.SetPercent`, `SetBarColor → Bar.SetFillColorAndOpacity`); zero added variables, functions, or events (no OnHPChanged, no Siegebound references).
- Structural diff of the on-disk file vs the pristine UE 5.8 template copy (`C:\Program Files\Epic Games\UE_5.8\Templates\TP_ThirdPerson\Content\Variant_Combat\UI\UI_LifeBar.uasset`): name-table differences are exclusively project-creation substitutions (`/Script/TP_ThirdPerson` → `/Script/GitClaudeUnrealTest`, header path metadata) and 5.6→5.8 serializer deltas (engine-version string, editor-only pin-name strings culled). Same widgets, same events, same hierarchy names. 71-byte size delta.
- Caveat for build-master: the 14:54:46 PIE boot recompiled UI_LifeBar ("Compiling UI_LifeBar before play"), proving the orphan left the donor dirty-needing-recompile IN MEMORY even after its Save All. Current in-memory state reads template-correct, but the donor is presumably still flagged dirty in the editor — **do NOT "save all"**; the on-disk 14:41 write is the state I judged functionally intact. Git adjudication of the staged delta remains build-master's call per the board note. Useful fact: my byte-identical result means the STAGED index blob for WBP_CastleHealthBar equals the current working tree — unstaging is conflict-free.

## PIE acceptance results (fresh PIE in L_Arena, started 15:19 local, stopped after battery)

| Acceptance item | Result | Evidence |
|---|---|---|
| Both castles show full overhead bars at boot | **PASS** | Seed broadcasts fired at boot with zero errors; readback both bars Percent=1.0, ObservedCastle correctly paired (C_0↔Castle_0/Blue, C_1↔Castle_1/Red); screenshot shows Red bar full over castle |
| Hero swings drop Red bar live in 20-HP steps | **PASS** | 3 OS-injected LMB swings (foreground + window-under-cursor verified each time): HP 2000→1980→1960→1940, bar Percent 1.0→0.99→0.98→0.97 read after each swing; Blue bar unaffected (1.0) |
| Played Footman's attacks drop it in 12-HP steps | **PASS** | Card played through the REAL path (key "1" → placement mode confirmed via bShowMouseCursor=true → verified click on Blue-half ground → BP_Unit_Footman_C_0 spawned, mode exited). Sub-cadence sampling (0.38 s) while it attacked: 1412→1400→1388→1376→1364→1352→1340→1328 — every step exactly −12 at ~1.0-1.2 s, bar tracking each step at exactly HP/2000 (0.706→0.664). Screenshot mid-fight shows the bar visibly drained |
| At 0 HP the bar disappears with the castle | **PASS** | The footman ground the castle to 0 solo (pure gameplay damage, no stat cheats needed). Readback: CurrentHP=0, bar Percent=0 (destroyed broadcast ran clean), HPBarWidget bVisible=false; screenshot: Victory screen up, castle mesh and bar both gone |
| Play Again → both bars full and visible again | **PASS** | Real click on Play Again: both castles 2000/2000, both HPBarWidget bVisible=true, both bars Percent=1.0, footman destroyed (find=[]), gold reset (108 = 50 + ticks), input mode restored (bShowMouseCursor=false); screenshot: hero at spawn, Red bar full again |
| ZERO "Accessed None" across the whole session | **PASS** | Full-log sweep after StopPIE: newest "Accessed None" entries in the entire day's log remain the orphan-era pair at 14:48:23 — none during my session (PIE 15:19→15:46). Only my own MCP path-probe warnings ("not valid Object for property 'instance'", 15:32) appear — tool artifacts, not gameplay errors |

## Notes / incidents for the orchestrator

1. **Leftover PIE at session start** (the orphan's 14:54 session, running ~7 h) — stopped before my clean run; its existence is why the "editor free" assumption was slightly off, no harm done.
2. **First click injection aborted safely on Windows foreground-lock** (fg verification worked as designed — zero stray input; the 017 Chrome incident did not repeat). Subsequent injections used ALT-tap+AttachThreadInput focus with per-click foreground AND window-under-cursor verification.
3. Evidence screenshots are in the session scratchpad (transient): pie_boot / placement / footman_attacking / victory / after_playagain .png; the durable evidence is the session log (timestamps cited above) and the readbacks recorded in this handoff.
4. For QA: no C++ was touched (TASK-018 code already QA-passed); the review surface is this handoff + the widget asset. The one deliberate scope exclusion: donor's SetBarColor path was stripped with the rest of the template logic — bar fill color is the donor's default red for both teams (placeholder-acceptable; team-tinting is an M7 concern).
