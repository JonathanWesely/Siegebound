# BOARD-STALENESS-audit — git-resolved staleness sweep of TASKBOARD.md

- **Author:** build-master · **Date:** 2026-09-07 · **Mode:** READ-ONLY (no board edit, no compile, no commit, no push)
- **Why:** write-discipline rule 7, End 2. The manager's End-1 fence sweep resolved each fence by reading the *named row's own `status:`* — and that field lied, so 2 of its 3 annotations were wrong. The board cannot audit the board. Only `build-master`/`gameplay-programmer` hold `Bash`, so this is the resolution the manager could not perform.

---

## 0. Positive control — the reader is proven before any zero is trusted

`SC-§96`/`SC-§99`: a grep that has only ever returned empty is unproven. Controls run **first**:

```
$ git rev-parse --show-toplevel
C:/GitProjects/GitHub/GitClaudeUnrealTesting          <-- git root is ONE LEVEL ABOVE the project dir (SC-§102)
$ git rev-parse --show-prefix          (from the project dir)
GitClaudeUnrealTest/

POSITIVE CONTROL 1 — TASK-944:
84eec02 2026-09-03 TASK-944: the stack split ships — a WatchTower CAN now be stacked, to ×2 (TASK-941..944; the ceiling licensed by TASK-956)

POSITIVE CONTROL 2 — TASK-1117:
42734b7 2026-09-07 TASK-1117: the graphics facade ships - and its Auto-Detect no longer walks around its own save guard ... (TASK-1113, gated by TASK-1114)

NEGATIVE CONTROL — TASK-999999:
(empty)                                                <-- the reader CAN return zero, so a zero is informative
```

Both controls also re-verified **inside the derived table** the classification is actually computed from (not just in the ad-hoc command):

```
TASK-1117   OWN   42734b7   2026-09-07
TASK-944    OWN   84eec02   2026-09-03
```

**Method note — no pathspec was used anywhere.** Resolution is by `--grep`/message-text over `git log --all`, which takes no pathspec, so the `SC-§102` mis-anchoring trap (a pathspec relative to the project dir returning "no commits", indistinguishable from "never landed") cannot fire here.

### 0.1 Two reader hazards found *while* controlling the reader

1. **Prefix trap.** `git log --all --grep="TASK-110"` returns **5** commits — but 4 of them are `TASK-1104`, `TASK-1107`, `TASK-1131`… Naive substring `--grep` on any 3-digit ID silently collects its 4-digit descendants. This audit extracts **maximal `TASK-[0-9]+` tokens** instead, which returns the correct **1** commit for `TASK-110`.
2. **Suffix trap.** Token extraction stops at a letter, so `TASK-296b` yields the token `TASK-296`. Git contains `TASK-289b`, `TASK-292c`, `TASK-296b`, `TASK-296c`, `TASK-595b`, `TASK-733b` — and **no bare `TASK-296` commit at all**. Handled explicitly below.

---

## 1. Counts — queried BEFORE matched

| Quantity | Count |
|---|---|
| `#### TASK-` headers on the board | **1073** |
| Distinct task IDs | **1058** |
| Headers paired with a `- status:` line | 1068 |
| IDs with **no `- status:` line at all** | 4 — `TASK-354`, `TASK-355`, `TASK-356`, `TASK-357` |
| **IDs QUERIED against git** | **1058 (all of them)** |
| Commits in history (`git log --all`) | 256 |
| — of which distinct IDs **named anywhere** in a commit message | **596** |
| — of which IDs that **own** a commit (subject leads `TASK-N…`) | **151** |
| Board rows with a **terminal** status (done/shipped/superseded/closed/void/integrated/adopted…) | 485 |
| Board rows with an **open** status | **583** |

Every ID was queried, including the terminal ones — the extraction is a single full-history pass, so nothing was skipped on the assumption that a `done` row needs no check.

### Disposition of the 583 open rows

| Disposition | Count |
|---|---|
| **STALE** — a commit's subject leads with this ID, status does not say done | **13 IDs / 14 rows** |
| **CONSISTENT** — a commit names it, but board and git agree it is not finished (or the row *does* record done deeper) | 2 (`TASK-481`, `TASK-1055`) |
| **AMBIGUOUS** — named in a commit, but not as that commit's own shipped subject | **220** (`TASK-296` folded in here as a suffix false-positive) |
| **NOT COMMITTED** — no commit names the ID; open status is correct | **345** |

---

## 2. STALE — 13 IDs (14 rows). Git names them; the board does not say done.

Age is measured to **2026-09-07**.

| ID | status-says | git-says (commit subject, leading ID = its own commit) | hash | date | **age** |
|---|---|---|---|---|---|
| **TASK-082** | `qa-passed` (QA-loop 2 PASS 2026-07-07) | `TASK-082/083/084: Trellis Stage-1/2 tooling + hooks + pipeline docs` | `1e923d4` | 2026-07-07 | **62 days** |
| **TASK-089** | `qa-passed` (QA PASS 2026-07-08) | `TASK-089/090: gold economy balance — StartingGold 10, base income 1 gold per 2 s` | `aec6572` | 2026-07-08 | **61 days** |
| **TASK-093** | `qa-passed` (2026-07-08) | `TASK-093/094/097-102: M5 spell system + M4.5 terrain code batch` | `2c65164` | 2026-07-08 | **61 days** |
| **TASK-110** | `qa-passed + compiled-clean` (2026-07-09) | `TASK-110..112: overhead health bars on units/towers/hero — hide-at-full, team-tinted` | `9a8a75f` | 2026-07-09 | **60 days** |
| **TASK-113** | `qa-passed` (2026-07-09) | `TASK-113..121: M6 deck-builder meta — WBP_DeckBuilder, USaveGame named decks…` | `975ee90` | 2026-07-09 | **60 days** |
| **TASK-268** | `qa-passed` (COMPILE-FIX 2026-07-24) | `TASK-268..272: deck-builder card details — click-a-card 'how it works' side panel` | `24b1f0a` | 2026-07-26 | **43 days** |
| **TASK-273** | `ready-for-integration` | `TASK-273..277: Shield Wall unit commands (ATTACK/HOLD/DEFEND) — updated W1 build` | `70487d5` | 2026-07-24 | **45 days** |
| **TASK-310** | `backlog` ("all 11 `-verify` blockers now CLEARED — dispatchable") | `TASK-310: re-capture the in-engine fleet verify shots and commit them` | `bb6df70` | 2026-07-26 | **43 days** |
| **TASK-349** (row A) | `**qa-passed**` (2026-07-29, loop 4) | `TASK-349: team-gated castle interior … loop-4 PASS … empirical closure in qa/TASK-349.md L4 append (5/5 closed)` | `949c252` | 2026-07-29 | **40 days** |
| **TASK-349** (row B) | `backlog` | same commit — **this ID has TWO `#### TASK-349` headers with contradictory statuses** | `949c252` | 2026-07-29 | **40 days** |
| **TASK-438** | `✅ **ready-for-integration**` — its own text admits *"**PLACED + WIRED**, done inside TASK-447's session"* | `TASK-438: Btn_Settings on WBP_MainMenu — the main-menu entry` | `87c4784` | 2026-08-03 | **35 days** |
| **TASK-447** | `backlog` | `TASK-447: Wave 1 code — settings screen + in-match LLM command assistant (compile-gated)` (+ `56acf10` record, + `cf8ef8e` closing record) | `cd5f4ed` | 2026-08-03 | **35 days** |
| **TASK-768** | `**ready-for-qa**` (2026-09-02) | `TASK-768: the repaired reimport path lands - all four files together (QA PASS)` | `c6ee4a7` | 2026-09-02 | **5 days** |
| **TASK-780** | `backlog` | `TASK-780: the hero and units climb by walking into the ladder (TASK-768/775-778/783-788)` | `1231bc4` | 2026-09-02 | **5 days** |

**Nine of the thirteen are 40+ days stale; five are 60+ days stale.** The two known rows (`TASK-942`, `TASK-944`) are **not** in this list — the manager already flipped both to `done` today, and this sweep confirms those flips are correct against git (`84eec02`).

### 2.1 Each STALE row was re-checked for a *deeper* `done` status line

The manager's error was resolving one board field by reading another. To avoid the mirror-image error, every candidate row was scanned end-to-end for a second, nested `- status:` line or a `done`/`COMMITTED` record further down the block. **All 13 confirmed single-status** — none of them records completion anywhere in its row. (`TASK-1055` did have one, and was reclassified out of STALE because of it — see §3.)

---

## 3. CONSISTENT despite owning a commit — 2 rows the naive rule would have called STALE

These are the two rows where "git names it ⇒ stale" is **wrong**, and they are the reason this audit did not just diff two lists.

- **`TASK-481`** — status `**in-progress — ⛔ MEASUREMENT-ONLY REMAINS. ⛔ NOT done.**`. Two commits lead with its ID, but **the commits themselves say the work did not land**:
  - `23d380e` *"TASK-481: compile PASSES + all four git-dependent proofs discharged — **but the §16 instrument re-run is BLOCKED, so the Stage-A source is NOT committed**"*
  - `0b7dd2f` *"TASK-481: the handoff + the board told straight — **PARTIAL, blocked on the engine, source held**"*
  - **Board and git agree.** A commit existing is not the same as the row's work shipping. **CONSISTENT — leave it alone.**

- **`TASK-1055`** — first `- status:` line reads `GATE ⛔ DISCHARGED 2026-09-05 …`, which scans as open. But the row carries a **second, deeper `- status:` line** from the `BOARD-TRUNCATION-2026-09-05` restoration: `✅⭐⭐ **done 2026-09-05 (TASK-1055-BUILD-DONE) — 🔧 build-master. COMMITTED c4955cb.**` — which matches `c4955cb 2026-09-05 TASK-1055: the fog docs stop lying about BP_SiegeFog…`. **CONSISTENT.**
  ⚠️ **Reader hazard for any future sweep:** a first-`status:`-line reader reports this row stale. Rows restored after the truncation incident can carry more than one `- status:` line, and **the first one is not necessarily the current one.**

---

## 4. AMBIGUOUS — 220 IDs named in a commit that is not their own

Per rule 7, a commit naming a row **as a gate, in a range, or in a parenthetical** is not that row's commit, and must not be collapsed into STALE. Breakdown of the 1060 ambiguous mention-lines:

| Context bucket | mention-lines |
|---|---|
| `weak-other` (named in a compound subject `TASK-A..B` / `TASK-A/B`, or incidental prose) | 1004 |
| **`STRONG-body-bullet`** (a body line `- TASK-N: <work>` enumerating this row as shipped content) | **32** |
| `weak-fence` (`blocked…`, `licensed by TASK-N`) | 14 |
| `weak-gate` (`gated by TASK-N`) | 10 |

### 4.1 The 30 AMBIGUOUS rows worth the manager's eye first (STRONG body bullets)

These commits contain a body bullet that reads as *this row's own shipped content*, not a gate or a range. They are **still AMBIGUOUS** — I am not flipping the classification — but they are the highest-probability stale rows in the ambiguous pile:


- **TASK-016** — `f6fa7ba` (2026-07-03) — quoted: `- TASK-016: AHeroCharacter montage/impact-VFX/camera-shake hooks (null-safe,`
- **TASK-017** — `4f95730` (2026-07-03) — quoted: `- TASK-017: BP_HeroCharacter feedback wiring (AttackMontage=AM_ComboAttack`
- **TASK-018** — `f6fa7ba` (2026-07-03) — quoted: `- TASK-018: ACastle FOnCastleHPChanged + screen-space HPBarWidget component;`
- **TASK-019** — `4f95730` (2026-07-03) — quoted: `- TASK-019: /Game/UI/WBP_CastleHealthBar (UI_LifeBar duplicate reparented to`
- **TASK-020** — `f6fa7ba` (2026-07-03) — quoted: `- TASK-020: ASummonedUnit procedural attack lunge (sine-eased, zero-drift)`
- **TASK-074** — `218b4c9` (2026-07-07) — quoted: `- TASK-074: C++ fix, QA PASS (0 blocker / 0 warn / 1 nit). QA rulings 4/5`
- **TASK-075** — `218b4c9` (2026-07-07) — quoted: `- TASK-075: CANCELLED — audit proved no editor-asset change is needed (the`
- **TASK-076** — `218b4c9` (2026-07-07) — quoted: `- TASK-076: compile PASS clean; direct-L_Arena regression PIE: fresh-BeginPlay`
- **TASK-094** — `2c65164` (2026-07-08) — quoted: `- TASK-094: projectiles die on tagged terrain/obstacles — full-segment trace, zero damage (Projectile)`
- **TASK-097** — `2c65164` (2026-07-08) — quoted: `- TASK-097: Set III card data — ESpellEffect + 6 M5 columns, 28-row cards.csv, 50-card M5 test deck (CardRow, cards.csv)`
- **TASK-098** — `2c65164` (2026-07-08) — quoted: `- TASK-098: USpellLibrary::ResolveSpell resolver + USiegeDamageType_Spell + castle 50% spell scaling (SpellLibrary NEW, DamageTypes, Castle)`
- **TASK-099** — `2c65164` (2026-07-08) — quoted: `- TASK-099: freeze + combat-buff APIs — ApplyFreeze/IsFrozen on units and buildings, ApplyCombatBuff (SummonedUnit, Building)`
- **TASK-100** — `2c65164` (2026-07-08) — quoted: `- TASK-100: spell targeting mode — reticle-anywhere surface-traced cast, deduct-then-resolve with refund (SiegePlayerController)`
- **TASK-101** — `2c65164` (2026-07-08) — quoted: `- TASK-101: Crystal Tower instant chain zap + tower freeze-gate on both fire paths (Tower)`
- **TASK-102** — `2c65164` (2026-07-08) — quoted: `- TASK-102: bot v4 — rule-3 spell casts: Fireball at unit clusters, Lightning at defended towers (SiegeBotController)`
- **TASK-104** — `979f552` (2026-07-08) — quoted: `TASK-104: DT_Cards in-place update — 28-row Set III + M5 test deck (draw pile = 50, every Set III card reachable).`
- **TASK-105** — `979f552` (2026-07-08) — quoted: `TASK-105: SM_CrystalTower blockout mesh (1,212 tris, UCX footprint hull) + M_CrystalGlow emissive cyan material + CrystalTower.fbx source.`
- **TASK-106** — `979f552` (2026-07-08) — quoted: `TASK-106: Set III card art — 6 T_CardArt_* textures (512^2, TEXTUREGROUP_UI, sRGB) + source PNGs; /Game/UI/CardArt/ now 28.`
- **TASK-107** — `979f552` (2026-07-08) — quoted: `TASK-107: BP_Building_CrystalTower — data-only ATower child, slot-0-only team recolor, all stats bind from DT_Cards.`
- **TASK-108** — `979f552` (2026-07-08) — quoted: `TASK-108: spell Niagara VFX — NS_Spell_{Fireball,FrostNova,Lightning,BattleCry,Pickpocket} + NS_ChainZap + M_SpellReticle decal material.`
- **TASK-109** — `979f552` (2026-07-08) — quoted: `TASK-109: M5 final assembly — machine-only exit-criteria PIE verification (desktop LOCKED, input-driven checks WATCH-listed per TASK-076). Pre-…`
- **TASK-223** — `cae0412` (2026-07-19) — quoted: `- TASK-223: Meshy auto-rig + 4 preset clips for the remaining 8 rigged units (Knight/Cavalry/Pikeman/MilitiaMob/Sapper/Cleric/Longbowman/Miner) �…`
- **TASK-226** — `cae0412` (2026-07-19) — quoted: `- TASK-226: sine emissive pulse (0.1 Hz, +/-12%) spliced into M_GoldGlow + M_CrystalGlow via desc-tagged Multiply ("TASK226 pulse mult"); no para…`
- **TASK-274** — `70487d5` (2026-07-24) — quoted: `- TASK-274: ESiegeUnitCommand + ASiegePlayerController command state, T/R/E input`
- **TASK-275** — `70487d5` (2026-07-24) — quoted: `- TASK-275: ASummonedUnit Standard-body command dispatch. Gate = Profile==Standard &&`
- **TASK-346** — `3068286` (2026-07-27) — quoted: `TASK-346: compile GREEN (16s, 0 warn); full real-PIE matrix PASS on L_Arena (flow,`
- **TASK-763** — `cf45f13` (2026-09-01) — quoted: `- TASK-763: /Game/Blueprints/BP_SiegeGhostPawn, all SEVEN designer slots`
- **TASK-764** — `cf45f13` (2026-09-01) — quoted: `- TASK-764: SiegeGameMode.cpp:67 sets GhostPawnClassAsset to`
- **TASK-765** — `cf45f13` (2026-09-01) — quoted: `- TASK-765: the now-false "ships unset" assertion replaced by an equality`
- **TASK-766** — `cf45f13` (2026-09-01) — quoted: `- TASK-766: the stale clause in the :293 log string.`

### 4.2 The weak-reference roster (the remaining ~190)

The bulk of the AMBIGUOUS pile is a row named inside a **compound subject** — `TASK-077..081:`, `TASK-093/094/097-102:`, `TASK-104..109:` — or as a **gate/licence** clause. Two representative quotes, one of each kind, showing why neither is that row's own commit:

- **Range, not a ship:** `TASK-944`'s own commit reads *"…(TASK-941..944; the ceiling **licensed by** TASK-956)"*. `TASK-956` is named only as the thing that **licensed** the ceiling — that message is not `TASK-956`'s commit.
- **Gate, not a ship:** `42734b7` reads *"TASK-1117: the graphics facade ships … (TASK-1113, **gated by** TASK-1114)"*. `TASK-1114` is the QA gate; a gate row does not ship code and never gets its own commit. Its open status is **correct**.

⚠️ **`TASK-296` is a suffix false-positive and is filed here, not under STALE.** Git contains `TASK-296b: board M7.6 COMPLETE — merged to main LOCALLY…` and `TASK-296c: record M_GoldGlow keep-both verification on board (art-director ready-for-integration…)` — and **no bare `TASK-296` commit**. Token extraction reads `TASK-296` out of `TASK-296b`. Worse, `296c` is a *board-record* commit that explicitly records `TASK-296` as still `ready-for-integration`, so git **corroborates** the open status rather than contradicting it.

---

## 5. Where the cost actually lands — STALE rows with other rows fenced behind them

This is the section that matters: a stale row is cheap until something is waiting on it.

**9 of the 13 STALE rows fence at least one still-open row. 16 distinct open rows are blocked behind work that already shipped.**

| STALE row (shipped) | age | Open rows fenced behind it | also fences (already closed) |
|---|---|---|---|
| **TASK-447** `cd5f4ed` | 35 d | **TASK-438, TASK-448, TASK-458, TASK-460** | 451, 457, 459, 462 |
| **TASK-780** `1231bc4` | 5 d | **TASK-781, TASK-789, TASK-792, TASK-793, TASK-795** | 790, 791, 799 |
| **TASK-113** `975ee90` | 60 d | **TASK-114, TASK-116** | 117 |
| **TASK-093** `2c65164` | 61 d | **TASK-096, TASK-100** | 103, 190 |
| **TASK-110** `9a8a75f` | 60 d | **TASK-111** | 112 |
| **TASK-268** `24b1f0a` | 43 d | **TASK-269** | — |
| **TASK-273** `70487d5` | 45 d | **TASK-274** | 277 |
| **TASK-438** `87c4784` | 35 d | **TASK-447** | 445 |
| **TASK-768** `c6ee4a7` | 5 d | **TASK-769** | — |
| TASK-082 `1e923d4` | 62 d | *(none open)* | 084 |
| TASK-089 `aec6572` | 61 d | *(none open)* | 090 |
| TASK-310 `bb6df70` | 43 d | *(none open)* | 324 |
| TASK-349 `949c252` | 40 d | *(none open)* | 341, 350 |

### 5.1 The two worst

- **`TASK-447` is the single most expensive stale row.** Status `backlog`, three commits already carry its name (`cd5f4ed` Wave 1 code, `56acf10` pipeline record, `cf8ef8e` closing record), and **four open rows sit behind it**. Its status has read `backlog` for 35 days while its code was in the tree the whole time.
- **`TASK-780` is the most expensive recent one** — `backlog` for 5 days with **five open rows** behind it, while `1231bc4` shipped the ladder climb it describes.

### 5.2 A mutual fence — `TASK-438` ⇄ `TASK-447`

`TASK-438` is fenced behind `TASK-447` **and** `TASK-447` is fenced behind `TASK-438`. **Both are STALE and both shipped** (`87c4784`, `cd5f4ed`, same day). Read literally, the board describes a deadlock that git says was resolved 35 days ago. Neither row can be unblocked by consulting the other — this pair is unresolvable from board state alone and is the cleanest possible demonstration of rule 7's premise.

---

## 6. Findings for the manager (no edits made here — the flips are yours)

1. **13 IDs / 14 rows are STALE** and want a status flip, each with the hash and date in §2. `TASK-349` needs **two** rows reconciled — it has duplicate `#### TASK-349` headers with contradictory statuses (`qa-passed` and `backlog`).
2. **`TASK-481` and `TASK-1055` must NOT be flipped** despite owning commits — §3. `TASK-1055` already records `done` in a *second* status line.
3. **4 board rows carry no `- status:` line at all** — `TASK-354`, `TASK-355`, `TASK-356`, `TASK-357`. They cannot be swept by any status-based rule, in either direction.
4. **The suffix/prefix traps are load-bearing for the next sweep** (§0.1). Any future resolver must extract maximal `TASK-[0-9]+` tokens, not substring-`--grep`, or it will attribute `TASK-1107`'s commit to `TASK-110` and invent a stale row.
5. **345 open rows have no commit naming them** — their open status is correct. The board is not broadly rotten; the staleness is concentrated, and concentrated specifically in **batch commits whose subject leads with one ID and silently carries several others**.

## 7. The mechanism, stated once

Every STALE row here was shipped by a commit whose subject **leads with a single ID** while the work of several rows rode along (`TASK-093/094/097-102`, `TASK-110..112`, `TASK-113..121`, `TASK-268..272`, `TASK-273..277`). The lead ID gets remembered and flipped; the passengers do not. That is the manufacturing process for End 2 — and End 2 then manufactures End 1, because the passengers are exactly the rows other tasks fence against.

---

*Read-only audit. No board edit, no `Content/`, no editor, no MCP, no compile, no commit, no push. The report file is the only write.*
