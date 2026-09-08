# TASK-1131 — [ROLL-LAW-HOST] build-master handoff

**Marker:** `TASK-1131-ROLL-LAW-HOST` · **Date:** 2026-09-07 · **Agent:** build-master
**Commit:** `001b3314bde05ab6a5f785a24d691caf74b27205` (`001b331`)
**Law:** `§25c` · `SC-§68` · `SC-§91` · `SC-§96` · `SC-§97` · `TL-§5e` cl. 1 + cl. 7a **as amended 2026-09-07** (`TL-5E-7A-ZERO-CENSUS`)
**Gate:** WAIVED per the row — text-only host. **No compile. No suite.** The verification IS clauses (1) and (3).

> ⚠️ **THIS FILE IS BORN OUTSIDE ITS OWN COMMIT.** It carries the hash of `001b331`, so it cannot be
> inside it. Per the cl. 7a classifier table (*"YOUR OWN `handoffs/TASK-###-buildmaster.md` — MINTED ⇒
> STRUCTURALLY EXCLUDED, bounded at ONE, NEVER amend"*), it is left untracked **deliberately** and
> **the NEXT commit host takes it under row 1.** It is this host's one minted orphan.

---

## 1. RE-DERIVED HEAD AND AHEAD-COUNT (never inherited)

Both figures were re-derived at this host's own instant. **No earlier "N ahead" was inherited.**

| fact | before | after |
|---|---|---|
| `HEAD` | `e1ba2c4` — *"main character and tree visual update"*, 🧑 **Jonathan's own commit** | `001b331` (this commit) |
| `origin/main...HEAD` (left/right) | `0  0` — **level, nothing unpushed** | `0  1` — **1 AHEAD** |
| `.git/index.lock` | **absent** (checked before staging; never deleted) | n/a |
| branch | `main` | `main` |

⚠️ **NOT PUSHED.** `main` stands **1 ahead of `origin/main`**, exactly as the row requires.

🧑 Jonathan's `e1ba2c4` is confirmed as the true parent — the memory note that he committed and
pushed it himself is **measured true here**, not assumed.

---

## 2. THE DERIVATION — VERBATIM, AT MY OWN INSTANT

Command run **exactly as the amendment (7a-ii)(a) fixes it**, `--ignored` included:

```
$ git status --short --untracked-files=all --ignored -- .claude/pipeline/
 M .claude/pipeline/CONVENTIONS.md
 M .claude/pipeline/TASKBOARD.md
 M .claude/pipeline/handoffs/TASK-1102-programmer.md
 M .claude/pipeline/handoffs/TASK-1104-buildmaster.md
 M .claude/pipeline/qa/TASK-1103.md
?? .claude/pipeline/handoffs/TASK-1105-programmer.md
?? .claude/pipeline/handoffs/TASK-1106-programmer.md
?? .claude/pipeline/handoffs/TASK-1111-buildmaster.md
?? .claude/pipeline/handoffs/TASK-1112-programmer.md
```

**9 lines. NON-EMPTY.**

- ⇒ **(7a-i) does not apply** — a commit is owed, and it was made. `--allow-empty` never came near this run.
- ⇒ **(7a-ii)(b)'s on-disk-vs-tracked count control is NOT triggered.** That control is mandatory
  **only on a zero derivation**, because zero is the one output where *clean* and *broken* print the
  same thing. This derivation returned **nine** lines and therefore **proved its own liveness** —
  a reader that returns nine names cannot be a dead reader.
- ⇒ **`!!` (ignored) lines: ZERO.** The new flag surfaced nothing. **No ignored-file finding is
  routed to the manager from this run.** The class the flag was added to catch is **clean here** —
  and, per the amendment's own warning, that is a *measurement at this instant*, not a standing fact.

**List-print instant: `2026-09-07 19:06:07`.** Every candidate's mtime precedes it (see §3), so
**nothing falls in the "ARRIVED ⇒ LEAVE IT" class** (`TL-§6` refinement).

---

## 3. THE FULL CLASSIFIED CENSUS — every line, its verdict, and the test that produced it

**An omitted candidate is indistinguishable from one never seen** (cl. 7a). All 9 appear below.

| # | line | mtime | test applied | verdict |
|---|---|---|---|---|
| 1 | ` M CONVENTIONS.md` | 18:57:43 | **cl. 7a-iii precedence** — the table excludes it *by default*; **my row names it explicitly** in spec (2) | ✅ **TAKEN — the NAMING WINS.** See §4 |
| 2 | ` M TASKBOARD.md` | 19:02:50 | **cl. 7a-iii precedence** — same; **my row names it explicitly** | ✅ **TAKEN — the NAMING WINS.** See §4 |
| 3 | ` M handoffs/TASK-1102-programmer.md` | 18:58:58 | named in row floor (3); **strike site 1 — the ORIGIN of the false claim** | ✅ **TAKEN** (payload) |
| 4 | ` M handoffs/TASK-1104-buildmaster.md` | 18:58:41 | named in row floor (3); **strike site — §6(1)** | ✅ **TAKEN** (payload) |
| 5 | ` M qa/TASK-1103.md` | 18:58:32 | named in row floor (3); **strike site ×2** | ✅ **TAKEN** (payload) |
| 6 | `?? handoffs/TASK-1105-programmer.md` | 18:36:14 | untracked `.md` from a **closed** row (`TASK-1105-VEIL-SWING`, 150 lines, complete); in **no** other host's pathspec | ✅ **TAKEN — ORPHAN** (cl. 7) |
| 7 | `?? handoffs/TASK-1106-programmer.md` | 18:49:51 | named in row floor (3); **this is the MEASUREMENT that falsified the claim** — the evidence the whole commit rests on | ✅ **TAKEN** (payload) |
| 8 | `?? handoffs/TASK-1111-buildmaster.md` | 18:35:37 | **another host's minted handoff.** Table: minted ⇒ excluded *for its own author*, *"the NEXT host takes it under row 1"* — **I am the next host** | ✅ **TAKEN — ORPHAN** |
| 9 | `?? handoffs/TASK-1112-programmer.md` | 18:54:04 | untracked `.md` from a closed row (`TASK-1112-GFX-MEASURE`, 368 lines, complete) | ✅ **TAKEN — ORPHAN** |

**Counts: 9 candidates · 9 TAKEN · 0 named-and-left inside the pathspec · 0 `!!` ignored · 0 ARRIVED.**

### 3b. The floor item that was ABSENT from the derivation (7a-i's new classifier rows)

My row's floor (3) names `handoffs/TASK-1129-programmer.md` **"IF it exists at your instant."**
It did not appear in the derivation, and the amendment supplies exactly the vocabulary for that:

```
$ git cat-file -e HEAD:.claude/pipeline/handoffs/TASK-1129-programmer.md   → NO  (not in HEAD)
$ test -f .claude/pipeline/handoffs/TASK-1129-programmer.md                → NO  (not on disk)
```

⇒ **NEVER MINTED — not a candidate.** `TASK-1129` had not reported at my instant. **Saying so
explicitly is the point of the rule:** an *unwritten* handoff and a *swept* one are different facts,
and a silent census conflates them. **Nothing of `TASK-1129`'s is missing from this commit** — there
was nothing to miss. It will be minted by that row and swept by a later host.

### 3c. Named-and-left OUTSIDE the pathspec — the concurrent `Source/` lanes

Present in the **full-tree** status, deliberately **NOT** in my pathspec (row spec (1)):

| path | owner | disposition |
|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeCombatStatics.cpp` / `.h` | `TASK-931` (veil predicate) / `TASK-1126` | ⛔ **NAMED AND LEFT** |
| `Source/GitClaudeUnrealTest/Siegebound/CombatantHealthBarComponent.cpp` / `.h` | concurrent lane | ⛔ **NAMED AND LEFT** — *these two appeared **during** my run; they were **not** in my first status read* |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeGraphicsSettingsSubsystem.h` | `TASK-1113` (graphics subsystem) | ⛔ **NAMED AND LEFT** (untracked) |

**No other lane's uncommitted work was absorbed.** The editor's source-control provider re-staged
nothing (🧑 Jonathan set `Provider=None` on 2026-09-07); no unstage was needed and **no `git restore`
was run at any point in this task.**

---

## 4. THE cl. 7a-iii DISPOSITION, PRINTED WITH ITS REASON (required)

> **`CONVENTIONS.md` and `TASKBOARD.md` were STAGED.**
>
> **Which of the two applied:** the **ROW**, not the table. The cl. 7a table's last row excludes both
> as *"not this duty's business — they ride on a row that NAMES them."* **`TASK-1131`'s spec (2)
> names them explicitly.** Under **7a-iii** the table's exclusion is a **DEFAULT, not a prohibition**,
> and *"an explicit naming in the host's own row overrides it — that naming IS the 'row that names
> them' the table defers to."*
>
> **Why the row names them:** the law text **is the payload**. `SC-§97` and `SC-§98` live in
> `CONVENTIONS.md`; two of the six strikes live in `TASKBOARD.md`. A host that left them would have
> committed the corrected *handoffs* while the **false law stayed uncommitted** — the precise failure
> this row exists to prevent.
>
> ⚠️ **`TASK-1128` takes the OPPOSITE disposition on these same two files, and that is correct:**
> that row does **not** name them, so for it **the table wins and it leaves them.** Opposite actions,
> same law, no contradiction — the difference is entirely whether the row does the naming.

---

## 5. THE ZERO-`Source/` / ZERO-`Content/` PROOF (row spec (1))

Run over **the commit's own file list**, not the index.

### 5a. ⚠️ The first attempt was a BROKEN READER — recorded, because it would have passed

The git root is **`C:/GitProjects/GitHub/GitClaudeUnrealTesting`**, one level **above** the project
dir, so every tracked path carries a **`GitClaudeUnrealTest/`** prefix. My first guard was anchored:

```
git diff --cached --name-only | grep -c '^Source/'     →  0
```

That `0` was **true for the wrong reason**: `^Source/` can **never** match
`GitClaudeUnrealTest/Source/...`. **A guard that cannot fail is not a guard** — this is `SC-§96`'s
shape exactly (*a broken reader is indistinguishable from a clean result*), and it is the same
family as `SC-§97`, which this very commit exists to record. **It was caught and replaced before
the commit, not after.**

### 5b. The corrected guard, WITH ITS POSITIVE CONTROL

```
$ printf 'GitClaudeUnrealTest/Source/Foo.cpp\n' | grep -c 'Source/'
1                                    ← POSITIVE CONTROL: the grep CAN match a Source/ path

$ git status --short --untracked-files=all | awk '{print $2}' | grep -c 'Source/'
5                                    ← the same grep finds the concurrent lanes' 5 dirty Source files

$ git show --pretty=format: --name-only HEAD | grep 'Source/'  | wc -l
0                                    ← THE COMMIT
$ git show --pretty=format: --name-only HEAD | grep 'Content/' | wc -l
0                                    ← THE COMMIT
```

✅ **ZERO `Source/**` and ZERO `Content/**` in commit `001b331` — proven by a grep demonstrated to
match when a match exists.** `Content/Maps/L_Arena.umap` was never staged, never touched.

### 5c. The commit's complete file list (9 files, all under `.claude/pipeline/`)

```
GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md
GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md
GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1102-programmer.md
GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1104-buildmaster.md
GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1105-programmer.md
GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1106-programmer.md
GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1111-buildmaster.md
GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1112-programmer.md
GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1103.md
```

---

## 6. `git show --stat HEAD` — the COMMIT, not the index (`§25c` cl. 2)

```
commit 001b3314bde05ab6a5f785a24d691caf74b27205
Author: Jonathan Wesely <wesely.jonathan@gmail.com>
Date:   Mon Sep 7 19:07:35 2026 -0700

    TASK-1131: a false, load-bearing law is struck in six places - "RotationRate.Roll = 0
    freezes roll" was a citation nobody ever measured, and TASK-1106 measured it false

 9 files changed, 1719 insertions(+), 15 deletions(-)
 create mode 100644 GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1105-programmer.md
 create mode 100644 GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1106-programmer.md
 create mode 100644 GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1111-buildmaster.md
 create mode 100644 GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1112-programmer.md
```

**Staging note:** `git commit -- <paths>` **refuses untracked paths** (*"did not match any file(s)
known to git"*). The four `??` orphans were staged first with an **explicit `git add -- <4 paths>`**,
the index was then **read back and verified to contain exactly those four**, and the commit was made
by full explicit pathspec. **No `add -A`, no `add .`, no `commit -a` at any point.**

---

## 7. THE READ-BACK — the strikes are IN THE COMMIT (`git show HEAD:<path>`)

Read out of the **commit object**, not the working tree, not the index.

**Site 1 — `handoffs/TASK-1102-programmer.md` §2b (THE ORIGIN):**
```
77: ### 2b. ~~WHY A ROLL, ONCE PRESENT ON THE HERO, IS NEVER SELF-CORRECTED~~ 🚨⛔ **THIS
     SECTION IS THE ORIGIN OF A FALSE LAW. STRUCK IN PLACE 2026-09-07 BY THE MANAGER.**
79: ~~`AGitClaudeUnrealTestCharacter` ctor `:27` — `RotationRate = FRotator(0.0f, 500.0f, 0.0f)`.
83: handed a value that nothing upstream will ever clean, and it has to clean it itself.~~
```

**Site 2 — `qa/TASK-1103.md` (×2):**
```
~~`RotationRate = (0, 500, 0)` (`GitClaudeUnrealTestCharacter.cpp:27`) means `PhysicsRotation`
corrects **yaw only**, so any roll that lands is held forever.~~
~~`RotationRate = (0, 500, 0)` means nothing ever cleans it;~~
```

**Site 3 — `handoffs/TASK-1104-buildmaster.md` §6(1):**
```
254:   Nothing in `Source/**` writes roll onto the hero capsule, and ~~`RotationRate = (0, 500, 0)`
256:   that lands is held forever.~~
```

**Sites 4+5 — `TASKBOARD.md`:** `3` struck `RotationRate` lines present in the committed blob, incl.
```
1123: RotationRate = (0, 500, 0)` MEANS A ROLL THAT LANDS IS ⛔ HELD FOREVER~~ 🚨⛔⛔ **FALSE — ...
```

**The two new laws, out of the committed `CONVENTIONS.md`:**
```
4434: ### SC-§97 ... AN INHERITED *"FACT"* THAT NO ROW EVER MEASURED IS A CITATION, NOT A
      MEASUREMENT — AND A CITATION GAINS AUTHORITY AT EVERY HOP WHILE NOBODY ANYWHERE
      HOLDS THE EVIDENCE.
4425: ### SC-§98 ... A LOG CHANNEL IS AN INSTRUMENT WITH AN OFF SWITCH. SET THE VERBOSITY
      FIRST, READ IT BACK, AND QUOTE THE LINE THAT PROVES THE CHANNEL SPEAKS.
```
Markers `SC-97-CITATION-NOT-MEASUREMENT` and `SC-98-CHANNEL-SPEAKS`: **1 each, present in the commit.**

> ⚠️ **A SECOND FAILED READ, RECORDED.** My first read-back regex (`~~[^~]*RotationRate[^~]*~~`)
> returned **empty** for sites 1 and 3 — because both strikes **span multiple lines** and a
> single-line regex cannot cross a newline. **That empty result was NOT reported as an absence.**
> It was re-read with a line-oriented grep, which found both. **Twice in one task an instrument
> returned a clean-looking nothing that was actually a broken reader** (§5a and here) — on the very
> commit that writes `SC-§97` and `SC-§98` into law. Recorded because the coincidence is the lesson,
> not an anecdote: *a silence is only evidence once you have proven the instrument can speak.*

---

## 8. WHAT STAYED DIRTY, AND WHOSE

Re-run of the standing command **after** the commit:

```
$ git status --short --untracked-files=all --ignored -- .claude/pipeline/
(no output)
```

✅ **The `.claude/pipeline/` floor is EMPTY at this instant — every candidate was swept.**

Still dirty in the wider tree, **all of it another lane's, none of it mine to take:**

| path | owner |
|---|---|
| `Source/.../SiegeCombatStatics.cpp` · `.h` | `TASK-931` / `TASK-1126` |
| `Source/.../CombatantHealthBarComponent.cpp` · `.h` | concurrent lane — **arrived mid-run** |
| `Source/.../SiegeGraphicsSettingsSubsystem.h` (untracked) | `TASK-1113` |

**Two files that WILL be dirty for the next host, by construction:**

1. **`handoffs/TASK-1131-buildmaster.md`** — this file. **Born outside its own commit** (it carries
   the hash). Minted orphan, bounded at one, **never amend** — **the next host takes it.**
2. **`TASKBOARD.md`** — re-dirtied immediately after the commit by my own single-line `- status:`
   edit closing this row. **The next host takes that.**

⚠️ **The concurrent lanes edit their own `- status:` lines in `TASKBOARD.md` while I work.** Per the
row: **whatever state the file held at my staging instant is what shipped** (mtime `19:02:50`); any
later edit of theirs stays dirty for the next host. **I did not chase them and did not re-read the
board to catch up.** If a `TASK-931`/`TASK-1113` status line looks stale in `001b331`, that is
**expected and correct**, not a loss.

---

## 9. FINDINGS FOR THE MANAGER

1. ⚖️ **No `!!` findings.** The amended `--ignored` flag returned **zero** ignored files under
   `.claude/pipeline/` — nothing needs the *"is the ignore right, or is the file an orphan?"* ruling.
2. ⚠️ **Three handoffs had gone unswept by prior hosts** — `TASK-1105`, `TASK-1111`, `TASK-1112`
   (plus `TASK-1106`, which my own floor named). This is the **`TASK-1094` shape the amendment was
   written to catch, and cl. 7a caught it on this run.** `TASK-1111-buildmaster.md` is notable: it is
   *the handoff that documents the amendment*, and it was **itself still unswept** — the same irony
   already recorded in the law about `TASK-1110`. **No new row needed; the mechanism worked.**
3. 💡 **Possible law refinement for the manager's judgement, not a defect:** `git commit -- <paths>`
   silently **cannot** take untracked paths. Every cl. 7a run whose census includes `??` lines must
   `git add` them first. The rule says *"commit by explicit pathspec"* without noting this, and a
   host that omits the `add` gets a **hard error** (safe) — but one that reaches for `add -A` to fix
   it gets a **catastrophe** (staging the concurrent `Source/` lanes). **Worth one sentence in
   cl. 7a: *"`??` candidates are staged with an explicit `git add -- <paths>`; never `add -A`."***

---

## 10. WHAT THIS COMMIT MEANS

`TASK-1106` measured that `RotationRate.Roll = 0` **does not freeze roll** — it **arms** UE's
snap-upright override (`CharacterMovementComponent.cpp:6698-6710`, gated on
`p.PreventNonVerticalOrientationBlock`, **default `1`**); 89.9° on a living hero was gone in
**0.119 s**. The belief was the **exact inverse** of the engine's behaviour, and it had travelled
**four files, one board row twice, and two verbal relays to 🧑 Jonathan**, gaining authority at every
hop while **nobody anywhere held the evidence.**

**It is now false in `HEAD`, not merely in a working tree.** That was the entire point of this row:
a correction that lives only on disk is **one `git restore` from being the falsehood again.**

**No behaviour changed. No `Source/` byte changed. Not pushed.**
