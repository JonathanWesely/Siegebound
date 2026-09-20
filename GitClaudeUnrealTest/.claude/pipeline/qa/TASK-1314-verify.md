Verdict: VERIFIED
# Verification — TASK-1314 (PLAYAGAIN-KEY-REACHABLE), acceptance (7b) — **LIMB B of two: the POST-SAVE VERDICT LIMB**

verifier: playtest-verifier · 2026-09-20 · subject `TASK-1314` (`built`) · gate `TASK-1315` (PASS) · host `TASK-1316` · **attempt 1 of 3**
⛔ **THIS FILE IS THE CANONICAL PATH AND IT WAS AMENDED IN PLACE (`VER-§1` cl. 7 · AMENDMENT C2).** Line 1 was re-written from `UNOBSERVABLE` to `VERIFIED`; **LIMB A survives WHOLE beneath this section** — its four artefacts (the pre-image hash pair, the (7b)-iii regression `pass`, the RIDER 1(f) coupling table, the four measured ceilings) are untouched. ⛔ No second file was written.
Reads performed whole (`SC-§38a`): `qa/TASK-1314-verify.md` (Limb A) · TASKBOARD `#### TASK-1314` acceptance (7b) + RIDERS 1–3 + AMENDMENTS C1/C2/C3/C4/C5 + the `status:` line · `CONVENTIONS.md` `VER-§1` cl. 1–7 (marker `VER-1-7-THE-TWO-LIMB-VERIFICATION`, located by marker, ⛔ not by line number).

## 🚨 CHECK ONE — THE SIDE-OF-THE-LINE TEST, TAKEN BEFORE ANY PIE CALL, AT MY OWN INSTANT (`SC-§91` · `VER-§1` cl. 7)

| | sha256 | bytes | mtime |
|---|---|---|---|
| **Limb A pre-image** (2026-09-19) | `7817dd7f19bcf1028bd21ddfc4d92731c01c14d567c19641cc10e9f1d12bd6ee` | 153,489 | `2026-07-29 12:23:05` |
| **MY READING, pre-run** | **`a26ad9a09d332ccd8a29df6a87d22a878b3bfd531d3199d934074addedc4b40b`** | **153,825** | **`2026-09-20 01:20:53`** |
| **MY READING, post-run** | `a26ad9a09d332ccd8a29df6a87d22a878b3bfd531d3199d934074addedc4b40b` | 153,825 | `2026-09-20 01:20:53` |

⇒ **+336 bytes and the hash MOVED** ⇒ 🧑 his BP-editor `Compile` → `Ctrl+S` **LANDED**; `SC-§125`'s hash-must-change discharge is **SATISFIED**. Pre- and post-run readings are **identical** ⇒ **he did not save mid-flight.**

⭐ **AND THE PROPERTY ITSELF, READ RATHER THAN INFERRED FROM THE HASH.** The hash proves *a* write; it does not prove *which* one. I read the object directly: `/Game/UI/WBP_VictoryScreen.WBP_VictoryScreen:WidgetTree.Btn_Jump` (class `Button`) ⇒ **`IsFocusable = True`** (both the `is_focusable` and `IsFocusable` spellings returned `True`). The **root's** `bIsFocusable` still reads **`False`** — correctly **untouched**, since focusing the root is the exact defect `TASK-1311` removed. ⇒ **the right one bit moved, and only it.**

## Editor/Aura state

| item | measured at my own instant (`SC-§91`) |
|---|---|
| Aura / MCP | **connected** — read-only Python probes + the full `unreal_editor` PIE surface all returned live data |
| map | `/Game/Maps/L_Arena.L_Arena` — **already the open level; I did NOT call `load_level`** |
| editor instance | **PID 22560** (`os.getpid()` inside the Aura-hosted editor), matching the dispatch |
| ✅ identified by **COMMAND LINE** (`SC-§118`) — **PARTIAL, by cl. 9, AND STRONGER THAN LIMB A'S** | The live log carries the **actual command line** (Limb A's `LogInit: Command Line:` was empty): `LogCsvProfiler: Display: Metadata set : commandline="" C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\GitClaudeUnrealTest.uproject""` ⇒ **plain `.uproject`, `-game` token ABSENT, no switches at all.** ⛔ A full OS process enumeration remains unreachable (`subprocess` and `ctypes`/`windll` are both rejected by the read-only lane) ⇒ **labelled `PARTIAL, by SC-§118 cl. 9`**, with that clause's substitute: of **75** `Saved/Logs/*.log`, exactly **one** was being written (`GitClaudeUnrealTest.log`, age **0.0 s**); the only other fresh file was `cef3.log` (0.8 s — the **same** process's CEF child); the next was **222.4 s** stale. `is_pie_active` → `num_instances: 1`. |
| 🧑 **HIS `-game` SESSION — FOUND, IDENTIFIED BY COMMAND LINE, AND ALREADY EXITED** | The 222 s-stale file is `GitClaudeUnrealTest_2.log`, whose command line reads `… GitClaudeUnrealTest.uproject **-game -windowed -ResX=3200 -ResY=1800**` ⇒ 🧑 **his**. It ends `LogExit: Exiting.` / **`Log file closed, 09/20/26 01:26:53`** ⇒ **it had already terminated ~70 min before my PIE started.** ⛔ **Nothing of his was driven, stopped, killed or restarted.** ⭐ Note the timing: it opened and closed in the minutes **after** his `01:20:53` save — i.e. this is the session in which he took his (7c) hand answer. |
| Build Configuration | **`LogInit: Build Configuration: Development`** — quoted because the (7b)-iii zero is worthless in a Shipping/Test log (the emitter is inside `#if !(UE_BUILD_SHIPPING \|\| UE_BUILD_TEST)`) |
| PIE mode | standalone, 1 client, windowed 1280×720 (viewport 1280×725); session ran **173.86 s** of game time |
| log file NAMED (`VER-§1` cl. 2) | `Saved/Logs/GitClaudeUnrealTest.log`, line 1 `Log file open, 09/19/26 20:07:36`. **My slice = bytes 405,114 → 464,989 = 59,875 bytes**, all of it this PIE session |
| attempts used | **1 of 3** — one PIE session, no retry needed |
| wall time | **≈ 12 min** |
| credit | not surfaced by any tool this run |
| 🧑 `VER-§3` | **ASLEEP; unattended work authorised in the dispatch.** His `-game` instance was already closed (above). |

**Binaries — proven, not assumed:** `Binaries/Win64/UnrealEditor-GitClaudeUnrealTest.dll` mtime **`2026-09-19 18:51:19`**, sha256 `e7b37c64bf55b401434435ff…`; this editor's log opened **`20:07:36`**, i.e. **after** the DLL was written ⇒ it carries `TASK-1316`'s 5a build of `TASK-1314`. ⇒ ✅ **BOTH HALVES — the compiled code and the saved asset — are in the one running process, each confirmed separately.**

## Acceptance lines → observations

| # | acceptance line | observable chosen | observed (quoted values, evidence path) | pass/fail/unobs |
|---|---|---|---|---|
| **(7b)-i** | *"`UNOBSERVABLE` IS PRE-AUTHORISED ON THIS LEG … `simulate_key_press` is delivered to the player controller and NOT to Slate … the verifier does NOT re-trace that named dead end"* | none — **deliberately not attempted** (`VER-§5` cl. 5 · `VER-§8` cl. 1 · AMENDMENT C2(ii)) | I pressed **no** key at the victory screen and make **no** claim about whether `Enter` activates Play Again. **Zero** budget spent re-proving a named dead end. 🧑 This half is his (7c), recorded below and **never merged**. | unobs |
| **(7b)-ii** | *"at a PIE that ACTUALLY REACHED A MATCH END on `L_Arena`, read WHICH widget holds Slate keyboard focus … AFTER/positive = the Play Again button (`Btn_Jump`) BY NAME"* | `ui_snapshot` on `WBP_VictoryScreen`, every node's `focused` field — the same instrument that read `false` on all 16 nodes at Limb A and `true` for `WBP_DeckBuilder_C_0` at `TASK-1306` (**proven to fire both ways, on this very asset, 14 hours apart**) | **Match end REACHED organically** (chain below; destruction at PIE **`65.28 s`**). At **t=1m24s** (PIE `84.49 s`): **16 nodes walked, `nodes_truncated: false`**, and ⭐ **`Btn_Jump` reads `"focused": true` — BY NAME, `index_path "0/0/0"`, `name_path "Overlay_19/SizeBox_0/Btn_Jump"`, class `Button`** — and it is the **ONLY** node of the 16 with `focused: true` (root `WBP_VictoryScreen_C_0`, `Overlay_19`, `SizeBox_0`, `TextBlock_0` *"Play Again"*, `TextBlock_1` *"Defeat"*, both `UI_Thumbstick_C` subtrees: all `false`). `Btn_Jump` also read `visibility: Visible`, `hit_testable: true`, `enabled: true`, geometry **174.37 × 53.65** at abs (1840.32, 1337.30) — **the same geometry Limb A measured, so the only thing that changed is the focus bit.** Corroborated on the live widget: `get_widget_property_in_pie(Btn_Jump).IsFocusable = true`. Evidence: `Saved/AuraVerify/pie_composited_c1_t84.53s_f1028360.png` (promotion **OWED**, `VER-§4` cl. 1). | **pass** |
| **(7b)-iii** | *"AND the engine string `InputMode:UIOnly - Attempting to focus Non-Focusable widget` must STILL be 0 — `TASK-1311`'s gain may not regress"* | exact-string count in the **named** `Development` log slice this PIE instance wrote, plus a **firing** positive control and a negative control | **0** — and in a slice where the emitter site **provably executed** (`HandleMatchEnd`'s last statement is in it, quoted below). Broad sweep: `Non-Focusable` = **0**, `NonFocusable` = **0**. The slice is **not silently empty**: it carries **13 `: Error:` lines**, all `LogLiveCoding: Error: Cannot enable module …` (pre-existing, unrelated). **Positive control (`SC-§39`): the SAME reader finds the string 33 times across 28 of the 75 logs**, chronologically last at **`[2026.09.19-00.31.19:252]`** in `GitClaudeUnrealTest_2-backup-2026.09.19-00.31.58.log`, verbatim `LogPlayerController: Error: InputMode:UIOnly - Attempting to focus Non-Focusable widget SObjectWidget [Widget.cpp(976)]!` — **pre-`1c93610`, naming `SObjectWidget`**, exactly the emission the dispatch predicted. **Negative control:** the bogus variant *"…Non-Focusable gizmo"* scored **0** across all 75. | **pass** |

⚖️ **Derivation:** 2 `pass`, **0 `fail`**, 1 `unobs` (pre-authorised at boarding). `VER-§1` cl. 7's suspension of cl. 5 **no longer applies** — it is scoped to *"a human asset write that has **not** landed at 5b"*, and the write **has** landed and is measured twice above. ⇒ **cl. 5's ordinary derivation resumes and yields `VERIFIED`**, which is now the *true* token: line 1 says *"at a real match end, the Play Again button holds Slate keyboard focus"*, and that is precisely what was measured.

## ⭐⭐ RIDER 1(f) — THE COUPLING. **VERDICT: THEY MOVED TOGETHER. AGREE. NO CONTRADICTION. NO STOP.**

RIDER 1(f) makes the guard's `Warning` a **one-bit instrument on the asset half** (`SButton::SupportsKeyboardFocus()` returns `bIsFocusable`, baked from the UMG property at `RebuildWidget()`), and the dispatch fixed the prediction **in advance**: both instruments must flip, and **either one alone is a STOP**.

| instrument | LIMB A (pre-save) | **LIMB B (post-save, mine)** | predicted | moved? |
|---|---|---|---|---|
| guard `Warning` *"does not support keyboard focus … (IsFocusable is False …)"* | **1** (2 ms after the destruction line) | **0** | must drop to 0 | ✅ |
| `ui_snapshot` focus on `Btn_Jump` | **`false`** (and false on all 16) | **`true`** (and the only `true` of 16) | must land on the button BY NAME | ✅ |
| engine `Error` *"Attempting to focus Non-Focusable widget"* | 0 | **0** | must stay 0 | ✅ |
| lookup-failure `Warning` *"victory screen has no widget named"* | 0 | **0** | — | ✅ |

⇒ ⭐ **BOTH BITS FLIPPED IN THE SAME DIRECTION, ON THE SAME RUN, ON THE SAME FRAME.** RIDER 1(f)'s two contradiction shapes — a `Warning` present *with* focus on the button, or absent *with* focus off it — **did not occur.** What the pair pins down that neither alone could:
1. **The silence is load-bearing, not incidental.** The guard branch is the one that logged at Limb A, 2 ms after the destruction line, *inside* `HandleMatchEnd`. This run has the **destruction line and `HandleMatchEnd`'s last statement 1 ms apart with nothing between them** — so the `Warning` did not merely fail to appear, its `if` was **evaluated and taken the other way**.
2. **`GetWidgetFromName(TEXT("Btn_Jump"))` still resolves** — the lookup-failure branch scored 0, so the `TASK-1286` inert shape is absent at runtime, again.
3. **`SetWidgetToFocus` ran and its argument was the BUTTON**, which is the half `TASK-1315` could only read statically: focus landed on `Btn_Jump`, **not** on the root, so `TASK-1311`'s fix is not undone.

## Was a real match end reached? — **YES.** By AMENDMENT C1's measured route, not `TASK-1311`'s

⛔ Budgeted **nothing** on the bot, per C1. The staging is declared line-by-line (`VER-§7` cl. 2); everything below is **PIE-transient** and reached disk nowhere.

1. **PIE start** — `L_Arena`, 153 actors, `Castle_Blue` at `x −25000` reading **`CurrentHP 2000.0 / MaxHP 2000.0 / bDestroyed false / Team Blue`**.
2. **t = 31.10** — `pie_scene_edit spawn_actor` `/Game/Blueprints/Units/BP_Unit_Knight.BP_Unit_Knight_C` → `BP_Unit_Knight_C_0` (label `BP_Unit_Knight0`) at (−21250, 0, 120). ⚠️ **C1's ordering trap CONFIRMED LIVE, not taken on trust:** it read **`Team: Blue`**, `AttackDamage 15.0`, `AttackRange 120.0`, `CurrentHP 200.0`.
3. **t = 62.57** — three ops, in this order: `Team` **`Blue → Red`** (flipped **before** expecting damage) · transform re-set to (−21250, 0, 120) · `Castle_Blue.CurrentHP` **`2000.0 → 40.0`**. ⛔ **1,960 HP were removed by my staged write.** Geometry: the castle's near colliding face is at **x ≈ −21,292.6** (`location −25000` + `bounds_extent 3707.45`, both read this run) ⇒ the Knight stood **≈ 42 uu** off it, well inside `AttackRange 120`.
4. **t = 62.62** — `start_state_recording` armed **AFTER the staging**, per AMENDMENT C4(i)'s recipe note: `every_n_ticks 2`, `max_samples 1200`, `dt 0.0333`. **794 rows returned — it did NOT expire early.**
5. ⭐⭐ **THE PER-TICK STEPS LIMB A MISSED ARE MEASURED THIS TIME**, and they are the organic grind:

| row time (rel. t0 = 62.6245) | PIE time | `Castle_Blue.CurrentHP` | `bDestroyed` | Δ |
|---|---|---|---|---|
| 0.0167 → 0.2167 | 62.641 → 62.841 | **40** | 0 | (staged floor) |
| **0.2334** | **62.858** | **25** | 0 | **−15** |
| **1.4513** | **64.076** | **10** | 0 | **−15** |
| **2.6513** | **65.276** | **0** | **1** | **−10 (clamped), 0-CROSSING** |

⇒ **three blows of `AttackDamage 15.0` at a ≈1.22 s cadence, the third clamped at zero.** ⚖️ **Limb A declined to claim exactly this as *"arithmetic, not a measurement"* — it is now measured, and Limb A's arithmetic was right. The pipeline got the honest report first and the number second, which is the correct order.**
6. **⛔ No write of mine touched `Castle_Blue` after t = 62.57.** The 0-crossing and `bDestroyed = true` came through the real `ACastle::TakeDamage` path — **a property write cannot produce `bDestroyed = true` or the GameMode line** (`CurrentHP` is mutated only by `TakeDamage`/`ResetCastle`).
7. **The engine agrees, in the game's own log** (`[204]` = one frame, three lines, 1 ms apart):

```
[2026.09.20-08.32.42:685][204]LogGitClaudeUnrealTest: [SiegeGameMode_0] Castle 'Castle_0' (Blue) destroyed — match over, winner: Red.
[2026.09.20-08.32.42:686][204]LogGitClaudeUnrealTest: [SiegeGameMode_0] Match-end freeze: 2 unit(s) frozen, 0 tower(s) silenced, 0 barracks frozen, 0 projectile(s) cleared, income paused, clock stopped, bot decision loop stopped.
[2026.09.20-08.32.42:686][204]LogGitClaudeUnrealTest: ASiegePlayerController 'SiegePlayerController_0': match ended — winner Red.
```
🚨 **THE GAP BETWEEN LINE 1 AND LINE 3 IS THE DELIVERABLE.** At Limb A the guard `Warning` sat **between** these two lines. Here there is **nothing** between them.
8. **The widget layer agrees:** `TextBlock_1` = **"Defeat"** (winner Red — correct for the Blue player), `Btn_Jump/TextBlock_0` = **"Play Again"**.

## Evidence (promoted)

Promotion is **OWED to the 5c host** (`VER-§4` cl. 1 — the verifier holds no tool that moves bytes; ⛔ I do not claim a path that does not yet exist):

- **SOURCE** `Saved/AuraVerify/pie_composited_c1_t84.53s_f1028360.png` (**1,359,958 bytes**, 1086×615, PIE `game_time_seconds = 84.53`, frame 1028360, `mean_luma 121`, `pct_near_black 0.0044`)
  → **TARGET** `.claude/pipeline/playtest-evidence/2026-09-20/VER-TASK-1314-t01m24s-focus-holder-at-match-end.png`
  — the composited end screen at a real match end: the hero's back centre-frame in a ragged white surcoat on sunlit grass, between four grey standing stones and two yellow-leafed trees under a blue sky; **"Defeat"** in white outlined text across his shoulders; the small pale **"Play Again"** pill directly below it; `Gold: 75` top-left, `60 FPS  16.7 ms` top-right; and a **red engine overlay** spanning the middle third reading *"Video memory has been exhausted (8.242 MB over budget). Expect extremely poor performance."*
  🚨 **WHAT THIS FRAME DOES AND DOES NOT SHOW, STATED PLAINLY:** the pill carries **no conspicuous focus outline at this resolution**. ⛔ **The pixels therefore do NOT independently show focus and I do not claim they do.** The focus reading rests on `ui_snapshot`'s `focused: true` **plus** the guard `Warning`'s silence. ⭐ This is (7c)'s parenthesis running in reverse and is worth recording: it warns that *an outline is not proof of the feature*; this frame shows that *the absence of an outline is not proof against it either*. **Neither direction of the outline argument is evidence — which is exactly why the acceptance line was written against the focus instrument and not against a screenshot.**

⚠️ **VRAM banner — reported, not glossed, and NOT an acceptance line.** It is drawn on screen by the renderer and appears **0 times** in my log slice (`"Video memory has been exhausted"` and `"over budget"` both count 0), so it is a pixel-only condition here. Frame timing still read **60 FPS / 16.7 ms** on the same frame, the match ended normally, and every instrument above returned live data. Before PIE, `is_pie_active` reported GPU `budget 7123 MB / usage 2521 MB / available 4601 MB` on an RTX 5070 Laptop. **Cause unmeasured** (H2).

## `.sav` NET ZERO — measured before AND after MY run

**5 files before, 5 after, every one byte-identical with its mtime untouched:**

| file | sha256 (head) | bytes | mtime (unchanged both reads) |
|---|---|---|---|
| `SiegeAccounts.sav` | `2fd96fe18af9a4cf…` | 3083 | 2026-08-28 22:43:52.307531 |
| `SiegeDecks.sav` | `646d442fc11c2770…` | 3814 | 2026-08-02 10:09:34.952297 |
| `SiegeDecks_4E46A9EE49D3A7C91C583B8457E1EE34.sav` | `d81f2b64f2b58cde…` | 6518 | 2026-09-20 01:22:04.358541 |
| `SiegeSettings.sav` | `c8555088e2918c51…` | 2004 | 2026-08-04 22:24:38.554086 |
| `SiegeSettings_4E46A9EE49D3A7C91C583B8457E1EE34.sav` | `684ae64f9378bdf3…` | 2056 | 2026-09-09 13:48:19.329635 |

**NET ZERO = TRUE for this run.** ⚠️ **NAMED RATHER THAN SMOOTHED:** `SiegeDecks_4E46A9EE…sav` **differs from LIMB A's recorded value** (Limb A: `7ad6029879fc8a25…` / 6,521 B / `2026-09-18 17:31:50`). ⛔ **That write is not mine.** Its mtime is **`2026-09-20 01:22:04`** — **71 seconds after 🧑 his `01:20:53` asset save and inside the window of his `-game` session** (which closed at `01:26:53`) — i.e. ~70 minutes **before** my PIE started. It was already at `d81f2b64…` in my **pre-run** baseline and is unchanged in my post-run one. **I record the delta because a later reader comparing the two limbs would otherwise find an unexplained change and have to re-derive it.**

Also measured after the run: **dirty content packages = 0, dirty map packages = 0** (nothing to decline, `VER-§2` cl. 5) · `WBP_VictoryScreen.uasset` re-read **identical** · no `.uasset` opened, compiled or saved · no level loaded or saved · no Git command run · no code or asset edited · **no editor launched, closed or restarted** · 🧑 his `-game` instance was already gone and was never touched.

## 🧑 RECORDED BESIDE MY FINDINGS, NEVER MERGED INTO THEM (`VER-§8` cl. 4 · AMENDMENT C4(c))

**(7c) — ANSWERED. 🧑 His words, verbatim, 2026-09-19 evening:**

> ***"I can confirm that pressing enter does trigger the 'play again' button and cause a match restart."***

And the (4c) answer that AMENDMENT C5 turned the mouse-exclusion premise from an assertion into a measurement:

> ***"yes, clicking Play Again does restart the match"***

🚨 ⛔ **THESE DO NOT MAKE MY VERDICT `VERIFIED` AND ARE NOT COUNTED IN ITS DERIVATION.** My `VERIFIED` rests on (7b)-ii and (7b)-iii alone, both measured by the rig. His answer measures the half **no rig here can reach** (`VER-§8` cl. 9 — a cursor/key tool would synthesise the input; his claim is about a **player's** press).

⭐ **BUT AS AN INDEPENDENT CHECK, HIS HANDS AND MY INSTRUMENTS AGREE, AND THE AGREEMENT IS WORTH MORE THAN EITHER ALONE.** The dispatch fixed this test in advance: *if my instruments said focus never reached the button, that disagreement would itself be the finding.* **There is no disagreement to report.** They are genuinely independent — he pressed a physical `Enter` in a `-game` build and watched the arena come back; I read a Slate focus flag and a log's silence in a PIE editor build, and I pressed nothing. **They meet at the same conclusion from opposite ends: the button is now keyboard-reachable, and reaching it restarts the match.** The corroboration is also **timed**: his `-game` session (`GitClaudeUnrealTest_2.log`, command line `-game -windowed -ResX=3200 -ResY=1800`) ran and exited at `01:26:53`, **after** the `01:20:53` save whose hash I measured — so his answer was taken against **the same asset bytes I verified**, not against a pre-save build. ⛔ **Still recorded beside, never merged.**

## Hypotheses (not verdicts)

- **H1 — the mechanism.** `SButton::SupportsKeyboardFocus()` returns `bIsFocusable`, baked from the UMG property at `RebuildWidget()`; ticking `Btn_Jump` → `Is Focusable` therefore made the guard's predicate true, so the `else` branch ran and `SetWidgetToFocus(Btn_Jump->TakeWidget())` executed. **HYPOTHESIS about the internal route.** What is **measured** is the pair of end states: the guard's `Warning` is gone and focus is on `Btn_Jump`. I did not step the predicate.
- **H2 — the VRAM banner.** Unmeasured. Candidates: the editor + PIE sharing a 7.1 GB budget already at 2.5 GB before PIE; the 1086-px capture path's transient render targets. Nothing in this report depends on it and no instrument misbehaved.
- **H3 — Slate's focus brush.** The button may be drawing a focus visual too subtle to resolve at 1086×615, or this button style may draw none. **Not measured, and deliberately not used as evidence in either direction** (see the Evidence note).
- **H4 — why Limb A's bot never summoned.** Still unisolated; my run did not need it and I budgeted nothing on it, per C1. Left named so it is not re-traced blind.

## Not examined / limitations this run

1. **The keypress half of (7b) was not attempted at all** — pre-authorised `UNOBSERVABLE`, named dead end, **zero** budget spent (`VER-§5` cl. 5 · C2(ii)).
2. **I did not measure that `Enter` activates the button.** Focus is a **precondition** for it, not the same claim. 🧑 His (7c) answer covers that half and is recorded beside, not merged.
3. **1,960 of `Castle_Blue`'s 2,000 HP were removed by my staged property write.** The organic portion is the final **40** — now measured as **three blows of 15** — and, critically, the **0-crossing event** itself.
4. **`pie_scene_edit` `spawn_actor` / `set_actor_property` / `set_actor_transform` were reached as STEPS inside `run_verification_sequence`** — census §5 names on neither my `tools:` line nor `permissions.allow`; reported exactly as `VER-§7` cl. 2 requires. **Everything written was PIE-transient**: 0 dirty packages after the run, nothing reached disk.
5. **One match end, one PIE session, standalone only.** Not repeated; **not** tested in listen-server or client net modes.
6. **A Defeat, not a Victory.** Same as Limb A and `TASK-1311`. Both branches run the same `HandleMatchEnd` and the same focus block, but that is **reasoning, not a second measurement**.
7. **Scope of the zero.** Scoped to the **59,875-byte** slice this PIE instance appended to `Saved/Logs/GitClaudeUnrealTest.log`.
8. **Positive-control count is mine (`SC-§91`):** **33 hits / 28 files of 75**. Limb A read 33/28 of 73; `TASK-1311` read 36/30 of 72. Log rotation is the obvious explanation, **unmeasured**, and nothing depends on it — the control **fired**, which is all a control is for, and its chronologically-last emission matches the one the dispatch named.
9. **`SC-§118` census is `PARTIAL, by cl. 9`** — but note it is **stronger than Limb A's**: I have the running editor's **actual command-line string**, not merely an empty field. The unreachable half is a full OS process enumeration.
10. **I flipped only `TASK-1314`'s own `status:` line** and amended only this report. No other row, no other agent's report, no source, no asset, no `CONVENTIONS.md`. I did not commit — `TASK-1316` owns 5c.

Law applied: `VER-§0` · `VER-§1` cl. 1/2/3/4/5/6/**7** · `VER-§2` · `VER-§3` · `VER-§4` cl. 1/2/5 · `VER-§5` cl. 1/2/3/5 · `VER-§6` cl. 5 · `VER-§7` cl. 2/4 · `VER-§8` cl. 1/2/3/4/7/8/9 · `SC-§38a` · `SC-§39` · `SC-§68` · `SC-§82` · `SC-§91` · `SC-§101` · `SC-§118` cl. 8/9 · `SC-§125`.

---

# ⬇️⬇️ LIMB A — PRESERVED WHOLE BENEATH THE AMENDMENT (`VER-§1` cl. 6/7 — every attempt's evidence is kept)

⛔ **Everything below this line is LIMB A exactly as written on 2026-09-19 and is NOT the current verdict.** Its historical line 1 read `Verdict: UNOBSERVABLE`; the live verdict is line 1 of this file. Limb A's measurements stand unaltered — they are the **pre-save control** against which LIMB B's readings are the contrast.

# Verification — TASK-1314 (PLAYAGAIN-KEY-REACHABLE), acceptance (7b) — **LIMB A of two: the PRE-SAVE CONTROL**

verifier: playtest-verifier · 2026-09-19 · subject `TASK-1314` (`built`) · gate `TASK-1315` (PASS) · host `TASK-1316`
Reads performed whole (`SC-§38a`): TASKBOARD `#### TASK-1314` (`status:` + ROUTE K-2 + the `Btn_Jump` lines + KEY CENSUS + RIDERS 1(a)–(f), 2, 3) · AMENDMENT B3 · acceptance (7)(a)/(b)/(c) · `handoffs/TASK-1314-programmer.md` · `qa/TASK-1311-verify.md` (method reused, not re-derived) · `CONVENTIONS.md` `VER-§0`–`VER-§8` · `SiegePlayerController.cpp:2285-2327`.

## 🚨 READ THIS BEFORE THE VERDICT WORD — WHY IT IS `UNOBSERVABLE` AND NOT `VERIFIED`

This run is **AMENDMENT B3's free positive control**, taken deliberately **BEFORE** 🧑 Jonathan's one-property save. I measured that side of the line **first, objectively, and twice** (before and after the run):

`Content/UI/WBP_VictoryScreen.uasset` = sha256 **`7817dd7f19bcf1028bd21ddfc4d92731c01c14d567c19641cc10e9f1d12bd6ee`**, **153,489 bytes**, mtime **`2026-07-29 12:23:05`** — **byte-identical to the pre-image** at both instants ⇒ **`Btn_Jump.IsFocusable` is still `False`; the asset half has NOT landed; I am in the control limb and he did not save mid-flight.**

⚖️ **`VER-§` HAS NO REPORT SHAPE FOR A TWO-LIMB VERIFICATION, AND I AM SAYING SO RATHER THAN FORCING A TOKEN.** `VER-§1` cl. 5's derivation is mechanical — *no `fail` and ≥1 `pass` ⇒ `VERIFIED`* — and this run has one genuine `pass` (the regression line) and no `fail`. **Applied literally it would put `Verdict: VERIFIED` on line 1 of TASK-1314's report**, and `head -1` IS the verdict (`VER-§1` cl. 1). Every later reader would read that as *"Play Again is keyboard-reachable, proven"* — which is **false today and cannot be true until he saves**. That collides head-on with `VER-§0` cl. 2 (*a `VERIFIED` is "the engine, driven this way, **did this**"*). `VER-§5` cl. 4 (*partial rows are NOT `UNOBSERVABLE`*) does not fit either, because the unobserved line is not unobserved through partiality — it is unobserved because a **declared, dated, human-owned precondition is not in the build**.

⇒ **I chose `UNOBSERVABLE`**, which is the only token that is true of the row's headline deliverable on this run, **never blocks** (`VER-§6` cl. 5(ii)), leaves the row at `built` for its host (`VER-§5` cl. 2), and claims **neither** a pass **nor** the `VERIFY-FAILED` the board expressly forbids here. **Proposed amendment is at the end of this report.**

⛔ **THIS FILE IS THE CANONICAL PATH ON PURPOSE.** The verdict limb (post-save) **AMENDS THIS SAME FILE** rather than landing beside it, so `head -1 qa/TASK-1314-verify.md` never points at a stale or missing verdict. Limb A's measurements are kept whole underneath it (`VER-§1` cl. 6 — every attempt's evidence is kept).

## Editor/Aura state

| item | measured at my own instant (`SC-§91`) |
|---|---|
| Aura / MCP | **connected** — `get_headless_status` not needed: a read-only Python probe and 20+ `unreal_editor` PIE calls all returned live data |
| map | `/Game/Maps/L_Arena.L_Arena` — **already the open level; I did NOT call `load_level`** |
| editor instance | **PID 6764** — `os.getpid()` inside the Aura-hosted editor process reads **6764**, matching the dispatch. Corroborated: `Saved/Logs/cef3.log` line 1 is prefixed `[6764:17752:...]`, i.e. the live CEF child belongs to 6764 |
| ⚠️ identified by COMMAND LINE (`SC-§118`) — **PARTIAL, AND NAMED** | `LogInit: Command Line:` in PID 6764's own log is **EMPTY** (GUI launch via the `.uproject` association) ⇒ **`-game` token absent, `-RenderOffScreen` absent, no switches at all**. ⛔ I could **not** run a full OS process enumeration: the read-only Python lane **rejects `subprocess`** (*"Aura disallows import of subprocess module"*) and **rejects `ctypes`/`windll`**. Substitute census, stated as what it is: of **73** files in `Saved/Logs`, **exactly one** was being written (`GitClaudeUnrealTest.log`, age **0.0 s**); the next-freshest was **387 s** stale ⇒ **exactly one live UE instance**, and it is the one I drove. `is_pie_active` reported `num_instances: 1`. **Nothing of 🧑 his was driven, and nothing was closed, launched or restarted.** |
| Build Configuration | **`LogInit: Build Configuration: Development`** — quoted because the regression count below is worthless in a Shipping/Test log |
| Engine | `5.8.0-55116800+++UE5+Release-5.8` |
| PIE mode | standalone, 1 client, windowed 1280×720 (viewport 1280×725); session ran **863.6 s** of game time |
| log file NAMED (`VER-§1` cl. 2) | `Saved/Logs/GitClaudeUnrealTest.log`, line 1 `Log file open, 09/19/26 18:54:06`. **My slice = bytes 362,367 → 723,102 = 360,735 bytes / 2,053 lines**, all of it written by this PIE session |
| attempts used | **1 of 3** (one PIE session; several input routes tried inside it) |
| wall time | **≈ 25 min** — measured, not estimated: my first pre-flight probe stamped `2026-09-19 19:03:41` and my closing probe stamped `2026-09-19 19:24:52`; PIE itself ran `18:54:06`-based log clock from `02:04:52` to `02:17:59` UTC (the log is UTC, local is UTC−7) |
| credit | not surfaced by any tool this run |
| 🧑 his `VER-§3` go | **GIVEN**, recorded in the dispatch: he was asked how to run this leg and chose *"Bank the free control first"*, which the question stated was his go for driving PIE |

**Binaries — proven, not assumed:** `Binaries/Win64/UnrealEditor-GitClaudeUnrealTest.dll` mtime **`2026-09-19 18:51:19`**, sha256 `e7b37c64bf55b401434435ff424697de10e43835d3cbe04f7e76d540a67377c2`; the editor's log opened **18:54:06**, i.e. **after** the DLL was written ⇒ the running editor carries `TASK-1314`'s compile. Corroborated at runtime by the guard `Warning` below, which **exists only in this diff**.

## Acceptance lines → observations

| # | acceptance line | observable chosen | observed (quoted values, evidence path) | pass/fail/unobs |
|---|---|---|---|---|
| **(7b)-i** | *"`UNOBSERVABLE` IS PRE-AUTHORISED ON THIS LEG … `simulate_key_press` is delivered to the player controller and NOT to Slate … the verifier does NOT re-trace that named dead end"* | none — **deliberately not attempted** (`VER-§5` cl. 5, `VER-§8` cl. 1) | I pressed **no** key at the victory screen and make **no** claim about whether `Enter` activates Play Again. The ceiling was declared at boarding and I spent **zero** budget re-proving it. | unobs |
| **(7b)-ii** | *"at a PIE that ACTUALLY REACHED A MATCH END on `L_Arena`, read WHICH widget holds Slate keyboard focus … BEFORE/negative = `SViewport` / the game-viewport widget. AFTER/positive = the Play Again button (`Btn_Jump`) BY NAME"* | `ui_snapshot` on `WBP_VictoryScreen`, every node's `focused` field — the instrument that returned `"focused": true` for `WBP_DeckBuilder_C_0` three times at `TASK-1306` | **Match end REACHED** (chain below). At **t=13m23s** (PIE `803.98 s`), **16 nodes walked, `nodes_truncated: false`, and `focused: false` on EVERY ONE** — root `WBP_VictoryScreen_C_0`, `Overlay_19`, `SizeBox_0`, **`Btn_Jump`**, `TextBlock_0` (text **`"Play Again"`**), `TextBlock_1` (text **`"Defeat"`** — winner Red, correct), both `UI_Thumbstick_C`. `Btn_Jump` read `visibility: Visible`, `hit_testable: true`, `enabled: true`, geometry **174.37 × 53.65** at abs (1840.32, 1337.30). ⇒ **focus is NOT on the Play Again button and NOT anywhere in the victory screen's tree.** Evidence: `.claude/pipeline/playtest-evidence/2026-09-19/VER-TASK-1314-t13m23s-focus-holder-at-match-end.png` (promotion **OWED**, `VER-§4` cl. 1). 🚨 **RECORDED AS *"ASSET HALF NOT YET LANDED"*, ⛔ NOT AS A `VERIFY-FAILED`** — the row's own status line, `handoffs/TASK-1314-programmer.md` §(7b), and the sha256 above all say this is the **correct and expected** pre-save reading. ⛔ It is equally **not a pass**. | unobs |
| **(7b)-iii** | *"AND the engine string `InputMode:UIOnly - Attempting to focus Non-Focusable widget` must STILL be 0 — `TASK-1311`'s gain may not regress"* | exact-string count in the **named** `Development` log slice this PIE instance wrote, plus a firing positive control and two negative controls | **0**, in a slice in which the emitter site **provably executed** (the `match ended` line below is the last statement of `HandleMatchEnd`). Broad sweep: `Non-Focusable` = **0**, `NonFocusable` = **0**. The slice is not silently empty — it carries **17 `: Error:` lines** (all `LogLiveCoding: Error: Cannot enable module …ggml-cpu-*.dll`, pre-existing, unrelated). **Positive control (`SC-§39`): the SAME reader finds the string 33 times across 28 of the 73 `Saved/Logs/*.log` files**, last emission `[2026.09.19-00.31.19:252]` in `GitClaudeUnrealTest_2.log`, verbatim `LogPlayerController: Error: InputMode:UIOnly - Attempting to focus Non-Focusable widget SObjectWidget [Widget.cpp(976)]!` — **pre-`1c93610`**, naming `SObjectWidget`, the root wrapper `TASK-1311` stopped focusing. **Negative control:** the same reader scored **0** for a bogus variant (*"…Non-Focusable gizmo"*) across all 73 logs. | **pass** |

⚖️ **Derivation, stated openly:** 1 `pass`, 0 `fail`, 2 `unobs`. `VER-§1` cl. 5 would emit `VERIFIED`; I emit `UNOBSERVABLE` for the reason set out at the top. The `pass` on (7b)-iii is **real and stands on its own** — `TASK-1311`'s gain did not regress — and it is the reason the code half is safe to commit.

## ⭐ RIDER 1(f) — THE COUPLING. **VERDICT: AGREE. NO CONTRADICTION. NO STOP.**

RIDER 1(f) makes the guard's `Warning` a **one-bit instrument on the asset half** that must **agree** with the focus read. It does, and the two lines are 2 ms apart in the same function:

```
[2026.09.20-02.17.59:041][137]LogGitClaudeUnrealTest: [SiegeGameMode_0] Castle 'Castle_0' (Blue) destroyed — match over, winner: Red.
[2026.09.20-02.17.59:043][137]LogGitClaudeUnrealTest: Warning: ASiegePlayerController 'SiegePlayerController_0': victory screen button 'Btn_Jump' does not support keyboard focus (IsFocusable is False on it in WBP_VictoryScreen) — no focus target set, Play Again stays mouse-only (TASK-1314 Route K-2 is owed).
[2026.09.20-02.17.59:043][137]LogGitClaudeUnrealTest: ASiegePlayerController 'SiegePlayerController_0': match ended — winner Red.
```

| instrument | reading | predicted BEFORE his save (RIDER 1(f)) |
|---|---|---|
| guard `Warning` *"does not support keyboard focus"* in my slice | **1** (exactly once, at the match end) | **FIRES** ✅ |
| `ui_snapshot` focus on `Btn_Jump` | **`false`** (and false on all 16 nodes) | **NOT on the button** ✅ |
| engine `Error` *"Attempting to focus Non-Focusable widget"* | **0** | structurally impossible either way ✅ |
| lookup-failure `Warning` *"victory screen has no widget named"* | **0** | — |

⇒ ⭐ **The two independent instruments AGREE, and they agree on the pre-save limb.** RIDER 1(f)'s two contradiction shapes — a `Warning` present *with* focus on the button, or absent *with* focus off it — **did not occur**. Three further things this pins down that no static read could:
1. **`Btn_Jump` is the right name and it RESOLVES at runtime** — the lookup-failure branch scored **0**, so `GetWidgetFromName(TEXT("Btn_Jump"))` returned a live widget. The `TASK-1286` inert shape is **absent at runtime**, not merely absent in review.
2. **The guard branch is the one that ran**, and it ran **inside** `HandleMatchEnd`, between the destruction line and the function's last statement.
3. **The `Warning`'s own text corroborates the sha256 from the other side:** the running engine says `IsFocusable is False on it in WBP_VictoryScreen`. The disk hash and the live Slate object agree that the asset half is not in.

## Was a real match end reached? — **YES.** The chain, measured, with the staging declared

Method reused from `qa/TASK-1311-verify.md` rather than re-derived, as the dispatch directed; ⛔ **its route did not reproduce and I had to build a different one — that is the single most reusable finding in this report** (see Ceilings, below).

1. **`TASK-1311`'s organic route did NOT reproduce on this seed.** For the first **585 s** of PIE the actor count sat at **153**, both castles at **2000.0**, and `LogSiegeBot` emitted **exactly one line in the whole session** — the deck select (*"chose curated deck 1 of 2 'Bot Defensive Economy' (50 cards, avg cost 21.06)"*). **Zero units were summoned by either side.** `TASK-1311` had four marching Red units and 300 HP of organic damage by t≈205 s.
2. **Staging, declared exactly** (`pie_scene_edit`, live PIE world only, nothing on disk):
   - t=585.64 — `spawn_actor` `/Game/Blueprints/Units/BP_Unit_Knight.BP_Unit_Knight_C` → `BP_Unit_Knight_C_0`. It read `Team: Blue`, `AttackDamage: 15.0`, `AttackRange: 120.0`, `CurrentHP/MaxHP: 200.0`, and **drove itself** (moved ~430 uu unaided within 2 s).
   - t=654.70 — `Castle_Red.CurrentHP` `2000.0 → 40.0`. **Reverted at t=709.01 (`40.0 → 2000.0`)**; the Red castle finished the run at full HP and **played no part in the match end**.
   - t=709.01 — `Castle_Blue.CurrentHP` `2000.0 → 40.0`. ⛔ **1,960 HP were removed by my staged write.**
   - t=763.80 — `BP_Unit_Knight_C_0.Team` `Blue → Red`, and its transform set to (−21250, 0, 120), at the Blue castle's gate.
3. **The last 40 HP came off through the real `ACastle::TakeDamage` path, and no write of mine touched `Castle_Blue` after t=709.01.** Reads at **t=722.89** and **t=743.28** both returned `CurrentHP 40.0, bDestroyed false`. The **only** change between then and the destruction was the Knight turning hostile at the gate. At t=775.63 the castle read **`CurrentHP 0.0, bDestroyed true`**, and at the same instant the GameMode logged the destruction. ⛔ **A property write cannot produce `bDestroyed = true` or the GameMode line** — `CurrentHP` is *"mutated only by TakeDamage and ResetCastle"* (`Castle.h:883`) and the destroy event fires inside `TakeDamage`. **The 0-crossing was organic; the grind to reach it was not.**
4. **Geometry corroborates:** at t=775.65 the Knight stood at **x = −21,305.48, y ≈ 0** with the castle's measured colliding near face at **x ≈ −21,308** (castle x −25,000, half-extent 3,692 per the GameMode's own log line) ⇒ **≈ 2.5 uu from the wall, far inside `AttackRange 120`.**
5. **The engine agrees, in the game's own log** — the three lines quoted above, ending with `HandleMatchEnd`'s **last statement** (`SiegePlayerController.cpp`, the `match ended — winner Red` line), which is what makes (7b)-iii's zero **load-bearing rather than merely true** (`SC-§39`).
6. **The widget layer agrees:** `WBP_VictoryScreen_C_0` was live in the world, `TextBlock_1` = **"Defeat"** (winner Red — correct), `Btn_Jump/TextBlock_0` = **"Play Again"**.

⚠️ **What I did NOT capture, named rather than glossed:** the **per-tick 15-damage steps**. My `start_state_recording` on `Castle_Blue.CurrentHP` hit `max_samples` (1,200 rows @ `every_n_ticks 2`, `dt 0.0333`) at **t≈752.7 s — 11 s before the team flip** — so all 1,200 rows read a flat `40`. *"Three blows of 15"* is **arithmetic from `AttackDamage 15.0` against a staged 40, not a measurement.* What **is** measured is the bracket: `40.0 / false` at t=743.28 → `0.0 / true` at t=775.63, with only the hostile Knight in between. `TASK-1311` did capture its two organic steps; **I did not, and I do not claim them.**

## Evidence (promoted)

Promotion is **OWED to the 5c host** (`VER-§4` cl. 1 — the verifier holds no tool that moves bytes; ⛔ I do not claim a path that does not yet exist):

- **SOURCE** `Saved/AuraVerify/pie_composited_c2_t803.98s_f82270.png` (1,541,948 bytes, 1086×615, PIE `game_time_seconds = 803.98`, frame 82270, `mean_luma 121`, `pct_near_black 0.0047`)
  → **TARGET** `.claude/pipeline/playtest-evidence/2026-09-19/VER-TASK-1314-t13m23s-focus-holder-at-match-end.png`
  — the composited end screen at a real match end: **"Defeat"** in white outlined text at centre-frame over the hero's back, and directly below it the pale grey **"Play Again"** pill (≈174 × 54 px) carrying **no focus outline, no highlight and no glow of any kind**; `Gold: 999` top-left, `60 FPS / 16.7 ms` top-right, sunlit grass filling the frame because the camera is the pitched-down (−60°) pose I set while probing card placement. **The unhighlighted pill is the pixel side of the same fact the `ui_snapshot` and the guard `Warning` report: nothing on this screen holds keyboard focus.**

Kept in `Saved/` (gitignored, **not** promoted, `VER-§4` cl. 5): `Saved/AuraVerify/pie_composited_c1_t469.06s_f63278.png` — the placement ghost probe at t=469 s.
⚠️ The auto-armed background film (`Saved/AuraVerify/rec_1789869892739915900_1`) returned a **2.4 MB manifest** but its directory contains **0 PNGs** at post-run inspection; I did not chase this and promote nothing from it (see Not examined).

## `.sav` NET ZERO — measured before AND after

**5 files before, 5 after, every one byte-identical with its mtime untouched:**

| file | sha256 (head) | bytes | mtime (unchanged both reads) |
|---|---|---|---|
| `SiegeAccounts.sav` | `2fd96fe18af9a4cf…` | 3083 | 2026-08-28 22:43:52.307531 |
| `SiegeDecks.sav` | `646d442fc11c2770…` | 3814 | 2026-08-02 10:09:34.952297 |
| `SiegeDecks_4E46A9EE49D3A7C91C583B8457E1EE34.sav` | `7ad6029879fc8a25…` | 6521 | 2026-09-18 17:31:50.121201 |
| `SiegeSettings.sav` | `c8555088e2918c51…` | 2004 | 2026-08-04 22:24:38.554086 |
| `SiegeSettings_4E46A9EE49D3A7C91C583B8457E1EE34.sav` | `684ae64f9378bdf3…` | 2056 | 2026-09-09 13:48:19.329635 |

**NET ZERO = TRUE.** Also measured after the run: **dirty content packages = 0, dirty map packages = 0** (nothing to decline, `VER-§2` cl. 5) · `WBP_VictoryScreen.uasset` re-read **identical** · no `.uasset` opened, compiled or saved · no level loaded or saved · no Git command run · no code or asset edited · no editor launched, closed or restarted.

## 🚨 CEILINGS MEASURED THIS RUN — NEW NAMED DEAD ENDS, so no later row re-derives them

1. ⛔ **THE BOARD'S NAMED DAMAGE LEAD DOES NOT EXIST.** (7b) names *"`SiegeCheatManager`'s `UFUNCTION(exec) void ApplyTestDamage(float Amount)`"* as the candidate route to a match end, with both halves declared unmeasured. **I measured it: a case-insensitive grep for `CheatManager|UFUNCTION\(\s*[Ee]xec|ApplyTestDamage` across `Source/` returns ZERO hits.** There is no cheat manager and no exec function anywhere in the project. **The lead is not merely unmeasured — it is absent.** (`SC-§101`: a prescribed remedy is a claim.)
2. ⛔ **CARD PLACEMENT IS NOT VERIFIER-ACTUABLE TODAY, AND THE REASON IS SPECIFIC.** Entering placement works: `inject_input_action IA_Card1` produced *"placement mode entered for card 'Cleric' (cost 18)"*. **Confirming does not.** The confirm is **polled from the RAW key state** — `if (WasInputKeyJustPressed(EKeys::LeftMouseButton)) { TryConfirmPlacement(); }` at `SiegePlayerController.cpp:921-924` — so ⛔ `inject_input_action IA_Attack` **can never confirm a placement** (it sets the Enhanced Input action, not the key state). ✅ **`simulate_key_press "LeftMouseButton"` DOES reach it** — proven by the refusal lines it produced. But the placement then fails one layer deeper: `UpdatePlacementGhost` (`:2963-2966`) takes its position from `TraceCursorToGround` (`:4999-5005`, `GetHitResultUnderCursor`), and the **OS mouse cursor is not over the PIE game window**, so every confirm logged *"placement click refused for 'Cleric' — no ground hit, outside the Blue spawn box and any Blue-owned capture zone, or off the navmesh"* — **3 times, at 3 different camera pitches**. The verifier holds no tool that moves the OS cursor into the PIE viewport (`ui_perform` restores the cursor at the end of its own call). ⇒ **Playing a card from hand is a NAMED DEAD END until a cursor-positioning route exists.**
3. ⛔ **`TASK-1311`'s ORGANIC-RED-UNITS ROUTE IS NOT RELIABLY REPRODUCIBLE.** On this seed the bot summoned **nothing in 13 minutes** (`LogSiegeBot`: one line, the deck select; actor count flat at 153 for 585 s; both castles at 2000.0). A later row must **not** budget on the bot attacking. ✅ **The route that DID work, for reuse:** `pie_scene_edit spawn_actor` a `BP_Unit_Knight_C` → `set_actor_property Team = "Red"` → `set_actor_transform` to the target castle's gate (≈2 uu off the measured colliding face) → it attacks organically within ~12 s. **Note the ordering trap I hit: the unit spawns `Team: Blue` and a Blue unit will never damage the Blue castle** — flip `Team` *before* expecting damage.
4. ⛔ **THE READ-ONLY PYTHON LANE BLOCKS BOTH PROCESS-CENSUS ROUTES.** `import subprocess` → *"Aura disallows import of subprocess module"*; `import ctypes` → *"Aura disallows import of ctypes module"* + *"disallows use of windll/shell32"*. ⇒ **A full `SC-§118` command-line census is not reachable from the verifier's granted read-only lane.** The substitute I used (single live log-writer + the driven process's own `os.getpid()` + the log's own `LogInit: Command Line:`) is weaker and is labelled as such above. 📌 **LEAD, not law (`SC-§101`):** if `SC-§118`'s census is to be a *verifier* duty, the lane needs a granted read-only process-list tool, or the duty needs restating for agents without a shell. That is the manager's call, not mine.

## Hypotheses (not verdicts)

- **H1 — the focus holder is specifically `SViewport`.** `ui_snapshot` proves focus is **not** anywhere in `WBP_VictoryScreen`'s tree; it **cannot name** a widget outside that tree. That the holder is the game-viewport widget rests on the inherited static chain (`FInputModeDataBase::SetFocusAndLocking`, `PlayerController.cpp:6313-6317`, issues `SetUserFocus` **only** when the pointer is valid — and the guard left it unset — so `SetFocusAndLocking` falls through to the viewport widget; `SViewport::SupportsKeyboardFocus()` is `override { return true; }`). **HYPOTHESIS. I measured the NEGATIVE half of B3's named pair (`not Btn_Jump`), not the positive identification of `SViewport`.**
- **H2 — why the bot never summoned.** Candidates, none measured: the curated *"Bot Defensive Economy"* deck's play policy; a gold threshold never crossed; the bot's decision tick gated behind something that did not fire. I did not isolate it and I make no claim. Named so it is not re-traced blind.
- **H3 — the empty background-film directory.** The recorder announced `recording_armed` at PIE start and `stop_pie_recording` returned a 2.4 MB manifest, yet the output dir held 0 PNGs — consistent with the NVENC video path (`LogAura: [recorder] NVENC first present: source=viewportRT size=1280x725`) writing a clip instead of stills. **HYPOTHESIS**; I did not open the manifest and nothing in this report depends on it.

## 🧑 RECORDED BESIDE MY FINDINGS, NEVER MERGED INTO THEM (`VER-§8` cl. 4)

**(7c) — his one-sentence hand check, verbatim, parenthesis intact, and ⛔ ONLY VALID AFTER his save:**

> ***"Play a match to the end on the arena, then — without touching the mouse at all — press Enter once: does the match actually restart (arena back, units gone, gold reset)? (A highlight or outline appearing on 'Play Again' is a NO — that is Slate's own navigation and it draws with every line of this feature dead. The screen just sitting there is a NO. And if you moved or clicked the mouse at any point the answer does not count, because the click already worked before this change.)"***

**NOT ANSWERED — still owed.** ⛔ Asking it before he flips `Btn_Jump.IsFocusable` guarantees a "no" that means nothing. *Adjacent observation, recorded beside his check and NOT merged into it:* `Btn_Jump` read `visibility: Visible`, `hit_testable: true`, `enabled: true`, 174.37 × 53.65 px at abs (1840.32, 1337.30) — the button is **on screen and hit-testable**; that says nothing about whether a key reaches it, and nothing about whether a click restarts the match.

**His step, unchanged:** `/Game/UI/WBP_VictoryScreen` → select `Btn_Jump` → Details → Interaction → tick **`Is Focusable`** → **Compile** → **`Ctrl+S`**. Nothing else. The discharge is measured against **`7817dd7f…2bd6ee` / 153,489 bytes** — a save that leaves the sha **equal** did not happen.

## ⚖️ PROPOSED AMENDMENT — a shape for a two-limb verification (the manager's to accept or refuse, `SC-§82`)

`VER-§8` cl. 2's `.uasset` twin already makes *"a row whose deliverable is a compiled Blueprint carries a human keystroke"* a **boarding** duty. What it does **not** yet give is the **verifier's** report shape when that keystroke is deliberately deferred and a pre-save run is banked as a control. I propose adding to `VER-§1`:

> **cl. 7 — THE TWO-LIMB VERIFICATION.** When a row's runtime acceptance is gated on a 🧑 human asset write that has **not** landed at 5b, the verifier writes the **canonical** `qa/TASK-###-verify.md` with `Verdict: UNOBSERVABLE` on line 1 and the H1 marked **LIMB A**, records the **objective side-of-the-line measurement** (the pre-image sha256 + byte count) in the first section, and marks every gated acceptance line `unobs` with the reason *"asset half not yet landed"* — ⛔ **never `fail`** (the code is behaving as designed) and ⛔ **never `pass`**. Lines **not** gated on the write are scored normally, and a `pass` among them does **not** promote the verdict to `VERIFIED` — `VER-§1` cl. 5's derivation is **suspended for a two-limb row**, because a `VERIFIED` on line 1 would read as a claim about the gated feature. The post-save limb **AMENDS THE SAME FILE** (re-writing line 1 and appending LIMB B), so `head -1` is never stale. The row carries `verify: unobservable — limb A/B pending 🧑 <the write>`.

⛔ I did **not** write this into `CONVENTIONS.md` — that file is the manager's alone.

## Not examined / limitations this run

1. **The keypress half of (7b) was not attempted at all** — pre-authorised `UNOBSERVABLE`, named dead end, zero budget spent (`VER-§5` cl. 5).
2. **The focus holder is identified only negatively.** `not Btn_Jump`, `not anywhere in WBP_VictoryScreen`. The positive name `SViewport` is H1, not a measurement.
3. **1,960 of `Castle_Blue`'s 2,000 HP were removed by my staged property write.** The organic portion is the final **40** and, critically, the **0-crossing event** itself. The per-tick steps were **missed** (recording expired 11 s early) — see the ⚠️ above.
4. **A `pie_scene_edit` `spawn_actor` + `set_actor_property` + `set_actor_transform` chain was reached as STEPS inside `run_verification_sequence`** — census §5 names on neither my `tools:` line nor `permissions.allow`, reported here exactly as `VER-§7` cl. 2 requires of N1/N4. **Everything I wrote was PIE-transient**: `L_Arena` was not dirtied, 0 packages dirty after the run, nothing reached disk.
5. **One match end, one PIE session, standalone only.** Not repeated; **not** tested in listen-server or client net modes.
6. **A Defeat, not a Victory.** Same as `TASK-1311`. Both branches run the same `HandleMatchEnd` and the same focus block, but that is reasoning, not a second measurement.
7. **Scope of the zero.** Scoped to the 360,735-byte / 2,053-line slice this PIE instance appended to `Saved/Logs/GitClaudeUnrealTest.log` (the pre-PIE 362,367 bytes already contained 0).
8. **Positive-control count differs from both prior readings, and I report mine (`SC-§91`).** I measure **33 hits / 28 files** of 73 logs; `qa/TASK-1311-verify.md` measured **36 / 30** of 72; the dispatch said *"~10 older logs"*. Log rotation between the two instants is the obvious explanation — unmeasured, and nothing here depends on it. The control **fired**, which is all it is for.
9. **`SC-§118` census is partial** — see the Editor/Aura state table and Ceiling 4.
10. **I flipped only `TASK-1314`'s own `status:` line** and wrote only this report. No other row, no other agent's report, no source, no asset, no `CONVENTIONS.md`. I did not commit — `TASK-1316` owns 5c, and `VER-§5` cl. 2 means this `UNOBSERVABLE` **does not block it**.

Law applied: `VER-§0` · `VER-§1` cl. 1/2/3/4/5/6 · `VER-§2` · `VER-§3` · `VER-§4` cl. 1/2/5 · `VER-§5` cl. 1/2/3/5 · `VER-§6` cl. 5 · `VER-§7` cl. 2/4 · `VER-§8` cl. 1/2/3/4 · `SC-§38a` · `SC-§39` · `SC-§68` · `SC-§91` · `SC-§101` · `SC-§118` · `SC-§125`.
