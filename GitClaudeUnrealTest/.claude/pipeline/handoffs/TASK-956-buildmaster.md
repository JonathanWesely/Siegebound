# TASK-956 — build-master handoff

# ⭐⭐ THE NUMBER IS IN, AND IT IS `1.0`.

> ## ⛔ **(i) `AuthoredHeightScaleZ == 1.0` ⇒ ⭐ `STACK-§10`'s table stands ⛔ UNMODIFIED and `TASK-942` is ⛔ UNBLOCKED.**

⛔ **Read LIVE off the saved asset via MCP** — `BP_Building_WatchTower`'s `VisualMesh.RelativeScale3D` =
**`(1.0, 1.0, 1.0)`**. ⛔ **Not inferred, ⛔ not cited, ⛔ not hoped.**

⭐ **`STACK-§10` cl. 7 is DISCHARGED.** ⛔ **Nothing in `TASK-941`'s table shifts. The ceiling stays `n = 2`.**

⚠️ **This row was attempted TWICE. The first attempt was a `NO-GO` and is preserved in §7 — it is the
reason the number can be trusted now.**

---

## 1. ✅ THE MCP PRE-FLIGHT — A REAL REQUEST, NOT A PORT CHECK (`SHIP-§9e`)

| probe | result |
|---|---|
| `MainWindowTitle` | ✅ **`'GitClaudeUnrealTest - Unreal Editor'`** — ⛔ no longer `'Restore Packages'` |
| **`list_toolsets` (the real request)** | ✅ **RETURNED — 19 toolsets** |

⛔ **Port 8000 was listening during the NO-GO too, and answered nothing. The discriminator is the
RETURN, and it returned.** ✅ Re-checked after every read: still `'GitClaudeUnrealTest - Unreal Editor'`.

---

## 2. ⛔ THE FOUR READ-BACKS — AS NUMBERS (spec item (2))

### ⛔ (a) `VisualMesh` RELATIVE SCALE — **THE BINDING ONE**

`/Game/Blueprints/Buildings/BP_Building_WatchTower.Default__BP_Building_WatchTower_C:VisualMesh`

```json
{"RelativeScale3D":{"x":1,"y":1,"z":1},
 "RelativeLocation":{"x":0,"y":0,"z":0},
 "RelativeRotation":{"pitch":0,"yaw":0,"roll":0},
 "StaticMesh":{"refPath":"/Game/Meshes/SM_WatchTower.SM_WatchTower"}}
```

⇒ ⛔⛔ **X = `1.0` · Y = `1.0` · Z = `1.0`.** ⭐ **The mesh binding is also confirmed: `SM_WatchTower`.**

### ⭐ (b) THE ROOT COMPONENT — **CONFIRMED ON THE ASSET, ⛔ NOT INHERITED FROM THE `.cpp`**

The row demanded this be confirmed on the asset rather than read out of `Building.cpp:37-38`. Both
properties were queried on the CDO and **resolve to the SAME object**:

```json
{"VisualMesh":  {"refPath":"...Default__BP_Building_WatchTower_C:VisualMesh"},
 "RootComponent":{"refPath":"...Default__BP_Building_WatchTower_C:VisualMesh"}}
```

⇒ ⭐ **`VisualMesh` IS the root — by identity of refPath, ⛔ not by inference.** ⛔ **The BP did not
re-root.** ⇒ **the root-component relative scale IS the `(1,1,1)` above.**

### ⛔ (c) THE ACTOR'S CLASS-DEFAULT TRANSFORM SCALE

⛔ **Since `VisualMesh` is the root, (a) and (c) are the SAME storage** — the class-default root
transform is scale `(1,1,1)`, location `(0,0,0)`, rotation `(0,0,0)`. ⛔ **There is no second,
actor-level scale to disagree with it.**

### ⛔ (d) `SM_WatchTower` Z BOUNDS — **AS THE MESH REPORTS THEM**

`StaticMeshTools.get_bounds` on `/Game/Meshes/SM_WatchTower.SM_WatchTower`:

| axis | min | max |
|---|---|---|
| X | `-446.41793823242188` | `300` |
| Y | `-300.00018310546875` | `300` |
| **Z** | **`-0.000048876205`** | ⭐ **`1308.656005859375`** |

⚠️⭐ **AND THE THING NOBODY SHOULD SKIM PAST: THE MESH'S Z-MAX IS `1308.656`, ⛔ NOT `1200`.**
⛔ **That is ⛔ NOT a contradiction and it must ⛔ not be read as one:** `1200` is the **DECK**
(`PlatformHeightUU`, `LadderTopDefaultRelative.Z`, `RISE`), and `get_bounds` returns the **RENDER
BOUNDS**, which include the **railing/parapet standing ~108.66 uu ABOVE the deck.**
⇒ ⭐ **Had I equated mesh-Z-max with the rise, I would have "found" a scale of `1308.656 / 1200 = 1.0906`
and reported a false `≠ 1.0`.** ⚖️ ***The corroborant that needs the most care is the one that ALMOST
lines up.***
⛔ **Local Z-min is `≈ 0` (`−4.9e-05`)** ⇒ the mesh is authored with its base on the origin plane, which
is what makes a `×1` component scale put the deck at world `1200` exactly.

---

## 3. ⛔⛔ THE CONTROLS — AND THE ⛔ NEGATIVE ONE IS THE LOAD-BEARING ONE (`SC-§39`)

### ⛔ POSITIVE CONTROL (spec item (3)) — `BP_Building_ArrowTower`

```json
{"RelativeScale3D":{"x":1,"y":1,"z":1}, "RelativeLocation":{"x":0,"y":0,"z":0},
 "RelativeRotation":{"pitch":0,"yaw":0,"roll":0},
 "StaticMesh":{"refPath":"/Game/Meshes/SM_ArrowTower.SM_ArrowTower"}}
```

⛔ **BOTH CAME BACK IDENTICAL, AND THE ROW REQUIRED ME TO SAY SO AND SAY WHY IT IS STILL EVIDENCE.**
⭐ It is evidence of **fleet consistency** — the two towers are authored the same way, so the WatchTower
is not an outlier. ⛔⛔ **BUT ON ITS OWN IT IS ⛔ WORTHLESS AS AN INSTRUMENT CHECK: two `1.0`s from a
probe that can only say `1.0` is exactly `SC-§39`'s *"a probe that returns 1.0 for everything, including
things that are not 1.0, is not a probe."*** ⇒ **a third read was required.**

### ⭐⭐ NEGATIVE CONTROL — **THE ONE THAT MAKES THE MEASUREMENT MEAN ANYTHING**

`L_Arena` was **already the loaded level** (`get_current_level` → `/Game/Maps/L_Arena`) — ⛔ **nothing was
loaded, opened or changed to obtain this.** Its `Ground` actor:

`/Game/Maps/L_Arena.L_Arena:PersistentLevel.StaticMeshActor_1.StaticMeshComponent0`

```json
{"RelativeScale3D":{"x":560,"y":250,"z":1},"StaticMesh":{"refPath":"/Engine/BasicShapes/Cube.Cube"}}
```

⇒ ⛔⛔ **THE SAME TOOL, THE SAME PROPERTY NAME, A DIFFERENT OBJECT — AND IT RETURNS `560` AND `250`.**
⭐⭐ **AND IT IS BETTER THAN A BARE NON-UNIT READ: its Z is `1` while X/Y are not** ⇒ ⛔ **the reader
returns PER-COMPONENT values and is not collapsing a transform to one scalar — which is precisely the
axis this whole measurement turns on.** ⇒ ⛔ **the probe is NOT blind and NOT canned to `(1,1,1)`.**

### ⭐ INSTRUMENT CONTROL — THE READER RETURNS REAL DATA, AND LIES DOWN LOUDLY WHEN IT CANNOT

- `PlatformHeightUU` → **`1200`** · `CardID` → **`"WatchTower"`** ⇒ ⛔ genuine authored values, ⛔ not zeros
  and ⛔ not defaults.
- ⭐⭐ **AND A FREE NEGATIVE CONTROL I DID NOT PLAN: a property name that does not exist (`MaxHealth`)
  made the call FAIL with *"the following properties could not be read: MaxHealth"* — it did ⛔ NOT
  silently return a default.** ⇒ ⛔ **a mistyped `RelativeScale3D` could not have returned a
  fabricated `1.0`; it would have errored.** ⚖️ *The tool's strictness is itself an instrument guarantee.*

---

## 4. ⭐ THE THREE CORROBORANTS — EACH INDEPENDENT OF THE LIVE READ

**1. ⭐⭐ THE HEADLESS FName-TABLE PROBE — AND IT ⛔ PREDICTED THIS ANSWER ⛔ BEFORE MCP CAME BACK.**
A byte-level scan of `BP_Building_WatchTower.uasset` (entry = `[int32 len][ascii][NUL]`, counted only when
the preceding 4 bytes equal `len+1`) recovered **222 FName entries** and found **`RelativeScale3D`
ABSENT — zero entries containing "Scale" at all**, while `VisualMesh`, `SM_WatchTower`, `SCS_Node`,
`StaticMesh` were all **present**. UE **delta-serializes** against the archetype ⇒ no tag ⇒ no override.
Its own controls: `RelativeScale3D` **HITS** in `L_Arena.umap` @10283 and `L_MainMenu.umap` @9344 (the
needle fires); `BP_Unit_Witch`/`BP_Unit_Archer` **HIT** `RelativeLocation`+`RelativeRotation` inside a
**Blueprint** package while **not** hitting `RelativeScale3D` (a within-file differential, independently
corroborated by `TASK-946`'s own MCP readback of yaw −90 / Z −91).
⇒ ⭐⭐ **A headless instrument PREDICTED `1.0` and the live engine read CONFIRMED it. Two methods with
NOTHING in common agree.**

**2. ⭐ THE C++ ARCHETYPE — WHAT THE DELTA IS MEASURED AGAINST.**
`ABuilding`'s constructor creates `VisualMesh`, calls `SetRootComponent(VisualMesh)`, sets the collision
profile and nav relevance, and ⛔ **never sets a scale.** Scale-write census across
`Building.{h,cpp}` + `ClimbableTower.{h,cpp}` = ⛔ **exactly two, both inside `ApplyStackUpgrade`
(the runtime stack itself), ⛔ none in any constructor.** ⛔ **No intermediate Blueprint parent exists** —
read off the package: `NativeParentClass` **and** `ParentClass` are **both**
`/Script/CoreUObject.Class'/Script/GitClaudeUnrealTest.ClimbableTower'` ⇒ the BP derives **directly from
C++**, so there is no second BP in the chain to hide a delta.

**3. ⭐ GEOMETRIC CONSISTENCY AT `×1`.** The mesh's local Z-min is `≈ 0` and its deck sits at `1200`,
which is the number three independent shipped sources carry: `ClimbableTower.h:595`
`PlatformHeightUU = 1200.f` · `ClimbableTower.cpp:102` `LadderTopDefaultRelative(-160, 0, 1200)` ·
`build_watchtower.py:129` `RISE = 1200.0` with **:191** `DECK_Z0, DECK_Z1 = 1160.0, RISE`
(⭐ which independently reproduces `STACK-§10`'s **40 uu** slab `[1160, 1200]`).
⛔ **At a scale of `1.0`, local `1200` IS world `1200` — the three constants are consistent with the mesh
only at `×1`.** ⚠️ **Declared limit: `get_bounds` returns RENDER bounds; I did ⛔ not read the collision
hull `_07` directly, so the `[1160, 1200]` slab remains `STACK-§10`'s figure, ⛔ not mine.**

### ⛔ ONE THING THAT LOOKS LIKE A FOURTH CORROBORANT AND IS ⛔ NOT — DECLARED SO NOBODY COUNTS IT

⛔ **`AuthoredHeightScaleZ` reads `1` on the CDO** — ⛔ **and that is ⛔ NOT independent evidence.**
`Building.h:453` declares `float AuthoredHeightScaleZ = 1.f;`, so the CDO is merely reporting **the
member's own default initialiser**; the runtime value is captured lazily at `Building.cpp:424` and
`StackUpgradeCount` reads **`0`** (never captured). ⇒ ⛔ **Circular. It corroborates nothing about the
mesh and is excluded from the count.** ⚖️ *A number that agrees for the wrong reason is worse than
no number.*

---

## 5. ⭐ THE EXPOSURE, RESTATED NOW THAT IT IS MOOT — SO THE MANAGER RE-RULES NOTHING

⛔ **`AuthoredHeightScaleZ` already exists in shipped code and is already correct for ANY authored scale:**
`Building.cpp:424` captures it **at runtime from the live component**, `:440` applies
`AuthoredHeightScaleZ * StackHeightMultiplier(StackUpgradeCount)`. ⇒ ⛔ **the CODE never assumed `1.0`.**
The assumption lived **only** in `STACK-§10`'s **arithmetic table** (`1200n`, slab `[1160n, 1200n]`,
`40n ≤ HH`). ⇒ ⛔ **a non-`1.0` value would have meant the CEILING was derived at the wrong scale, ⛔ not
that anything was broken.** ✅ **It is `1.0`, so the table stands and there is nothing to re-derive.**

---

## 6. 📌 TWO FINDINGS THE ROW DID NOT ASK FOR — ⛔ REPORTED, ⛔ NOT REPAIRED

### ⚠️⚠️⭐ (1) A PER-CLASS CEILING WOULD BE **SILENTLY IGNORED** BY THE SHIPPED RESOLVER — `TASK-942` READ THIS FIRST

`MaxStackHeightMultiplier` **already exists** (`Building.h:346`, `EditDefaultsOnly`, **`= 5`**), and
`CanStackHeight` is **absent** (spec intact). ⛔ **But the resolver is:**

```cpp
float ABuilding::StackHeightMultiplier(int32 UpgradeCount)          // Building.cpp:294
{
    const ABuilding* const Defaults = GetDefault<ABuilding>();      // :302  ⛔ ABuilding, ALWAYS
    ...
    const int32 Cap = FMath::Max(1, Defaults->MaxStackHeightMultiplier);   // :314
```

⇒ ⛔⛔ **`GetDefault<ABuilding>()` is hardcoded to `ABuilding`'s CDO, ⛔ NOT the calling instance's class.**
⇒ ⛔ **Setting `MaxStackHeightMultiplier = 2` on `AClimbableTower`'s constructor would be ⛔ READ RIGHT
PAST, and the tower would keep stacking to `5` — the exact deck-unreachable state `STACK-§10` bans.**
⭐⭐ **`TASK-942`'s amended signature `StackHeightMultiplier(int32 UpgradeCount, int32 MaxMultiplier)` is
therefore ⛔ LOAD-BEARING, ⛔ not a tidy-up — it is the ⛔ ONLY thing that makes the ×2 ceiling real.**
⚠️ **And the comment at `:296` reads *"THE CAP IS READ FROM THIS CLASS'S CDO"* — ⛔ FALSE AS WRITTEN**
(it is `ABuilding`'s CDO unconditionally); the class comment at `:340-345` states the true behaviour.
⛔ **Prose-vs-code drift of the class `SC-§39.1` cl. 7 legislates. ⛔ I changed nothing.**

### ⚠️ (2) `Content/FogArea/` IS UNTRACKED AND IS ⛔ NOT MINE

⛔ **Zero `Content/` writes were made by this row** (every asset read was `rb` or an MCP getter). It
postdates the session-start snapshot ⇒ likely the editor or another lane. ⚠️ **Worth settling before
`TASK-944` derives its pathspec from its own `git status` — an unaccounted `Content/` path is exactly how
something gets swept into a commit.**

---

## 7. ⛔ THE FIRST ATTEMPT — PRESERVED, BECAUSE IT IS WHY THIS NUMBER IS TRUSTWORTHY

⛔ **Attempt 1 was a `NO-GO` and delivered no number.** `list_toolsets` **TIMED OUT** while PID 42252 was
**ALIVE** with `Responding=True`, `MainWindowTitle='Restore Packages'`, and port 8000 in **`Listen`** —
⭐ **measured proof that a listening port is not liveness.** Per item (5) that is a NO-GO, reported and
**not worked around**: the modal was **left untouched** because its buttons restore autosaved packages,
`L_Arena` is under a never-save law with a pinned hash, synthetic input is documented to lie here
(`SHIP-§9`), and a kill+relaunch reproduces the prompt.
✅ **🧑 Jonathan cleared it himself with *Skip Restore*. ⛔ Nothing was restored.**
⭐⭐ **The item (4) verdict sentence was WITHHELD on attempt 1 rather than guessed at the value the
evidence was leaning toward — which is the only reason issuing it now means anything.**
⚠️ **This modal has now wedged this editor FOUR times in this batch. A fifth is a finding, not luck.**

---

## 8. ⛔ FENCE COMPLIANCE

- ⛔ **ZERO writes to any asset.** Tools used were **all getters**: `list_toolsets`, `describe_toolset`,
  `get_bounds`, `get_default_object`, `get_properties`, `get_current_level`, `find_actors`.
  ⛔ **`set_properties`, `reset_properties`, `save_actor`, `load_level` were NEVER called.**
- ⛔ **No save prompt and no restore prompt was accepted.** ⛔ **`L_Arena` was already open — ⛔ not loaded
  by me — and was not modified.**
- ⛔ **No compile. No suite. No Git operation. No `Source/**` write.**
- ⛔ **`AssetTools.is_dirty` was never consulted** (it returns `true` for every asset that exists).
- ✅ **Editor verified clean after the reads:** PID 42252, `'GitClaudeUnrealTest - Unreal Editor'`.
- ✅ **Files written: this handoff + the `TASK-956` board row only.** ⛔ `TASK-944`'s pathspec takes this
  file (`TL-§5d`).

---

## Files read (⛔ none modified)

- `.claude/pipeline/TASKBOARD.md` · `.claude/pipeline/CONVENTIONS.md` (`STACK-§10`, `SC-§39`, `SC-§39.1`,
  `SC-§40`) · `.claude/pipeline/handoffs/TASK-941-programmer.md` §6/§7
- `Source/GitClaudeUnrealTest/Siegebound/Building.{h,cpp}` · `ClimbableTower.{h,cpp}` ·
  `Tools/ArtPipeline/build_watchtower.py`
- **MCP (read-only):** `BP_Building_WatchTower` · `BP_Building_ArrowTower` · `SM_WatchTower` ·
  `L_Arena` `StaticMeshActor_1` (negative control)
- **Byte-read only:** `BP_Building_{WatchTower,ArrowTower,Wall,CrystalTower,Barracks}.uasset` ·
  `BP_Unit_{Witch,Archer}.uasset` · `{L_Arena,L_MainMenu}.umap`
