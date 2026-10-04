<!-- ARCHIVED from .claude/pipeline/TASKBOARD.md on 2026-10-04 by Tools/archive_board.py. Every row below was in a terminal state when moved; bytes are unchanged and this comment is the only addition. Law: TASKBOARD.md '## Archive'. -->
#### TASK-273 — Command input assets: `IA_CmdAttack`/`IA_CmdHold`/`IA_CmdDefend` + `IMC_Hero` T/R/E mappings (art, editor)
- assignee: art-director
- status: ✅ **done — COMMITTED `70487d5` (2026-07-24).** 📋 **MANAGER FLIP 2026-09-07, ⛔ 45 DAYS LATE, ⛔ ON GIT EVIDENCE** — `handoffs/BOARD-STALENESS-audit.md` §2 (build-master's git-resolved sweep; ⛔ the manager holds ⛔ NO `Bash`, `SC-§71b`). ⛔ **LOGGED EXCEPTION TO WRITE-DISCIPLINE RULE 1** — `status:` is the ⛔ assignee's field; the ⛔ commit host never flipped it, so this row asserted a falsehood for ⛔ 45 days. ⛔ **BATCH HOST: `TASK-273..277: Shield Wall unit commands (ATTACK/HOLD/DEFEND) — updated W1 build`** (rule 8). ⚠️ **`ready-for-integration` is the ⛔ most dangerous stale value on an ART row: it reads as *"⛔ an integrator still owes this"* — ⛔ an OWED ACTION — where `qa-passed` merely reads as ⛔ incomplete.** ⛔⛔ **A FLIP IS ⛔ NOT A GO** — and the one-circle HOLD flow these keys drove is ⛔ PARTIALLY SUPERSEDED (CONVENTIONS *"Group orders — 3-zone HOLD + AMBUSH"*). Marker `BOARD-STALE-FLIP-2026-09-07`. ← was: ~~ready-for-integration~~
- blocked-by: none — ~~**dispatchable NOW**~~ ⛔ **STRUCK 2026-09-07 BY THE MANAGER: this row ⛔ SHIPPED at `70487d5`; *"dispatchable NOW"* would ⛔ re-run a landed art task.** (parallel with TASK-274; the C++ soft-resolves these by path, null-safe if absent)
- parallel-safe: yes (Content/Input assets only; disjoint from all C++ and from the M7.6 branch's owned files)
- spec: >
    On `m7.6-arena10x` (Jonathan granted editor-close this session). (1) Create three Input Action assets in
    `/Game/Input/Actions/`: `IA_CmdAttack`, `IA_CmdHold`, `IA_CmdDefend` — Value Type **Digital (bool)**, no modifiers,
    matching the existing `IA_Card*` action shape. (2) **BEFORE adding mappings, OPEN `/Game/Input/IMC_Hero` and READ BACK
    its existing key list** — confirm keys **T, R, E** are UNBOUND (manager check says they are free: the only letter-key
    binding is Rally=Q). If ANY of T/R/E is already mapped, STOP and FLAG in 🎨 Art + 🚨 Blockers — do NOT stomp an
    existing binding. (3) Add three mappings to `IMC_Hero` (the SAME context that carries `IA_Card1..6`/`IA_Rally` — do NOT
    create a new IMC): `IA_CmdAttack`→**T**, `IA_CmdHold`→**R**, `IA_CmdDefend`→**E**. (4) Save. Handoff: the readback of
    `IMC_Hero`'s key list proving T/R/E were free + the three new rows added. NOT IN SCOPE: any C++, the HUD, compiling, Git.
    Post the readback + completion in 🎨 Art.
- names: >
    `/Game/Input/Actions/IA_CmdAttack`, `/Game/Input/Actions/IA_CmdHold`, `/Game/Input/Actions/IA_CmdDefend` (Digital/bool);
    mappings in `/Game/Input/IMC_Hero` — IA_CmdAttack↔T, IA_CmdHold↔R, IA_CmdDefend↔E. Law: CONVENTIONS "Unit commands
    (Shield Wall stances)…", "Asset prefixes" (IA_/IMC_ rows).

#### TASK-276 — Optional HUD command indicator + HOLD-point marker (art, editor) — FAST-FOLLOW / non-blocking
- assignee: art-director
- status: done (BUILD-MASTER 2026-07-24 — WBP_HUD stance indicator committed as fast-follow `6e7c706` on m7.6-arena10x (WBP_HUD.uasset ONLY; staged LFS pointer oid sha256:82da25b… verified == worktree; DeckBuilderWidget stayed parked; no push). No compile needed — art-director already compiled GREEN (warnings-as-errors) + saved is_dirty=false. On-screen appearance (renders top-center, flips ATTACK/HOLD/DEFEND on T/R/E, "—" until first command + after Play Again) = Jonathan W1 WATCH. --- Was: ready-for-integration (ART-DIRECTOR 2026-07-24 — stance indicator BUILT + wired + compiled GREEN (warnings-as-errors) + saved in `/Game/UI/WBP_HUD` (`is_dirty=false`). New var `CommandIndicatorText:TextBlock`; new fns `SetupCommandIndicator` (runtime-construct + attach top-center + seed) + `UpdateCommandDisplay` (re-reads `HasIssuedCommand`/`GetCurrentCommand`, enum switch `SwitchonESiegeUnitCommand` -> SetText ATTACK/HOLD/DEFEND, else "—"). Bind = `AssignOnUnitCommandChanged` on the owning `ASiegePlayerController`, appended granularly to the EventTick first-frame tail (auto custom event `OnUnitCommandChanged_Event_0` -> `UpdateCommandDisplay`); seed-then-bind honored; re-read-getters makes Play-Again show "—". EventConstruct byte-intact (write_graph_dsl only on the 2 NEW fns; all EventGraph edits granular create_node/connect_pins). build-master: WBP_HUD is now a saved working-tree change to fold into the commit. HOLD-marker DEFERRED (secondary fast-follow). On-screen appearance = Jonathan pixel/human WATCH (readback has passed on broken UMG here). Detail: handoffs/TASK-276.md. Was: blocked (recipe-needs-rework).)
- blocked-by: TASK-274 (needs `OnUnitCommandChanged` + `GetCurrentCommand`) — and TASK-274 compiled (TASK-277 provides the nodes; if TASK-277 runs first, fold this in before the commit)
- parallel-safe: yes (editor UMG; disjoint from the code tasks — but shares the single editor with TASK-273/277, so sequence in the editor)
- spec: >
    OPTIONAL / fast-follow — the feature integrates WITHOUT it (FLAGGED to Jonathan, Q5). Edit `/Game/UI/WBP_HUD`:
    add a small always-present `CommandIndicatorText` showing the active stance (Attack / Hold / Defend), bound to the
    player controller's `OnUnitCommandChanged` (seed from `GetCurrentCommand()` first, then bind — seed-then-bind law);
    show nothing / "—" while `HasIssuedCommand()` is false. Optionally add a lightweight HOLD-point ground marker
    (reuse a decal or a simple mesh actor placed at `GetHoldLocation()` while the stance is Hold). Keep it minimal and
    readable (§6 bar). NOT IN SCOPE: any C++, the input assets, Git. If Jonathan defers this (Q5), leave `backlog`.
    Post in 🎨 Art.
- names: >
    `/Game/UI/WBP_HUD` element `CommandIndicatorText`. Consumed as-is: `OnUnitCommandChanged`, `GetCurrentCommand`,
    `HasIssuedCommand`, `GetHoldLocation`. Law: CONVENTIONS "Unit commands (Shield Wall stances)…" (HUD indicator bullet).

#### TASK-277 — Integration: compile + PIE command-suite + branch commit (build)
- assignee: build-master
- status: done (BUILD-MASTER 2026-07-24 — Shield Wall command feature integrated on m7.6-arena10x, commit `70487d5` (no push). COMPILE GREEN (`Result: Succeeded` ~13s) after both QA-loop-1 defects were fixed (TASK-274 h:1057 comment + TASK-268 C7595). PIE smoke on L_Arena: feature loads + runs a live match with ZERO errors/ensures/Accessed-None; input actions bound (no missing-IA log); CDO tunables read back correct — HoldRadius=1500, ASummonedUnit.DefendRadius=2500, ACastle.SpawnBoxHalfExtent=(840,840). Behavioral suite (Standard-only gate / ATTACK spawn-box-first / HOLD disc+kite-fix / DEFEND / Play-Again reset / no legacy regression) verified STRUCTURALLY via TASK-274+275 QA PASS + source — live command-driven retarget, T/R/E hardware keys, and on-screen look are Jonathan W1 WATCH items (command state is non-UPROPERTY + no headless input injection, so not machine-drivable). Committed pathspecs: UnitCommand.h, SiegePlayerController.{h,cpp}, SummonedUnit.{h,cpp}, Castle.{h,cpp}, IA_CmdAttack/Hold/Defend, IMC_Hero + CONVENTIONS/board. NOT staged: WBP_HUD (untouched — HUD recipe TASK-276 deferred as fast-follow), DeckBuilderWidget.{h,cpp} (TASK-268 parked). No push. Editor left running for the art-director's HUD pass. Was: blocked.)
- blocked-by: TASK-274 + TASK-275 (both qa-passed) + TASK-273 (IA_/IMC assets exist); TASK-276 optional (fold in if done, else commit without it)
- parallel-safe: no (single editor + compiler + Git)
- spec: >
    On `m7.6-arena10x`. (1) Compile the TASK-274/275 C++ (editor bounce as usual — Jonathan's close/reopen grant covers
    this session); GREEN, report time + `Result: Succeeded`. (2) **VERIFY the branch's owned files are otherwise untouched**
    — `git diff --stat` should show ONLY `UnitCommand.h`, `SiegePlayerController.{h,cpp}`, `SummonedUnit.{h,cpp}`,
    `Castle.{h,cpp}`, the 3 `IA_Cmd*` + `IMC_Hero` + (if done) `WBP_HUD`; if L_Arena.umap / DA_BattlefieldScatter /
    SiegeBotController / CaptureZone changed unexpectedly, STOP and report. (3) **PIE command-suite** (best-effort, honest
    about what is machine-verifiable): spawn a few player Standard units; press **T/R/E**; confirm via log/readback that
    `CurrentCommand` latches and `HasIssuedCommand` flips; ATTACK — a unit/tower placed inside the RED spawn box is
    prioritized before Castle_Red; HOLD — press R, confirm the reticle appears and LMB sets `HoldLocation`, units gather
    there and only engage enemies inside `HoldRadius`; DEFEND — units fall back toward Castle_Blue and only fight enemies
    within `DefendRadius`; confirm Siege (Ogre), Support (Cleric), miners, and BOT units are UNAFFECTED; Play Again resets
    the stance. Note plainly what needs Jonathan's hands (real key input on an unlocked desktop). (4) **COMMIT on the branch**
    referencing TASK-273..277 + the directive. **DO NOT PUSH.** (5) Record the human WATCH (T/R/E feel, HOLD reticle
    readability, stance legibility) for Jonathan's W1 look. Leave the editor running + saved. Post results + hash in
    🔧 Build & Git.
- names: >
    Branch `m7.6-arena10x` commit (NO push); `UnitCommand.h`, `SiegePlayerController.{h,cpp}`, `SummonedUnit.{h,cpp}`,
    `Castle.{h,cpp}`, `IA_CmdAttack`/`IA_CmdHold`/`IA_CmdDefend`, `IMC_Hero`, `WBP_HUD`. Law: CLAUDE.md hard gates
    (PASS QA before commit, never push unasked), CONVENTIONS "Unit commands (Shield Wall stances)…", M7.6 branch-ownership.

---

