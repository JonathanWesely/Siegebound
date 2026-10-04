<!-- ARCHIVED from .claude/pipeline/TASKBOARD.md on 2026-10-04 by Tools/archive_board.py. Every row below was in a terminal state when moved; bytes are unchanged and this comment is the only addition. Law: TASKBOARD.md '## Archive'. -->
#### TASK-352 — [M8-audit] Single-player-assumption + replication audit of the whole gameplay module (gameplay-programmer, READ-ONLY)
- assignee: gameplay-programmer
- status: **done** (2026-07-28 — audit delivered, `handoffs/TASK-352-audit.md`; read-only, nothing edited; the per-class matrix + the GetFirstPlayerController/team/mutation site enumerations fed TASK-353 directly. The CASTLE-3X code-delta addendum remains OWED post-TASK-350, per the doc's §7.) ← was: backlog — dispatchable NOW
- blocked-by: none
- parallel-safe: yes
- spec: >
    Audit the COMMITTED tree (HEAD) — do NOT read TASK-349's uncommitted working files. Deliver, with file:line evidence:
    (1) **every `GetFirstPlayerController()` / first-local-player site** (ruling 4 — the group-orders polling `3068286` is the known
    big one; find them ALL); (2) **every team assumption** (Blue = local player, Red = bot — spawn, HUD, placement, win/lose, team
    visuals); (3) **every client-authoritative gameplay mutation** (economy tick, card play/discard, placement confirm, spell
    targeting/resolution, upgrades, group-order confirm, Play-Again reset) — the future `Server*` RPC surface; (4) a **per-class
    replication-needs matrix** (`ASummonedUnit`, `AHeroCharacter`, `ACastle`, `ABuilding`+subclasses, `AGoldNode`, `ACaptureZone`,
    projectiles, GameMode/GameState/`SiegePlayerState`, both controllers, `ASiegeBattlefieldScatter` — note its per-match random seed
    must replicate or be server-sent): what state, what OnReps, what stays server-only, what is client-cosmetic (MIDs, decals,
    widgets, debris); (5) **SaveGame deck locality** (client deck → server handoff shape, for P3); (6) the two NAMED newest systems
    (ruling 7): group-orders polling model + the CASTLE-3X team-gating DESIGN (from CONVENTIONS — BeginPlay ordering vs networked
    spawn/possession; record that a code-delta addendum is OWED after TASK-350 lands); (7) timers/latency hazards (0.25 s polls,
    match timer, overtime). NO fixes, NO edits. Write `handoffs/TASK-352-audit.md` (the matrix is the deliverable). Post in ⚙️ Dev & QA.
- names: >
    Read-only: everything under `Source/GitClaudeUnrealTest/Siegebound/` at HEAD + `Docs/GDD.md` §9.8/§10. Report
    `handoffs/TASK-352-audit.md`. Law: CONVENTIONS "Networked 1v1 (M8)".
#### TASK-353 — [M8-arch] Net architecture design: replication matrix + RPC table + migration plan (gameplay-programmer, doc-only)
- assignee: gameplay-programmer
- status: **done — SIGNED by the manager 2026-07-28** (`handoffs/TASK-353-architecture.md`; decision table D1–D14 ratified). **The §9 open questions, ruled one by one:** (1) `HeroCharacter.{h,cpp}` ADDED to TASK-356 (349-lane — its .cpp is 349-touched-uncommitted per `handoffs/TASK-349-programmer.md` §5, so single-owner puts it post-350 regardless; no micro-task). (2) Observer posture SIGNED; refusal wording `"Not available yet in online matches"` APPROVED as-is. (3) Scatter cull residual ACCEPTED for P1 (superset-never-rubber-bands argument holds; attempt counts logged; P2 upgrade only if felt). (4) Dual `bNetworkedMatch` latch SIGNED as-is. (5) **Clean-file carve-out DECLINED — and with a STRONGER reason than the doc's:** a 356a landing file-only would sit PARKED-UNCOMMITTED in the shared module exactly when TASK-350 compiles it — the TASK-268/TASK-277 collision trap verbatim (parked C7595 broke the W1 build). One task, whole byte-identity argument, one QA, entirely post-350. (6) VictoryScreen Play-Again rewire APPENDED to TASK-355 (host-only fallback recorded — TASK-357 records which way it landed). (7) Victory-widget relative text: SIGNED as recommended — music/flow correct in P1; consuming `SetLocalVictory` is an OPTIONAL TASK-355 item, else the widget's absolute branch is a recorded P2 flag, not gate-blocking. (8) Remote-controller server deck = curated DEFAULT deck (never the host's SaveGame) — RECORDED as a BINDING P2 item on the P2 one-liner. (9) `LogSiegeNet` cross-task reference ACCEPTED (both lanes compile together at 357); the category is now CONVENTIONS law (`LogSiege<Domain>`). **Also ratified: the OnRep bool-prefix reading (`bDestroyed` → `OnRep_Destroyed`) is now the written naming law.** ← was: backlog
- blocked-by: TASK-352 ✅ · manager gate ✅ **SATISFIED 2026-07-28** — TASK-354 and TASK-356 may dispatch per their own blockers
- parallel-safe: yes (doc-only)
- spec: >
    From the audit + rulings 1–5 + the CONVENTIONS M8 law, author the buildable design: (1) per-class replication plan (properties +
    conditions, `OnRep_*` handlers, authority-only members, spawn/ownership); (2) the FULL RPC table (`Server*`/`Client*`/`Multicast*`
    per command surface — card play, discard, placement, spell target, group-order stage confirms, upgrades — with validation notes);
    (3) team assignment + PostLogin registration design (host=Blue/client=Red) and the `GetFirstPlayerController` REPLACEMENT pattern
    (owning-team-controller resolve for group orders — must degrade byte-identical in standalone); (4) session flow (listen-server
    travel `L_Arena?listen`, client `open <ip>`, leave/return-to-menu); (5) PHASE MAPPING — exactly which class/surface lands in P1
    (the increment: GameState/PlayerState/Castle HP/gold/match flow) vs P2 vs P3, honoring the M8 PARALLEL LAW file constraints;
    (6) the single-player byte-identity argument. Doc-only → `handoffs/TASK-353-architecture.md`. Post in ⚙️ Dev & QA.
- names: > Report `handoffs/TASK-353-architecture.md`. Law: CONVENTIONS "Networked 1v1 (M8)" (all clauses).
#### TASK-354 — [M8-session] `USiegeSessionSubsystem` + `USessionMenuWidget` — host/join plumbing, NEW FILES ONLY (gameplay-programmer)
- assignee: gameplay-programmer
- status: **done — committed in the M8 P1 unit `f0d7190` (TASK-357 RE-RUN #2, 2026-07-29 evening).** Its four new files landed with zero defects; the session subsystem was additionally exercised LIVE this run — `HostListenMatch()` opened `/Game/Maps/L_Arena?listen` (listen server up on port 7777) and `JoinMatch("127.0.0.1:7777")` client-travelled and connected, both logging their designed transitions. WARN-2's joiner-drop observable verified by log as planned. ← was: **qa-passed** (2026-07-28 — qa/TASK-354.md PASS 0 BLOCKER / 2 WARN / 3 NIT, all 5 flags APPROVED. WARN-1 recorded for the manager residue pile: mid-session error broadcasts have no listening widget after a mid-match disconnect [blank menu; suggested LastSessionError cache pulled at NativeConstruct — P1-acceptable defer]. WARN-2: joiner-drop observable is log-only, TASK-357 verifies via log. new-files-only proven by reference-grep in QA; build-master runs the literal git-status belt at TASK-357. Chain: 356 [post-350] → 356-QA → 357 hosting 355)
- blocked-by: TASK-353 (manager-signed)
- parallel-safe: yes (**NEW FILES ONLY:** `SiegeSessionSubsystem.{h,cpp}`, `SessionMenuWidget.{h,cpp}` — zero edits to any existing file; if a hook into an existing file seems unavoidable, STOP and route to the manager — the file-ownership matrix decides, not the task)
- spec: >
    Per the signed TASK-353 design: `USiegeSessionSubsystem` (UGameInstanceSubsystem) — `HostListenMatch()` (travel
    `L_Arena?listen`), `JoinMatch(const FString& Address)` (validated address, client travel), `LeaveMatch()` (clean return to menu);
    null-safe, logs each transition. `USessionMenuWidget` (UUserWidget base) — BlueprintCallable wrappers + BIEs for status/error
    text (FString-only params, widget law) so TASK-355 can build `WBP_SessionMenu` on it. **NO `DefaultEngine.ini` edit** (parallel
    law d — net config, if any is genuinely required, lands at TASK-357 post-350). `Build.cs`: only if a module dependency is truly
    needed; it is unowned — claim it single-owner in the handoff if touched. Compile traps apply. File-only; write
    `handoffs/TASK-354-programmer.md`; post in ⚙️ Dev & QA; ready-for-qa.
- names: >
    NEW `Source/GitClaudeUnrealTest/Siegebound/SiegeSessionSubsystem.{h,cpp}` + `SessionMenuWidget.{h,cpp}` (names per CONVENTIONS
    M8 law). WBP lands at TASK-355 (`/Game/UI/WBP_SessionMenu`). Report `handoffs/TASK-354-programmer.md`.
