# TASK-949 — BUILD-MASTER HANDOFF — the Witch Blueprint + the roster gate

**Commit `09b9b50`** · 2026-09-03 · `main` **17 ahead** of `origin/main`, ⛔ **NOT PUSHED**
Parent `950d8c5` · 5 files, 1,355 insertions

> ⛔ **`TL-§5e` cl. 7: this file carries the hash and therefore ⛔ cannot be in its own commit. ⛔ It is the MINTED ORPHAN. ⛔ `TASK-951` takes it.**

---

## 0. ⛔ WHAT I EXECUTED AND WHAT I DID NOT — ABOVE THE CLAIM (`TL-§5c` cl. 5(a))

**I EXECUTED:** the compile · the full `Siegebound` suite · the census at both endpoints · the `§25b` cl. R/S sweep · the LFS oid verification · the secret sweep · the commit.

⛔ **I DID NOT observe a COMPILED RED.** See §4. ⛔ **The RED half of item (1) remains evidenced only on `TASK-947`'s shell mirror.** I say so before I report the green.

⛔ **I opened no editor and used no MCP.** The editor was down and port 8000 clear at my start; the compile needed it closed and I never needed it open — the integration check on this row is a **test execution**, not an in-editor read-back, so the mid-row editor flip the spec warned about never had to happen.

---

## 1. THE COMPILE — verbatim

```
Result: Succeeded
```

- ⛔ **Log-parsed. `$LASTEXITCODE` never trusted** (the standing law: `Build.bat` returns 0 on a failed build).
- **Parser positive control:** a synthetic file containing `Result: Failed` read **`1`** on the same `grep`; the real build log read **`0`**. The parser is live, not blind.
- `Result: Failed` count in the real log: **0**. `error C####` / `LNK####` / `fatal error`: **0**.
- ⭐ **Proof the new gate is genuinely in the binary, not merely on disk:**
  `[1/6] Compile [x64] SiegeCardRosterTest.cpp` and `[Adaptive Build] Excluded from GitClaudeUnrealTest unity file: SiegeCardRosterTest.cpp`, then `[5/6] Link [x64] UnrealEditor-GitClaudeUnrealTest.dll`.

Log: `<scratchpad>/t949/build.log` (32 lines).

---

## 2. THE SUITE — EXECUTED, with the positive control on the zero

Command: `UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests Siegebound;Quit" -nullrhi -unattended -nopause -nosplash -NoLiveCoding -log -abslog=<scratchpad>/t949/suite-949.log`

| | |
|---|---|
| **`Result={Success}`** | ⭐ **433** |
| **`Result={Fail}`** | ⭐ **0** |

⛔ **THE ROWS ARE PROVEN TO BE IN THE FILE I PARSED.** `suite-949.log` = **5,768 lines / 975,105 bytes**, carrying real `Test Started` / `Test Completed` / `BeginEvents` traffic. ⛔ **This is the specific trap the dispatch named** — a predecessor once parsed a 15-line build preamble and read an empty log as a clean run. Mine is not empty and I checked its size and content before trusting its zero.

⛔ **POSITIVE CONTROL ON THE ZERO, as demanded.** I appended one synthetic row —
`...LogAutomationController: Display: Test Completed. Result={Fail} Name={SyntheticPositiveControl}...` —
to a **copy** of the log and re-ran the identical `grep`:

| file | `Result={Fail}` |
|---|---|
| copy + synthetic row | **1** ✅ |
| real log | **0** ✅ |

⇒ the zero is a **measured** zero, not a parser that cannot see failures. (Copy deleted; the real log untouched.)

---

## 3. THE DELTA — `+1`, both endpoints re-measured by me (`TL-§5b`/`§5c`)

⛔ **I did NOT reconcile to the relayed absolute.** I measured the baseline out of git myself.

| | declarations | files |
|---|---|---|
| `d101b1e` (last executed), via `git show` per file | **432** | **31** |
| worktree now | **433** | **32** |
| ⭐ **DELTA** | **+1** | **+1** |

Scope: `Source/GitClaudeUnrealTest/Siegebound/Tests/*.cpp`, needle `^IMPLEMENT_SIMPLE_AUTOMATION_TEST`.

⭐ **The executed pass count `433` and the declaration census `433` agree — but I am asserting the DELTA (`+1`), not the matching absolute.** The dispatch was explicit that `426 == 426` was refused earlier for exactly this reason, and the two figures are independent measurements that happen to coincide because every declaration in scope produced exactly one test that ran.

---

## 4. ⭐⭐ ITEM (1) — THE `RED → GREEN`, AND THE HALF I OWE

### The GREEN — mine, compiled, verbatim

```
LogAutomationController: Display: Test Completed. Result={Success}
  Name={EverySpawnableCardRowResolvesItsComposedActorClassPath}
  Path={Siegebound.CardRoster.EverySpawnableCardRowResolvesItsComposedActorClassPath}
```

⭐ **And the gate emitted its own anti-vacuity evidence, which is better than my asserting it:**

```
card table derived from the shipped CDO (never typed here): '/Game/Data/DT_Cards.DT_Cards', row struct FCardRow.
Economy-building exception derived from the shipped CDO: 1 CardID(s).
ROSTER — 32 row(s) read; 22 SPAWNABLE (Unit | Economy | Building) probed; 10 EXCLUDED (not a spawnable CardType).
  CardType Unit          13 row(s)
  CardType Economy        2 row(s)
  CardType Building       7 row(s)
  CardType Utility        1 row(s)   [EXCLUDED — not a spawnable CardType]
  CardType HeroUpgrade    4 row(s)   [EXCLUDED — not a spawnable CardType]
  CardType Spell          5 row(s)   [EXCLUDED — not a spawnable CardType]
```

⇒ ⛔ **`SpawnableRows == 22`, not `0`.** `qa/TASK-948.md` §1's vacuous-green fear is **measurably** not what happened: this green was earned over 22 real probes, and it says so inside itself.

### ⛔ The RED — NOT OBSERVED BY ME, and I will not imply otherwise

I attempted to synthesise a compiled RED the way `SHIP-§9` asks — move `BP_Unit_Witch.uasset` aside, run only `Siegebound.CardRoster`, watch it redden naming `Witch`, restore, verify by digest.

⛔ **The permission system REFUSED the action** (it is a move of a *staged* `Content/` asset). ⛔ **I did not work around it.** I verified afterwards that nothing had been touched: the command was blocked atomically, `sha256 = b4f375…fec2ab` unchanged, index entry still `A `.

**Therefore, stated plainly:**
- the **RED naming `Witch`** is evidenced on `TASK-947`'s **shell mirror of the predicate** at 19:00:57, and the **RED → GREEN transition** was observed live on **that same mirror** when `TASK-946` landed at 19:02 — recorded in `handoffs/TASK-947-programmer.md`, adjudicated in `qa/TASK-948.md` §5;
- the **compiled** gate has now been seen **only passing**;
- ⭐ what stops that from being `SHIP-§9c` cl. 1's "indistinguishable from no gate" is **not** my word — it is the **22-probe count the green prints**, plus the blind-probe control `qa/TASK-948.md` §2 verified runs on **every** green pass (`TestFalse(DoesPackageExist(BogusComposed))`, on the unconditional path with no early return before `return true`).

⚖️ **A row that synthesises the compiled RED without touching a staged asset is worth boarding** — point the gate at a measured-absent synthetic CardID, exactly as `TASK-947`'s RUN 3 did on the mirror. **That is a manager decision, not mine.** See §7.

### ⛔ The predicted FALSE RED did not materialise

`qa/TASK-948.md` §7(3) warned that the mirror walked `Docs/Data/cards.csv` while the C++ walks `DT_Cards.uasset`, so a reimport drift could throw a **data** red on my first compiled run. **It did not.** I measured both sides before concluding:

- `Docs/Data/cards.csv:33` carries the `Witch` row;
- `Content/Data/DT_Cards.uasset` carries `Witch` (binary probe, 4 occurrences);
- the compiled gate read **32 rows** from `DT_Cards` and went green.

⛔ **No QA loop burned, and none was warranted.**

---

## 5. ⛔ THE PATHSPEC — DERIVED FROM MY OWN `git status` (`§25b` cl. R)

⛔ I built this from `git status --porcelain --untracked-files=all`, **not** from the `names:` line and **not** from the spec's enumeration, which I treated as a head start only (`cl. S`).

**COMMITTED — 5 paths:**

| path | state found |
|---|---|
| `Content/Blueprints/Units/BP_Unit_Witch.uasset` | ⛔ **`A ` — ALREADY STAGED** |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCardRosterTest.cpp` | `??` |
| `.claude/pipeline/handoffs/TASK-946-artist.md` | `??` |
| `.claude/pipeline/handoffs/TASK-947-programmer.md` | `??` |
| `.claude/pipeline/qa/TASK-948.md` | `??` |

### ⛔ The pre-staged asset — confirmed, not assumed

The spec predicted it (editor Revision Control auto-adds on save; every Git call the artist made was read-only). ⛔ **I confirmed it was the file I expected and that nothing rode in with it:**

- **only ONE path in the whole index was staged** — no other lane's asset was swept in by the provider;
- ⛔ **LFS verified by OID-vs-SHA256, NEVER by size**, at four points, all `b4f375305aea3ea812e25fb0e30bfabd39f82d21a64fe52b02298c3223fec2ab`:
  worktree `sha256sum` (**re-measured by me** — a recorded prior is the file least likely to ship wrong) · index pointer blob · `HEAD:` pointer after commit · `git lfs ls-files -l HEAD` (with `*`, i.e. object present locally, not pointer-only). Size `39609` agreed too, but ⛔ **the size is not what I trusted.**
- ⛔ **No `git reset`.** I committed by explicit pathspec, as `§25b` cl. R requires.

### ⛔ FOUND IN MY STATUS AND DELIBERATELY LEFT ALONE — named, per item (4)

| path(s) | count | why not mine |
|---|---|---|
| `.claude/pipeline/CONVENTIONS.md`, `TASKBOARD.md` | 2 `M` | manager writes them concurrently — item (7) |
| `Content/FogArea/**` | **24** untracked | `TASK-927`, awaiting Jonathan |
| `.claude/pipeline/playtest-evidence/2026-09-03/VID-005-*.png` | **7** untracked | VID-005 footage lane — ⚠️ **see §7 finding 1** |
| `footage/VID-005-…md` · `handoffs/STACK-BUGS-diagnosis.md` · `handoffs/TASK-928-buildmaster.md` · `handoffs/TASK-941-programmer.md` · `handoffs/TASK-952-programmer.md` · `handoffs/TASK-953-programmer.md` · `qa/TASK-934.md` · `qa/TASK-950.md` | 8 `.md` | stack lane / diagnose-only / prior tail — `TASK-951`'s class |

✅ **Item (5): `L_Arena` is ABSENT from the diff** — grepped the staged name list explicitly. ✅ **Secret sweep over the staged text diff clean** (no token/key/bearer/private-key shapes).

---

## 6. THE COMMIT MESSAGE — what I refused to claim

⭐ I grepped my own five files before writing it, per the predecessor's discipline. **What I claimed:** the Witch's actor Blueprint; the gate that walks every spawnable row and fails when the composed actor class cannot resolve; the RED→GREEN **attributed to the shell mirror**, with my compiled run named as the **first** compiled execution; the executed `Result:` figures; `SC-§50` as the law bought.

⛔ **What I refused to claim:** that she is fully playable. The message says, in the commit itself:

> She spawns **STATIC and UNANIMATED**. `SK_Witch` does not exist, and she is the **only 1 of 14** unit Blueprints without a skeletal mesh, carrying `SM_Witch` instead. The cast bar was cancelled in favour of an animation that cannot exist until she is rigged, so her 3-second interruptible cast currently has **NO TELL AT ALL** — neither caster nor victim can see that it started, completed, or was broken.

⛔ **I re-measured that claim rather than copying it from the dispatch:** 14 `BP_Unit_*.uasset`; `SK_` present for Archer, Cavalry, Cleric, Footman, Knight, Longbowman, MilitiaMob, Miner, Ogre, Pikeman, Sapper, Sorcerer, Wizard = **13**; Witch absent. ✅ **The dispatch's figure is correct.**

⛔ I also did **not** claim the card row, `DT_Cards` or `cards.csv` — those already shipped, which is exactly why the symptom was "Card actor unavailable" rather than a missing card.

---

## 7. ⛔ FOLLOW-UPS FOR THE MANAGER — reported, not boarded, not ranked

1. ⛔⛔ **A REAL `TL-§5e` ORPHAN, AND IT IS STRUCTURAL: the 7 `playtest-evidence/2026-09-03/VID-005-*.png` files have NO HOST.**
   `TASK-951` is the trailing doc-host, but its own item **(4)(d) excludes non-`.md`**, and they are outside `TASK-944`'s and my pathspecs. ⇒ ⛔ **as the board stands, nothing will ever commit them**, and `CLAUDE.md` explicitly wants promoted evidence PNGs in git (only `testvideo/` is barred). ⛔ **This is the orphan shape one level out: files that exist in the tree, are invisible to `HEAD`, and nothing goes red about it.**

2. ⚖️ **`14` unit Blueprints on disk vs `13` `CardType Unit` rows in `DT_Cards`** (measured, both sides). One unit BP has no `Unit` card row. It may be entirely correct — a unit summoned by a Spell or Building rather than played directly — ⛔ **but this gate walks card→Blueprint and is structurally blind to the reverse direction**, so nothing in the suite can tell you which. Worth one look.

3. ⚖️ **The compiled RED is still owed** (§4). A row that synthesises it against a **measured-absent synthetic CardID** — never by moving a staged asset — would close `SHIP-§9c` cl. 1 on the compiled instrument. `TASK-947`'s RUN 3 is the working template.

4. 📌 **`qa/TASK-948.md`'s own WARN W-2 survives this commit and is now settled on one side:** this project sets no warning configuration, and the build ran on **MSVC 14.50** — so the "compile-time `-Wswitch` failure" half of the code comment is, as QA suspected, **not** what protects a 7th enumerator here. ⛔ **The runtime tell is what protects it, and it is unaffected.** The comment overclaims; the gate does not.

5. 📌 `qa/TASK-950.md` named one **live orphan** — `Tools/ArtPipeline/cardart_render.py` — untouched by this row and still unhomed.

---

## 8. GATES

| gate | state |
|---|---|
| `qa/TASK-948.md` **PASS, 0 blockers** (code) | ✅ verified on the report, line 3 |
| `TASK-946` `ready-for-integration` (art) | ✅ verified on its row |
| integration check | ✅ the compiled gate resolves `/Game/Blueprints/Units/BP_Unit_Witch.BP_Unit_Witch_C` |
| compile | ✅ `Result: Succeeded` |
| suite | ✅ `433 / 0`, executed |
| `L_Arena` absent | ✅ |
| secrets | ✅ none |
| **push** | ⛔ **NOT PUSHED — Jonathan pushes his own milestones** |

*build-master · 2026-09-03 · editor never opened · no MCP · commit `09b9b50` · `main` 17 ahead, unpushed*
