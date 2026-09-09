# TASK-1162 — [FOGFLOOR-COMMENT-REVERSAL] — gameplay-programmer handoff

**Date:** 2026-09-08 · **Baseline:** `61702e1` (last **measured** suite `552 / 0`) · **Gate:** WAIVED (board grounds (a)–(c)) · **Host:** `TASK-1149`
**Law:** `FOG-§12.1` *as corrected TWICE* · `GFX-§9` reversal bullet · `SC-§96` · `SC-§97` · `SC-§102`
**Suite delta: `0`** — and that is a **DERIVATION, not a measurement** (§ 5). I did not compile and did not run the suite; `TASK-1149` hosts both.

**Comment text only. SEVEN strikes across THREE files, ZERO compiled bytes — proven in § 4, not asserted.**

---

## 0. What this row reversed, and why it is not tidying

`TASK-1161` (hours earlier) rewrote `SiegeGraphicsMenuWidget.cpp`'s Shadows-hint comment block to say **the floor "does not reach the card's fog"**, on the then-best-supported ground that `BP_SiegeFog` is a raymarched mesh outside the froxel grid. A gate ruled it sound. **`TASK-1160` then photographed the opposite** (`handoffs/TASK-1160-artist.md` §3–§4, 5 promoted PNGs):

- With a Fog card's `BP_SiegeFog` up and Shadows at **LOW**, the card's fog **does not render** — the frame is the **no-fog frame to within 0.2 %** (far-field RGB 193→104, contrast 0.4–10 → 19.5, featureless 70–100 % → 2.4 %); ground visibility `≤464 uu` → **no collapse anywhere** (castle legible at ~43,000 uu) ⇒ **≈93× lower bound**.
- ⭐ A **2×2 isolates the mechanism to ONE cvar**: EPIC + forced `r.VolumetricFog 0` ⇒ fog **GONE**; LOW + forced `r.VolumetricFog 1` ⇒ fog **BACK** ⇒ the effect tracks `r.VolumetricFog`, **not** `sg.ShadowQuality`. Row D is a hand-simulation of `TASK-1147`'s floor.

⇒ **The floor IS the fix**, not defensive depth. ⚖️ *A correct conclusion from refuted premises is still a defect, because the premise is what the next change will be reasoned from.*

Every strike is **in place** (`~~…~~`, the `TASK-1083` precedent), names `TASK-1160` as the measurer, and says what replaced it. **No text was silently deleted anywhere.**

---

## 1. THE STRIKES — file:line, old, new

Line numbers are **post-edit** and were read back after the final edit. The struck text is quoted here only by its opening words; the full old text is still **in the file**, inside the `~~ ~~`, which is the point.

### 1.1 `SiegeGraphicsMenuWidget.cpp:162-168` → strike; replacement `:169-199` — board cl. (1)/(1a)/(1b)

**OLD (struck, kept verbatim):** `(a) ⛔ THE FLOOR DOES NOT REACH THE CARD'S FOG. It pins r.VolumetricFog and the two froxel-grid cvars at ECVF_SetByCode … BP_SiegeFog is a raymarched translucent mesh OUTSIDE the froxel grid, so no variable in that set governs it — which is exactly what the "What IS established" paragraph at :196-202 below says correctly …`

**NEW (`:169-199`), the four specified contents:**
- `REVERSED 2026-09-08 (TASK-1162)` + *"STRUCK IN PLACE, NEVER SILENTLY DELETED: A COMMENT THAT RECORDS ITS OWN REVERSAL TEACHES THE NEXT READER THAT THIS QUESTION IS HARD; A CLEAN ONE TEACHES HIM IT WAS OBVIOUS"* + ⚠️ *"TASK-1161 WAS NOT SLOPPY — the struck premise was this project's best-supported position and a gate ruled it sound. Then TASK-1160 PUT A CAMERA ON IT."*
- The measurement with its numbers and its evidence path.
- **(i)** the floor pins `r.VolumetricFog` + the two froxel-grid axes at `ECVF_SetByCode`; **(ii)** the 2×2, both directions, Shadows held fixed; **(iii)** *"THE FLOOR REACHES THE CARD'S FOG AND IS THE FIX FOR THE DEMONSTRATED EXPLOIT."*
- **(iv)** ⚠️ ***"THE MECHANISM IS ISOLATED TO THE CVAR; THE COUPLING HAS NOT YET BEEN READ AT THE MATERIAL"*** + an explicit ban on writing *"because `bUsedWithVolumetricFog`"* (`SC-§97`). **`bUsedWithVolumetricFog` is nowhere asserted as the cause in any line I wrote.**
- The stale `:196-202` inside the struck text is **kept verbatim as struck history**, with a new line saying the paragraph has moved and must be found by its opening words.

### 1.2 `SiegeGraphicsMenuWidget.cpp:200-201` — ⚠️ FLAGGED, NOT REWRITTEN (declared decision, `:202-207`)

Clause **(b)** (*"The old text never governed the card's fog in the first place"*) rests on the **same refuted premise** as (a): if `r.VolumetricFog` drives both fogs, the old string's unqualified *"volumetric fog"* did cover the card's, and the floor does bear on it. **The board named only (a) and the two paragraphs below**, so I left (b) standing with an in-file flag rather than adjudicate an unnamed sentence, and **routed it to the manager** (`SC-§82`). 🚩 **This is the one judgement call in the row — overturn it and I will strike (b) too.**

### 1.3 `SiegeGraphicsMenuWidget.cpp:212-217` → strike; replacement `:218-236` — board cl. (1)(b)

**OLD (struck):** `⛔⛔ AND THE CARD'S HALF IS STILL OPEN: no measured route deletes BP_SiegeFog … ⇒ the reported exploit is UNEXPLAINED — NOT confirmed, NOT refuted, NOT CLOSED. NO COMMIT MESSAGE, ROW, HANDOFF OR SLACK POST MAY SOURCE A "the exploit is fixed" SENTENCE FROM THIS BLOCK.`

**NEW:** the exploit is **DEMONSTRATED**; ⭐ *what survives the strike* — `TASK-1147`'s 28-package scan **was not wrong**, it enumerated `DetailMode`/`QualitySwitch`/draw-distance and correctly found none, *the route was a fourth kind nobody enumerated* ⇒ *an exhaustive search of an incomplete list is still an incomplete search*; and 🚨 **the prohibition is NARROWED, not lifted** — a commit **may** now say *"the floor is the fix for the demonstrated exploit"*, ⛔ **nothing may say ask (A) is CLOSED**, with the **seven** untested conditions written out by name: `r.SceneColorFormat` and `r.TranslucencyLightingVolume` **individually** (the Effects **group** is refuted, those two cvars are not), Shadows = **Medium**, the shipped **menu path**, a **packaged** build, **frame time**, and the **material-level why**. *He closes ask (A) on `TASK-1159`, not from this file.*

### 1.4 `SiegeGraphicsMenuWidget.cpp:252-258` → strike; replacement `:259-277` — board cl. (2)

Found **by its opening words** `"What IS established"` (its old `:196-202` pin had already drifted twice today). **OLD (struck):** `⭐ What IS established … BP_SiegeFog, a raymarched TRANSLUCENT MESH and NOT a froxel participant …, so r.VolumetricFog — the one cvar the Shadows group owns here — does not govern it …`

**NEW:** struck **even though no gate named it**, because `TASK-1161` rewrote the block above to *agree* with it — repairing only the named half would leave the block self-contradictory again, the very defect `TASK-1161` existed to remove. Separates **what is still true** (`TASK-1151`'s node census stands as a *description of the asset*) from **what was false** (*the inference* — being outside the froxel grid does not mean `r.VolumetricFog` leaves it alone). Restates the establishment: **one cvar governs both fogs, so one floor covers both halves of the Shadows row.** Carries the *mechanism-isolated-to-the-cvar* caveat.

### 1.5 `SiegeGraphicsMenuWidget.cpp:280-289` → strike; replacement `:290-318` — board cl. (4), the hint-string coupling

Falsifier **(1)** of the string said *"NO FRAME OF THIS GAME HAS EVER BEEN CAPTURED AT SHADOWS=LOW WITH A FOG CARD UP … if the capture shows the wash dying, THIS SENTENCE IS FALSE."* **That falsifier FIRED.** Left standing it would have been a third false sentence in the same block. Struck and replaced with the coupling the board specified — see § 3.

### 1.6 `FogVolume.cpp:813-816` → strike; replacement `:817-836` — **the second host `TASK-1160` §4 named**

**OLD (struck):** `⇒ ⚖️ THIS FLOOR IS CORRECT FOR THE AMBIENT FOG AND DOES NOT REACH THE CARD'S FOG. It is not the fix for 🧑 his report, and it must never be described as one …` — this is the sentence `TASK-1160` §4 quotes **by name** as the record it refutes.

**NEW:** the measurement, the 2×2, ⇒ *"THIS FLOOR REACHES BOTH FOGS AND IT IS THE FIX FOR 🧑 HIS REPORT"*; the honest bound (*mechanism isolated to the cvar; coupling not read at the material*), with the pre-existing measured line *"neither material sets `bUsedWithVolumetricFog`"* explicitly demoted to **interesting rather than dispositive**; and *what does not change* — ask (A) still does not close.

### 1.7 `FogVolume.cpp:843-845` → strike; replacement `:846-852`

**OLD (struck):** `⇒ NO MEASURED ROUTE by which a graphics setting deletes the card's fog. That makes the EXISTENCE of the reported exploit an OPEN QUESTION rather than a settled fact …`
**NEW:** the decisive measurement **was taken**; the scan above is **not withdrawn and was not wrong**; the fourth-kind lesson.

### 1.8 `FogVolume.h:745-749` → strike; replacement `:750-762`

**OLD (struck):** `⇒ ⚖️ ***r.VolumetricFog GOVERNS THE AMBIENT FOG. IT DOES NOT GOVERN THE CARD'S FOG.*** ⇒ THIS FLOOR IS CORRECT FOR WHAT IT CLAIMS AND INSUFFICIENT FOR WHAT HE ASKED …`
**NEW:** `r.VolumetricFog` governs **both** fogs ⇒ the floor **reaches the card's fog and is the fix**; cvar/material caveat; ask (A) does not close; and the reversal law written where the mistake was made.
⛔ Line `:744`–`:745`'s first half (*"is a RAYMARCHED TRANSLUCENT MESH … NOT a froxel participant."*) is **left standing on purpose** — it is a description of the asset and is still true. Only the **inference** is struck.

### 1.9 `FogVolume.h:770-773` → strike; replacement `:774-780`

**OLD (struck):** `⇒ NO MEASURED ROUTE … an OPEN QUESTION … Nobody has yet rendered BP_SiegeFog at Shadows=Low or Effects=Low …`
**NEW:** somebody has now rendered it at Shadows=Low; the scan stands; ⚠️ **Effects=Low is still untested at the cvar level** (`r.SceneColorFormat` / `r.TranslucencyLightingVolume` individually) ⇒ sentences about *that* stay narrow.

### 1.10 `FogVolume.h:936-940` → strike; replacement `:941-953` — 🚨 **the most consequential strike, and it was not on the board's list**

This is the doc comment on **`bEnableFogRenderFloor`**, the switch a profiling run can flip. **OLD (struck):** `this switch does NOT govern the Fog card's own visual … turning this off does NOT restore 🧑 the exploit he reported, and turning it on does not by itself close it.`
**NEW:** exactly backwards ⇒ **setting it `false` DOES restore the demonstrated exploit**; ⇒ *"THIS SWITCH IS A PROFILING TOOL, NEVER A SHIPPING CHOICE; turning it off SHIPS THE EXPLOIT"*; and it carries the **user-facing string** with it (see § 3).
⚠️ **QA should scrutinise this one first** — it is the strike most likely to change someone's behaviour, and I added it from the census rather than from the row text.

### 1.11 `FogVolume.h:1155-1158` → strike; replacement `:1159-1167`

**OLD (struck):** `SCOPE, NARROW AND DELIBERATE … It does NOT reach BP_SiegeFog … Do not widen this sentence without a measurement behind it.`
**NEW:** *"WIDENED … AND ONLY BECAUSE THE MEASUREMENT THE STRUCK LINE DEMANDED ARRIVED"* — the struck line set a condition and the condition was met, which is the honest way to widen it. Scope is **both fogs**; still narrow on the material-level why and on ask (A).

---

## 2. 🚨 CLAUSE (3) — THE CENSUS. **Count: 15 sites. 9 struck. 4 left standing with reasons. 2 are new text of mine.** ⛔ **It is NOT zero.**

Searched **all of `Source/**`** (`*.cpp`, `*.h`, `*.cs`, `*.py`) in every spelling the board named plus four more: `does not reach` · `not a froxel` / `NOT a froxel` · `outside the froxel` / `OUTSIDE the froxel` · `froxel participant` · `raymarched` · `UNEXPLAINED` / `unexplained` · `not confirmed` / `NOT confirmed` · `not refuted` / `NOT refuted` · `NOT CLOSED` / `not closed` · `CORRECT FOR THE AMBIENT` · `DOES NOT REACH THE CARD`.

**`FogVolume.{h,cpp}` was named as a likely second host and the census CONFIRMED it** — `FogVolume.cpp:813` held the sentence `TASK-1160` §4 quotes verbatim, and `FogVolume.h` held **three** more, one of them on the `bEnableFogRenderFloor` switch (§ 1.10). ⇒ **fixing only `SiegeGraphicsMenuWidget.cpp` would have left four live copies of the refuted claim, in the file the floor itself lives in.**

**Struck (9):** `SiegeGraphicsMenuWidget.cpp` ×4 (§ 1.1, 1.3, 1.4, 1.5) · `FogVolume.cpp` ×2 (§ 1.6, 1.7) · `FogVolume.h` ×3 (§ 1.8, 1.9, 1.10, 1.11 — 1.8/1.9 sit in one block, counted as two strikes; four `FogVolume.h` strikes total).

**Left standing, and WHY — ⛔ all four are COMPILED BYTES, and the row's own fence (cl. 5) forbids me to touch one:**

| # | site | text | why untouched |
|---|---|---|---|
| 1 | `FogVolume.cpp:969` | `UE_LOG` (floor-could-not-engage): *"This does NOT govern the Fog card's own visual (BP_SiegeFog is a raymarched mesh outside the froxel grid, TASK-1151) — do not read this line as 'the fog is gone'."* | **string literal ⇒ compiled byte.** Also read as text by `SiegeFogVisualTest`'s literal rows; editing it is a suite risk, not a comment edit. **NOW MISLEADING: it tells an operator the failed floor does not affect the card's fog. It does.** |
| 2 | `FogVolume.cpp:1071` | `UE_LOG` (floor-did-not-take): *"the Fog card's own visual is BP_SiegeFog, a raymarched mesh this floor does not reach (TASK-1151)"* | same — **string literal**, and this is the **worst** of the four: it fires exactly when the floor failed, and tells the reader the card's fog is fine when it is not. |
| 3 | `FogVolume.cpp:1109` | `UE_LOG` (floor-engaged): *"SCOPE: this is the AMBIENT volumetric fog. … this line says NOTHING about it (TASK-1151)."* | same — **string literal**. Understates what the floor achieved. |
| 4 | `Tests/SiegeGraphicsMenuTest.cpp:1004` | `TestFalse` message: *"TASK-1151 measured that BP_SiegeFog is a raymarched TRANSLUCENT MESH, not a froxel participant, so what Effects/Textures do to it is a DIFFERENT and UNMEASURED question."* | **string literal in a test** ⇒ compiled byte. ⚠️ Its *assertion* is still correct (the Effects/Textures question **is** still unmeasured at the cvar level); only its **premise clause** is now the refuted one. |

⇒ 📌 **FOR THE MANAGER: these four need a row of their own** — they are the same defect in a place a comment-only row may not reach, and #2 is an operator-facing lie on the failure path. They are **compiled** text, so that row needs a real gate and a suite run, not this row's waiver.

**Also examined and deliberately NOT touched (not the refuted claim):** `Building.cpp:398`, `MinerUnit.h:750`, `SiegeCombatStatics.cpp:146`, `SiegeCombatStatics.h:355`, `SiegeAssistantVocabulary.cpp:164`, `SiegeAssistantZoneATest.cpp:457/:868`, `SiegeInvisibilityTest.cpp:1980`, `SiegeClimbableTowerTest.cpp:1515`, `SiegeHeroLadderClimbTest.cpp:267`, `SiegePlacementTest.cpp:2945`, `SiegeAssistantSelectionTest.cpp:5312`, `SiegeFogVisualTest.cpp:845`, `SiegeGraphicsSettingsSubsystem.h:551` — all keyword collisions on unrelated subjects (navmesh reach, an unexplained omission, an unconfirmed video mode).

---

## 3. ⚖️ CLAUSE (4) — THE HINT STRING: BYTE-IDENTICAL, AND THE COUPLING IS NOW IN THE FILE

**The string is untouched. Proven, not asserted:** `md5` of the `ShadowHintFogOn` / `ShadowHintFogOff` declarations **plus their string lines**, pre vs post = `7117476313b920c8511f51b6551d1fdc` **both**. They now sit at `:327` / `:330` (were `:224` / `:227`).

The coupling and its consequence are written **beside the string**, at `SiegeGraphicsMenuWidget.cpp:290-318`, in the board's own terms:

> **THE STRING IS TRUE POST-FLOOR, AND TRUE ONLY BECAUSE `EnforceFogRenderFloor()` HOLDS `r.VolumetricFog` UP.** `TASK-1160`'s row D is the hand-simulation … ⇒ 🚨 **IF `EnforceFogRenderFloor()` IS EVER REVERTED, DISABLED (`bEnableFogRenderFloor`) OR OUT-RANKED (a console `r.VolumetricFog 0` beats `ECVF_SetByCode`), THIS USER-FACING STRING BECOMES A LIE TO THE PLAYER** — and that is MEASURED, not feared … ⛔ **SO THE STRING AND THE FLOOR SHIP TOGETHER AND REVERT TOGETHER.**

It also records that **`TASK-1161` §5's ruling stands but its reason (3) is refuted** (*"because BP_SiegeFog is NOT a froxel participant, NOT because the floor protects it"* — exactly backwards), quotes the manager's law, and states that **cl. (4) is a ruling, not an edit**. The same consequence is mirrored at the switch that can cause it, `FogVolume.h:941-953` (§ 1.10) — the two places a reader can arrive from.

⭐ One more thing recorded there, because it flips twice: the struck line *"TAKING THAT CAPTURE REQUIRES DEFEATING THE FLOOR ON PURPOSE"* was **false for `TASK-1160`** (pre-floor binary — DLL built `09-08 01:32:13`, `FogVolume.cpp` written 12 h 43 m later, zero Live Coding patches, and `sg.ShadowQuality 0` **did** drive `r.VolumetricFog` to `0`, which a floored binary would have refused) and is **true from now on** — any re-capture after `TASK-1149` ships needs the deliberate defeat.

---

## 4. 🚨 ZERO EXECUTABLE LINES — PROVEN, NOT ASSERTED

All three files were **already dirty** at session start (`TASK-1147` + `TASK-1161` uncommitted), so `git diff HEAD` cannot isolate me. I snapshotted all three **before touching them** and diffed pre → post.

**(a) Line census against `CountOccurrencesInCode`'s OWN comment test**, character for character (`SiegeAcquisitionFunnelTest.cpp:124-129` — trimmed line starts with `//`, `* `, `*/`, `/*`, or is a bare `*`):

| file | added | removed | added/removed lines that are **not** comment lines |
|---|---|---|---|
| `SiegeGraphicsMenuWidget.cpp` | +111 | −8 | **0** |
| `FogVolume.cpp` | +30 | −4 | **0** |
| `FogVolume.h` | +51 | −8 | **0** |

⇒ every line I touched is skipped by the wiring tests that read these files as text. (`//~` starts with `//`, so the header's Doxygen-suppressed comments qualify.)

**(b) The stronger check — comment-stripped byte identity.** Strip every full-line comment from pre and post and compare the remainder:

```
SiegeGraphicsMenuWidget.cpp : IDENTICAL   (2394 non-comment lines)
FogVolume.cpp               : IDENTICAL   ( 624 non-comment lines)
FogVolume.h                 : IDENTICAL   ( 148 non-comment lines)
```

Not "no statement changed" — **the non-comment content of all three files is byte-for-byte what I was handed.** No line mixes code and comment; no line was modified in place.

**(c) Delimiter and brace balance** (a comment edit's real failure mode is commenting code out): `/*` and `*/` unchanged in all three (`35→35`/`35→35`, `1→1`/`1→1`, `54→54`/`54→54`); `{` / `}` unchanged (`359`, `72`, `5` each side). **Nothing I wrote contains `/*`, `*/`, `{` or `}`** — the `**bold**` and `***bold***` runs I used are `*` sequences only, and the balance counts confirm none of them formed a delimiter.

**(d) The two hint strings:** `md5` identical pre vs post — § 3.

**(e) Why the suite delta is `0`, and why that is a DERIVATION:** the only tests reading these files as text are `SiegeFogVisualTest`'s wiring rows via `CountOccurrencesInCode` (comment-stripping, proven in (a)) and `ExtractFunctionBody`/`SubstringBefore` over `EndPlay` / `RefreshFogVisual` / `EnforceFogRenderFloor` — **none of my edits is inside a function body and none adds a brace**. `SiegeGraphicsMenuTest`'s fog rows assert on the **composed runtime strings**, which are byte-identical. ⛔ **I did not compile and did not run the suite. `TASK-1149`'s compile and run are the check** — and per the waiver's ground (c), that compile is what proves a comment edit did not comment out code.

---

## 5. FENCES

| fence | result |
|---|---|
| `Content/**` · `Config/**` · `.uasset` | **zero** writes |
| `L_Arena` | never opened |
| Editor / MCP | **never touched** — an art-director holds it for `TASK-1152`/`TASK-1154` |
| compile · suite run · Git · push | **none** (`TASK-1149` is the host) |
| user-facing strings | **byte-identical**, proven (§ 3) |
| `CONVENTIONS.md` | **not edited** — findings routed to the manager (`SC-§82`), § 2 and § 1.2 |
| `TASKBOARD.md` | **`TASK-1162`'s single `- status:` line only**, via `Edit` |

**Files touched:** `Source/GitClaudeUnrealTest/Siegebound/SiegeGraphicsMenuWidget.cpp` (comments only) · `Source/GitClaudeUnrealTest/Siegebound/FogVolume.cpp` (comments only) · `Source/GitClaudeUnrealTest/Siegebound/FogVolume.h` (comments only) · this handoff · the board status line.

---

## 6. WHAT QA / `TASK-1149` SHOULD SCRUTINISE

1. **§ 4 (b)** is the load-bearing claim and the waiver rests on it. `git show HEAD:` **will not** re-derive it (the files were already dirty) — the pre-edit content is `TASK-1147`+`TASK-1161`'s working tree. The practical check is `TASK-1149`'s clean compile plus `552 / 0`.
2. **§ 1.10** — the `bEnableFogRenderFloor` strike. Not named by the board; I added it from the census because it is attached to a switch someone might flip, and the struck text told them flipping it was harmless. Confirm you agree it belongs in a comment-only row.
3. **§ 1.2** — clause (b) **flagged, not rewritten.** The single judgement call. If the manager rules it should have been struck, that is a one-edit follow-up.
4. **§ 2's four untouched string literals** — they are compiled bytes and out of this row's scope, but `FogVolume.cpp:1071` is an **operator-facing false statement on the floor's failure path** and wants a real row.
5. **Nothing I wrote claims ask (A) is closed**, and **nothing asserts `bUsedWithVolumetricFog`** — both were checked by re-reading every inserted paragraph after the final edit. The phrase written everywhere instead is *"mechanism isolated to the cvar; coupling not yet read at the material."*
