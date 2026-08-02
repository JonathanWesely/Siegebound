# TASK-362 — [AG-T4] Boost-bar plumbing: `IHealthBarProvider` defaulted virtuals + widget BIE + C++ banding

**Agent:** gameplay-programmer
**Date:** 2026-08-01
**Status:** ready-for-qa
**Constraints honored:** NO compile, NO Build.bat, NO Git, NO editor, NO MCP. Files only.

---

## 1. THE CONTRACT TASK-368 CONSUMES VERBATIM (copy from here, do not re-derive)

### 1.1 The BIE — exact signature, character-for-character

```cpp
UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|UI")
void SetDamageBoost(float FillFraction, float R, float G, float B, float RowOpacity);
```

Declared on `UCombatantHealthBarWidget` (`CombatantHealthBarWidget.h`), in the `public:` block beside
`OnHPChanged` / `SetTeamColor`. **Five floats, in that order.** No enums, no structs, no `FLinearColor`
(widget-param law). **ONE event, not two** — the color changes with the value, so splitting fill from
color would leave a frame where band-3 purple paints at a band-4 fill.

**Required EventGraph — one linear exec chain, ONE `MakeLinearColor` fanned to both consumers:**

```
event SetDamageBoost (FillFraction, R, G, B, RowOpacity)
  $C = MakeLinearColor(R, G, B, 1.0)          <-- alpha literal MUST be 1.0; READ IT BACK
  SetPercent(BoostBar, FillFraction)
    -> SetFillColorAndOpacity(BoostBar, $C)
    -> SetBrushColor(BoostOutline, $C)
    -> SetRenderOpacity(BoostOutline, RowOpacity)
    -> SetRenderOpacity(BoostBar, RowOpacity)
```

**ZERO conditionals in the widget.** No branch, no compare, no select — every decision is already made in
C++. If the graph needs an `if`, the C++ contract was misread.

⚠️ `SetDamageBoost` landing as a **`K2Node_CustomEvent`** never fires from C++, silently, and still reports
`bIsImplemented = true`. Author via `add_event`, then assert the node's object **class** is `K2Node_Event`
via `get_node_infos`. This is the exact defect that hid the health-bar bug five times (`handoffs/TASK-131.md`).

### 1.2 The four band colors + the track — verbatim linear RGB

| Band | Boost range | Linear RGB | ≈ hex | C++ property |
|---|---|---|---|---|
| 1 | 0–100% | `(0.55, 0.80, 1.00)` | `#C4E7FF` | `BoostBand1Color` |
| 2 | 100–200% | `(0.010, 0.020, 0.350)` | `#1927A0` | `BoostBand2Color` |
| 3 | 200–300% | `(0.200, 0.010, 0.420)` | `#7C19AD` | `BoostBand3Color` |
| 4 | 300–400% | `(0.010, 0.010, 0.014)` | `#191920` | `BoostBand4Color` |

**`BoostBar` track color: `(0.22, 0.22, 0.24, 0.85)`** — RGBA, alpha 0.85. Recorded in C++ as
`BoostBarTrackColor` **for the record only**; the component never pushes it. **TASK-368 authors `BoostBar`'s
background brush tint from THIS number.** (`Bar` keeps its existing near-black `(0.03,0.03,0.03)@0.7` track —
do not touch it. The separate medium-grey track exists solely so band-4 black is legible: on the near-black
health track it computes ~1.3:1, i.e. the strongest unit would get the worst indicator.)

**The band colors are DATA, pushed every update. NEVER hardcode any of them in the WBP** — the widget only
ever sees them as the R/G/B floats on the wire.

### 1.3 Widget-tree names the C++ assumes (LAW — TASK-367 human step + TASK-368)

```
[ROOT] VerticalBox "BarStack"
  ├─ [0] Border      "BoostOutline"   slot Size=Fill 1.0, Padding 1.5 uniform, Pad Bottom 1
  │        └─ ProgressBar "BoostBar"
  └─ [1] ProgressBar "Bar"            slot Size=Fill 2.0   <- EXISTING, properties UNTOUCHED
```

Both NEW widgets: `WhiteSquareTexture`, `DrawAs = Image`, tint white. **A material fill brush swallowing a
runtime tint was TASK-131's actual root cause — do not reopen that door.**

---

## 2. Files touched (4) — and the files deliberately NOT touched

**Edited:**
- `Source/GitClaudeUnrealTest/Siegebound/HealthBarProvider.h`
- `Source/GitClaudeUnrealTest/Siegebound/CombatantHealthBarWidget.h`
- `Source/GitClaudeUnrealTest/Siegebound/CombatantHealthBarComponent.h`
- `Source/GitClaudeUnrealTest/Siegebound/CombatantHealthBarComponent.cpp`

**NOT touched (acceptance criteria — verify these are clean in the diff):**
- `SummonedUnit.{h,cpp}` — **TASK-360 owns them exclusively this batch.** I declare the delegate TYPE only;
  TASK-360 declares the member, the two overrides, and every broadcast site.
- `Building.{h,cpp}` and `HeroCharacter.{h,cpp}` — **zero changes by construction**, which is the entire
  reason the two new interface methods are defaulted rather than pure virtual. Do not add overrides there.
- `CombatantHealthBarWidget.cpp` — a `BlueprintImplementableEvent` has no C++ body; nothing to add.

---

## 3. What each file got

### 3.1 `HealthBarProvider.h`

```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatantDamageBoostChanged, float, BoostPercent);
```
Declared immediately after the existing `FOnCombatantHPChanged`, with the "broadcast on EVERY mutation"
discipline documented on it.

Two **DEFAULTED** virtuals appended to `IHealthBarProvider` (`public:`, after `IsHealthBarActorAlive()`):
```cpp
virtual float GetDamageBoostPercent() const { return 0.f; }
virtual FOnCombatantDamageBoostChanged* GetDamageBoostChangedDelegate() { return nullptr; }
```
These are the only two non-pure methods on the interface, on purpose: **the defaults ARE the contract for
"this actor cannot be boosted".** The delegate accessor returns a **POINTER** so "not boostable" is
expressible without every building and the hero carrying a dead delegate member just to return a reference.

Matches the CONVENTIONS §7 pinned registry character-for-character; TASK-360's
`float GetDamageBoostPercent() const override;` / `FOnCombatantDamageBoostChanged* GetDamageBoostChangedDelegate() override;`
bind to these.

### 3.2 `CombatantHealthBarWidget.h`

The `SetDamageBoost` BIE from §1.1, plus a class-doc line stating that the boost row is driven **entirely by
the component**: the widget holds NO boost state and does NOT bind the boost delegate. One push path, no
second source of truth. `CombatantHealthBarWidget.cpp` is unchanged.

### 3.3 `CombatantHealthBarComponent.h`

- `UFUNCTION() void HandleOwnerDamageBoostChanged(float BoostPercent);` — the delegate handler.
- `void PushDamageBoost(float BoostPercent);` — bands + pushes (protected, non-UFUNCTION).
- `FLinearColor GetBoostBandColor(int32 Band) const;` — band index → tint; out-of-range clamps to band 4.
- Six new `EditDefaultsOnly` UPROPERTYs, category `Siegebound|HealthBar|Boost`:
  `BoostBand1Color` … `BoostBand4Color` (the §1.2 triplets), `BoostBarTrackColor` `(0.22,0.22,0.24,0.85)`,
  and `MinBoostFillFraction = 0.04f`.

### 3.4 `CombatantHealthBarComponent.cpp`

**DrawSize `(90,12)` → `(90,22)`** — the namespace-scope `CombatantHealthBarDrawSize` applied in the
constructor, so every existing owner (units, miners, buildings, towers, hero) picks it up with zero per-class
edits.

**BeginPlay — SEED UNCONDITIONALLY, THEN bind:**
```cpp
PushDamageBoost(Provider ? Provider->GetDamageBoostPercent() : 0.f);   // unconditional seed
if (Provider)
{
    if (FOnCombatantDamageBoostChanged* BoostDelegate = Provider->GetDamageBoostChangedDelegate())
    {
        BoostDelegate->AddUniqueDynamic(this, &UCombatantHealthBarComponent::HandleOwnerDamageBoostChanged);
    }
}
```
The unconditional seed is **not redundant**: it is what drives a non-boostable owner's row to `RowOpacity 0`
instead of leaving it at whatever design-time state the WBP was authored with, and it also covers a boosted
actor whose bar is created after the boost was granted. Bind-only goes stale forever — `qa/TASK-005` major 2,
the same law the HP seed above it obeys.

To reach `Provider` from both the HP block and the boost block I **hoisted** the existing
`if (IHealthBarProvider* Provider = Cast<IHealthBarProvider>(OwnerActor))` into a plain local + `if (Provider)`.
The HP seed-then-bind body inside it is byte-identical to before.

**The banding, in `PushDamageBoost` (all of it, C++ only):**
```cpp
const int32 Band = FMath::Clamp(FMath::CeilToInt(BoostPercent / 100.f), 1, 4);
const float Frac = FMath::Max((BoostPercent - (Band - 1) * 100.f) / 100.f, MinBoostFillFraction);
```
`CeilToInt` is **upper-inclusive by design**: 100% = full light blue · 100.1% = nearly-empty dark blue ·
400% = full black. No special case anywhere. That boundary ambiguity is exactly what the band-colored
outline resolves, which is why the outline receives the SAME color as the fill.

`BoostPercent <= 0` ⇒ `SetDamageBoost(0, band-1 RGB, RowOpacity 0)` — the row is hidden by opacity, never by
`SetVisibility`, so the health bar keeps a constant head offset for boosted and unboosted units alike. The
fill/color are still fully specified (empty, band 1) so no stale band-3 purple can flash if the row is later
re-shown.

Both paths drive the **current on-screen `GetWidget()`** (not the `BarWidget` cached at BeginPlay) and then
`RequestRedraw()` — the identity-proof + stale-render discipline `HandleOwnerHPChanged` already uses.

---

## 4. Flagged for QA to scrutinize

1. **ONE deliberate superset of the spec'd formula, and it is documented in-line:** after computing `Frac`
   exactly as CONVENTIONS §5 writes it, I clamp the wire value with `FMath::Min(Frac, 1.f)`. The `Clamp`
   caps the band *index* but not the numerator, so a hypothetical `BoostPercent > 400` would compute
   `Frac > 1`. **Unreachable through the shipping path** (`MaxPermanentDamageStacks = 80` == exactly +400%,
   and TASK-363's cheat clamps) and it changes **no in-range value** — for `0 < B <= 400` the expression
   already lands in `[0.04, 1]`. If QA rules it noise, deleting the one `Min` line is behavior-neutral today.
2. **No `EndPlay` unbind for the boost delegate.** Deliberate parity: the existing HP delegate binding
   (`HandleOwnerHPChanged`, shipped since TASK-130) has no unbind either — component and owner die together,
   and dynamic delegates drop bindings to destroyed UObjects. Adding one here alone would be inconsistent;
   adding both is out of scope for this task.
3. **Interface change ⇒ module-wide rebuild.** `HealthBarProvider.h` is included by `SummonedUnit.h`,
   `Building.h`, `HeroCharacter.h`, `CombatantHealthBarWidget.h` and `SiegeCheatManager.cpp`. Adding
   *defaulted* virtuals is source- and ABI-compatible for all of them (no existing override changes, no
   pure-virtual added), but TASK-366 should expect a wide recompile.
4. **`BoostBarTrackColor` is intentionally unreferenced in code.** It is the single home for a number the
   artist types by hand at TASK-368. An unused UPROPERTY produces no warning.
5. **`MinBoostFillFraction` is a UPROPERTY, not a literal `0.04f`** — matching how CONVENTIONS §5 names it.
6. **Widget-param law compliance:** five floats. `RowOpacity` is a float (`SetRenderOpacity`), not a
   visibility enum — that is the law, and it is also what keeps the head offset constant.

---

## 5. Interim visual consequence — REAL, not a defect (flag to Jonathan at TASK-377)

`DrawSize` is now `(90,22)` but the WBP is still the old single-`ProgressBar` tree until TASK-367 (Jonathan's
human step) + TASK-368 land. **Between this commit and TASK-368, every overhead health bar renders ~2× taller
than before** — the existing `Bar` stretches to fill the new 22 px box. Nothing is broken, no boost row exists
yet, and `SetDamageBoost` on an unimplemented BIE is a safe no-op. It self-corrects the moment the
`BarStack` tree exists (`Bar` at Fill 2.0 returns to ~12 px). Worth knowing so an intermediate PIE run does
not get filed as a regression.

## 6. Multiplayer reality (state it, per manager ruling 13)

In M8 P1 units are server-only, so on a Red client there are **no units, no sorcerers, and no boost bars** —
this feature is play-verifiable single-player/host only until M8 P2. The boost row itself adds no replication:
it is driven entirely by a local delegate on a locally-simulated actor. `PermanentDamageStacks` becoming
`ReplicatedUsing` is TASK-360's recorded P2 duty, and its `OnRep` re-broadcasting keeps this exact
seed-then-bind path identical on the client with **zero changes to this component**.

## 7. Downstream dependencies

- **TASK-360** consumes `FOnCombatantDamageBoostChanged` and overrides both defaulted virtuals on
  `ASummonedUnit`. Its broadcast discipline is what makes this row live.
- **TASK-367 (Jonathan)** builds the `BarStack`/`BoostOutline`/`BoostBar` tree.
- **TASK-368 (art-director)** authors the `SetDamageBoost` override + brushes, consuming §1 of this document
  verbatim.
- **TASK-363** exercises the whole chain from a PIE console (`SetTestDamageBoost`), which is the only
  practical way to land on exactly 100.0 / 200.0 / 300.0 for the boundary check.
