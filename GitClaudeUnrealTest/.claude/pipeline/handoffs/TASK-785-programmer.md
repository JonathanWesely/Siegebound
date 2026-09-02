# TASK-785 — the ladder-socket FALLBACK literals move to match the translated mesh

**Agent:** gameplay-programmer · **Date:** 2026-09-02 · **Status:** `ready-for-qa`
**Suite delta: ⭐ ZERO tests added.** One assertion REPAIRED (its expectation and its two label strings); no
`IMPLEMENT_SIMPLE_AUTOMATION_TEST` added or removed — the file still declares **12** test macros, exactly as it did
before this task. ⛔ Nothing to add to TASK-780's one suite total on my account.

⛔ **No compile, no editor, no MCP, no Git.** Two files touched, and only two.

---

## 1. THE LITERALS — before / after

`Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.cpp` (was `:70-71`, now `:91-92`):

| | BEFORE | AFTER |
|---|---|---|
| `AClimbableTower::LadderFootDefaultRelative` | `(-450.f, 0.f, 0.f)` | **`(-460.f, 0.f, 0.f)`** |
| `AClimbableTower::LadderTopDefaultRelative` | `(-150.f, 0.f, 1200.f)` | **`(-160.f, 0.f, 1200.f)`** |

⭐ **`Δ = Top − Foot` is STILL `(300, 0, 1200)`** ⇒ length `1236.9317`, lean `75.9638°` — **unchanged**, because
TASK-783's move is a **pure translation** (248/558 verts by exactly `δ = (−10,0,0)`, 310 unmoved; body, plinth, deck
and all 8 hulls byte-identical). ⇒ ⛔ **no other number anywhere had to be re-derived**, and the units' shipped
`A_SiegeBiped_Climb` needs no re-export (rung plane held at `−22.0`).

**The derivation comment above them moved with them, because a stale derivation is how the next person "restores"
the old number:** foot `−460` clears the `X ≤ −364` eroded nav carve by **96 uu** (was 86 — the move *improves* it);
top `−160` sits **76 uu** inside the deck poly's surviving `X ∈ [−236, +236]` (was 86 — **the one margin the move
spends**). Both figures are TASK-783's **measurements**, not my arithmetic.

I also wrote the *reason* into the file, at length and deliberately: these two constants are **no longer "just the
defaults"** — since 2026-09-02 they carry `TOWER-§8.5a`'s **licence**. The old pair reconstructs the line whose hero
standoff is `51.619` against a required `56`, i.e. the exact geometry `§8.5a` is **void** on, and it would have done
so **degrade-open, with one Warning and every readback correct**.

## 2. THE REPAIRED ASSERTION

`Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeClimbableTowerTest.cpp` — the fixture pins (was `:244-245`, now
`:256-257`) and the two rows that consume them in test 6 (`LadderLinkIsABothWaysSmartLinkOnThePinnedClimbLine`, now
`:853-856`):

```cpp
const FVector PinnedLadderFootRelative(-460.f, 0.f, 0.f);   // was (-450.f, 0.f, 0.f)
const FVector PinnedLadderTopRelative(-160.f, 0.f, 1200.f); // was (-150.f, 0.f, 1200.f)
```
```cpp
TestEqual(TEXT("(c) The link's START is the pinned LadderFoot (−460, 0, 0) — 96 uu clear of the body's eroded nav carve"),
    LeftPoint, PinnedLadderFootRelative);
TestEqual(TEXT("(c) The link's END is the pinned LadderTop (−160, 0, 1200) — 76 uu inside the deck's surviving navmesh poly"),
    RightPoint, PinnedLadderTopRelative);
```

⛔ **NOT weakened, and ⛔ NOT "synced" by reading the constants back off the class under test** — the pins are still
typed **from the law**, in that direction, which is the only reason they could fire at all. I recorded *in the
fixture comment* that this tripwire **has now actually fired once**, so the next person to see it red knows it is the
instrument working.

⭐ **The two DERIVED pins (`PinnedClimbLineLengthUU = 1236.9`, `PinnedClimbLeanDegrees = 76.0`) are untouched** — `Δ`
is translation-invariant. Their comment now says so, which is the cheapest possible proof the move was pure.

⭐ **No CONTACT scenario row changed meaning.** Every scenario point in the file is expressed *relative* to the two
pins (`PinnedLadderFootRelative + FVector(-100, …)`, `PinnedLadderFootRelative.X - 400.f`, …) and the predicate is a
function of distances and bearings only ⇒ translating both endpoints by the same `δ` leaves every expectation
(`0.500 s`, `0.180 s`, the `TooFar` rows, the latch rows) arithmetically identical. QA should confirm this rather than
take my word: it is the one way this "surgical" change could have quietly broken nine other rows.

---

## 3. ⭐ MY JUDGEMENT ON THE SHAPE — **proposed, ⛔ NOT implemented**

**The question asked: is a silent numeric fallback the right shape at all, now that it has been wrong once?**

**My answer: the fallback is right and must stay degrade-open — but its *classification* changed on 2026-09-02 and
the code does not reflect that yet.** Three things, in order of how strongly I hold them:

**(a) ⛔ Do NOT make the fallback refuse to start a climb.** That is degrade-**closed**, and `TOWER-§8.4(A)` already
refuses it in terms — a tower whose deck cannot be reached is the *one* outcome the law forbids, and the fallback is
also what makes the art lane and the code lane genuinely parallel. Worse, it would be **aimed at the wrong pawn**: the
licence that was at risk is the **hero's** (capsule radius 42 ⇒ clearance `61.32` on the new line, `51.62` on the old);
the **unit's** clearance is `69.32` and was never in question. A blanket refusal would kill a working feature for units
to protect a hero-only licence. ⇒ **refused.**

**(b) ⭐ The shape I would actually propose — make the fallback LICENCE-AWARE rather than merely loud.** The real
defect was never "the log was too quiet"; the warning fired correctly and named the missing socket. The defect is that
**nothing downstream can tell a licensed line from an unlicensed one**, because the fallback's product is
indistinguishable from the mesh's. Concretely, and cheaply: `ConfigureLadderLink` already computes
`bFootUsedDefault || bTopUsedDefault || bDegenerate` — **keep that one boolean as a member (`bLadderLineFromFallback`)
and let the HERO's `§8.5a` deck-breach window consult it**, so the one path whose safety argument is measured against
the *authored* mesh can decline the *unmeasured* one, while units climb exactly as they do today. That is a real
behavioural change on a failing path, it reaches into `HeroCharacter.{h,cpp}` (**TASK-778 is live in it**), and it is a
**manager/law question** (`TOWER-§8.4(A)` vs `§8.5a`), so I implemented **none** of it. ⚖️ It is also arguably
unnecessary: if the sockets ever fail to import, the *first* thing that happens is TASK-780's PIE eye sees the Warning.

**(c) ⚠️ "Just raise the Warning to an Error" is NOT the free strict addition it looks like — and this is measured.**
The suite **never** exercises this path (every one of the 12 tests is CDO / reflection / pure-statics; ⛔ no
`SpawnActor`, ⛔ no `UWorld`, so `ConfigureLadderLink` never runs headlessly), so a verbosity bump would **not** turn
the suite red — that much I checked. **But `TASK-779` item (7) explicitly makes the *absence of that Warning line* in
TASK-780's PIE log the runtime confirmation that the sockets resolved.** Changing its severity moves the string the
gate is looking for, and an `Error` during PIE additionally trips the editor's own "PIE encountered errors" surfacing.
⇒ **a live gate reads this line; I am not moving it under them mid-batch.** If the manager wants it, it is one word
plus one line in TASK-779's checklist, and it should be **boarded**, not slipped in here.

**⚖️ The one-line version for the record:** *the fallback's job is to keep the tower working; it is not qualified to
vouch for a safety licence, and until something makes that distinction explicit the tripwire in the test file is the
only thing standing between a socket-import failure and a silently unlicensed hero climb.* It worked this time.

---

## 4. ⚠️ WHAT QA SHOULD SCRUTINISE — including three things I was fenced OUT of

1. ⛔⛔ **`ClimbableTower.h:445` / `:448` STILL DERIVE THE OLD MARGINS** — *"`−450` clears it by **86 uu**"* /
   *"**86 uu inside it**"*, on the very declaration my `.cpp` comment points at (*"derived in full on the header's
   declaration"*). ⛔ **Out of my fence** (the fence names `ClimbableTower.cpp` only, and **TASK-784 holds a live
   narrow grant in that header** — `LadderContactRadiusUU`), so I did not touch it. **It should be boarded, and it is
   not cosmetic: it is the one place a future reader is told to look for the derivation, and it currently teaches the
   old numbers.** The replacement text is exactly the `.cpp`'s: foot `−460` clears `X ≤ −364` by **96 uu**; top `−160`
   sits **76 uu** inside `X ∈ [−236, +236]`.
2. ⚠️ **`SiegeLadderClimbStatics.h:214-215`** carries the same stale prose (*"the top sits 86 uu inside … the foot
   clears … by 86 uu"*). ⛔ **Explicitly fenced out.** Comment-only, no behaviour — but same repair.
3. ✅ **`Tests/SiegeLadderClimbTest.cpp:78-79` and `Tests/SiegeHeroLadderClimbTest.cpp:84` still use `(-450,0,0)` /
   `(-150,0,1200)` — and that is BENIGN, deliberately reported rather than "fixed".** They are *scenario* endpoints for
   pure statics (one is literally named `ArbitraryFoot`), not law pins, and every predicate they feed depends only on
   `Δ` — which is invariant. ⛔ Both files are other live tasks' (776/777 and 778). ⚖️ Worth a tidy-up later purely so
   nobody greps `-450` and thinks the move was half-done.
4. ⛔⛔ **A SECOND TRIPWIRE IS ABOUT TO FIRE IN MY FILE, AND IT IS ⛔ NOT MINE TO SILENCE:**
   `SiegeClimbableTowerTest.cpp:1584` asserts *"`LadderContactRadiusUU` ships at `K-5`'s 150 uu"* against the fixture
   pin `PinnedContactRadiusUU = 150.f` (`:278`). **TASK-784's item (4) changes that class default to `300.f` (`K-6`).**
   ⇒ **that row goes RED the moment TASK-784 lands, in a file TASK-784 does not own.** ⛔ I did **not** pre-emptively
   change it — asserting `300` before the default moves would be asserting a world that does not exist, and my fence is
   the literal move. **Whoever lands second must reconcile the pin, its label, and `K-5`→`K-6` in the comment.**
   ⚠️ **This is a real collision between two live tasks, not a nit — flag it to the manager.**
5. 📌 **A stale line citation for the gate:** `TASK-779` item (7) cites the fallback Warning at
   `ClimbableTower.cpp:331-342`. It is now at **`:389-400`** (it was already `:368-379` before my edit — the drift is
   TASK-777's, not mine). Same line, same text, ⛔ unchanged by me.
6. ✅ **Everything QA passed on the tower is intact and I re-read each one to be sure:** the ninth-exit guard
   `const bool bLadderOccupied = ActiveClimber.IsValid();` with **no identity comparison** · `EvaluateLadderEntry`'s
   identity → team → occupancy precedence · `GetSocketTransform(..., RTS_Actor)` in `ResolveLadderSocketRelative`
   (⭐ the spawn-squash divide-out) · `CanTeamAscend` with no capacity/type term · `ShouldLinkAllowPathfinding`'s
   fail-open ladder · the contact trigger's radius/cone/dwell/latch and its contact-terms-**first** ordering.
   ⛔ **Not one of them is in my diff.** My diff is: two `FVector` literals, two `TestEqual` expectations, two label
   strings, and comments.

---

## 5. FILES TOUCHED — exactly two

- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\ClimbableTower.cpp`
  — `:91-92` the two literals; `:65-90` the derivation + the "why this is not cosmetic" block.
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\Tests\SiegeClimbableTowerTest.cpp`
  — `:256-257` the two pins (+ their fixture comment, which now records that the tripwire fired); `:844-856` the (c)
  block's derivation comment and its two assertion labels; the derived-pins comment at `:259-262`.

⛔ **Assets referenced:** `/Game/Meshes/SM_WatchTower`'s sockets **`LadderFoot (−460, 0, 0)`** and
**`LadderTop (−160, 0, 1200)`** — by name only, exactly as `CONVENTIONS` "Static-mesh SOCKET names" spells them.
⛔ I did not open the editor, the mesh, or `Content/`.
