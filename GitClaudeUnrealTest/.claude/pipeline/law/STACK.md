<!-- MOVED from .claude/pipeline/CONVENTIONS.md on 2026-10-04 by Tools/split_conventions.py. Sections are byte-identical to the original; this comment is the only addition. Cite clauses by tag (e.g. `VER-§3 cl. 6`); CONVENTIONS.md '## Law index' maps tags to files. -->
## ⚖️ TOWER STACKING — the upgrade ghost · the height/health ladder · the wheel footprint (2026-09-02) — namespace **`STACK-§`**

**Trigger — Jonathan's directive, verbatim (2026-09-02):** *"lets go ahead and make it possible to spawn multiple towers stacked on top of each other … if you hover directly on another tower, the outline instead appears blue … instead of placing a new tower, it will instead double the height of the old one … the same model as the old tower, but instead it is twice as tall while keeping the same width and length and will gain 1.5 times the health … The maximum height it can reach is 5 times taller, and there is no maximum on the health … Lets also add a feature where when they have the green/red placement icon they can also use the mouse wheel to change the width and length of the tower. The maximum width/length it can be is 1.5 times the current default length and width."*

### STACK-§0 ⭐⭐ **WHAT ALREADY SHIPS — ⛔ READ THIS BEFORE ESTIMATING ANYTHING. ⭐ HALF OF WHAT HE DESCRIBED IS ⛔ ALREADY IN THE GAME.**

⚠️ **His sentence reads as if the green/red placement icon is new. ⛔ It is not, and mis-reading it would buy a feature twice.**

| His words | ⭐ Shipped today | Source |
|---|---|---|
| *"the tower placement icon that is either green or red depending on if it is a valid spawn location"* | ✅⭐ **ENTIRELY SHIPPED.** A ghost actor spawning the real `SM_<CardID>` follows a per-frame cursor trace, and `M_Ghost`'s vector param **`GhostColor`** is driven **every frame** to `ValidGhostColor` **(0,1,0)** or `InvalidGhostColor` **(1,0,0)**. | `SiegePlayerController.cpp:2160-2247`; `.h:1211-1213, 1541-1547` |
| the validity rule behind it | ✅ **Five gates in cost order** — spawn box / captured zone · navmesh projection · slope · obstacle clearance · building clearance — each recording a **first-failing reason** in `EPlacementInvalidReason` so the confirm click can name it | `SiegePlayerController.cpp:2169-2221` |
| *"the outline instead appears blue"* | ⭐ **A THIRD VALUE ON AN EXISTING MID PARAMETER.** ⛔ Not a new material, ⛔ not a new ghost, ⛔ not a new render path — **one more `FLinearColor` beside the two shipped ones.** | same MID, `:2241-2247` |

⇒ ⭐⭐ **THE BLUE STATE IS THE ***CHEAPEST*** PART OF THIS ASK, ⛔ NOT THE EXPENSIVE PART.** ⚖️ *The expensive parts are the two he described in half a sentence each: the per-instance scale model, and what a scaled tower does to the ladder.*

### STACK-§1 ⚖️⭐⭐⭐ **THE HEIGHT/HEALTH SERIES — ⛔ HIS OWN TWO STATEMENTS CONTRADICT EACH OTHER. ⭐ THE ENUMERATION WINS, AND THE ⛔ CAP IS WHAT PROVES IT.**

> ### ⛔ **RULED, ⛔ NOT GUESSED — AND THE LOOSE SENTENCE IS FLAGGED TO HIM ⛔ RATHER THAN SILENTLY DISCARDED.**

**THE TWO STATEMENTS, QUOTED SO THE CONFLICT IS AUDITABLE:**

1. ⭐ **THE ENUMERATION (four terms, written out by him):** *"twice as tall … 1.5 times the health … 3 times taller than the original height and 2.25 times the original health … 4 times taller, and 3.375 times the health and so on."*
2. ⛔ **THE SUMMARY:** *"It basically gains **twice the current height** and 1.5 times the current health every upgrade."*

| Series | ⚖️ **RULED** | Reading 1 (enumeration) | Reading 2 (summary) |
|---|---|---|---|
| **HEIGHT** | ⭐ **ADDITIVE — `+1 ×` the ORIGINAL height per upgrade** | ×2, ×3, ×4, ×5 ✅ **matches all four of his terms** | ×2, ×4, ×8, ×16 ⛔ **contradicts terms 2, 3 and 4** |
| **HEALTH** | ⭐ **MULTIPLICATIVE — `×1.5` of CURRENT per upgrade** | ×1.5, ×2.25, ×3.375 ✅ | ×1.5, ×2.25, ×3.375 ✅ ⭐ **the two readings AGREE here** |

- ⭐ **THE HEALTH SERIES IS CONFIRMED TO THE DIGIT AND THERE IS ⛔ NOTHING TO RULE:** `1.5² = 2.25` and `1.5³ = 3.375` — **he wrote both, exactly.** ⇒ **`HP(n) = HP₀ × 1.5ⁿ`, ⛔ UNCAPPED, his word.**
- ⭐⭐⭐ **AND THE HEIGHT RULING IS ⛔ NOT A COIN-FLIP BETWEEN TWO DEFENSIBLE READINGS — ⛔ HIS OWN CAP FALSIFIES THE SUMMARY, AND THIS IS THE DECISIVE ARGUMENT:**
  > **He pinned the ceiling at *"5 times taller"*. Under the DOUBLING reading the reachable heights are 2, 4, 8, 16 — ⛔⛔ `×5` IS NEVER REACHED. His stated maximum would be ⛔ UNHITTABLE, and the cap sentence he wrote would describe a state the game can ⛔ never enter.** Under the additive reading `×5` is reached **exactly**, at the 4th upgrade. ⇒ ⭐ **the enumeration is the only reading under which his ⛔ OWN cap means anything.**
- ✅ **AND HIS LAST SENTENCE CORROBORATES IT A THIRD TIME:** *"at some point if they keep upgrading it would only upgrade health by 1.5 times and not height"* ⇒ **the two series are ⛔ deliberately different in kind — one saturates, one does not.**
- ⇒ 📌 **PINNED, and this is the ⛔ whole arithmetic of the feature:**
  - **`HeightMultiplier(n) = min(1 + n, 5)`** — `n` = upgrades applied, `n ≥ 0`. ⛔ **Integer, ⛔ exact, ⛔ no float accumulation.**
  - **`HealthMultiplier(n) = 1.5ⁿ`** — ⛔ **UNCAPPED, and it keeps growing after the height saturates.**
- 🧑⛔ **THE FLAG — `J-0`, and it is stated to him ⛔ plainly, ⛔ not buried:** *"Your summary sentence says **'twice the current height'**, which would give ×2 → ×4 → ×8 and contradicts the list you wrote one sentence earlier — and it would make your own ×5 cap unreachable. **I built your list** (×2 → ×3 → ×4 → ×5, +1× per upgrade). **Your health half was right in both sentences** and I built that verbatim. ⭐ **One word switches height to doubling if that is what you meant.**"
- ⛔⛔ **`SC-§37` BINDS THE TESTS: measure the PROPERTY, ⛔ never restate the value.** A test asserting `HeightMultiplier(3) == 4` transcribed from this table proves ⛔ nothing. ✅ **Assert the *shape*:** that height increments are **CONSTANT** across `n` (additive) while health **RATIOS** are constant (multiplicative) · that the height cap **saturates and health does ⛔ not** · and that ⭐ **the cap is REACHED EXACTLY** — the property that falsifies the doubling reading, and therefore the one test that could ⛔ actually fail if the wrong series shipped.

### STACK-§2 ⛔⛔⛔ **THE STACK BREAKS THE LADDER WE SHIPPED YESTERDAY — ⭐ AND THE FIX IS A ***SCOPE*** DECISION, ⛔ NOT A GEOMETRY WAVE**

> ### ⚠️⚠️ **THIS IS THE BATCH'S REAL COST AND IT IS ⛔ NOWHERE IN HIS SENTENCE. ⛔ IT MAY ⛔ NOT BE DISCOVERED AT INTEGRATION.**

**THE COLLISION, MEASURED:**

- `AClimbableTower : ABuilding` (`ClimbableTower.h:219`) is climbed along a line pinned by **two sockets read off the mesh** — `LadderFoot (−460, 0, 0)` → `LadderTop (−160, 0, 1200)` (`TOWER-§8.3`, amended 2026-09-02), `Δ = (300, 0, 1200)`, length **1236.93169**, lean **76.0°**.
- ⛔ **A `Z` SCALE MOVES THE SOCKET WITH THE MESH.** At **×2** the deck stands at **2400** and the line becomes `Δ = (300, 0, 2400)` ⇒ **lean `atan(2400/300)` = 82.87°**; at **×5**, `Δ = (300, 0, 6000)` ⇒ **88.1°**. ⇒ ⭐ **the ladder does ⛔ not merely get longer — it gets STEEPER, and its RUNG PITCH scales with it.**
- ⛔⛔ **AND THAT FIRES `TOWER-§8.5a`'s ⛔ OWN VOIDING CONDITION.** The deck-breach exception — the ⛔ only thing that lets `§8.5` permit a swept move through the deck slab at all — is **licensed by `TOWER-§8.3`'s standoff and by ⛔ NOTHING ELSE**, and it voids when the geometry on the line changes. ⇒ ⛔⛔ **A SCALED WATCH TOWER DOES ⛔ NOT DEGRADE THE CLIMB. IT ⛔ VOIDS THE LICENCE, AND `§8.5`'s OUTRIGHT REFUSAL APPLIES ⇒ THE CLIMB STOPS WORKING AT ALL.** ⚖️ *We would ship a card that breaks itself when used as designed.*
- ⛔ **THE RUNG PLANE DIES TOO, AND IT IS THE F5 DEFECT CLASS VERBATIM.** `TOWER-§8.3` pins the rung mid-plane at **−22.0 uu measured along the in-plane normal** and states that a ⛔ **PURE TRANSLATION** preserves it (which is why the `δ = 10` amendment was free). ⛔ **A non-uniform SCALE is not a translation and preserves nothing** ⇒ **`A_SiegeBiped_Climb`'s hands grip air, and ⭐ every readback stays correct while they do** — *"a mesh change that moves the rungs and leaves the clip alone is a SILENT defect"*, which has ⛔ already cost this project one defect.
- ⛔ **AND `CONTACT-§13` COMPOUNDS IT:** a mesh **translation** is ⛔ not UV-neutral; a **non-uniform scale** is strictly worse — a visible `Z` stretch across the whole tower texture.
- ⇒ ⛔⛔ **THE HONEST PRICE OF LETTING THE WATCH TOWER STACK: `TOWER-§8.3`'s THREE-NUMBER CHECKLIST (climb line · `dist(spine, geometry) ≥ 98.0` standoff · rung plane) RE-MEASURED ⛔ PER HEIGHT MULTIPLE — ×2, ×3, ×4, ×5 = ⛔ FOUR VARIANTS — plus a `A_SiegeBiped_Climb` RE-EXPORT PER VARIANT, plus a `CONTACT-§13` UV repack.** ⭐ **That is an art wave of its own. ⛔ It is not a rider on a placement task.**

**⚖️⭐⭐ THE RULING — AND IT COSTS HIM ⛔ NOTHING HE ASKED FOR:**

| Option | ⚖️ Verdict |
|---|---|
| **(a) the ladder scales with the tower** | ⛔ **REFUSED for this batch.** Four full re-derivations + four clip re-exports + a UV repack, all art, all gated on measurements nobody has taken. **Priced above, ⛔ not dismissed** — 🧑 his to buy. |
| **(b) each stack adds a ladder segment** | ⛔ **REFUSED.** `TOWER-§8.3` already rejected multi-segment traversal in writing: *"three times the code, three times the interruption cases, and three chances to desync from the mesh. **One straight segment cannot desync from itself.**"* ⚖️ **Re-litigating a settled ruling is exactly what `TOWER-§8.0`'s scope fence forbids.** |
| **(c) a stacked Watch Tower is deliberately unclimbable** | ⛔⛔ **REFUSED, and this is the ⛔ WORST option, ⛔ not the safe one.** `TOWER-§8.1` M-1/M-2: without its link the deck is *"an unreachable island"* and *"the 30-gold card does nothing."* ⇒ **the upgrade would silently DESTROY the card's only purpose.** ⚖️ *That is a trap, ⛔ not a design.* |
| ✅⭐⭐ **(d) `WatchTower` is EXCLUDED from stacking and from the wheel — ⛔ BY A STRUCTURAL PREDICATE, ⛔ NOT A NAME CHECK** | ⚖️ **RULED.** ⭐ **The voiding condition ⛔ never fires because the geometry ⛔ never changes.** ⛔ Zero art. ⛔ Zero re-measurement. ⛔ Zero risk to a feature that shipped 48 hours ago. |

- ⭐⭐ **AND THE REASON IT IS ⛔ CHEAP RATHER THAN A COMPROMISE: he almost certainly ⛔ does not mean this tower.** He wrote *"the tower button"* — the shipped `<Name>Tower` family is **`ArrowTower` · `BombTower` · `BallistaTower` · `CrystalTower`**, all of which he has played with for weeks. **`WatchTower` shipped 2026-08-30 and is the ⛔ only one carrying a pinned socket contract.** ⇒ **the exclusion removes the one card he probably wasn't pointing at, and it is the one card that costs an art wave.** 🧑 **Row `J-9` — one word re-opens it, ⭐ and the price is already written above so the answer costs him no thinking.**
- 📌 **THE PREDICATE — ⛔ STRUCTURAL, so a future climbable building inherits the protection instead of needing to be remembered:**
  ```cpp
  /** May this building's footprint/height be scaled by the STACK-§ upgrade and the placement
   *  wheel? Default true. ⛔ AClimbableTower overrides FALSE: any non-uniform scale of
   *  SM_WatchTower moves the LadderFoot/LadderTop sockets and the rung plane, which fires
   *  TOWER-§8.5a's voiding condition ⇒ the deck-breach licence dies and the climb stops
   *  working entirely (STACK-§2). ⛔ NOT a style choice, ⛔ NOT a name check. */
  virtual bool CanScaleFootprint() const { return true; }
  ```
- ⛔⛔ **A `CardID == "WatchTower"` STRING COMPARISON ANYWHERE IN THE PLACEMENT PATH IS AN AUTOMATIC QA FAIL.** ⚖️ *The next climbable building must be protected by inheriting, ⛔ not by somebody remembering this paragraph.*
- ✅ **AND THE REFUSAL IS ⛔ VISIBLE, ⛔ NEVER SILENT:** hovering a `WatchTower` with a `WatchTower` in hand shows the **shipped RED**, ⛔ never blue, with a refusal message in the existing `RefuseCardPlay` vocabulary. ⚖️ **A feature that quietly does nothing on one card is a bug report waiting to happen; ⛔ one that says why is a design.**

### STACK-§3 ⭐ **WHAT STACKING DOES ⛔ NOT TOUCH — ⛔ MEASURED, BECAUSE TWO "OBVIOUS" INTERACTIONS ARE ⛔ BOTH FALSE**

- ✅⭐ **`HIGH-§` — ⛔ NO INTERACTION AT ALL, AND THIS SURPRISED ME.** A ×5 `ArrowTower` gains ⛔ **zero** damage from its height: `HIGH-§4` pins the selector as **`bRanged == true` AND `CardType == Unit`** ⇒ ⛔ **buildings are excluded from the elevation bonus entirely.** ⇒ ⛔ **no `HIGH-§` edit, ⛔ no balance spill, ⛔ no re-derivation.** ⚖️ *`HIGH-§3`'s "⛔ never a special case for towers" pays out again — the rule never learned towers exist, so it cannot be broken by one getting taller.*
- ✅ **RANGE — unchanged.** Range is measured from `GetActorLocation()`, and a `Z` scale on `VisualMesh` moves ⛔ no actor origin. ⚠️ **A projectile SPAWN point derived from a mesh socket WOULD rise** — ⛔ **the implementer states which it is, from the source, in the handoff. ⛔ Do not assume either way.**
- ✅ **COLLISION + NAV — free, and correct by construction.** `ABuilding` roots `VisualMesh` with `BlockAll` + `bCanEverAffectNavigation(true)` (`Building.h:43-51`) ⇒ **a scaled component carves a scaled hole with ⛔ zero new code.** ⭐ **A `Z`-only stack changes ⛔ no XY nav footprint at all** — which is precisely why the height half is cheap and the **wheel** half is not.
- ⛔ **UV STRETCH IS REAL AND IT IS ⛔ NOT A BUG — it goes on his sheet.** A ×5 `Z` scale visibly stretches the tower's texture (`CONTACT-§13`'s lesson, generalised). ⛔ **No task, ⛔ no re-author, ⛔ no tri-planar experiment.** 🧑 **Row `J-11`** — if he hates it, the answer is a purpose-built tall mesh, which is an art wave he can buy knowingly.

### STACK-§4 ⛔⛔ **THE WHEEL IS A ***THIRD*** NAMED CONSUMER — `MARK-§4` IS AMENDED ⛔ BY NAME, AND THE FEARED COLLISION IS ⛔ REFUTED AT SOURCE**

- ⛔ **THE CLAUSE THIS WOULD OTHERWISE BREACH, quoted:** `MARK-§4` — *"the wheel has exactly **TWO** consumers — the controller's group-pick poll, and `UWarMapWidget` while the map is open and the cursor is over it. ⛔ It stays inert everywhere else, and ⛔ **no third consumer may be added without amending this line.**"*
- ✅⭐⭐ **THE FEAR — *"the placement ghost and the group-pick may both be active-ish states"* — IS ⛔ FALSE, AND `PlayerTick` SAYS SO IN ITS OWN COMMENT** (`SiegePlayerController.cpp:642-647`): *"the third cursor mode on the SAME input surface as placement/targeting. **Mutually exclusive with the other two, so at most one of the three branches runs.**"* ⭐ **And it is enforced ⛔ structurally, ⛔ not by discipline: the group-pick branch `return`s (`:671`), targeting `return`s (`:686`), the war map `return`s (`:729`), and placement is reached ⛔ only past `if (!bInPlacementMode) { return; }` (`:732-735`).**
- ⇒ ⭐⭐ **THE THIRD CONSUMER IS THE ⛔ CHEAPEST ONE POSSIBLE: same mechanism (polled `WasInputKeyJustPressed`), same file, same function, a ⛔ SIBLING branch that can ⛔ never run in the same frame as consumer 1.** ⇒ **`ApplyGroupPickWheel`'s exact shape is COPIED** (`:2956-2989`), ⛔ not reinvented.
- ✅ **`MARK-§4`'s SENTENCE IS AMENDED TO READ: the wheel has exactly ⛔ THREE consumers — the controller's group-pick poll · `UWarMapWidget` while the map is open and focused · ⭐ the controller's PLACEMENT-mode footprint poll. ⛔ It stays inert everywhere else, and ⛔ no fourth consumer may be added without amending this line again.**
- ⚠️⚠️ **AND `MARK-§4`'s OWN CONFUSION HAZARD GETS ⛔ WORSE, SO IT IS RE-STATED:** the game already has **two** things called "circles" resized by the wheel. **This adds a third wheel meaning — a footprint.** ⇒ ⛔ **`HELP-§`'s controls screen MUST distinguish all three**, and `HELP-§2`'s standard applies: *a help screen that conflates them is worse than no help screen.*
  - ⛔⛔ ~~**AMENDED 2026-09-02 — ⭐ THAT INSTRUCTION IS CURRENTLY ⛔ UNSATISFIABLE, AND THE REASON IS ⛔ MEASURED: the THIRD wheel meaning (`MARK-§`'s numbered circles) has ⛔ NO HELP ROW AT ALL** — the 24 shipped rows cover the war map's ⛔ place-name markers and ⛔ nothing of the marks feature.~~ ⇒ ✅⭐⭐ **SATISFIED 2026-09-02 BY `TASK-823`, ⛔ AND SATISFIED IN THE SHAPE THIS CLAUSE ASKED FOR: ⛔ THREE wheel meanings ⇒ ⛔ THREE DISTINCT ROWS in ⛔ THREE DIFFERENT CATEGORIES** — `Cards.PlacementResize` (Cards) · `PickMode.Resize` (⛔ shipped, ⛔ untouched) · `Interface.MapMarks` (Interface) — ⛔ **each stating ⛔ WHEN its gesture applies before what it does, and ⛔ cross-linked by `RelatedActionIds` rather than by ⛔ repeated prose.** ⭐ **`TASK-823` test 16 (`…TheThreeWheelMeaningsAreThreeDistinctRows`) asserts it as a ⛔ PROPERTY — ⛔ including this section's ⛔ CEILING OF THREE: a ⛔ FOURTH wheel consumer documented without amending `MARK-§4` turns that test ⛔ RED.** 📌 *the ⛔ "24 shipped rows" figure above is a ⛔ dated measurement, ⛔ not a live count — the registry is ⛔ 27 as of 2026-09-02 and `GetActions().Num()` is the ⛔ only answer that cannot rot (`SC-§38`).* ⚠️ **And `CARDBAR-§8` adds the same obligation for a ⛔ second gesture: right-click now has ⛔ five consumers — ⛔ that half is ⛔ STILL OWED and is ⛔ NOT satisfied by the above.**
  - ⛔⛔⭐⭐ **AND THE OWED HALF HAS ⛔ STOPPED BEING MERELY OWED — 2026-09-02 IT BECAME A ⛔ LIVE `HELP-§2` DEFECT, ⛔ SHIPPED IN ⛔ TWO PLACES AT ONCE** (`qa/TASK-816.md` W-1/W-2).
    ⛔ **`Interface.WarMap` says the map is *"closed by ⛔ right-click or Escape, polled every frame"*. ⛔ `Interface.MapMarks` says *"a right-click that hits no circle does ⛔ NOTHING AT ALL."* ⛔ THEY CANNOT BOTH BE TRUE.**
    ⛔ **What ⛔ IS measured: `WarMapWidget.cpp`'s `NativeOnMouseButtonDown` returns `FReply::Handled()` on ⛔ EVERY RMB while the map is open** — ⛔ consistent with the widget ⛔ consuming the gesture, which would make the ⛔ `PlayerTick` poll ⛔ unreachable and ⛔ `Interface.WarMap`'s sentence the false one. ⚠️ **`TASK-818` reached the ⛔ same conclusion independently and called it *"ONE TRUE COLLISION"*.**
    ⛔⛔ **BUT IT IS ⛔ NOT SETTLED BY READING, AND ⛔ SAYING SO IS THE POINT: ⛔ IT TURNS ON ⛔ SLATE EVENT ROUTING UNDER THE ⛔ LIVE INPUT MODE ⇒ ⛔ AN ⛔ OBSERVATION, ⛔ NOT A DEDUCTION.** ⇒ ✅ **ONE PIE row settles it (`TASK-814(3c)`: ⛔ open the map, ⛔ right-click empty ground, ⛔ report whether it closes) and ⛔ `TASK-870` boards the ⛔ one-sentence repair on ⛔ whichever page the observation names.**
    ⚖️ *⛔ Two careful readers and a gate ⛔ all reasoned toward the same answer and ⛔ none of them could ⛔ confirm it — ⛔ which is `SC-§40` cl. 5 in its ⛔ strongest form: ⛔ here even ⛔ opening the file is ⛔ not enough, because the fact does ⛔ not live in a file.*
    ✅⛔⛔⭐⭐ **RESOLVED 2026-09-03 ⛔ BY OBSERVATION — 🧑 JONATHAN'S, ⛔ IN HIS OWN WORDS, ⛔ ONE CLICK:** > *"opening the war map and right clicking empty ground does not cause it to close, the map seems to function exactly as it should"*
    ⇒ ✅ **`Interface.MapMarks`'s sentence is ⛔ TRUE — ⛔ `TASK-823` WROTE IT CORRECTLY.** ⇒ ⛔⛔ **`Interface.WarMap`'s sentence is the ⛔ FALSE ONE** (*"closed by right-click or Escape, polled every frame"*). ⇒ ⭐ **`TASK-870`'s target is now ⛔ DETERMINED, ⛔ NOT CONDITIONAL: it repairs the ⛔ `WarMap` row and ⛔ leaves `MapMarks` alone. ⛔ Editing the true one is now an ⛔ AUTOMATIC FAIL, ⛔ not a judgement call.**
    ⛔⛔ **AND THE PART WORTH MORE THAN THE ANSWER, ⛔ NOW LAW AS ⭐ `SC-§42`: ⛔ THE THREE READERS WHO CONVERGED ON THE RIGHT ANSWER WERE ⛔ REASONING FROM `FReply::Handled()`, AND ⛔ A HANDLED EVENT IS ⛔ NOT AN ACTIONED EVENT.** ⛔ **They were ⛔ right and their ⛔ instrument could ⛔ not have told them so.** ⇒ ⛔ **the PIE row was ⛔ not belt-and-braces; it was the ⛔ ONLY instrument in the building.**
    ⛔ **AND THE SPELL-TARGETING CANCEL HAS ⛔ NO ROW AT ALL** — ⛔ measured against `CARDBAR-§8`'s registry (consumers 1–4 + 4b; row 5 struck when Jonathan cut right-click discard). ⛔ **The screen has the group-pick cancel, the placement cancel, the map close and the mark delete; ⛔ nothing for spell targeting, and ⛔ none of the four states its ⛔ exclusivity against the others.** ⇒ ⛔ **`TASK-870` items (2)+(3).**
- ✅ **TUNABLES: new, separately named, `EditDefaultsOnly`, ⛔ NEVER reusing `GroupRadiusWheelStep`/`Min`/`Max`** — those are world-space radii and meaningless as a scale factor. **Each ships with its consequence written beside it (`HIGH-§1`).** Range **`[1.0, 1.5]`** (`STACK-§5` `J-3`), step small enough that the max is reachable in a few notches.

### STACK-§5 ⚖️ **THE RULINGS — ⛔ ALL PROCEEDING DEFAULTS. ⛔ NOT ONE OF THEM BLOCKS. ⭐ EACH IS A ONE-WORD OVERRULE.**

| 🧑 | Question — ⛔ his sentence does not answer it | ⚖️ Proceeding default | Why |
|---|---|---|---|
| **J-0** | which height series | ⭐ **the ENUMERATION** | `STACK-§1` — ⭐ **his own ×5 cap is unreachable under the other reading** |
| **J-1** | the `"1"` on the card | ⭐ **it is the DISCARD button; KEEP it, relabel `Discard`, key index goes where PLAY was** | `CARDBAR-§0` — removing it deletes the ⛔ only discard route in the game |
| **J-2** | what an upgrade costs | ✅ **the card's own `Cost` from `DT_Cards`** (⛔ never a literal — `ArrowTower`'s row cost, whatever it is) | ⭐ **derived ⇒ retuning the card retunes the upgrade.** Insufficient gold ⇒ shipped `RefuseCardPlay` refusal, ⛔ no gold moves |
| **J-3** | a MINIMUM wheel width/length | ✅ **×1.0 — ⛔ no shrinking** | He gave only a max. **Shrinking below the authored footprint is a ⛔ new balance lever he did not ask for** and would let players hide buildings |
| **J-4** | does an upgrade keep the wheel-set W/L | ✅ **YES — ⛔ his own words: *"keeping the same width and length."*** The upgrade multiplies `Z` ⛔ only; X/Y are inherited verbatim | ⛔ Nothing to rule — ⭐ he answered it |
| **J-5** | does BLUE require enough gold | ✅ **NO — blue means *"this click will upgrade"*. Can't afford ⇒ ⛔ RED + the shipped "Not enough gold" line** | ⭐ **`UpdatePlacementGhost` already holds this exact standard** (`:2946-2947`): *"the confirm click refuses on the same flag, so **what the player sees is what the click does**"* |
| **J-6** | what happens at the height cap | ✅ **health-only — ⛔ his own sentence answers it.** The ghost stays **BLUE** (the click still does something) + a one-line HUD note so the player can ⛔ see the cap | ⛔ A silent behaviour change at ×5 is indistinguishable from a bug |
| **J-7** | do ENEMY towers show blue | ✅ **NO — own-team only**, reusing the shipped team predicate. An enemy building hovers **RED** | ⛔ It is not a valid placement point today either — ⭐ **no new refusal is invented** |
| **J-8** | which cards can stack | ✅ **every `Building` card with `CanScaleFootprint() == true`** ⇒ all of them ⛔ except `WatchTower`. ⛔ Not units, ⛔ not spells, ⛔ not castles | `STACK-§2` |
| **J-9** | may the `WatchTower` stack | ~~⛔ **NO — excluded.**~~ ⛔⛔⭐⭐ **REOPENED AND ⛔ OVERRULED BY ⛔ OBSERVATION 2026-09-03 — 🧑 HE ⛔ TRIED IT AND ⛔ FILED IT AS A BUG.** ⇒ ⛔ **the answer is now `STACK-§8`, ⛔ NOT this row. ⛔ Do ⛔ not read this cell as live law.** | `STACK-§2` → ⭐ **`STACK-§8`** |
| **J-10** | health on upgrade — does it heal? | ✅ **`MaxHP ×= 1.5`, `CurrentHP += (NewMax − OldMax)`.** The upgrade grants the ⛔ NEW hit points; it does ⛔ NOT repair existing damage | ⚖️ **A full heal would make the upgrade a repair tool — ⛔ that is the Masons card's job**, and it would make upgrading strictly better than defending |
| **J-11** | the ×5 UV stretch | ✅ **shipped as-is, ⛔ named, ⛔ not fixed** | `STACK-§3` — the fix is a purpose-built tall mesh, ⛔ an art wave, ⛔ not a rider |
| ⭐⭐ **J-12** ***(new 2026-09-03)*** | ⛔ **verify the climb FIRST, or ship the stacking and check the climb AFTER?** | ✅⛔ **VERIFY FIRST** — ⛔ `TASK-941` measures before `TASK-942` writes a line | ⛔ **A stackable ⛔ UNCLIMBABLE tower is ⛔ worse than a delayed feature, and it fails ⛔ SILENTLY with every readback correct** (`STACK-§2`, `TOWER-§8.5a`). ⛔ **One word ships it the other way round** |
| ⭐ **J-13** ***(new 2026-09-03)*** | ⛔ **may the `WatchTower`'s stack ceiling be ⛔ LOWER than every other building's ×5?** | ✅ **YES — ⛔ the ceiling is ⛔ per-class and the ⛔ MEASUREMENT sets it, ⛔ not this table** | ⭐ **`STACK-§8` cl. 4 — a ⛔ measured ×2 he can ⛔ use beats a ⛔ theorised ×5 that ⛔ voids the climb.** ⛔ **`ArrowTower` & co. are ⛔ UNAFFECTED at ×5** |

### STACK-§6 ⛔⛔ **`TASK-735` IS NOW A ***HARD PREREQUISITE*** OF THE WHEEL — AND ITS "SCALED vs LOCAL" FOOTNOTE JUST BECAME ⛔ LOAD-BEARING**

- 📌 **`TASK-735` (footprint-aware placement, `TOWER-§7`) is `status: backlog` and its ⛔ only blocker `TASK-731` is `done`** ⇒ ⭐ **it is DISPATCHABLE NOW.** ⛔ **It is ⛔ NOT re-boarded under a new ID** — it is amended in place (board law: renumbering is the manager's and duplication is a defect).
- ⛔⛔ **THE NEW OBLIGATION, and it is one word in `TASK-735` §1: the footprint radius MUST come from the ghost's ⭐ SCALED bounds, ⛔ never its local bounds.** `TASK-735` §1 already says *"State in the handoff which bounds call you used and why (scaled vs local)"* — ⭐ **that sentence was written as a disclosure and is now a REQUIREMENT**, because a wheel-scaled ghost differs from its mesh by up to **×1.5** and a local-bounds read would silently validate a **1.5× building at 1.0×**.
- ⇒ ⛔⛔ **SHIPPING THE WHEEL BEFORE `TASK-735` WOULD SHIP A BUILDING WHOSE CLEARANCE IS COMPUTED AT THE WRONG SIZE — ⛔ exactly the class of defect `TOWER-§7` exists to close.** ⇒ **the wheel task is `blocked-by: TASK-735`, ⛔ not "sequenced after it".**
- ⭐ **AND `TOWER-§7`'s "the size comes from the MESH, ⛔ never a literal" rule pays out a THIRD time:** because `TASK-735` was written to read bounds rather than the literal `2700`, ⭐ **a per-instance scale it was never designed for costs it ⛔ one word.** ⚖️ *That is now three separate features rescued by one sentence written on 2026-08-30.*

### STACK-§7 📌 NAMING + FILE MAP (the cross-task contract) + M8

| thing | law |
|---|---|
| the two series | ⭐ **`static float ABuilding::StackHeightMultiplier(int32 UpgradeCount);`** and **`static float ABuilding::StackHealthMultiplier(int32 UpgradeCount);`** — ⛔ **plain C++ statics, ⛔ NOT `UFUNCTION`s, one parameter, ⛔ none defaulted (`SC-§33`)**, ⛔ no world, ⛔ no actor ⇒ **headlessly testable** (the `HeightAdvantageMultiplier` / `FSiegeMapMark::MakeSymbol` precedent) |
| the cap | **`MaxStackHeightMultiplier = 5`** — `EditDefaultsOnly`, ⛔ integer, consequence in its comment (`HIGH-§1`) |
| the health step | **`StackHealthStep = 1.5f`** — `EditDefaultsOnly`, ⛔ **UNCAPPED by design; the comment says so** |
| per-instance state | **`int32 StackUpgradeCount`** on `ABuilding` — ⛔ the ⛔ ONE source of truth for both multipliers. ⛔ **No second copy of height or health scale is stored anywhere** |
| the exclusion | **`virtual bool ABuilding::CanScaleFootprint() const`** → `false` on `AClimbableTower`. ⛔ **A `CardID` string compare is an automatic FAIL** ⇒ ⛔⛔⭐⭐ **AMENDED 2026-09-03 BY `STACK-§8`: this virtual is ⛔ NO LONGER THE STACK GATE. ⛔ It keeps the ⛔ WHEEL (X/Y) ⛔ ONLY. ⛔ The stack (Z) gate is the ⛔ NEW `CanStackHeight()`. ⛔ Consulting `CanScaleFootprint()` from a ⛔ STACK site is now an ⛔ AUTOMATIC FAIL** |
| ⭐⭐ the ⛔ STACK gate ***(new 2026-09-03, `STACK-§8`)*** | **`virtual bool ABuilding::CanStackHeight() const`** — ⛔ **a ⛔ SIBLING of `CanScaleFootprint()`, ⛔ never an alias, ⛔ never a wrapper, ⛔ never `{ return CanScaleFootprint(); }`.** ⛔ **`AClimbableTower`'s override value is set by ⛔ `TASK-941`'s MEASUREMENT, ⛔ not by this table** |
| ⭐ the ⛔ PER-CLASS CEILING ***(new 2026-09-03)*** | **`MaxStackHeightMultiplier`** stays the ⛔ ONE `EditDefaultsOnly` member and is ⛔ set per class in the ⛔ CONSTRUCTOR (⛔ `AClimbableTower`'s value = the ⛔ measured licence). ⇒ ⛔ **the pinned static becomes `static float ABuilding::StackHeightMultiplier(int32 UpgradeCount, int32 MaxMultiplier)` — ⛔ still pure, ⛔ still headless, ⛔ still no world.** ⛔ **A ⛔ second copy of the ceiling anywhere is a FAIL** |
| ghost colour | **`UpgradeGhostColor`** beside the shipped `ValidGhostColor`/`InvalidGhostColor`, driven into the ⛔ SAME `M_Ghost` `"GhostColor"` param. ⛔ **No new material, ⛔ no new MID, ⛔ no second ghost actor** |
| the refusal | ⭐ **ONE new `EPlacementInvalidReason` value** for the height-cap / non-scalable case, ⛔ matching the shipped `{ None, Point, Slope, Obstacle, Clearance }` family's shape exactly. ⛔ **Do ⛔ not reuse `Clearance`** |
| the wheel | **`ASiegePlayerController::ApplyPlacementFootprintWheel()`** — ⛔ **a sibling of `ApplyGroupPickWheel`, same shape, called ⛔ ONLY from the placement branch of `PlayerTick`.** Tunables `PlacementFootprintWheelStep` / `Min = 1.0f` / `Max = 1.5f`, all `EditDefaultsOnly` |
| ⛔ untouched | `TOWER-§8.*` (⛔ **no socket, ⛔ no climb line, ⛔ no rung plane, ⛔ no standoff — `STACK-§2` is what guarantees this**) · `A_SiegeBiped_Climb` · `SM_WatchTower` · `HIGH-§` · `cards.csv` · `M_Ghost` · every WBP |

📌 **M8 DECLARATION — ⛔ do ⛔ NOT copy another batch's boilerplate; this one is ⛔ not free.** The **ghost, the wheel and the blue state are client-local pre-gate** and add ⛔ nothing replicated. ⚠️⚠️ **BUT `StackUpgradeCount` IS AUTHORITATIVE GAME STATE** — it drives `MaxHP` — ⇒ **it is set on the SERVER at confirm, exactly where `MaxHP` is already set (`Building.cpp:251-252`), and the existing `OnHPChanged` push carries the result to the bar.** ⛔ **No new RPC, ⛔ no new relevancy tier — but ⛔ the client may ⛔ never author it.**

⛔⛔⭐ **M8 LEDGER, AMENDED 2026-09-02 (`qa/TASK-816.md` W-3) — ⛔ THE PARAGRAPH ABOVE IS ⛔ TRUE ABOUT THE ⛔ HP AND ⛔ SILENT ABOUT THE ⛔ HEIGHT, ⛔ WHICH IS THE ⛔ VISIBLE HALF.**
- ⛔ **MEASURED: `StackUpgradeCount` is `UPROPERTY(VisibleInstanceOnly, ⛔ Transient)` with ⛔ NO `Replicated` specifier, and `ApplyStackUpgrade` writes `VisualMesh->SetRelativeScale3D(...)` on a component that is ⛔ NOT replicated.**
- ⇒ ⛔⛔ **THE STACKED HEIGHT IS ⛔ SERVER-LOCAL UNTIL M8. ⛔ In a listen-server build a ⛔ REMOTE CLIENT WOULD SEE AN ⛔ UN-STACKED TOWER ⛔ CARRYING STACKED HP.**
- ✅ **⛔ NO CODE CHANGE IS OWED TODAY — ⛔ M8 is deferred project-wide and the ⛔ authority guard is ⛔ correct** (`Building.cpp`'s `!HasAuthority()` refusal, ⛔ and it is deliberately ⛔ NOT a `UFUNCTION`, so ⛔ no Blueprint entry point exists — ⛔ belt ⛔ and braces).
- ⛔ **WHAT IS OWED IS ⛔ THIS LINE, so the ⛔ next reader does ⛔ not have to re-derive it** (`SC-§38` cl. 4: ⛔ a law that describes behaviour ⛔ rots silently). ⭐ **When M8 opens, ⛔ the height is a ⛔ named work item, ⛔ not a discovery.**

### STACK-§8 ⛔⛔⛔⭐⭐ **HEIGHT (Z) AND FOOTPRINT (X/Y) ARE ⛔ TWO DECISIONS, AND ⛔ ONE VIRTUAL WAS ANSWERING ⛔ BOTH — ⛔ THE SPLIT** (added 2026-09-03, from 🧑 Jonathan's playtest + `handoffs/STACK-BUGS-diagnosis.md` + `footage/VID-005-…md`; ⛔ **this section ⛔ SUPERSEDES `STACK-§5` `J-9` and ⛔ amends `STACK-§2` option (d)**)

> ### ⛔⛔⛔ **THE FINDING IS ⛔ NOT *"THE GATE IS WRONG."* ⛔ IT IS THAT THE CODE ⛔ PREDICTED HIS COMPLAINT ⛔ IN WRITING, ⛔ WORD FOR WORD, ⛔ AND SHIPPED ANYWAY.**
> `SiegePlayerController.cpp:2829-2842` (⛔ an ⛔ ANNOTATION — ⛔ locate by symbol, `SC-§38`) says: *"The WatchTower's refusal already has a voice — the ⛔ RED ghost plus ⛔ 'That building cannot be stacked' on the click."*
> ⇒ ⛔⛔ **THAT IS ⛔ EXACTLY THE PIXELS HE FILMED AND ⛔ EXACTLY THE SENTENCE HE QUOTED.** ⇒ ⚖️ ***⛔ THIS IS A ⛔ SPEC CONFLICT, ⛔ NOT A DEFECT: his ⛔ stacking requirement and the tower's ⛔ climb protection were ⛔ NEVER RECONCILED, and ⛔ the reconciliation was ⛔ deferred to a `J-9` row he was ⛔ never made to answer.*** ⭐ **`STACK-§2` priced the reversal ⛔ correctly and in full; ⛔ what nobody did was ⛔ put the price in front of him ⛔ before he hit the wall.**

**1. ⛔⛔ WHAT WAS ⛔ MEASURED — ⛔ CODE AND ⛔ PIXELS AGREEING ⛔ INDEPENDENTLY, WHICH IS WHY THIS SECTION IS ⛔ NOT A HYPOTHESIS.**

| # | measurement | instrument |
|---|---|---|
| a | ⛔ The stack-eligibility test consults **`CanScaleFootprint()` — the ⛔ IDENTICAL virtual the wheel uses** — at ⛔ **THREE** sites, plus a ⛔ FOURTH for the wheel | ⛔ code census, `handoffs/STACK-BUGS-diagnosis.md` §1.5 |
| b | `ABuilding` returns `true`; ⛔ **`AClimbableTower` is the ⛔ ONLY override in the codebase** (`false`); ⛔ `BP_Building_WatchTower` is the ⛔ ONLY building BP deriving it | ⛔ all ⛔ 8 building BPs censused, ⛔ negative control clean |
| c | ⇒ ⭐⭐ **the refusal is reachable in normal play by ⛔ EXACTLY ONE COMBINATION: a ⛔ WatchTower card on ⛔ your own WatchTower** — ⛔ gates (3) and (4) forbid every other hover | ⛔ exhaustive door enumeration |
| d | ⭐ The ghost is **RED** across the ⛔ ENTIRE SPAN of ⛔ BOTH hover episodes — ⛔ blue flat at its ⛔ scene baseline **1.16–1.66 %** while red swings to **58.2 %** | ⛔ whole-video tint census, ⛔ validated on ⛔ 3 positive controls |
| e | ⇒ ⛔⛔ **the defect is ⛔ UPSTREAM, at ⛔ ELIGIBILITY, ⛔ NOT at commit** — ⛔ it was red at ⛔ whichever instant the click landed, so ⛔ no click-frame pinpointing is needed | ⛔ (d) |
| f | ✅ **The two shipped series match his spec ⛔ EXACTLY** — height `min(1+n, 5)`, health `×1.5` compounding uncapped, wheel capped `1.5` | ⛔ `STACK-§1` re-verified at source ⇒ ⛔ **NOTHING TO BOARD** |
| g | ✅ **A WatchTower placed ⛔ successfully on open ground** at ≈13.25 s (Gold 61→32) | ⇒ ⭐ **the ⛔ card and ⛔ placement systems ⛔ WORK; ⛔ only stack-onto-a-tower refuses** |

**2. ⛔⛔⛔ THE CHEAP FIX IS ⛔ BANNED, AND ⛔ NOT FOR TIDINESS — ⛔ IT ⛔ SILENTLY BRICKS THE CLIMB WHILE ⛔ EVERY CHECK REPORTS ⛔ GREEN.**

⛔ **`AClimbableTower::CanScaleFootprint() → true` is an ⛔ AUTOMATIC QA FAIL and an ⛔ AUTOMATIC BOARD REJECTION.** `STACK-§2` measured the price and `:2491` records it in the source: a scaled `SM_WatchTower` moves `LadderFoot`/`LadderTop` and the rung plane, fires **`TOWER-§8.5a`**'s ⛔ voiding condition, and *"the climb stops working ⛔ ENTIRELY, with ⛔ every readback still reporting ⛔ correct."*
⇒ ⚖️ ***⛔ THE ONE-CHARACTER FIX AND THE ⛔ CORRECT FIX ARE ⛔ INDISTINGUISHABLE FROM ⛔ EVERY INSTRUMENT WE OWN ⛔ EXCEPT A HUMAN CLIMBING THE TOWER.*** ⛔ **That is why `J-12` is ⛔ VERIFY-FIRST.**

**3. ⛔⛔ THE SPLIT — ⛔ THE ⛔ PLUMBING HALF, AND IT IS ⛔ NOT THE FEATURE.**

- ⛔ **`CanScaleFootprint()` keeps the ⛔ WHEEL (X/Y) and ⛔ NOTHING ELSE.** ⛔ It stays `false` on `AClimbableTower` — ⛔ **that exclusion is ⛔ NOT reopened, ⛔ not by this section and ⛔ not by his playtest** (⛔ he reported ⛔ stacking, ⛔ never resizing).
- ⭐ **`CanStackHeight()` is ⛔ BORN, ⛔ default `true` on `ABuilding`**, and the ⛔ THREE stack gates repoint at it (`ResolvePlacementUpgradeState` · `ConfirmStackUpgrade` · `ABuilding::ApplyStackUpgrade`). ⛔ **Locate all three ⛔ BY SYMBOL.**
- ⛔⛔ **A ⛔ WRAPPER IS ⛔ NOT A SPLIT.** `bool AClimbableTower::CanStackHeight() const { return CanScaleFootprint(); }` ⛔ reproduces the defect ⛔ behind a new name ⇒ ⛔ **AUTOMATIC FAIL.** ⛔ **Two virtuals, ⛔ two independent answers, ⛔ or it is one virtual with two spellings.**
- ⛔⛔⭐ **AND THE HONEST SENTENCE, ⛔ WRITTEN SO NO ROW OVERSELLS ITSELF: ⛔ THE SPLIT ⛔ ALONE DELIVERS HIM ⛔ NOTHING.** ⛔ **It makes the two questions ⛔ SEPARATELY ANSWERABLE. ⛔ What he can ⛔ actually do with it is decided ⛔ entirely by cl. 4's measurement.** ⚖️ *⛔ A refactor that ships with the ⛔ same behaviour is ⛔ correct here and ⛔ must be ⛔ described as such.*

**4. ⛔⛔⭐⭐ THE MEASUREMENT IS THE ⛔ DELIVERABLE, AND ⛔ ITS ANSWER IS A ⛔ NUMBER, ⛔ NOT A YES/NO — ⛔ THE ⛔ CEILING IS ⛔ PER CLASS.**

⛔ **`TOWER-§8.3`'s THREE-NUMBER CHECKLIST is re-derived at Z-scale `n ∈ {1, 2, 3, 4, 5}`:** ⛔ the ⛔ CLIMB LINE · ⛔ the ⛔ STANDOFF (`dist(spine, geometry) ≥ 98.0`) · ⛔ the ⛔ RUNG PLANE (−22.0 uu). ⛔ **`n = 1` is the ⛔ POSITIVE CONTROL and it ⛔ MUST reproduce the published `103.330` / `1236.93169` / `76.0°`, ⛔ or the derivation is ⛔ wrong and ⛔ nothing downstream of it may be believed** (`SC-§39`).

- ⛔⛔ **AND THE ⛔ TWO HAZARDS MUST BE ⛔ SEPARATED, ⛔ BECAUSE THEY HAVE ⛔ DIFFERENT SEVERITIES AND `STACK-§2` ⛔ COLLAPSED THEM INTO ONE SENTENCE:**
  - ⛔⛔ **(i) THE ⛔ STANDOFF — ⛔ THIS ONE ⛔ VOIDS `TOWER-§8.5a` and ⛔ KILLS THE FEATURE.** ⛔ **BLOCKING.**
  - ⚠️ **(ii) THE ⛔ RUNG ⛔ PITCH — ⛔ rung ⛔ SPACING stretches with `n` while `A_SiegeBiped_Climb`'s ⛔ hand cycle does ⛔ not ⇒ ⛔ hands grip ⛔ BETWEEN rungs.** ⛔ **COSMETIC, ⛔ NOT a void** — ⛔ **provided the rung ⛔ PLANE (the −22.0 uu ⛔ DEPTH) survives, the hands still meet the ⛔ ladder.** ⇒ ⛔ **the measurement ⛔ REPORTS PITCH AND DEPTH ⛔ SEPARATELY. ⛔ Conflating them is what would ⛔ refuse a shippable feature.**
- ⛔⛔ **A ⛔ HYPOTHESIS THE MANAGER IS ⛔ FLAGGING AS ⛔ UNMEASURED, ⛔ EXPLICITLY, SO IT IS ⛔ TESTED AND ⛔ NEVER CITED: ⛔ `STACK-§2`'s *"preserves nothing"* is an ⛔ ANALYTIC ASSERTION, ⛔ not a computation.** ⛔ **It is ⛔ possible the standoff ⛔ IMPROVES with `n`** — the plinth top and the line's Z ⛔ scale together, while the ⛔ capsule's 54 uu spine ⛔ does not — ⛔ **and it is equally possible it ⛔ collapses.** ⇒ ⛔⛔ **⛔ MEASURE IT. ⛔ DO ⛔ NOT QUOTE THIS BULLET AS A FINDING** (`SC-§49` — ⛔ the manager does ⛔ not publish a census he did ⛔ not run).
- 📌 **THE ⛔ FOUR ADMISSIBLE OUTCOMES, ⛔ ENUMERATED SO ⛔ NONE IS IMPROVISED:**

| outcome | what ships |
|---|---|
| ⛔ **all of ×2..×5 hold** | `CanStackHeight() → true`, ceiling stays **5** ⇒ ⭐ **he gets exactly what he asked for** |
| ⭐ **some hold** (e.g. ×2 only) | `CanStackHeight() → true`, `AClimbableTower`'s `MaxStackHeightMultiplier` = ⛔ **the measured licence** ⇒ ⭐ **a ⛔ real, ⛔ usable, ⛔ honest partial — and `J-6`'s cap behaviour ⛔ already handles the ceiling gracefully** |
| ⛔ **none hold** | `CanStackHeight()` stays `false` ⇒ ⛔ **the split still ships** (the plumbing is right, the message becomes ⛔ truthful) and the ⛔ REAL cost of `STACK-§2` option (a) goes to 🧑 him ⛔ priced, ⛔ not hidden |
| ⛔⛔ **a number cannot be derived from pinned data** | ⛔ **SAY SO AND ⛔ NAME WHO CAN MEASURE IT** (⛔ art-director in Blender / ⛔ build-master via MCP). ⛔ **A ⛔ DECLARED *"the mesh-side rung positions are not pinned and I cannot compute this"* is a ⛔ COMPLETE AND ⛔ PASSING deliverable. ⛔ What fails is ⛔ SILENCE or an ⛔ ASSUMED number** |

- ⛔⛔ **THE ⛔ REFUSED SHAPE, ⛔ NAMED SO IT IS ⛔ NOT REINVENTED: ⛔ *"scale the shaft, ⛔ leave the ladder"* is ⛔ `STACK-§2` option ⛔ (c) — a ⛔ deliberately unclimbable stacked tower — ⛔ in disguise. ⛔ ALREADY REFUSED, ⛔ and it is the ⛔ WORST option, ⛔ not the safe one.**

**5. ⛔⛔⭐ A ⛔ CACHED CLIMB LINE IS A ⛔ SILENT FAILURE MODE AND ⛔ NOBODY HAS LOOKED AT IT.**
`TOWER-§8.4(A)`: *"the code reads them at ⛔ `BeginPlay` and ⛔ NEVER hardcodes the geometry."* ⇒ ⛔⛔ **IF THE LINE IS ⛔ CACHED AT `BeginPlay` AND THE TOWER IS ⛔ SCALED AT RUNTIME, THE ⛔ CACHE IS ⛔ STALE: the unit climbs to the ⛔ OLD deck height and ⛔ stops in mid-air, ⛔ with every readback correct.** ⛔ **This is ⛔ orthogonal to the standoff and ⛔ would survive a ⛔ perfect cl. 4 result.** ⇒ ⛔ **`TASK-941` ⛔ MUST rule on it ⛔ in writing.**

**6. ⛔ SCOPE FENCE — ⛔ WHAT THIS SECTION DOES ⛔ NOT REOPEN:** ⛔ the ⛔ two series (`STACK-§1`, ⛔ measured correct) · ⛔ the ⛔ wheel and its `1.5` cap · ⛔ `STACK-§2` options (a)/(b)/(c) (⛔ ruled, ⛔ `TOWER-§8.0`'s fence binds) · ⛔ `HIGH-§` (⛔ `STACK-§3`: ⛔ no interaction) · ⛔ the ⛔ UV stretch (`J-11`) · ⛔ any mesh, clip, socket or material.

### STACK-§9 ⛔⛔⭐⭐ **THE BLUE STATE ⛔ SHIPS, ⛔ WORKS, AND IS ⛔ ALREADY WHAT HE DESCRIBED — ⛔ THREE HYPOTHESES ⛔ DIED HERE AND ⛔ THE RECORD IS THE POINT** (added 2026-09-03)

> ### ⛔⛔⛔ **⛔ DO ⛔ NOT BOARD AN *"ADD THE BLUE GHOST STATE"* TASK. ⛔ IT EXISTS. ⛔ IT IS CORRECT. ⛔ IT IS ⛔ ONE `if` AWAY FROM BEING ⛔ VISIBLE.**

- ⛔ **FALSIFIED — *"the blue state was never built."*** ⛔ It is a ⛔ THREE-STATE ⛔ WHOLE-GHOST tint from ⛔ one `GhostColor` param on ⛔ every material slot: `UpgradeGhostColor = (0, 0.4, 1)` ⛔ BLUE / `ValidGhostColor` GREEN / `InvalidGhostColor` RED. ⭐ **⛔ Exactly the shape he asked for, ⛔ and ⛔ not an outline on the target.**
- ⛔ **FALSIFIED — *"hover tints from one predicate, the click refuses from another."*** ⛔ **ONE resolver, ⛔ ONE call site, ⛔ ONE member (`PlacementUpgradeState`); ⛔ both consumers read it and ⛔ nothing else. ⛔ There is ⛔ no second expression to drift.**
- ⛔ **FALSIFIED — *"the eligibility check never runs at hover time."*** ⛔ `PlayerTick` runs ⛔ ghost-then-LMB-poll in the ⛔ SAME tick.
- ⭐⭐ **AND THE ⛔ DECISIVE EVIDENCE IS ⛔ HIS OWN ERROR MESSAGE, WHICH IS WHY THIS IS ⛔ SETTLED AND ⛔ NOT MERELY ARGUED:** `EPlacementInvalidReason::Upgrade` is written at ⛔ **EXACTLY ONE SITE — ⛔ inside `UpdatePlacementGhost`.** ⇒ ⛔ **had eligibility ⛔ not run, he would have seen *"Too close to another building."*** ⇒ ⛔⛔ **the check ⛔ RAN, ⛔ RECOGNISED the stack case, and ⛔ DELIBERATELY REFUSED IT.**
- ⇒ ⛔⛔⭐⭐ ***"NO BLUE" AND "CANNOT BE STACKED" ARE ⛔ ONE DEFECT.*** ⛔ **Fix the ⛔ predicate and the ghost turns ⛔ BLUE ⛔ AND the click ⛔ works, with ⛔ ZERO tint edits.** ⚖️ *⛔ Two symptoms, ⛔ one decision — ⛔ and boarding them as two tasks would have bought the ⛔ same fix twice and ⛔ risked a ⛔ second source of truth for the tint.*

**⛔ THE ⛔ TWO USABILITY FINDINGS ON THIS PATH, ⛔ RULED ⛔ SEPARATELY:**

1. ⛔ **DEFERRED, ⛔ NAMED, ⛔ NOT BOARDED — the `NotStackable` red is ⛔ pixel-identical to ordinary invalid-location red.** ⭐ **Reason it is ⛔ not boarded: ⛔ `STACK-§8`'s split makes that state ⛔ UNREACHABLE IN NORMAL PLAY** (⛔ at the ceiling the ghost stays ⛔ BLUE per `J-6`; ⛔ the only remaining producer is a ⛔ future climbable building that ⛔ refuses outright). ⇒ ⛔ **boarding it ⛔ today buys a distinct tint for a state ⛔ nobody can reach.** ⛔ **If `TASK-941` returns *"none hold"*, ⛔ this comes ⛔ straight back — ⛔ it is ⛔ conditional, ⛔ not dismissed.**
2. ✅ **BOARDED as a ⛔ NAMED RIDER (⛔ same file, ⛔ same function, ⛔ two string constants) — ⛔ TWO SITES REUSE THE ⛔ WRONG SENTENCE.** ⛔ `ConfirmStackUpgrade`'s ⛔ target-died-between-ghost-and-click branch and its ⛔ M8-authority-guard-after-refund branch ⛔ both say *"That building cannot be stacked"*, which is ⛔ FALSE in both cases. ⇒ ⛔ **distinct `NSLOCTEXT` keys.** ⚖️ *⛔ A message that is ⛔ wrong on a ⛔ rare path is how the ⛔ next playtest report gets ⛔ misdiagnosed.*

### STACK-§10 ⛔⛔⛔⭐⭐ **THE ⛔ MEASURED CEILING IS `n = 2` — AND IT IS SET BY A TERM ⛔ NO CLAUSE OF `STACK-§8` OR `TOWER-§8.5a` HAD ⛔ NAMED: ⛔ THE DECK SLAB SCALES WITH THE MESH WHILE THE ⛔ BREACH WINDOW IS SIZED IN ⛔ CAPSULE HALF-HEIGHTS** (added 2026-09-03; ⛔ **discharges `STACK-§8` cl. 4 · outcome row 2, *"some hold"* · from `handoffs/TASK-941-programmer.md`, `n = 1` positive control ⛔ PASSING, 6 of 7 controls exact**)

> ### ⛔⛔⛔ **`AClimbableTower::MaxStackHeightMultiplier = 2`.**
> ⭐ **`STACK-§2`'s *"a non-uniform scale preserves ⛔ nothing"* is ⛔ MEASURED FALSE for the standoff, ⛔ in the GOOD direction — it ⛔ IMPROVES: `103.32 → 112.38 → 115.10 → 116.40 → 117.15`, ⛔ clearing `≥ 98.0` at ⛔ ALL FIVE `n`.**
> ⛔ **The tower ⛔ still stops at ⛔ ×2, and ⛔ not for any reason ⛔ anyone had written down.**

**1. ⛔⛔ THE ARITHMETIC THAT SETS THE CEILING — ⛔ WRITTEN OUT, ⛔ BECAUSE IT IS ⛔ CHECKABLE AND ⛔ NOBODY SHOULD HAVE TO RE-DERIVE IT.**

`FSiegeLadderClimbStatics::Begin` sizes the ⛔ non-swept deck-breach window as `DeckBreachCapsuleHalfHeights (3) × HalfHeight` — a property of the ⛔ **PAWN**, ⛔ invariant in `n`. The deck slab (`UCX_SM_WatchTower_07`, `Z [1160, 1200]`) is a property of the ⛔ **MESH** ⇒ ⛔ **`40 n` uu thick.**

```
window OPENS at capsule-centre Z  =  1200n − 2·HH
sweep  JAMS  at capsule-centre Z  =  1160n − HH      (capsule TOP meets the slab underside)

need OPEN <= JAM  :  1200n − 2·HH <= 1160n − HH   <=>   40n <= HH   <=>   n <= HH / 40
     unit HH = 88  ->  n <= 2.2   (⛔ BINDING)        hero HH = 96  ->  n <= 2.4
```

- ⛔⛔ **THE ⛔ AI UNIT BINDS, ⛔ NOT THE HERO** — ⭐ **which ⛔ INVERTS `TOWER-§8.5a`'s standing intuition that the hero's ⛔ FATTER capsule is the hazard. ⛔ Here the hero's ⛔ TALLER capsule buys it ⛔ MORE window.**
- ⭐⭐ **AND THE HEADROOM WAS ⛔ ALREADY DECLARED, ⛔ IN THE SOURCE, ⛔ BY THE CONSTANT ITSELF:** `DeckBreachCapsuleHalfHeights`' own doc comment says it is *"⛔ **2.2× the ~40 uu the mesh actually ships**"*. ⇒ ⚖️ ***⛔ A Z STACK SPENDS EXACTLY THAT HEADROOM, ⛔ LINEARLY IN `n`. ⛔ The margin was ⛔ documented and ⛔ nobody had connected it to ⛔ scaling.***

| `n` | slab | window opens @ `Zc` | sweep jams @ `Zc` | margin | outcome |
|---|---|---|---|---|---|
| 1 | 40 | 1024 | 1072 | **+48.00** | ✅ ships today |
| **2** | **80** | 2224 | 2232 | ⭐ **+8.00** | ✅ **PASS — the licensed ceiling** |
| 3 | 120 | 3424 | 3392 | ⛔ **−32.00** | ⛔ watchdog drop |
| 4 | 160 | 4624 | 4552 | ⛔ **−72.00** | ⛔ watchdog drop |
| 5 | 200 | 5824 | 5712 | ⛔ **−112.00** | ⛔ watchdog drop |

⛔ **WHAT `n ≥ 3` IS IN PLAY: the swept capsule's top meets the slab underside ⛔ BEFORE `ShouldSweep` turns sweeping off. It cannot advance, the window ⛔ never opens, and the climber hangs in `MOVE_Flying` until `§8.5a` cl. 7's watchdog ⛔ drops it from ~3,392 uu.** ⭐ **The one good thing: cl. 7 makes that failure ⛔ LOUD, ⛔ not silent.** ⛔ **It is still a tower whose deck ⛔ cannot be reached — ⛔ exactly the `STACK-§8` cl. 2 outcome the ban existed to prevent.**

**2. ⚖️⛔⛔ THE RULING, AND ⛔ HOW IT MUST BE DESCRIBED — ⛔ SHIP AT ×2, AND ⛔ RECORD THE SHORTFALL PLAINLY.**

- ⚖️ **DEFAULT (⛔ proceeding unless 🧑 Jonathan rules otherwise): ⛔ SHIP AT ×2.** ⭐ **`MaxStackHeightMultiplier` per class is ⛔ already the shape `STACK-§8` cl. 4 + `J-13` chose, so the partial costs ⛔ no new machinery, and `J-6`'s cap behaviour ⛔ already paints the ghost ⛔ BLUE at the ceiling with ⛔ zero new UI.** ⚖️ *⛔ A ⛔ MEASURED ×2 that ⛔ ships beats a ⛔ THEORISED ×5 that ⛔ voids the climb.*
- ⛔⛔ **AND THE HONEST SENTENCE, ⛔ WHICH IS ⛔ NOT OPTIONAL: 🧑 HIS SPEC SAID ⛔ ×2 → ×5. ⛔ THIS IS A ⛔ MEASURED SHORTFALL AGAINST IT, ⛔ NOT A DESIGN CHOICE, AND ⛔ NO ROW, HANDOFF, COMMIT MESSAGE OR SLACK POST MAY DRESS IT AS ONE.** ⛔ **The alternative — ⛔ HOLD the feature and ⛔ re-engineer the deck window to scale — was ⛔ put to him and is ⛔ his call, ⛔ not the board's.**
- ⛔ **`ArrowTower` and every other building are ⛔ UNAFFECTED and stay at ⛔ ×5** (`J-13`). ⛔ **The ceiling is ⛔ per class and ⛔ a second copy of it anywhere is a ⛔ FAIL.**
- ⛔ **IF the window is ever re-engineered to scale (⛔ i.e. `DeckBreachCapsuleHalfHeights` becomes a ⛔ slab-derived quantity rather than a pawn-derived one), ⛔ THIS CEILING IS ⛔ RE-DERIVED FROM THIS SECTION'S ARITHMETIC — ⛔ never bumped by hand.**

**3. ⭐ WHAT `n = 2` ACTUALLY BUYS — ⛔ MEASURED, ⛔ NOT HOPED.** A **2,400 uu** tower whose standoff is **+9.06 uu BETTER** than the one shipping today; whose ⛔ non-swept stretch **HALVES** (22.00 % → 11.00 % of the line, ⛔ i.e. ⛔ MORE of the ascent is swept ⇒ ⛔ strictly safer against `§8.5`'s named harms); whose foot and deck navmesh margins are **bit-identical at every `n`** (⛔ both are X/Y facts and a Z scale ⛔ cannot touch Y); and whose watchdog **scales with the line** (14.14 s → 27.64 s against a 6.91 s ascent) ⛔ because it was **derived, ⛔ never a literal**.

**4. ⛔ THE RUNG PLANE — ⛔ DEPTH SURVIVES · ⛔ PITCH IS ⛔ COSMETIC** (`STACK-§8` cl. 4's mandatory split, ⛔ discharged).

- ⭐ **DEPTH — the thing the hands ⛔ GRIP — ⛔ SURVIVES:** `22.000 → 22.502 → 22.599 → 22.633 → 22.649`, ⛔ **max +3.0 %**, closed form `depth(n) = 22n / sqrt(Ux² + n²Uz²)` ⛔ **bounded above by `22 / Uz = 22.677` at ⛔ ANY `n`.** ⇒ ⛔⛔ **`TOWER-§8.3`'s mesh↔clip binding does ⛔ NOT fire, and `A_SiegeBiped_Climb` needs ⛔ NO re-export on depth grounds. ⛔ No clip re-export is owed.**
- ⚠️ **PITCH ⛔ STRETCHES and is ⛔ COSMETIC:** `40 → 78.22 → 116.82 → 155.53 → 194.27`; ⛔ **rung COUNT unchanged (31)** and pitch-as-a-fraction-of-the-line ⛔ invariant. At `n = 2` the hands cycle at `4.47 Hz` against `8.75 Hz` of authored rung passage ⇒ ⛔ **hands grip between rungs about half the time.** ⛔ **This voids ⛔ NOTHING** (the depth is preserved, so the hands still land in the ladder slab) ⇒ ⛔ **it is a note for 🧑 Jonathan's eye, ⛔ NEVER a gate.** ⚖️ *⛔ Conflating pitch with depth is how a ⛔ shippable feature gets ⛔ refused.*

**5. ⚠️⚠️⛔ THE ⛔ NAV-LINK OCTREE HAZARD — ⛔ PREDICTED, ⛔ NOT MEASURED, AND IT IS ⛔ THE ONE THAT A ⛔ HERO-ONLY PLAYTEST ⛔ CANNOT SEE.**

- ✅ ⭐ **FIRST, THE HAZARD THAT WAS ⛔ REFUTED, ⛔ IN WRITING, SO IT IS ⛔ NOT RAISED AGAIN: ⛔ THE CLIMB LINE IS ⛔ NOT CACHED.** `ConfigureLadderLink()` has ⛔ exactly ⛔ ONE caller (`BeginPlay`) — ⛔ **but it stores a ⛔ RELATIVE transform.** `ResolveLadderSocketRelative` uses `RTS_Actor`, which ⛔ divides the actor transform (⛔ scale included) ⛔ straight back out; `NavLinkCustomComponent.cpp:518-525` ⛔ re-applies the ⛔ LIVE owner transform on ⛔ EVERY read. ⛔ **4 read sites, ⛔ ZERO caches** (⛔ negative control: the only `FVector` members on `AClimbableTower` are the two `static const` degrade-open fallbacks, ⛔ themselves relative). ⇒ ⛔ **a scaled tower does ⛔ NOT strand a climber at the old deck height.** ⭐ **`TOWER-§8.4(A)`'s `RTS_Actor` decision, made for the ⛔ spawn-squash, ⛔ pays for this for free.**
- ⚠️⚠️ **AND THE ⛔ ADJACENT ONE THAT IS ⛔ NOT REFUTED: ⛔ `UNavLinkCustomComponent` IS A `UActorComponent`, ⛔ NOT A `USceneComponent`.** ⇒ ⛔ **a root rescale runs `PropagateTransformUpdate` → `UpdateComponentInNavOctree` for the ⛔ SCENE component ⛔ only. ⛔ Nothing re-gathers the ⛔ LINK's octree element**, whose off-mesh connection was baked from the owner transform ⛔ at gather time. ⇒ ⛔ **the tower's ⛔ collision navmesh regenerates at the new height while the ⛔ registered connection may remain at the ⛔ old `Z = 1200`** ⇒ ⛔ **Recast can drop it and ⛔ AI UNITS STOP BEING HANDED A PATH TO THE LADDER.**
- ⛔⛔⭐ **THE PART THAT MAKES THIS LAW RATHER THAN A FOOTNOTE: ⛔ THE ⛔ HERO'S CONTACT CLIMB IS ⛔ UNAFFECTED — it reads `GetStartPoint()` ⛔ live and ⛔ never consults the navmesh.** ⇒ ⚖️ ***⛔ A HERO-ONLY PLAYTEST WOULD ⛔ PASS WHILE THE BOT'S UNITS ⛔ SILENTLY FAIL TO PATH UP. ⛔ That sentence goes on the ⛔ acceptance row ⛔ in those words, ⛔ every time side (iii) is discharged.***
- ✅ **CHEAP TO CLOSE, ⛔ AND THE CLOSURE IS ⛔ MANDATORY ON ANY ROW THAT APPLIES A RUNTIME SCALE: ⛔ re-call `ConfigureLadderLink()` ⛔ after a successful `ApplyStackUpgrade`** — it re-reads the ⛔ same scale-free relatives and drives `SetLinkData` → `UpdateNavigationBounds` + `RefreshNavigationModifiers`. ⛔ **Guard it with a ⛔ CALL-SHAPE test (`SC-§41`), ⛔ not a bare token.**

**6. ⚠️ TWO ⛔ LATENT TRAPS RECORDED BESIDE THE FEATURE — ⛔ NEITHER IS A DEFECT TODAY, ⛔ BOTH ARE ONE EDIT AWAY.**

- ⚠️⛔ **(a) `USiegeMeshJuiceComponent` CAN ⛔ SILENTLY UN-STACK A TOWER.** `SetTargetMesh` snapshots `BaseScale` and the squash's terminal branch writes `SetRelativeScale3D(BaseScale)` ⛔ verbatim. ⭐ **Measured ⛔ SAFE as shipped** (one call, at `ABuilding::BeginPlay`; the channel runs only while `bSquashActive`; `AClimbableTower : public ABuilding`, ⛔ not `ATower`, so `PlayRecoil` can never reach it). ⛔ **THE TRAP: ⛔ a stack upgrade is ⛔ exactly the moment someone would want a squash** ⇒ ⛔ **any future `PlaySpawnSquash()` on a stacked building ⛔ resets the mesh to the ×1 baseline and ⛔ UN-STACKS the tower — height ⛔ and climb line together — with ⛔ every readback correct.** ⇒ ⛔ **a comment naming this trap sits ⛔ beside `ApplyStackUpgrade`.**
- ⚠️ **(b) `AClimbableTower::PlatformHeightUU = 1200.f` becomes a ⛔ LIE at `n > 1`.** ⛔ **Measured harmless today: ⛔ ZERO non-test consumers of `GetPlatformHeightUU`** (⛔ 3 references, ⛔ all in `Tests/SiegeClimbableTowerTest.cpp`, ⛔ reading the ⛔ CDO, which a runtime instance scale cannot touch). ⛔ **It becomes a defect the moment a `HIGH-§` consumer binds to it.**

**7. ⛔ THE ⛔ ONE INPUT THIS SECTION'S NUMBERS ⛔ ASSUME AND ⛔ NOBODY HAS READ BACK.** ⛔⛔ **Every Z figure above assumes `BP_Building_WatchTower`'s authored `VisualMesh` relative Z scale is ⛔ `1.0`.** ⛔ **`TASK-941` was fenced from opening a `.uasset` and ⛔ said so.** ⛔ **Corroborated but ⛔ NOT measured:** `PlatformHeightUU = 1200.f`, `LadderTopDefaultRelative.Z = 1200.f` and `build_watchtower.py`'s `RISE = 1200.0` ⛔ all agree — ⛔ **that is a CITATION, ⛔ not a measurement** (`SC-§40` cl. 1). ⇒ ⛔⛔ **`TASK-956` reads it back, and ⛔ NOTHING that applies a stack multiplier to this tower ships before it does. ⛔ If it is not `1.0`, ⛔ every number in this section scales by it and the ⛔ ceiling is re-derived, ⛔ not adjusted.**

---

