# TASK-1050 — [FOGDOC-TRUTH] the documented parent that the shipped asset does not have

**Agent:** gameplay-programmer · **Date:** 2026-09-05 · **Status:** `ready-for-qa`
**Gate:** `TASK-1051` · **Blocker:** `TASK-1044` — **DISCHARGED** (committed `c79bf5b`, parent `ef2c901`)
**Sole write:** `Source/GitClaudeUnrealTest/Siegebound/FogVolume.h` — **comment block only**
**Suite delta:** **0 / 0** — ⛔ **DECLARED-NOT-EXECUTED** (`TL-§5c`). I did not compile and did not run the suite.

---

## 0. SCOPE, FIRST, BECAUSE `TASK-1051` CLAUSE 1 IS A SCOPE CHECK

| claim | how I proved it | result |
|---|---|---|
| only `FogVolume.h` touched under `Source/` | `git status --porcelain -uno` | ✅ **1 file** |
| **zero executable lines** | `git diff -U0` ⇒ strip `+`/`-` ⇒ filter out every line matching `^ \*` ⇒ **empty** | ✅ **0 non-comment lines** |
| `CONVENTIONS.md` untouched | not in `git status`; I never opened it for write | ✅ |
| `BP_SiegeFog` / `Content/FogArea/**` untouched | never addressed the editor; MCP not contacted this task | ✅ |
| nothing staged, no commit | index empty throughout | ✅ |

`git diff --stat` ⇒ **68 insertions, 6 deletions, 1 file**. Net **+62** lines, and the file went
473 ⇒ 535 lines, which agrees.

⚠️ **Line endings — checked, and NOT a finding.** `git diff` emits
`LF will be replaced by CRLF the next time Git touches it`. I measured rather than assumed: the
working tree is **535 CRLF / 535 total** (homogeneous), and the committed blob is **473 / 473**.
My edit preserved CRLF exactly; the warning is pre-existing `core.autocrlf=true` noise on a
CRLF-storing blob. **No mixed line endings were introduced.**

⚠️ **Expected, per spec cl. 5: this is a header edit, so it triggers a wide rebuild.** Not a finding.

🧑 `.claude/agents/qa-reviewer.md` was already dirty when I started (Jonathan's open call) and I
**did not touch it**. `TASKBOARD.md` was already dirty; I changed **only TASK-1050's own `status:`
line**.

---

## 1. THE CORRECTION — AND THERE WERE **TWO** FALSE SENTENCES IN THIS HEADER, NOT ONE

The row's fact list named one site (the `CoreRedirects` paragraph, `:98-105`). Locating by text
rather than by line number, as instructed, surfaced **a second false sentence in the same file** at
the old `:95`, in the `SPAWNED AT RUNTIME` paragraph. Both are inside my write fence and both are
the *same* falsehood, so I corrected both.

### 1a. The `CoreRedirects` paragraph — the row's named subject

The old text said the five `EditDefaultsOnly` tunables serialise *"into this class's CDO and into
any Blueprint child (`/Game/Blueprints/BP_SiegeFog`)"*. The parenthetical is false. The rewrite
says `any Blueprint child **OF THIS CLASS**`, then states plainly what was wrong, with the
evidence (`get_parent` ⇒ `BP_FogArea_C`, `TASK-1043`, committed `ef2c901`) and the one-parent rule,
and records the measurement: **`AFogVolume` has ZERO Blueprint children in this project.**

### 1b. The second sentence — and it was the more dangerous of the two

Old text: *"⛔ A level-placed `BP_SiegeFog` (`FOG-§6`'s BP child) would ALSO be found by `Find`, so
placing one later is safe and changes nothing here."*

This is false **in the direction that reads as reassurance**, which is why I did not leave it for a
follow-up row. `Find` iterates `TActorIterator<AFogVolume>`, which matches this class and its
subclasses; `BP_SiegeFog` is a `BP_FogArea` child, so it is **invisible to `Find`**. The old
sentence's *conclusion* ("safe") happens to be true while its *mechanism* is exactly backwards —
it is not "found too", it is **never found at all**. A sentence that is right by accident is the
kind that survives a review and then misleads the person who relies on the mechanism.

---

## 2. ⚖️ THE RULING: **(b) DORMANT**, NOT (a) RETIRED — WITH THE ARGUMENT, NOT THE INHERITANCE

Spec cl. 2 and `SC-§79` both say: argue it, do not inherit the expectation. Two people expecting
(b) is a reason for more scrutiny, so I tried to build the (a) case properly and it fails. Below is
the reasoning, including the strongest counter-argument I could construct.

### 2a. First, the predicate question: can a designer override exist at all today? **NO.**

Not asserted — enumerated over every writable home an `EditDefaultsOnly` value has:

| candidate home | verdict | why |
|---|---|---|
| Blueprint class defaults | ⛔ none exist | zero BP children of `AFogVolume` (measured) |
| a level-placed instance's details panel | ⛔ impossible | `EditDefaultsOnly` is **archetype-only by definition** — not editable on an instance |
| `.ini` / config | ⛔ impossible | **none of the five carries a `Config` specifier** (read off the five `UPROPERTY` lines) |
| the native CDO | ⛔ not designer-writable | its values come from the **constructor**, not from a package anyone edits |

⇒ Nothing serialised exists for a rename to orphan. **The hazard cannot fire today.** So the
paragraph is indeed currently moot, and the row is right to demand which *kind* of moot.

### 2b. Why not RETIRED

RETIRED means *it can never apply*. That requires `AFogVolume` to be **incapable** of having a
Blueprint child. It is not, and this is measurable rather than a matter of opinion:

- `AActor` is declared `UCLASS(BlueprintType, Blueprintable, config=Engine, …)` — read at
  `Engine/Source/Runtime/Engine/Classes/GameFramework/Actor.h:281`.
- `Blueprintable` is **inherited** by subclasses unless a subclass declares `NotBlueprintable`.
- `AFogVolume` is a **bare `UCLASS()`** — it declares no such opt-out.

⇒ **Right-click ⇒ Blueprint Class ⇒ `AFogVolume` succeeds today**, in one gesture, with no code
change and no review. The moment that child is saved with any of the five changed, a serialised
override exists and the full hazard is back. That is the textbook definition of dormant, not
retired.

And **nothing warns the person who does it**, which I also measured rather than assumed:
`grep -rn "CoreRedirects" Config/` ⇒ **zero entries**, and no test asserts on any of this.

### 2c. The strongest RETIRED case — stated fairly, then refuted

> *The project has committed to a two-object split, and `FindOrSpawn` spawns
> `AFogVolume::StaticClass()` explicitly. A BP child would therefore never be the instance the game
> uses, so its overrides could never take effect, so the hazard is moot forever.*

It fails on two independent counts, and the second is fatal:

1. **It conflates two different failures.** The `CoreRedirects` hazard is about a saved value being
   *silently dropped on load*. That drop happens at package load whether or not the game ever
   spawns the child. "The override wouldn't take effect" is a *different* bug; it is not a defence
   against this one.
2. **Its premise is false by construction.** `Find` matches **subclasses**, and `FindOrSpawn`
   **calls `Find` before it spawns** (`FogVolume.cpp:59-63`). A level-placed Blueprint child would
   not merely sit there holding overrides — it would be **returned as the one authoritative
   fog-state actor**, its tuned five in force, and the native spawn would never happen.

⇒ So the honest verdict is stronger than plain dormancy: **dormant with a live wire attached.**
The hazard is not just waiting to become reachable; the path by which it becomes reachable also
hands the new object *authority over fog state*. I put that in the comment.

### 2d. The reusable lesson, recorded in the comment

The wrong name did not merely fail to help — it **actively defeated the audit**. Anyone asking
"does a BP child exist yet?" would have found `BP_SiegeFog`, answered **yes**, and been wrong in
either direction: believing the hazard already live invites a pointless redirect; believing it
already handled invites a free rename. The comment now carries an explicit
⛔ **do not delete this paragraph on the strength of "no child exists"** — that absence *is* the
dormancy, not a refutation of it. (Guarding the deletion is the point of `TASK-1051` cl. 3, so I
armed the file against it directly.)

---

## 3. THE SEAM FINDING (cl. 3) — RECORDED AS A DATED MEASUREMENT

Recorded in the header, attributed to `TASK-841` (artist), dated **2026-09-05**, and phrased so it
**expires rather than forbids**: *"if someone later adds a `UFUNCTION`, this sentence EXPIRES
rather than forbids."*

**I re-counted it myself rather than relaying it** — `TASK-1051` cl. 4 asks QA to do the same, so
here is my number to check against:

```
grep -c "UFUNCTION" FogVolume.h  ⇒  1
```

⚠️ **That `1` is not a declaration.** The single occurrence is at the old `:286`, inside the comment
on `BrightSunWindowSeconds`, and it reads *"⛔ Public, plain C++ static, ⛔ **NOT** a `UFUNCTION`"*.
⇒ **`UFUNCTION` declarations: ZERO.** A bare `grep -c` returning `1` would read as "one exists",
which is the opposite of the truth, so I noted the discrepancy **inside the comment itself** to
stop the next counter tripping on it.

⇒ A Blueprint cannot poll fog state today; **C++ lifetime control is the only available seam.**
Corroborated from the other side by `TASK-1043`'s independent read-back: `list_variables` on
`BP_SiegeFog` ⇒ `[]`, so the visual holds no state either.

---

## 4. 🚩 THE OPEN QUESTION — ROUTED, NOT CLOSED (cl. 4)

> **Is the two-object split (`AFogVolume` = C++ rules + lifetime · `BP_SiegeFog` = vendor visual)
> THE DESIGN, or is a real Blueprint child of `AFogVolume` MISSING?**

**Status: OPEN. Routed to ⭐ MANAGER / 🧑 Jonathan.** I did not settle it, and the comment says in
so many words that it does not settle it: *"A comment is the wrong instrument for that answer and
this one does not pretend to give it."*

**What I can contribute as evidence, without deciding it:** the two readings are not
observationally equivalent. If the split **is** the design, then the `CoreRedirects` paragraph
guards a door nobody intends to open, and the honest follow-up is to consider marking `AFogVolume`
`NotBlueprintable` — which would convert my (b) into a genuine (a). If a BP child is **missing**,
then the five tunables currently have **no designer-facing home at all**, and that is a live gap
rather than a dormant one. ⛔ **I am not proposing either.** Both are decisions with owners who are
not me, and I flag them only so the decision is made knowing what hangs on it.

---

## 5. 🚨 FINDING FOR THE MANAGER — A **THIRD** FALSE SITE, OUTSIDE MY FENCE, DELIBERATELY LEFT

I censused the repository rather than trusting the row's list of sites, and `SC-§91` earned its
keep again: **the relayed count of 2 is a lower bound. The true count is 4.**

| # | site | verdict | who owns it |
|---|---|---|---|
| 1 | `FogVolume.h:95` (old) | ⛔ FALSE | ✅ **me — FIXED** |
| 2 | `FogVolume.h:104` (old) | ⛔ FALSE | ✅ **me — FIXED** |
| 3 | **`FogVolume.cpp:42`** | ⛔ **FALSE — STILL LIVE** | 🚩 **NOT MINE — needs a row** |
| 4 | `CONVENTIONS.md` `FOG-§6` | ⛔ FALSE | ⭐ manager's (already flagged `FOG-§6a`) |
| — | `SiegeFogStatics.h:73` | ✅ TRUE | — (names the visual, claims no parentage) |

**Site 3, verbatim**, inside `AFogVolume::Find`:

```cpp
// a fixed world, so a level that ALSO placed a BP_SiegeFog answers the same actor on
```

False for exactly the reason this whole row exists: a level that places `BP_SiegeFog` produces
**zero** additional `AFogVolume` instances, because it is not a subclass. The sentence describes
tie-breaking between two instances in a situation that cannot arise.

⛔ **I did not fix it, and the restraint is deliberate.** `FogVolume.cpp` is not in my `names:`
write set — `FogVolume.h` is the **SOLE WRITE**, and `TASK-1051` cl. 1 makes any second file a
BLOCKER. Fixing it would have been a one-line comment edit and would still have been the wrong
move: this row exists *because* two agents found a contradiction and neither edited outside their
fence. Doing it "while I'm here" is the same error the row was written to praise the avoidance of.
⇒ **Routed for a follow-up row.** It is comment-only, zero-risk, and non-urgent.

---

## 6. WHAT `TASK-1051` SHOULD SCRUTINISE HARDEST

1. **The scope claim.** Re-run the `git diff` filter yourself; do not take §0 on trust.
2. **My (b) argument, specifically §2c.** If the RETIRED case survives my refutation, the ruling in
   the header is wrong and it is now written into the file in bold. I would rather be corrected here
   than have a confident wrong ruling shipped.
3. ⚠️ **The premise you cannot measure** (`SC-§78`): the parent being `BP_FogArea_C` is
   **ACCEPTED-AS-DECLARED** from `handoffs/TASK-1043-buildmaster.md`. I did **not** open the editor
   and did **not** re-read the asset — I have no independent measurement of the one fact this entire
   row rests on. Say so explicitly in your report.
4. **My `UFUNCTION` count of "1 textual / 0 declarations"** — §3. Your census should reproduce both
   numbers, and the gap between them is the interesting part.
5. **Site 3 in §5** — confirm I left it alone, and that leaving it was right.
