# TASK-1163 — [FOGFLOOR-LITERAL-REVERSAL] — gameplay-programmer handoff

**Date:** 2026-09-08 · **Baseline:** `61702e1` (last **measured** suite `552 / 0`) · **Gate:** ⭐ `TASK-1164` — **REAL, NOT WAIVED** (`SC-§27`: compiled bytes) · **Host:** ⭐ `TASK-1149`
**Law:** `FOG-§12.7` (the compiled-literal carve-out) · `FOG-§12.1` *as corrected TWICE* · `GFX-§9` reversal bullet · `SC-§27` · `SC-§97` · `SC-§102`
**Measurer for every claim below:** ⭐ `TASK-1160` (`handoffs/TASK-1160-artist.md` §3–§4, 5 promoted PNGs).

**Suite delta: `0` — a DERIVATION, not a measurement.** I did not compile and did not run the suite (`TASK-1149` hosts both). **NO WITNESSED RED** — § 5.

**Compiled-byte delta, measured not asserted (§ 4):** `FogVolume.cpp` **+11 / −6 code lines** (the three literal groups, nothing else) · `SiegeGraphicsMenuTest.cpp` **+1 / −1 code line** (the description only; the predicate is byte-identical) · `SiegeGraphicsMenuWidget.cpp` **+0 / −0 code lines — ZERO compiled bytes** (clause (2)/(2a) is comment text, proven).

---

## 1. THE FOUR LITERALS — `file:line`, old, new, specifier counts

⚠️ Line numbers are **post-edit** and were read back after the final edit. Every site was **re-located by its quoted words**, not by the number the board carried (`FOG-§12.5`).

⭐ **A shape I applied to all four and want QA to rule on (declared decision D-1):** the struck literal is kept **verbatim in a comment immediately above its `UE_LOG`**, and the log line itself carries **only the corrected text**. Strike-in-place inside a shipped `Error` string would put `~~…~~` in front of an operator who cannot tell which half of it is live — which is the same failure mode as the falsehood. **History kept, operator not confused.**

### 1.1 🚨 `FogVolume.cpp:1096-1099` — **THE WORST, AND IT WAS FIXED FIRST** (board cl. (1)(a))

The `Error` on the **failure branch** — it fires exactly when the floor did **not** take, i.e. when `r.VolumetricFog` is **unpinned** and the exploit is **LIVE**.

**OLD (2 literal lines, struck; kept verbatim in the comment at `:1073-1089`):**
> `⚠️ It does NOT mean the battlefield is clear: the Fog card's own visual is BP_SiegeFog, a raymarched mesh this floor does not reach (TASK-1151). ⛔ Report this as a FLOOR failure, never as 'no fog'.`

**NEW (4 literal lines) — and this is the sentence the row asked me to quote:**
> `🚨 AND IT MAY MEAN EXACTLY THAT THE BATTLEFIELD IS CLEAR: the Fog card's own visual, BP_SiegeFog, is driven by this SAME console variable — measured on pixels (TASK-1160) — so an unpinned variable at Shadows=Low deletes the CARD'S wash too. ⛔ Report this as a FLOOR failure AND as a live 'no fog' exploit: LOOK AT THE CARD'S WASH before you rule 'no fog' out (reversed by TASK-1163).`

⚖️ **The instruction is now the opposite of the struck one, and it is actionable rather than merely true**: the old text told the operator to *rule "no fog" out*; the new one tells him to *go look at the card's wash before he rules it out*. A false statement misleads; a false instruction gets acted on — so the repair had to replace the **instruction**, not just delete the false clause.

| | specifiers | order | args |
|---|---|---|---|
| **PRE** | **8** | `%s %s %d %d %s %s %s %.0f` | **8** |
| **POST** | **8** | `%s %s %d %d %s %s %s %.0f` | **8** |

### 1.2 `FogVolume.cpp:976-979` — the cvar-missing branch (board cl. (1)(b))

**OLD (struck; verbatim in the comment at `:964-970`):**
> `⚠️ This does NOT govern the Fog card's own visual (BP_SiegeFog is a raymarched mesh outside the froxel grid, TASK-1151) — do not read this line as 'the fog is gone'.`

**NEW:**
> `🚨 AND DO NOT READ THIS LINE AS 'only the ambient fog is exposed': the console variable this build is missing is the one TASK-1160 measured driving the Fog card's own visual, BP_SiegeFog, as well — with no floor engaged, 'the fog is gone' is a reading this line WARRANTS, not one it rules out (reversed by TASK-1163).`

| | specifiers | order | args |
|---|---|---|---|
| **PRE** | **7** | `%s ×7` | **7** |
| **POST** | **7** | `%s ×7` | **7** |

### 1.3 `FogVolume.cpp:1144-1146` — the success path (board cl. (1)(c))

**OLD (struck; verbatim in the comment at `:1132-1139`):**
> `⛔ SCOPE: this is the AMBIENT volumetric fog. The Fog card's own visual is BP_SiegeFog, a raymarched mesh outside the froxel grid, and this line says NOTHING about it (TASK-1151).`

**NEW:**
> `⭐ SCOPE: this covers BOTH fogs. The same console variable also drives the Fog card's own visual, BP_SiegeFog — measured on pixels (TASK-1160) — so this line is ALSO the record that the card's wash now survives Shadows=Low (reversed by TASK-1163).`

⭐ Direction noted in the comment, because it is the one that reads as harmless: this literal was false **in the other direction** — it *disclaimed the floor's own value*, the very effect `TASK-1160` later measured.

| | specifiers | order | args |
|---|---|---|---|
| **PRE** | **13** | `%s %s %d %s %d %s %d %s %.0f %d %d %d %s` | **13** |
| **POST** | **13** | `%s %s %d %s %d %s %d %s %.0f %d %d %d %s` | **13** |

### 1.4 🚨 `Tests/SiegeGraphicsMenuTest.cpp:1012` — **DESCRIPTION ONLY; THE PREDICATE IS BYTE-IDENTICAL** (board cl. (1)(d))

**OLD description clause (struck; verbatim in the comment at `:1004-1011`):**
> `TASK-1151 measured that BP_SiegeFog is a raymarched TRANSLUCENT MESH, not a froxel participant, so what Effects/Textures do to it is a DIFFERENT and UNMEASURED question`

**NEW description** — the **conclusion is unchanged and the ground is replaced**, exactly as the row specified:
> `🚨⭐⭐ …and it does NOT claim anything about the other nine quality groups. ⛔ REASON REVERSED 2026-09-08 (TASK-1163), CONCLUSION UNCHANGED: TASK-1160 measured the SHADOWS route deleting BP_SiegeFog on pixels, so the old ground ('outside the froxel grid') is refuted — this guard now rests on the ground that SURVIVES, that only the Shadows route was measured and the OTHER NINE quality groups never were. ⛔ A menu sentence wider than the measurement behind it is the same defect this row is correcting, one draft later`

The adjacent comment also records **why the guard may not be relaxed**: `TASK-1149` cl. (4-R)(ii) leans on it.

**Specifiers: `0` before, `0` after** — `TestFalse(const TCHAR*, bool)` takes no format arguments, and I verified the new description contains **no `%` character at all**.

🚨 **THE PREDICATE, PROVEN NOT ASSERTED.** Line `:1013`, unchanged:

```cpp
		FogOn.Contains(TEXT("every setting")) || FogOff.Contains(TEXT("every setting")));
```

`md5` of that line **pre = post = `82487bc4a3efbd4b17be604ab877c03b`**. It was `:1005` before my edit and is `:1013` now — **the line moved, the bytes did not.** Nothing was deleted, inverted or relaxed.

---

## 2. CLAUSE (2) / (2a) — `SiegeGraphicsMenuWidget.cpp`, and it cost **ZERO compiled bytes**

Both strikes are in place (`~~…~~`, the `TASK-1083` precedent), naming ⭐ `TASK-1160` measurer and this row adjudicator.

**(b) at `:200-207` → struck; replacement `(b-R)` at `:213-215`.** The struck text: *"The old text never governed the card's fog in the first place, so nothing the floor did could have falsified it."* ⇒ replaced by: `r.VolumetricFog` drives **both** fogs, so the old text's unqualified *"volumetric fog"* **did** cover the card's and the floor **does** bear on it. ⭐ I also recorded that `TASK-1162`'s **flagging was correct restraint** (`SC-§82`/`SC-§101`), not a miss — its row named only (a); naming (b) was the manager's, and this row is that naming.

**`:216-219` (the old `:208-211`) → struck**, because (b) was its only support: *"a SCOPING repair"* only parses if the old noun was too **wide**, and it was not.

**⭐ THE REPLACEMENT, written where the mistake was made (`:220-231`):**

> **THE OLD MENU STRING WAS NOT A LIE.** It said *"drives volumetric fog, which the engine turns OFF at Low and Medium"*, and that was **ACCURATE — AND ITS ACCURACY WAS THE BUG. It correctly told the player how to TURN THE FOG OFF.** ⇒ the repair was **neither** a scoping fix **nor** the correction of a lie: it **removed a TRUE sentence whose truth was the defect**, and then `EnforceFogRenderFloor()` **made the new sentence true**. The string and the floor are one change in two files.

**Cited, not re-derived**, exactly as the row required — `Tests/SiegeGraphicsMenuTest.cpp:1001`: the pre-floor string *"ADVERTISED an exploit in the game's own menu, to the one population that would act on it."*

⇒ **`SiegeGraphicsMenuWidget.cpp` code lines: 2394 → 2394, `+0 / −0`.** Comment text only, measured in § 4.

---

## 3. CLAUSE (3) — THE CENSUS. **Count: 19 term-matching literal-bearing code lines. 4 asserted the refuted claim. 4 fixed. 0 remain. 1 ADJACENT SITE FLAGGED AND ROUTED, NOT ADJUDICATED.**

Swept **all of `Source/**`** (`*.cpp`, `*.h`, `*.cs`, `*.py`), **code lines only** (comments stripped by `CountOccurrencesInCode`'s own rule), using `TASK-1162`'s own terms as the row required: `raymarched` · `froxel` · `does not reach` · `says NOTHING about` · `BP_SiegeFog` · `TASK-1151`.

**19 matches. The four the board named are all of them; there were no extras.** The other 15 are keyword collisions or statements that are **still true**:

| site | why untouched |
|---|---|
| `FogVolume.cpp:470`, `:618` | the asset **path** literal and its diagnostic — not a claim |
| `FogVolume.cpp:1118` | *"the FROXEL GRID did not take"* — about the two grid cvars, **true**, and a `Warning` not an `Error` on purpose |
| `SiegeFogVisualTest.cpp:693` | *"BP_SiegeFog is not an `AFogVolume` subclass, so `TActorIterator<AFogVolume>` never sees it"* — a **class-hierarchy fact**, still true. ⛔ Not the refuted claim: the refuted claim was the **inference** from non-participation, never the description |
| `SiegeFogVisualTest.cpp:735/738/739/741/755` | token-count arguments — they name the symbol, they assert nothing about fog physics |
| `SiegeFogVisualTest.cpp:1355` | *"an Epic→Low player keeps the EPIC froxel grid"* — the **bill**, still true |
| `SiegeAssistantSelectionTest.cpp:3375` · `SiegeClimbableTowerTest.cpp:1515` · `SiegeHeroLadderClimbTest.cpp:267` · `SiegePlacementTest.cpp:2945` · `SiegeGraphicsMenuTest.cpp:620` | unrelated subjects (parser coverage, ladder reach, `MOVE_Flying` teardown, wheel wiring, a save probe) |

### 🚩 3a. THE SECOND SWEEP, WHICH THE ROW DID NOT ASK FOR — and the one site it found

Clause (2a) reverses a **different** premise (*"the old string was a lie / the repair was scoping"*), and that premise can also sit in a compiled literal. I swept for it separately: `false sentence` · `a lie` · `scoping` · `never governed` · `the old string` · `the old text` · `turns OFF at Low`.

**Exactly one hit that is not the player-facing hint string:**

> 🚩 **`Tests/SiegeGraphicsMenuTest.cpp:1014`** — a `TestTrue` **description**: *"…⛔ An unqualified 'volumetric fog' claim is **the false sentence this row exists to correct**."*
> Under clause (2a) that is refuted in the same breath as (b): the unqualified claim was **not false**, it was accurate, and its accuracy was the defect.

⛔ **I did not touch it, and this is deliberate.** Two independent reasons: (i) the row's `- names:` fence says `SiegeGraphicsMenuTest.cpp` (**1 literal, DESCRIPTION ONLY**) and this would be a second; (ii) cl. (3) says *flag and route ambiguous sites, do not adjudicate* (`SC-§82`) — and this one is genuinely arguable, because *"the false sentence this row exists to correct"* can be read as scoped to `TASK-1147`'s own framing rather than as a live claim. ⇒ **ROUTED TO THE MANAGER.** Its predicate (`FogOn/FogOff.Contains("ambient volumetric fog")`) is correct either way and I did not go near it.

⚠️ The two **hint strings** at `:348` / `:351` also match `turns OFF at Low` — they are **player-facing, protected by cl. (4), true post-floor, and untouched** (§ 6).

---

## 4. THE COMPILED-BYTE DELTA — MEASURED, NOT ASSERTED

All three files were **already dirty** at session start (`TASK-1147` + `TASK-1161` + `TASK-1162`, uncommitted), so `git diff HEAD` cannot isolate me. I snapshotted all three **before touching them** and diffed **code lines only** (comment lines stripped by `CountOccurrencesInCode`'s own rule, character for character — `SiegeAcquisitionFunnelTest.cpp:124-129`).

| file | code lines | delta | what changed |
|---|---|---|---|
| `SiegeGraphicsMenuWidget.cpp` | 2394 → **2394** | **+0 / −0** | **nothing** — clause (2)/(2a) is comment text |
| `FogVolume.cpp` | 624 → **629** | **+11 / −6** | **exactly the three literal groups and nothing else** (the full unified diff is three hunks, all inside `UE_LOG` format strings) |
| `SiegeGraphicsMenuTest.cpp` | 1612 → **1612** | **+1 / −1** | **exactly the one `TestFalse` description line** |

**Delimiter and brace balance** (a prose edit's real failure mode is commenting code out):

| file | `{` / `}` | `/*` / `*/` | `"` parity |
|---|---|---|---|
| `FogVolume.cpp` | 72 / 72 → 72 / 72 | 1 / 1 → 1 / 1 | even |
| `SiegeGraphicsMenuWidget.cpp` | 359 / 359 → 359 / 359 | 35 / 35 → 35 / 35 | even |
| `SiegeGraphicsMenuTest.cpp` | 115 / 115 → 115 / 115 | 26 / 26 → 26 / 26 | even |

**Every new literal uses `'single quotes'` internally** — no escaped `"` was introduced anywhere.

### 4a. The text-scan constraints these files live under — all re-verified GREEN post-edit

`FogVolume.cpp` is read as **text** by two automation tests, and a "just rewording a string" edit is exactly the shape that trips them. Every one of these is a **suite row that would have gone red** had I been careless; all were re-measured after the final edit:

| constraint (code lines of `FogVolume.cpp`) | required | measured |
|---|---|---|
| hand-typed `640` / `360` / `260` / `32000` / `18000` / `7000` / `26000` / `12000` | 0 each | **0 each** ⚠️ *this is why no measured figure appears in my new text — the numbers live in the comments, where they are stripped* |
| `TEXT("r.VolumetricFog` (cvar names written once) | **3** | **3** |
| `/Game/Blueprints/BP_SiegeFog` | **1** | **1** |
| `MarkPackageDirty` · `SavePackage` · `GConfig` · `SetQualityLevels` · `ScalabilityQuality` · `FSiegeFogStatics` · `TActorIterator<AActor>` | 0 each | **0 each** |
| `bFogVisible` · `bFogCleared` · `bFogPrevented` · `bVisualSpawned` · `FogVisualUntilTimeSeconds` · `FogVisualDeadline` | 0 each | **0 each** |
| inside `EnforceFogRenderFloor()`: `->Set(` | **3** | **3** |
| … `CaptureFogRenderFloorPriorState(` | **1** | **1** |
| … `AchievedVolumetricFog` / `bAchievedHeightFogVolumetric` | ≥ 2 | **4 / 4** |
| … `FogActiveUntilTimeSeconds` / `IsFogActive()` | 0 | **0 / 0** |
| … before the first `->Set(`: `Observed.VolumetricFogEnabled = ` and `CaptureFogRenderFloorPriorState(` | 1 each | **1 each** |

⭐ **All three of my edited literals sit INSIDE `EnforceFogRenderFloor()`'s body**, which `FSiegeFogRenderFloorWiringTest` extracts and counts — so this was not a theoretical risk.

---

## 5. TESTS AND NAMED MUTATIONS — **NO WITNESSED RED**

Per `SC-§104` cl. 5 I computed the asserted value of every candidate witness row **on both branches** (pre-edit tree vs post-edit tree). **Every row evaluates identically on both, so every row is struck** — no existing test reads a `UE_LOG` format string or an automation-test *description*, and the one row whose description I rewrote asserts on its **predicate**, which is byte-identical.

⇒ **NO WITNESSED RED.** Suite delta **`0`**, and that is a **DERIVATION, not a measurement** — I did not compile and did not run the suite.

**Named mutations (`SC-§83`, Addendum B form) — what WOULD have gone red, so the zero above is bounded rather than empty:**

1. **Put `"every setting"` into either hint string** ⇒ **≥ 1 row red, including** `Siegebound.GraphicsMenu.ShadowHintTracksVolumetricFog` (`FSiegeGraphicsMenuFogHintTest`) — **this is the guard `TASK-1149` cl. (4-R)(ii) leans on, and it is live.**
2. **Invert that `TestFalse` to `TestTrue`** ⇒ **≥ 1 row red, including** the same test (the hints contain no `"every setting"`, so the inverted assertion fails immediately). ⚠️ *Deleting* it, by contrast, goes **green** — which is why `TASK-1164`'s byte-identity check on `:1013`, not a suite run, is the real protection.
3. **Type any of the eight banned geometry digits into one of my new literals** ⇒ **≥ 1 of 8 rows red, including** `Siegebound.Fog.TheVisualAssetIsNamedInExactlyOnePlaceAndNoCallerOutsideTheStateOwnerKnowsIt` (`FSiegeFogVisualOwnershipTest`).
4. **Begin a new literal line with `TEXT("r.VolumetricFog`** ⇒ **≥ 1 row red, including** `Siegebound.Fog.TheIntegrityFloorIsEngagedAndReleasedByTheOneReconcilerAndByNothingElse` (`FSiegeFogRenderFloorWiringTest`, cvar-names-written-once, `3 → 4`).
5. **Write `/Game/Blueprints/BP_SiegeFog` into a new literal** ⇒ **≥ 1 row red, including** `FSiegeFogVisualOwnershipTest` (`1 → 2`).
6. **Write `->Set(` into my new prose** ⇒ **≥ 1 row red, including** `FSiegeFogRenderFloorWiringTest`'s three-writes row (`3 → 4`).

🚨 **7. THE MUTATION WITH NO WITNESS AT ALL, AND IT IS THE ONE THIS ROW COULD ACTUALLY HAVE COMMITTED: dropping a `%s` from a multi-line literal group.** No automation row reads these format strings; MSVC does not validate `UE_LOG`'s varargs. **The suite would stay green and the log would read garbage or crash at the call site.** That is precisely why § 1's specifier counts are stated per literal and computed by parsing the statement rather than by eye — **the counts ARE the test for this mutation, and they are `8/8`, `7/7`, `13/13`, all order-identical.**

---

## 6. FENCES

| fence | result |
|---|---|
| `Content/**` · `Config/**` · `.uasset` | **zero** writes |
| `L_Arena` | never opened |
| Editor / MCP | **never touched** — an art-director holds it for `TASK-1152`/`TASK-1154` |
| compile · suite run · Git · commit · push | **none** (`TASK-1149` is the host) |
| `ShadowHintFogOn` / `ShadowHintFogOff` (cl. 4) | **byte-identical**, proven below |
| the `TestFalse` **predicate** at (d) | **byte-identical**, `md5 82487bc4a3efbd4b17be604ab877c03b` (§ 1.4) |
| `CONVENTIONS.md` | **not edited** — findings routed to the manager (`SC-§82`), § 3a |
| `TASKBOARD.md` | **`TASK-1163`'s single `- status:` line only**, via `Edit` |

### 🚩 THE HINT-STRING `md5` — declared decision D-2, please read

**The strings are byte-identical, and I proved it two ways.** The two declarations plus their string lines, now at `:347-351` (were `:327-331`):

- `md5` **pre = post = `b78ce8d67d323cc225ac109040759d9e`**
- and a direct `diff` of the extracted block pre vs post: **IDENTICAL**, zero lines out.

⚠️ **That digest is NOT `TASK-1162`'s `7117476313b920c8511f51b6551d1fdc`, and it cannot be.** `TASK-1162` recorded the value but not the **method**, and I tried nine reconstructions (line ranges `327,328` / `327,330` / `327,331` / `327,332` / `326,332` / the two declarations without the blank line, each under both LF and CRLF) — **none reproduces it**. A digest without its method is not reproducible, so I declare mine: **`grep -A1 "static const TCHAR\* ShadowHintFogO"` over the file, LF line endings, `md5sum`.** ⇒ The fence is met by the **`diff`**, which needs no method at all; the digest is the convenience. 📌 **For the manager: `FOG-§12.5`'s re-prove-the-`md5` clause should name the method alongside the value, or the next row hits this same wall.**

**Files touched:** `Source/GitClaudeUnrealTest/Siegebound/FogVolume.cpp` (3 literals + 3 strike-record comments) · `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeGraphicsMenuTest.cpp` (1 description + 1 strike-record comment; predicate untouched) · `Source/GitClaudeUnrealTest/Siegebound/SiegeGraphicsMenuWidget.cpp` (**comments only, +0/−0 code lines**) · this handoff · the board status line.

---

## 7. ⚖️ WHAT NOTHING I WROTE SAYS — checked by re-reading every inserted line

- ⛔ **Nothing says ask (A) is CLOSED.** The seven untested conditions are named in the comment at `FogVolume.cpp:1073-1089` by reference to `FOG-§12.1`'s second correction, and the phrase written instead is *"mechanism isolated to the cvar; coupling not read at the material."*
- ⛔ **`bUsedWithVolumetricFog` appears nowhere** in any line I wrote — it is a candidate nobody has opened (`SC-§97`).
- ⛔ Nothing records `TASK-1147`'s 28-package scan as a miss (`FOG-§12.7`, final bullet).

---

## 8. FLAGGED DECISIONS FOR ⭐ `TASK-1164`

| # | decision | what I did | how to overturn |
|---|---|---|---|
| **D-1** | **Struck literals live in an adjacent COMMENT, not in the shipped string.** History is kept verbatim; the log line carries only corrected text. | applied to all 4 sites | if you want `~~…~~` inside the `Error` string itself, say so — it is a 4-edit change, but I believe it ships confusion to an operator at the moment of a failure |
| **D-2** | **The hint-string `md5` does not match `TASK-1162`'s**, because its method was never recorded. | proved identity by `diff` **and** by my own declared-method digest | re-derive by any method you like; the `diff` is method-free |
| **D-3** | 🚩 **`SiegeGraphicsMenuTest.cpp:1014`** (*"the false sentence this row exists to correct"*) carries clause (2a)'s refuted premise in a **compiled literal**, in a file I was already editing. | **FLAGGED AND ROUTED, NOT EDITED** — the `- names:` fence says *1 literal, description only*, and cl. (3) says do not adjudicate | if the manager rules it in, it is a one-edit follow-up in this same file, with the same predicate untouched |
| **D-4** | The three FogVolume comments carry the **numbers-free** form of the measurement (*"to within a fifth of a percent"*, not the digits) — because `FSiegeFogVisualOwnershipTest` bans eight geometry digits on code lines and I chose one habit for the whole file rather than a per-line judgement. | applied in comments too, conservatively | harmless to relax **in comments**; ⛔ never on a code line |

## 9. WHAT QA SHOULD SCRUTINISE FIRST

1. 🚨 **The `TestFalse` predicate at `SiegeGraphicsMenuTest.cpp:1013`** — the row's own first-sentence blocker test. `md5 82487bc4a3efbd4b17be604ab877c03b`, pre and post. It **moved from `:1005` to `:1013`**; check the bytes, not the number.
2. 🚨 **The specifier counts in § 1** — `8/8`, `7/7`, `13/13`. This is the one mutation the suite cannot witness (§ 5 item 7). Re-count them yourself; do not take my table.
3. **`FogVolume.cpp:1096`'s new instruction** — does it actually tell an operator something he can *do* at the moment of detection, or is it merely no longer false? I aimed for the former (*"LOOK AT THE CARD'S WASH"*).
4. **§ 3a's flagged site** — one compiled literal, adjacent premise, deliberately untouched. Rule on whether the restraint was right or whether it is another `TASK-1162` loop.
5. **§ 4's `+0 / −0` on `SiegeGraphicsMenuWidget.cpp`** — clause (2)/(2a) claims to be free, and that claim is the reason a comment-heavy edit sits inside a compiled-bytes row.
