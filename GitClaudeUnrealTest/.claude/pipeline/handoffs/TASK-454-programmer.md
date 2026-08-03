# TASK-454 — programmer handoff

**[W1-RELEASE] The T/E release law now fires on the assistant path too — ONE public entry point**

- **Status:** `ready-for-qa` (reviewed under TASK-446)
- **Date:** 2026-08-03
- **Files touched:** `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.{h,cpp}` — **and nothing else, not one other byte.**
- ⛔ **NOT compiled** (TASK-447 is the single gate). No Git, no editor, no MCP, no PIE, no `Content/`.

**M8 DECLARATION DUTY (verbatim, as required):** adds no replicated property, no new replicated class, no new relevancy tier.

---

## 1. THE DELIVERABLE — the signature

```cpp
// SiegePlayerController.h:474  — public: (section opens at :184, closes at :736)
UFUNCTION(BlueprintCallable, Category = "Siegebound|Commands")
void ApplyArmyWideStance(ESiegeUnitCommand NewCommand);
```

```cpp
// SiegePlayerController.cpp:1006 — the ONE implementation
void ASiegePlayerController::ApplyArmyWideStance(ESiegeUnitCommand NewCommand)
{
    if (bMatchEnded)
    {
        return;
    }
    CancelGroupPick();
    ClearAllUnitGroups();
    SetUnitCommand(NewCommand);
}
```

**Name provenance:** `ApplyArmyWideStance` is the name TASK-443 proposed in its own flag (a) — kept deliberately, so the reviewer reading 443's handoff finds the symbol it predicted rather than a synonym. ✅ **No collision:** `grep -rn "ApplyArmyWideStance" Source/ Plugins/ CONVENTIONS.md` → exit 1, zero hits before this task.

`BlueprintCallable` mirrors `SetUnitCommand`'s existing exposure, so a future HUD button gets the *correct* entry point rather than the bare latch.

---

## 2. VERIFIED MYSELF, NOT RELAYED (spec item 1 required this)

| Claim | How verified | Result |
|---|---|---|
| `CancelGroupPick` / `ClearAllUnitGroups` are `private` | access-specifier map of the header: `public:184` · `protected:736` · `protected:802` · `private:1177`; decls at **1341** and **1458** | ✅ **443 was right** — both private |
| ...and they **stay** private | same map re-run after my edit | ✅ **unchanged — not widened, as ⛔ required** |
| The two key handlers are identical mirrors | read both bodies at the artifact | ✅ byte-identical apart from the stance argument |
| No re-entrancy if the sequence is shared | read `ClearAllUnitGroups` (:3387→) and `CancelGroupPick` (:2945→) in full | ✅ neither calls `SetUnitCommand`; `CancelGroupPick` is explicitly no-op-safe |
| `bMatchEnded` reachable | header :1580 (private member), already used by both handlers | ✅ |

---

## 3. ⚖️ THE SHAPE DECISION — AND THE ONE PLACE I DEPARTED FROM A STATED PREFERENCE (§15)

Spec item (3) says: *"⚖️ **Prefer a shape where the component's existing call site needs no edit at all.**"*

**There is exactly one shape that achieves that, and I rejected it. Stating the mechanism, per §15.**

The component calls `Controller->SetUnitCommand(ESiegeUnitCommand::Attack)` directly (`SiegeAssistantComponent.cpp:1413` / `:1420`). The *only* way for that call site to gain the release law untouched is to **fold `CancelGroupPick()` + `ClearAllUnitGroups()` into `SetUnitCommand` itself**. I checked whether that was even safe before judging it, and it is:

- **No Blueprint callers.** `grep -rl "SetUnitCommand" Content/` → **zero `.uasset` hits.** ⚠️ **And a negative grep is not evidence on its own (§14), so I ran a positive control on the same mechanism:** `grep -rl "OnUnitCommandChanged" Content/` → **hits `Content/UI/WBP_HUD.uasset`.** The probe demonstrably finds BP-referenced symbols in these packages, so the negative result is real evidence, not a blind spot.
- **No recursion**, per the table above.
- **All four C++ callers** (T, E, assistant charge, assistant fallback) want the release law.

**So folding was *available*. I still refused it, on citable mechanisms rather than taste:**

1. ⛔ **`SetUnitCommand`'s own header (:430-438) defines it as a pure latch** — *"sets CurrentCommand, marks bHasIssuedCommand true, and broadcasts OnUnitCommandChanged."* Making it additionally destroy every group and every ground decal is a widening its **name does not advertise**.
2. ⛔ **`ClearAllUnitGroups`' header says it is *"Called by T/E (before SetUnitCommand)."*** Folding makes "before SetUnitCommand" **self-referential and false** — the doc would describe a nesting, not an ordering.
3. ⛔ **A P2 RPC is already named against the pure-latch contract:** `SiegePlayerController.cpp:981` names **`ServerSetUnitCommand`** (M8 doc §4.3#9). Silently widening the function that RPC will wrap changes a contract another milestone has already written down.
4. ⛔ **The spec's own item (1) forbids the half-sequence hazard**, and folding creates the mirror of it: a caller who wants *only* the latch (the reset path, a future RPC, a HUD re-affirm) would have **no way to get it** and would re-implement it — the parallel implementation §2 exists to prevent, re-created from the other side.

⇒ **The primitive stays a primitive; the entry point is the new thing.** This is the shape the spec's *pinned* clause demands ("exactly ONE implementation of the release-then-command sequence"), and the preference it overrides is explicitly labelled a preference.

### ⇒ 🚩 CONSEQUENCE — STOP AND REPORT, exactly as item (3) instructs

**`SiegeAssistantComponent.cpp` needs a two-line swap that I did NOT make, because it is TASK-442/443's file:**

| line | today | must become |
|---|---|---|
| `1413` | `Controller->SetUnitCommand(ESiegeUnitCommand::Attack);` | `Controller->ApplyArmyWideStance(ESiegeUnitCommand::Attack);` |
| `1420` | `Controller->SetUnitCommand(ESiegeUnitCommand::Defend);` | `Controller->ApplyArmyWideStance(ESiegeUnitCommand::Defend);` |

**Also stale once that lands:** the `Warning` logs at `:1412` and `:1419` declare *"Unlike the T key this does NOT run the TASK-344 release law (ClearAllUnitGroups is private), so standing group orders survive."* — that sentence becomes **false** at the moment of the swap and must go with it. `SiegeAssistantComponent.h:957` and `SiegeAssistantCommand.h:59-60` also describe the seam as `SetUnitCommand`.

⚠️ **UNTIL THAT LANDS, THE DEFECT IS STILL LIVE.** My deliverable is what the spec asked for — *"ONE public entry point on the controller that the assistant path **can** call"* — but "can" is not "does". **This needs one more task against the component, and the divergence is not closed until it lands.** I did not touch the component: `git status --porcelain` still shows both as `??`, mtimes unchanged at 12:41 / 12:36.

---

## 4. ⭐ THE READ-THROUGH ARGUMENT — PROVABLY ZERO BEHAVIOUR CHANGE ON THE KEY PATH (spec item 2)

**Before (both handlers, verbatim):**

```cpp
void ASiegePlayerController::OnCmdAttackPressed()          void ASiegePlayerController::OnCmdDefendPressed()
{                                                          {
    if (bMatchEnded) { return; }                               if (bMatchEnded) { return; }
    CancelGroupPick();                                         CancelGroupPick();
    ClearAllUnitGroups();                                      ClearAllUnitGroups();
    SetUnitCommand(ESiegeUnitCommand::Attack);                 SetUnitCommand(ESiegeUnitCommand::Defend);
}                                                          }
```

**After:** each handler is a single call to `ApplyArmyWideStance(<same stance>)`, whose body **inlines to the identical four statements in the identical order**.

**The proof obligations, each discharged:**

- ✅ **Same guard, same position.** `bMatchEnded` is tested **first**, before any call — not moved below the release.
- ✅ **Same order, release BEFORE latch.** `CancelGroupPick()` → `ClearAllUnitGroups()` → `SetUnitCommand()`, exactly as `ClearAllUnitGroups`' header documents. **The key path won the ordering question; nothing conformed to the assistant.**
- ✅ **Same argument.** `Attack` from T, `Defend` from E — the parameter is the *only* thing that ever differed between the two bodies, which is precisely why one entry point suffices.
- ✅ **No guard added and none removed.** In particular I did **NOT** add a `HasAuthority()` check. A client pressing T today runs the pick abort and the group release locally and is refused only inside `SetUnitCommand` (:982, the M8 D5 observer posture). Adding an outer authority gate would have been a *silent behaviour change to the shipped key path* — refused for that reason. The assistant inherits the same posture, which is the point.
- ✅ **Non-virtual, same class, no dispatch introduced.** No override, no delegate, no deferral — a direct call the compiler can inline.
- ✅ **`ApplyArmyWideStance` has no callers other than the two handlers** in this commit, so nothing else can perturb the path.

⇒ **The key path is textually the same four statements; only their address changed.**

---

## 5. ADDITIVE PROOF — `git diff --stat`, and every moved line accounted for

```
 .../Siegebound/SiegePlayerController.cpp           | 629 +++++++++++++++++----
 .../Siegebound/SiegePlayerController.h             | 315 ++++++++++-
 2 files changed, 836 insertions(+), 108 deletions(-)
```

| | insertions | deletions |
|---|---|---|
| baseline inherited from 440/449/453 | 757 | **94** |
| **after TASK-454** | **836** | **108** |
| delta | +79 | **+14** |

### ⚠️ I AM DECLARING A DEPARTURE FROM THE "DELETIONS MUST STILL READ 94" FRAMING — §15, WITH THE MECHANISM

My dispatch brief stated the deletion count *"must still read 94 when you finish."* **The board's own spec item (4) states it conditionally:** *"your edit should leave the DELETION COUNT unchanged **unless the extraction genuinely moves lines** — and if it does, **SAY SO and account for every moved line**."*

⇒ **The absolute is unsatisfiable alongside the pinned contract, and that is a mechanism, not a preference:** the contract requires *"exactly ONE implementation of the release-then-command sequence."* Two handlers cannot both keep their intact copies of that sequence **and** there be only one implementation. **Extraction necessarily deletes the copies.** I therefore followed the board spec, kept the move to the provable minimum, and account for all 14 below.

**`SiegePlayerController.h`: 264 → 305 insertions, deletions UNCHANGED at 10.** ⇒ **the header is 100% additive — I edited not one existing header line.**

**`SiegePlayerController.cpp`: 84 → 98 deletions = the full +14. Every one is a MOVE, not a loss:**

| # | deleted line | where it now lives |
|---|---|---|
| 1 | `void ASiegePlayerController::OnCmdAttackPressed()` | re-emitted verbatim at `:1037` (git paired the signature with the new `ApplyArmyWideStance` line) |
| 2-6 | the 5 `// ATTACK (T) is immediate …` comment lines | re-emitted **verbatim, unedited** at `:1039-1043` |
| 7 | `SetUnitCommand(ESiegeUnitCommand::Attack);` | became the parameterised `SetUnitCommand(NewCommand);` at `:1034` |
| 8-14 | Defend's 7 statement lines (`if (bMatchEnded)` `{` `return;` `}` `CancelGroupPick();` `ClearAllUnitGroups();` `SetUnitCommand(…Defend);`) | **these ARE the shared body** now at `:1028-1034` |

⇒ **Zero lines were destroyed. 14 were relocated into the single implementation, which is the entire point of the task.** Both handlers' original comment blocks survive **unedited in place** — I appended to them rather than rewriting, which is why the comment churn is insertions only.

---

## 6. 440 / 449 / 453 — LANDMARK CHECK, ALL UNDISTURBED

My edits sit at `.h:441-474` + `.h:1449-1456` and `.cpp:1006-1047` + `.cpp:1081-1087`. **The console work lives at `.cpp:4230-4430` — nowhere near.** Verified by re-grep after editing, not assumed:

- ✅ **453's ordering intact:** `Assistant->AttachConsoleWidget(Console)` at **:4348** still precedes `Console->OpenConsole()` at **:4357**. (Absolute line numbers shifted `4324 → 4348` because I inserted ~72 lines *above* them — **the ORDER, which is the actual contract, is unchanged.** QA re-checking 453 should expect the new number.)
- ✅ **449's guard-first intact:** `SetAssistantConsoleOpen(true)` at **:4298** still runs before any creation; `CreateAndAddToViewport` at **:4421** is still downstream of it. The refusal path still constructs nothing.
- ✅ **Single-writer intact:** exactly **one** `bAssistantConsoleOpen = ` assignment, at **:4240**.
- ✅ Nothing reverted, reformatted or "tidied" anywhere in either file.

---

## 7. FOR QA TO SCRUTINISE

1. ⛔ **The live-defect status (§3).** The component still calls the bare latch. **Do not pass this as "the divergence is closed"** — it is closed *on the controller side only*. Confirm you want the component swap as a follow-up task rather than an amendment here.
2. **The folding refusal (§3).** If the manager prefers `SetUnitCommand` to absorb the release law after all, that is **one branch** of my decision and reverses cleanly — but it rewrites the contract of a `BlueprintCallable` that `ServerSetUnitCommand` is already named against. Recorded rather than quietly decided.
3. **The `bMatchEnded` gate is now on the assistant path too** (it is inside the entry point). This is intended — post-match `charge` becomes as inert as post-match T. ⚠️ **But note it IS a behaviour change on the *assistant* path** (today its `charge` latches post-match), which is exactly the consistency the manager ruled for. Flagging it because it is a change, even though it is the wanted one.
4. **Unverifiable without a compile (TASK-447's gate):** that `UFUNCTION(BlueprintCallable)` on a function taking `ESiegeUnitCommand` by value generates cleanly (`SetUnitCommand` next to it proves the pattern, so this is low risk); and that no `.generated.h` ordinal shift disturbs anything. **I did not compile.**
5. ⚠️ **453's link-order warning still stands and is NOT mine:** `USiegeAssistantComponent::AttachConsoleWidget` is declared with no definition yet. Unchanged by this task; do not open a loop on my line for it.

---

## 8. §14 NOTE — THE UNTRACKED TRAP, PAID FORWARD

`SiegeAssistantComponent.{h,cpp}` are **untracked** (`git ls-files` → empty; `git status` → `??`). **`git grep` cannot see them.** Every component fact in this handoff — the two call sites, the two stale warning logs, the line numbers — came from the **Grep tool against the files on disk**, and I re-confirmed existence with `ls -la`. A reviewer who probes with `git grep` will get a **false negative** and conclude the call sites do not exist.
