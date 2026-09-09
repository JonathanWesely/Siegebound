# TASK-1172 — [FOGGREY-GENERALDATA] — art-director handoff

**Ask (C) is CLOSED on pixels. The fog is grey.** Band met with margin, on the route Jonathan's game
actually uses, replicated twice, with the acquisition control passed before any number was counted.

⛔ **Nothing committed.** `TASK-1175` hosts. ⛔ No Git, no compile, no push, `L_Arena` never saved.

---

## 1. 🚨 cl. (1) FIRST — THE MANAGER'S HYPOTHESIS IS **REFUTED**

> The hypothesis (`FOG-§12.4b`, flagged `SC-§101`, *not measured*): *"the vendor pushes the colours **at
> construction**, so a **post**-construction write reads back perfectly and changes nothing."*

**Prediction: the MID stays unchanged after a post-construction `generalData` write. It did not.**

| # | arm | write | MID `Emissive Color` after | predicted |
|---|---|---|---|---|
| 1 | **editor world**, actor already constructed | `generalData.emissiveColor = (0,0,1)` | **`(0,0,1)`** — followed **immediately** | unchanged |
| 2 | **Simulate world** (⛔ a construction rerun is *impossible* here) | `= (1,0,0)` | **`(1,0,0)`** — followed | — |

Arm 2 is the load-bearing one: `RerunConstructionScripts` is editor-only, so in Simulate **no
re-construction can occur** — and the MID followed anyway. The push is **not** construction-gated.

🚨 **AND THE PIXELS AGREE — which is the answer that actually matters** (`SC-§94`: verify the *consumer*).
Driven in Simulate, at the pinned vantage, captured:

| driven state | band sRGB | B/G | sat |
|---|---|---|---|
| `emissive = (1,0,0)` | (214.4, 176.7, 153.0) | 0.8656 | 28.67 % |
| `emissive = (0,0,1)` | (191.7, 180.2, **205.0**) | **1.1376** | 12.10 % |
| `baseColor = (1,0,0)` | (202.9, **80.6**, 55.7) | 0.6908 | **72.56 %** |
| shipped baseline | (184.7, 175.4, 148.4) | 0.8461 | 19.64 % |

⇒ ⛔ **The channel is ALIVE at every layer — struct → MID → rendered image.** The dispatch's stop
condition (*"if the write does not reach the rendered MID, stop"*) is **not** met, so I tuned. I did
**not** adopt the unmeasured mechanism; I measured it and it is false.

### 1a. ⚠️ A SECOND, BIGGER REFUTATION THAT THE MANAGER MUST SEE

`TASK-1154` §3 is the row that killed answer #1. Its decisive test was *"set `base Color` to pure red"*,
and it reported **no change at all** — B/G `0.890` vs baseline `0.891`, *"renders EXACTLY the same beige"*.
`FOG-§12.4b`'s table records that as **REFUTED on pixels**.

**I ran the identical test. It is a violent change:** `base Color = (1,0,0)` ⇒ band **(202.9, 80.6, 55.7)**,
B/G **0.6908**, saturation **72.56 %**, against a baseline of (184.7, 175.4, 148.4) / 0.8461 / 19.64 %.
Single variable: `ACQ-03` and `ACQ-04` differ **only** in `base Color`. Frame is promoted for his eye.

⇒ ⚖️ **`general Data.base Color` was NEVER inert. Answer #1 was RIGHT, and the row that refuted it had a
broken instrument.** `TASK-1154` → `TASK-1165` → `TASK-1167` — three rows and a full compile/QA/556-suite
/13-mutation gate — were spent chasing `BoxMaterials` because of **one false null result**.

⛔ **WHY it was null, I do NOT know, and I will not guess** (this row is under an explicit ban on adopting
unmeasured mechanisms). **Lead, not finding:** the most economical explanation is that the write landed on
an actor in a *different world* from the one being rendered (editor actor vs the Simulate actor that owns
the visible MID) — the two are separate objects with separate MIDs, and I had to address them by distinct
paths (`/Game/Maps/L_Arena…` vs `/Game/Maps/UEDPIE_0_L_Arena…`) throughout this row. **Routed to the
manager; `FOG-§12.4b`'s row-1 verdict needs re-adjudicating, not by me.**

⭐ **Transferable, and offered as a candidate law:** `SC-§112` names **two** instruments in series — the
**acquirer** and the **analyser**. This row found the **third, and it is upstream of both: the DRIVER.**
*Did the change reach the object that is actually being rendered?* A broken driver produces a perfectly
acquired, perfectly analysed, perfectly confident **null** — and a null reads as *"lever refuted"*, which
is the one conclusion nobody re-tests.

---

## 2. ⭐ THE ACQUISITION CONTROL (`SC-§112`) — AND HOW I PROVED IT

### 2a. cl. 3(a) — every frame carries its own scalars, before any B/G is trusted
`ACQ_max`, whole-frame `mean`, and **distinct-colour count** are computed on **every** capture, and a
frame failing `max > 0 and ncolours > 16` is **refused, never scored** (the sweep raises rather than
returns). Measured across all 20 captures: `max` **237–255**, distinct colours **21,288 – 339,780**.
Camera echoed back in every result — `(-20000, 0, 176)`, pitch `-11.499999999999996`, yaw `0`, FOV `90`,
2764×828 — a second independent coordinate on every frame (cl. 3(c)).

### 2b. cl. 3(b) — WHAT THE NULL FRAME WOULD HAVE SCORED, **measured, not asserted**
I synthesised the exact null `TASK-1167`'s GDI acquirer produced (all-black, 2764×828) and ran it through
**this row's** metric:

```
band mean RGB = (0.0, 0.0, 0.0)     max = 0
sat           = 0.0 %      <-- INSIDE the pass band (sat <= 8.0). The best possible score.
B/G           = nan
```

🚨 **Say it plainly: half of my pass condition is satisfied by a black frame.** The saturation test alone
would have reported the best result this row could produce. It is only the **`B/G ≥ 0.945`** half that
refuses it, and only because `nan >= 0.945` is `False` — i.e. the metric is saved by an accident of IEEE
semantics, not by design. **The `ACQ_live` gate is what actually protects this row**, and it runs before
scoring.

### 2c. cl. 3(c) — which instrument shipped, and the stronger check I added
GDI window capture was already discarded by `TASK-1167`. I used **MCP `CaptureViewport`**, pulled over a
direct streamable-HTTP client so the PNGs land on **disk** and never enter the agent's context.

⭐ **Beyond a single non-blank frame, I proved the acquirer tracks LIVE STATE:** I drove the fog to four
*mutually distinguishable, independently predicted* states (red / blue / red-base / shipped) and the
captures moved **in the predicted direction each time** (table in §1). A frozen, cached or stale frame
would have returned the same numbers four times. It did not.

⚠️ **One honest limit:** the acquirer proved live is `CaptureViewport`; it was never shown a *known*
reference image, only a *known-different* scene. That is why §3 exists.

---

## 3. ⭐ CALIBRATION — the analyser, against five published numbers
`SC-§112` cl. 2: the analyser control runs on **archived files**, the acquisition control on **live
captures**. Two ledgers, never one sentence. My analyser reproduces every published figure of this lane
**to the last digit**:

| archived frame | published | my probe |
|---|---|---|
| `VID-007-t00m27s` — 🧑 **his own beige** | 0.891 / 15.5 | **0.8907 / 15.50** |
| `TASK-1167-A` no fog | 0.548 / 45.2 | **0.5483 / 45.17** |
| `TASK-1167-B` shipped | 0.845 / 20.2 | **0.8453 / 20.17** |
| `TASK-1167-C` grey material direct | 0.947 / 7.0 | **0.9472 / 7.00** |
| `TASK-1167-D` generalData | 0.911 / 10.7 | **0.9107 / 10.67** |

⭐ **And the probe reads Jonathan's beige AS BEIGE** (0.891 / 15.5) before it was ever asked to read
anything as grey — it does not answer *grey* to everything.
⭐ **The live rig agrees with the archive too:** my own Simulate capture of the *shipped* fog reads
**0.8461 / 19.64** against `TASK-1167` lane B's **0.845 / 20.2** — same scene, same instrument, different
session. And driving the colours back to `20c1bea`'s values in-session reproduces **0.8940 / 15.84**,
i.e. 🧑 **his recorded beige (0.891 / 15.5) to within 0.003**.

---

## 4. 🚨 THE VALUES I LANDED ON, AND THE ITERATIONS BEHIND THEM

### ⭐ SHIPPED PAIR — `/Game/Blueprints/BP_SiegeFog` → Class Defaults
```
general Data.base Color      = (0.86, 1.00, 1.00)     was (1.00, 1.00, 1.00)
general Data.emissive Color  = (0.05, 0.07, 0.325)    was (0.05, 0.05, 0.05)
```
⛔ **Both, as the law requires:** `base Color` **multiplies** (cuts R, cannot add blue); `emissive Color`
**adds** (the only way to add blue, cannot subtract).

⛔ **This is NOT `MI_SiegeFog_Grey`'s pair transcribed.** That pair — `(0.88,1,1)` / `(0.05,0.08,0.26)` —
was measured **in my own rig** at `B/G 0.9380`, ⛔ **short of the `0.945` floor** (`B2-P3`), corroborating
the dispatch's warning independently.

### The iterations — 14 measured points, 3 batches
**Batch 1** (mechanism + acquisition control, §1). **Baseline established: 0.8461 / 19.64.**

**Batch 2** — per-lever sensitivity + the two published candidates + two extrapolations:

| point | base | emissive | B/G | sat | |
|---|---|---|---|---|---|
| P1 | (0.88,1,1) | (0.05,0.05,0.05) | 0.9044 | 11.87 | `base Color` alone: ΔB/G **+0.058** |
| P2 | (1,1,1) | (0.05,0.05,0.26) | 0.9176 | 12.45 | `emissive` alone: ΔB/G **+0.072** |
| P3 | (0.88,1,1) | (0.05,0.08,0.26) | 0.9380 | 7.82 | ⛔ `MI_SiegeFog_Grey`'s pair — **misses B/G** |
| P4 | (0.86,1,1) | (0.05,0.05,0.33) | 0.9925 | 2.77 | ✅ band, but ~dead neutral |
| P5 | (0.85,1,1) | (0.05,0.065,0.36) | 0.9869 | 2.63 | ✅ band, ~dead neutral |
| P6 | (0.82,1,1) | (0.05,0.075,0.44) | 0.9643 | 3.76 | ✅ band, costs 5 % luma |

⇒ Both levers needed: neither alone reaches the band from 0.8461.

**Batch 3** — converge on the *ideal* (0.963 / 5.9) rather than settle for dead-neutral, **plus a repeat
of one setting to measure my own noise**:

| point | base | emissive | B/G | sat | |
|---|---|---|---|---|---|
| C1 | (0.865,1,1) | (0.05,0.072,0.31) | 0.9823 | 3.34 | ✅ |
| C2 | (0.870,1,1) | (0.05,0.075,0.295) | 0.9267 | 8.38 | ⛔ fails both |
| **C3** | **(0.860,1,1)** | **(0.05,0.070,0.325)** | **0.9564** | **5.77** | ✅ **closest to ideal on both axes** |
| C1-repeat | (0.865,1,1) | (0.05,0.072,0.31) | 0.9818 | 3.42 | **repeatability** |

⭐ **Noise, measured not assumed:** C1 vs C1-repeat = **ΔB/G 0.0005, Δsat 0.08** ⇒ the differences between
candidates are real, ~50× the noise floor. **C3 selected.**

### ⚠️ THE TUNING RIG AND THE SHIP PATH DISAGREE — and I report the SHIP PATH
C3 measured **0.9564 / 5.77** while being driven **in-place** in a running Simulate (1.2 s settle). The same
pair, arriving by **fresh construction from the saved Class Default with the specified 8 s warmup**, reads
**0.9772 / 3.90**. That gap (+0.021 B/G) is ~5× my noise floor, so it is **real, not scatter**.

⛔ **I do not have a measured mechanism for it** and will not invent one; the specified vantage mandates
*"Simulate-In-Editor, 8 s warmup"*, and only the ship path satisfies that. **Candidate cause (lead, not
finding): exposure adaptation had not settled in the in-place rig.** ⇒ **The rig was sound for RANKING;
the ship path is the number that ships.** This is `SC-§94` recurring one more time inside my own row: the
instrument I tuned with is not the instrument the player uses.

---

## 5. ✅ ACCEPTANCE — ALL FOUR LANES

| lane | required | **measured** | |
|---|---|---|---|
| **TARGET BAND** | `B/G ≥ 0.945` **AND** `sat ≤ 8.0 %` | **B/G 0.9772 · sat 3.90 %** | ✅ **PASS** |
| ideal | 0.963 / 5.9 | 0.9772 / 3.90 | ⚠️ passes; **more neutral than ideal** (§7) |
| **positive control** — 🧑 his beige | ≈ 0.891 / 15.5 | **0.8907 / 15.50** (archive) · **0.8940 / 15.84** (live in-session) | ✅ |
| **negative control** — no fog | ≈ 0.548 / 45.2 | **0.5764 / 42.36** (live) · **0.5483 / 45.17** (archive) | ✅ |
| **acquisition control** — `SC-§112` | non-blank, scalar reported | max **237–255**, ncolours **21k–340k**, 4-state tracking | ✅ §2 |
| today's shipped fog (start point) | 0.845 / 20.2 | **0.8461 / 19.64** | ✅ reproduced |

**Ship-path detail — 3 frames per run, TWO independent runs (fresh spawn each, 8 s warmup each):**

| run | frame 1 | frame 2 | frame 3 | mean | spread |
|---|---|---|---|---|---|
| A | 0.9746 / 4.14 | 0.9777 / 3.86 | 0.9790 / 3.75 | **0.9771 / 3.92** | 0.0044 / 0.39 |
| B | 0.9746 / 4.11 | 0.9777 / 3.87 | 0.9793 / 3.73 | **0.9772 / 3.90** | 0.0047 / 0.38 |

⇒ ⭐ **Replicated across two independent Simulate sessions to `ΔB/G 0.0001`.**

**Vantage, character for character, never moved:** band `y ∈ [0.42,0.48]`, `x ∈ [0.30,0.70]` · camera
`(-20000, 0, 176)` · pitch `-11.5°` · yaw `0` · Simulate-In-Editor · 8 s warmup · FOV 90 · 2764×828.
Camera echoed back by the engine on **every** frame.

**The ship path is proven end to end** (`SC-§94` — verify the consumer):
saved `.uasset` on disk → CDO reads `(0.86,1,1)` / `(0.05,0.07,0.325)` → **fresh spawn** inherits both →
its **MID** carries both → **pixels** land in the band. No step assumed.

---

## 6. ⛔ ASK (B) NON-REGRESSION — QUOTED FROM THE ASSET, BEFORE AND AFTER

| field | BEFORE (read from asset) | AFTER (read from **saved** CDO) | |
|---|---|---|---|
| `general Data.density` | **0.5** | **0.5** | ✅ |
| `noise Data.sharpness` | **0.1** | **0.1** | ✅ |
| `general Data.wind Speed` | **0.5** | **0.5** | ✅ |

Untouched and restated at their shipped values in every write. ⛔ No other field on `BP_SiegeFog` moved
(`wInd World Space` `true`, `mask Margin` `3`, `material Mode` `Dynamic`, `mode` `Base`,
`boxMaterials.Base` still the **vendor** `MI_FogArea_Box` — ⛔ I did **not** repoint it).

### ⚠️ NIT-8 SETTLED **BY MEASUREMENT** — and it was never a disagreement
Two documents say `0.005` and `0.5`. **Both are right, about different layers:**

```
CDO general Data.wind Speed  = 0.5      <-- the Blueprint field.  THIS is ask (B)'s number.
MID scalar  "Wind Speed"     = 0.004999999888241291   ( = 0.5 x 0.01 )
MID scalar  "Density"        = 0.5                    ( 1:1 )
MID scalar  "Base Noise Sharpness" = 0.10000000149    ( 1:1 )
```
⇒ The vendor applies a **×0.01 conversion to `Wind Speed` only**. `TASK-1154` §4's table already carried
the annotation *"0.5 (×0.01)"*. **This is a units/layer conflation, not a contradiction — no asset needs
correcting.** Routed to the manager under `SC-§82`; ⛔ I did **not** "correct" the asset to match a document.

---

## 7. ⚠️ WHERE I LANDED VS THE IDEAL — stated, not buried

The band is met with margin. **But `sat 3.90 %` is more neutral than the ideal `5.9 %`**, i.e. slightly
*greyer* than `TASK-1154`'s deliberate aesthetic ( *"he asked for **more of a grey**, not for grey… a fog
with no hue at all reads as a flat slab"* — it declined dead-neutral `t1` on purpose).

- ⚖️ It errs in the direction 🧑 **he asked for** — *"more of a grey instead of a yellow"* — so I did not
  spend a fourth batch chasing 2 points of saturation (cl. 5: **do not tune past diminishing returns**).
- 🧑 **A warmer notch is measured and available for `TASK-1159`, no re-derivation needed:** driving this
  same pair **in place** (rather than by fresh construction) reads **0.9537 / 5.95** — essentially the
  ideal — and the lever direction is: **raise `base Color.r` toward 0.87 and lower `emissive.b` toward
  0.30** to add warmth back; `C2 = (0.870,1,1)/(0.05,0.075,0.295)` is the **measured overshoot** at
  0.9267 / 8.38 (outside the band). The usable warm edge sits between C3 and C2.

---

## 8. ⭐ cl. (5) — THE RESIDUAL QUESTION, AND `TASK-1165`'s FATE

> *Is the residual traceable to a parameter the vendor does not overwrite?*

**The question does not arise: THERE IS NO RESIDUAL. The band was reached on the `generalData` channel
alone**, with margin (0.9772 vs 0.945 floor), using **only** the two fields `FOG-§12.4b` names, and with
**zero** reliance on `BoxMaterials`, on `MI_SiegeFog_Grey` being in the render path, or on any code change.

⇒ ⚖️ **By `TASK-1165`'s own mechanical decision rule, branch (a) fires: `TASK-1165` closes
`superseded — reverted unshipped`.** Its three `Source/**` files are still dirty in the tree exactly as its
author left them; ⛔ **I did not touch them** and the restore is not mine to perform. The `0.947` vs `0.911`
gap `FOG-§12.4b` called *"unexplained"* is now moot — lane C's ceiling was never the limit; the tune simply
had to be **stronger** than `MI_SiegeFog_Grey`'s pair, which is what the extra `emissive.b` (0.26 → 0.325)
and lower `base.r` (0.88 → 0.86) buy.

---

## 9. cl. (6) — `MI_SiegeFog_Grey` UPDATED TO MATCH

A value reference that disagrees with the shipped value is worse than no reference.

| | before (read from asset) | after |
|---|---|---|
| `Base Color` | (0.88, 1, 1) | **(0.86, 1, 1)** |
| `Emissive Color` | (0.05, 0.08, 0.26) | **(0.05, 0.07, 0.325)** |

⛔ It stays **wired to nothing** — I referenced it from nowhere, and `boxMaterials.Base` still points at the
vendor material. Its `FOG-§12.4a` pin (*value reference only — unreachable by design*) is unchanged.

---

## 10. HASHES (`SC-§108`) — I am the last writer, so I re-derive

⚠️ **The prior row's hash is superseded, and I name it:** `handoffs/TASK-1152-artist.md` §4 declared
`dbea1466…` / 38,335 B; `SC-§108` records that **disk already carried `6cf85fa9…f342d5` / 38,225 B** because
a second row saved the same asset. **I read `BP_SiegeFog` from the asset, not from any handoff**, and
confirmed the on-disk pre-state below.

| asset | BEFORE | AFTER |
|---|---|---|
| **`Content/Blueprints/BP_SiegeFog.uasset`** | `6cf85fa9bb25131d387e4335e6440e9743ae0612ba7962bda1ef39b4f7f342d5` · **38,225 B** | **`411a8bc0859b99175cfe0e06e00fce4d50275b3f9abe60106423a273c1d19271`** · **38,595 B** |
| `Content/Materials/Instances/MI_SiegeFog_Grey.uasset` | `0b4e19565249d70eb937146d6fcbda390f402da950d5c2828e4f1fd56ab73b86` · 7,288 B | **`554154c4c5564d0f494e2551b9135ccb615ce75a6d49b3f931503ebe1df33709`** · **7,288 B** |
| `Content/Maps/L_Arena.umap` | `1f78419d…0af15622` · 605,098 B | **`1f78419d…0af15622` — IDENTICAL, never saved** |

🚨 **`MI_SiegeFog_Grey` is BYTE-IDENTICAL IN SIZE (7,288 B) BEFORE AND AFTER, AND ITS CONTENT CHANGED.**
Both edited assets are **LFS-tracked** (`git check-attr filter` ⇒ `lfs` on both). ⇒ ⛔ **`TASK-1175` MUST
verify by oid-vs-`sha256`, NEVER by size** (`SC-§68`) — this asset is a live example of why. That LFS check
was recorded **owed** by `TASK-1167` §7 and is inherited by the commit host.

---

## 11. FENCES

| Fence | Result |
|---|---|
| `Content/Maps/L_Arena.umap` | ✅ sha256 **identical** at start, after the save, and at the end. **Never saved.** Map left dirty in memory only; ⛔ discarded. |
| Probe actors | 5 spawned across the row (`BP_SiegeFog_C_0..C_4`), **all removed**; `find_actors` returns **none**. |
| `save_assets` | ⛔ **never** called with `[]`. Only `["/Game/Blueprints/BP_SiegeFog"]` and `["/Game/Materials/Instances/MI_SiegeFog_Grey"]`, explicitly. |
| `Content/FogArea/**` (vendor) | ⛔ untouched — `git status` clean. Read only, through reflection on spawned instances. |
| `Source/**` | ⛔ **ZERO**. `TASK-1165`'s held diff is byte-untouched by me. |
| `Config/**` · `Tools/**` | ⛔ untouched — `git status` clean on both. |
| `Content/` dirty set | exactly **two** files, both intended (§10). |
| Git index | ✅ **empty** — the UE Git plugin auto-staged nothing. |
| Commit / push | ⛔ **never.** `TASK-1175` hosts. |
| `TASKBOARD.md` | **mine = ONE `- status:` line**, `Edit` tool only. Verified: my text appears **once**, the old `TASK-1172` status text is **gone**. ⛔ The file arrived **already dirty** (the manager's boarding of `1172`–`1175`); ⛔ that 145-line block is **not mine**. |
| `CONVENTIONS.md` | ⛔ **not mine** — arrived dirty, ⛔ I did not touch it. |
| Pathspec anchoring | ✅ `SC-§102`/`SC-§106`: `Content`+`Config` report clean **while `Source` correctly reports the 3 held files** — the silence is real, not a mis-anchored pathspec. |

---

## 12. EVIDENCE — promoted for 🧑 his eye, no tooling needed

`.claude/pipeline/playtest-evidence/2026-09-08/`

| file | what |
|---|---|
| ⭐ **`TASK-1172-BEFORE-vs-AFTER-fog-yellow-to-grey.png`** | **the one to look at** — stacked A/B, **same Simulate session, same scatter, same camera, ONLY the two colours differ** |
| `TASK-1172-BEFORE-shipped-beige-BG0.894-sat15.8.png` | the yellow, driven back to `20c1bea`'s values in-session |
| `TASK-1172-AFTER-grey-BG0.976-sat4.1.png` | its pair |
| `TASK-1172-AFTER-grey-via-generalData-shippath-BG0.978-sat3.9.png` | the **ship-path** frame (fresh construction from the saved CDO) |
| `TASK-1172-control-negative-no-fog-BG0.560-sat44.0.png` | live negative control |
| `TASK-1172-ACQcontrol-baseColor-RED-…-BG0.691-sat72.6.png` | acquisition control **and** the §1a refutation in one frame |
| `TASK-1172-ACQcontrol-emissive-BLUE-BG1.138-sat12.1.png` | acquisition control |

⚠️ The first BEFORE/AFTER composite I built paired frames from **two different Simulate sessions**; the
grass scatter differed, so it was **not** a clean A/B. I noticed it by **looking at the picture**, rebuilt
it same-session, and **discarded the cross-session version**. Recorded because the numbers were fine and
only the eye caught it.

---

## 13. 🙋 FOR THE MANAGER

1. 🚨 **Your cl. (1) hypothesis is REFUTED** (§1). The push is **not** construction-gated: a
   post-construction write reaches the MID **and the pixels**, in the editor world *and* in Simulate where
   a rerun is impossible. `FOG-§12.4b`'s *"missing variable = when the write happens"* needs replacing.
2. 🚨 **`FOG-§12.4b`'s row-1 verdict is wrong** (§1a). `general Data.base Color` is **not** inert —
   pure red renders at `sat 72.56 %`. `TASK-1154`'s null was an instrument failure. **Answer #1 was
   correct from the start**, and three rows were spent because one measurement said otherwise.
   ⛔ I did not diagnose *why*; leading candidate named as a **lead**.
3. ⭐ **Candidate law offered:** `SC-§112` names **acquirer + analyser**. There is a **third instrument,
   upstream of both — the DRIVER**. A broken driver yields a flawlessly measured **null**, and a null
   reads as *"lever refuted"* — the one verdict nobody re-tests.
4. ⚖️ **`TASK-1165`: branch (a) fires** — band reached with **no residual** ⇒
   `superseded — reverted unshipped` (§8). Its 3 `Source/**` files are still dirty; the restore is not mine.
5. ✅ **NIT-8 closed by measurement** (§6): `0.5` is the BP field, `0.005` is the MID scalar after the
   vendor's ×0.01. **Not a contradiction; no asset correction needed.**
6. ⚠️ **`TASK-1175` inherits the LFS oid-vs-sha256 check** — and `MI_SiegeFog_Grey` changed content at an
   **identical byte size** (§10), so a size check would pass on a stale blob.
7. ⚠️ Landed **more neutral than the ideal** (3.90 % vs 5.9 %); a measured warmer notch is on file for
   `TASK-1159` (§7). ⛔ Ask (C) is closed on **pixels** — ⛔ **only 🧑 he closes it on taste.**

---

## 13a. ⚠️ EDITOR STATE I AM LEAVING BEHIND

I **launched** the editor (it was **down** when this row started — MCP answered `Unable to connect`, and
`Get-Process` found no `UnrealEditor`). **I am leaving it UP**, on `L_Arena`, MCP answering on
`http://127.0.0.1:8000/mcp`. I did **not** close it, because I cannot see whether `TASK-1168` — which the
board marks **NO vs this row, same `.uasset`** — is in flight in the same editor.

🚨 **THE ONE HAZARD, STATED LOUDLY: `L_Arena` IS DIRTY IN MEMORY.** I spawned and removed five probe
actors in it. **The file on disk is byte-identical and must stay that way** (`GFX-§11`). ⇒ ⛔ **Whoever
touches this editor next must NOT save the level, and must NEVER call `save_assets([])`** — an empty list
saves *all* dirty packages and would write `L_Arena` as a side effect. Close-and-discard is safe and
correct here; saving is not.

---

## 14. ⭐ THE TRANSFERABLE LESSON

**Three rows refuted the right answer because one instrument produced a confident, well-controlled, false
null — and a null is the one result that ends an investigation instead of continuing it.**

`TASK-1167` bought the lesson that a green gate can be blind to the *consumer*. This row adds the stage
before it: **a lever declared dead is a claim about an instrument, not about the engine — and it deserves
exactly the same positive control a lever declared alive would get.** The cheapest possible check would
have caught it: set the parameter to something absurd and look. That is a single frame. It cost this lane
three rows, a compile, a 556-test suite and thirteen mutations to not do it.

---
---

# ⭐⭐ REV-2 — THE 0.9772-vs-0.9372 CONFLICT IS SETTLED. ⛔ NEITHER LANE WAS WRONG, AND THE LEAD I WAS HANDED IS REFUTED.

⛔ **Rev-1 above is untouched** — it is the record of what was believed. This section is appended.

⛔ **Nothing was written. `BP_SiegeFog.uasset` and `MI_SiegeFog_Grey.uasset` are byte-identical to how this row found them.** No Git, no compile, no push, `L_Arena` never saved.

---

## R1. 🚨 THE HEADLINE, IN ONE SENTENCE

**The far-field `B/G` at the pinned vantage is not a property of the fog's COLOUR. It is dominated by the fog's THICKNESS, and the fog *breathes* on a ~181-second cycle — so at completely fixed Class Defaults the pinned metric sweeps `0.9235 → 0.9857`, straight through the `0.945` floor, twice every six minutes.**

⇒ ⭐ **`0.9772` and `0.9372` are both correct readings of the same asset, taken at different points on one smooth curve.** Measured, 128 consecutive frames, colour never touched: `TASK-1172rev2-the-fog-breathes-both-lanes-on-one-curve.png` — both published numbers are drawn on it as horizontal lines and the curve crosses each of them.

⚖️ **The dispatch's lead — *"an in-place drive appears to read ~0.05 high versus a fresh spawn"* — is REFUTED by direct measurement (R4).** It was a reasonable reading of the evidence available, and it is wrong: the two actuations agree, and *both* wander.

---

## R2. ⭐ INSTRUMENTS, CONTROLLED BEFORE ANY NUMBER WAS TRUSTED (`SC-§112`, `SC-§114`)

### R2a. The ANALYSER is not the variable — it reproduces BOTH lanes to the last digit

| archived frame | published | mine |
|---|---|---|
| `VID-007-t00m27s` — 🧑 his beige | 0.891 / 15.5 | **0.8907 / 15.50** |
| `TASK-1167-A` no fog | 0.548 / 45.2 | **0.5483 / 45.17** |
| `TASK-1167-B` shipped beige | 0.845 / 20.2 | **0.8453 / 20.17** |
| `TASK-1167-C` grey direct | 0.947 / 7.0 | **0.9472 / 7.00** |
| `TASK-1167-D` generalData | 0.911 / 10.7 | **0.9107 / 10.67** |
| `TASK-1172` rev-1 ship-path | 0.9772 / 3.90 | **0.9777 / 3.86** |
| `TASK-1167` rev-2 REVERTED | 0.9372 / 7.79 | **0.9373 / 7.79** |
| `TASK-1167` rev-2 PATCHED | 0.9406 / 6.66 | **0.9412 / 6.62** |
| `TASK-1167` rev-2 C3 in-place | 0.9445 / 6.96 | **0.9447 / 6.95** |

⇒ ⛔ **One instrument reproduces every figure of both lanes. The disagreement was never in analysis.**

### R2b. `SC-§112a` — the black-frame trap, measured not asserted
Synthetic 2764×828 black ⇒ `sat 0.00 %` (**inside** the pass ceiling — the best score available) and `B/G = nan`. My `ACQ_live` gate (`max > 0 and ncolours > 16`) **refuses it before scoring**, and is shown firing. Every live frame carries `ACQ_max` 237–255 and `ncolours` 26,570–340,657.

### R2c. The vantage, quoted from `FOG-§12.5`, echoed by the engine on **every** frame
Band `y ∈ [0.42, 0.48]`, `x ∈ [0.30, 0.70]` · camera `(-20000, 0, 176)` · pitch `-11.5°` · yaw `0` · Simulate-In-Editor · 8 s warmup · `2764×828`.
⛔ **`unreal.Rotator` was built with KEYWORDS** — `unreal.Rotator(roll=0.0, pitch=-11.5, yaw=0.0)` — so the positional-order trap never had a chance to fire. Engine echoed `roll 0.0 / pitch -11.5 / yaw 0.0` on all ~300 captures.

### R2d. The spawn transform — re-derived from source, never inherited
`AFogVolume::FogVisualTransform` + `USiegeScatterConfig::ArenaHalfExtent (26000, 12000)` + `FogVisualHorizontalMarginUU 6000` + `FogVisualCeilingAboveGroundUU 20000` + `FogVisualUnitCubeEdgeUU 100` ⇒ **`(0,0,7000)`, scale `(640,360,260)`**. **Achieved** read back on every spawn: `(0,0,7000)` / `(640,360,260)`.
⛔ The `TASK-1074` `(20,20,5)` substitution **never occurred** — asserted against the achieved value, not the request.

### R2e. Live controls
- **negative — no fog:** `0.5774 / 42.26`
- **positive — beige, driven:** `0.8555 / 18.03` … `0.8709 / 17.51`
- ⭐ The probe reads 🧑 his beige AS BEIGE; it does not answer *grey* to everything.

### R2f. Acquirer
Editor-side `HighResShot 2764x828` over the Python remote-execution lane (`bRemoteExecution=True`); PNGs land on disk, no base64 in context. ⛔ GDI window capture (`TASK-1167`'s all-black null) never used.

---

## R3. 🚨 THE DIAGNOSIS — SEVEN ARMS, SINGLE-VARIABLE, ONE SESSION, ONE ANALYSER

| # | arm | measured | what it kills |
|---|---|---|---|
| **A** | **Fresh spawn, shipped CDO, settle curve** | t=14 s **`0.9373 / 7.59`** → t=36 s `0.9649` → t=59 s `0.9768` → t=81 s `0.9794` → t=103 s `0.9798` | ⭐ **The build-master's `0.9372 / 7.79` REPRODUCED TO 0.0001** — and the *same actor* reaches `TASK-1172`'s `0.977` 45 s later. |
| **B** | **No fog, same 60 s window** | `0.5774` → `0.5777` (**Δ 0.0003**) | ⛔ Kills exposure / Lumen / grass-streaming / sun-motion. **The scene is stable from frame one. Only the fog moves.** |
| **C** | `r.VolumetricFog.TemporalReprojection 0` | transient **survives**; converges to `0.9807` vs `0.9816` with it on | ⛔ Kills volumetric-fog temporal history. (cvar **restored to 1**, verified by read-back; session-only) |
| **D** | ⭐ **In-place drive back to the shipped values** | reads **`0.9796` IMMEDIATELY**, no re-settle | ⛔ **KILLS THE DISPATCH'S LEAD.** An in-place write reaches the pixels at once, at whatever phase the fog is already in. |
| **E** | …then hold those values and keep watching | `0.9796` → `0.9851` → `0.9855` → `0.9834` → `0.9630` → **`0.9247`** | 🚨 **It is NOT a settle. It came back DOWN.** What looked like convergence was one limb of an oscillation. |
| **F** | Dense series, values frozen | smooth monotonic ramp **`0.9304 → 0.9809` in 53 s**; `ncolours` `133,470 → 44,725` | ⛔ Not scatter, not noise: a continuous systematic sweep, confirmed **on the pixels**. |
| **G** | **128 frames / 262 s, values frozen** | period **≈ 181 s**; `B/G` **min `0.9235` max `0.9857`**; `sat` min `2.99` max `8.30` | ⇒ **The pinned metric's own range (`0.062`) is TWICE the band's entire margin (`0.032`).** |

### R3a. ⭐ THE SECOND BAND — the finding that reconciles everybody
The same frames measured over the **optically-thick** region `y ∈ [0.05, 0.25]`, where the fog saturates and the ground contributes nothing:

| | pinned `y[.42,.48]` | thick `y[.05,.25]` |
|---|---|---|
| spread over the 22-frame ramp | **0.0505** | **0.0026** |
| spread over all 128 frames | **0.0622** | **0.0030** — ⭐ **20.7× tighter** |

**And it dissolves every cross-lane conflict this row has carried:**

| frame (all the SAME grey asset) | pinned | **thick** |
|---|---|---|
| `TASK-1172` ship-path ("PASS 0.9772") | 0.9777 | **0.9851** |
| `TASK-1167` rev-2 REVERTED ("FAIL 0.9372") | 0.9373 | **0.9871** |
| `TASK-1167` rev-2 C3 in-place | 0.9447 | **0.9864** |

| frame (all BEIGE) | pinned | **thick** |
|---|---|---|
| 🧑 **his own recording**, `VID-007` | 0.8907 | **0.9153** |
| `TASK-1167-B` probe beige | 0.8453 | **0.9145** |
| `TASK-1172` rev-1 BEFORE (in-place drive) | 0.8940 | **0.9134** |
| my live beige, 4 frames | 0.8300–0.8709 | **0.9155–0.9284** |

🚨 **On the thick band 🧑 HIS OWN GAMEPLAY RECORDING and the probe rig agree to `0.0008`.** On the pinned band the same pair differ by `0.045`. ⇒ ⭐ **The long-standing *"his beige is 0.891 but our probe beige is 0.845"* mismatch — carried unexplained for three rows, and the observation that fed the actuation hypothesis — was ALSO thickness phase.**

---

## R4. ⚖️ WHICH ACTUATION IS AUTHORITATIVE, AND THE OFFSET

⛔ **NEITHER — because actuation is not the variable.** Arm **D** drove the colour in place and the pixels followed immediately at the ambient phase; arm **A** spawned fresh and also sat at the ambient phase. **No measured offset between them.** What both share is a ±0.031 phase wander that swamps any actuation difference.

⇒ 📌 **THE AUTHORITATIVE METHOD IS NOT AN ACTUATION — IT IS AN AVERAGING WINDOW.** A single frame cannot decide this band. Authoritative = **phase mean over ≥1 breathing cycle (~181 s)**:

| method | `B/G` | `sat` | band `≥0.945` / `≤8.0` |
|---|---|---|---|
| **⭐ cycle mean (89 frames, exactly 181 s)** | **0.9667** | **4.60** | ✅ **PASSES BOTH** |
| full-span mean (128 frames, 262 s) | 0.9650 | 4.78 | ✅ PASSES BOTH |
| median | 0.9794 | 3.48 | ✅ |
| ⛔ instantaneous minimum | 0.9235 | 8.30 | ⛔ fails both |
| ⛔ instantaneous maximum | 0.9857 | 2.99 | ✅ |
| fraction of frames passing both | — | — | **78 %** |

**Offsets of the two published numbers from the cycle mean:** `TASK-1172`'s `0.9772` = **+0.0105** (peak limb) · the build-master's `0.9372` = **−0.0295** (trough limb). Magnitudes sum to `0.040` — the whole disputed gap, accounted for.

⭐ **And the cycle mean lands essentially ON the ideal:** target `~0.963 / 5.9` vs measured **`0.9667 / 4.60`**.

---

## R5. 🚨 THE VALUES I LANDED ON — ⛔ NO CHANGE, AND THE NUMBERS FOR WHY

```
general Data.base Color      = (0.86, 1.00, 1.00)     UNCHANGED
general Data.emissive Color  = (0.05, 0.07, 0.325)    UNCHANGED
```

**The band is met on the authoritative method without touching anything.** A retune would only be chasing the instantaneous trough, and I measured what that costs.

### R5a. ⭐ Sensitivity, measured with a PHASE-CANCELLING interleaved A/B
Because the phase drifts ~0.0025 per capture, two colour settings cannot be compared minutes apart. I alternated **A/B/A/B on consecutive captures (~2.5 s apart)** so the drift cancels in the pair difference. 14 pairs, `emissive.b 0.325 → 0.400` (+0.075):

- pinned `B/G` **`+0.0104`**, sd `0.0019` — ⭐ **all 14 pairs the same sign**
- pinned `sat` `−1.02`
- ⇒ sensitivity **`+0.0014` B/G per `0.01` of `emissive.b`**

### R5b. ⛔ WHAT A RETUNE WOULD COST — MEASURED
To lift the **instantaneous trough** (`0.9235`) to the `0.945` floor needs **`+0.0215`**, i.e. `emissive.b ≈ 0.325 → ≈ 0.48`.

🚨 At merely `+0.075` the fog's own colour (thick band) already moves `0.9950 → 1.0047` — **`B/G > 1.0` means blue now EXCEEDS green: the fog has stopped being grey and started being BLUE.** At the `≈0.48` the trough needs it would sit near **`1.015`**, visibly cold.
⛔ **And `+0.075` is not even enough** — that arm's own trough only reached **`0.9398`**, still under the floor.

⚠️ **The corridor's true width, stated rather than pretended comfortable:** along the *phase* axis `B/G` and `sat` are **not** competing constraints — they are two views of one quantity (fog thickness) and pass and fail *together*; the sat ceiling is never what stops you. Along the *colour* axis, more blue raises `B/G` **and** lowers `sat`, so `≤8.0` is not a wall either. **The real wall is neutrality: `B/G = 1.0` on the thick band.** Between today's `0.995` and that wall there is **`+0.005` of headroom** — and the trough needs `+0.0215`. ⇒ ⛔ **The instantaneous trough is NOT reachable while the fog is still grey.**

⚖️ The measured warmer notch on file (`0.9267 / 8.38`) is consistent with this: it is a **trough-phase sample** of a warmer pair, not a distinct failure mode.

---

## R6. ⛔ ASK (B) NON-REGRESSION — QUOTED FROM THE ASSET, BEFORE AND AFTER

| field | BEFORE | AFTER | |
|---|---|---|---|
| `general Data.density` | **0.5** | **0.5** | ✅ |
| `noise Data.sharpness` | **0.1** | **0.1** | ✅ |
| `general Data.wind Speed` | **0.5** | **0.5** | ✅ |

⛔ **Nothing was written to the asset at all**, so this is a read-read pair, not a restore. MID readback on every spawned probe: `Density 0.5` · `Base Noise Sharpness 0.10000000149` · `Wind Speed 0.004999999888241291` — ⭐ the vendor's ×0.01 on **Wind Speed only** (`FOG-§12.4c`), re-confirmed. ⛔ `0.005` was never written into the field.

## R7. HASHES (`SC-§108`)

| asset | BEFORE | AFTER |
|---|---|---|
| `Content/Blueprints/BP_SiegeFog.uasset` | `411a8bc0859b99175cfe0e06e00fce4d50275b3f9abe60106423a273c1d19271` · 38,595 B | **IDENTICAL** |
| `Content/Materials/Instances/MI_SiegeFog_Grey.uasset` | `554154c4c5564d0f494e2551b9135ccb615ce75a6d49b3f931503ebe1df33709` · 7,288 B | **IDENTICAL** |
| `Content/Maps/L_Arena.umap` | `1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622` | **IDENTICAL** — verified at start, after all engine work, and **after the editor kill** |

## R8. FENCES

| Fence | Result |
|---|---|
| `L_Arena.umap` | ✅ never saved; hash unmoved at 3 checkpoints. Dirty in memory only, **discarded by killing the editor**. |
| `save_assets` | ⛔ **never called at all**, with `[]` or otherwise. |
| Probe actors | staged in the editor world (`BP_SiegeFog_C_0`/`_1`), **all destroyed**; a final sweep returns none. |
| `Content/FogArea/**` · `Source/**` · `Config/**` · `Tools/**` | ⛔ untouched — `git status` clean on all four. |
| `Content/**` dirty set | ⛔ **unchanged from what this row inherited** — the two rev-1 assets, neither re-touched. |
| Git index | ✅ **empty** — the UE Git plugin auto-staged nothing. |
| Git / compile / push | ⛔ **none.** |
| `TASKBOARD.md` | **mine = ONE `- status:` line**, `Edit` tool only. |
| `CONVENTIONS.md` | ⛔ **not mine** — arrived dirty, not touched. |
| cvar hygiene | `r.VolumetricFog.TemporalReprojection` driven to 0 for arm C and **restored to 1**, verified by read-back. |
| Editor | I **launched** it (down at row start) and **killed** it at the end. |

## R9. ⚠️ WHAT I COULD NOT MEASURE

- ⛔ **`RaiseFog()` is still unreachable** — no `UFUNCTION`, no MCP invoke tool. Every number here, mine and both prior lanes', comes from a **hand-spawned probe at the code-derived transform**, never from playing the Fog card. `FOG-§12.4b`'s standing disclosure is unchanged.
- ⛔ **I did not identify WHAT in the vendor material breathes.** Wind advection of the noise volume is the obvious candidate (`wind Speed 0.5`), and I did **not** test it, because the only clean test is setting wind to 0 and ask (B) fences that field. **Named as a lead, not a finding.**
- ⚠️ The `181 s` period comes from an autocorrelation over `1.45` cycles (`r = 0.16`). Treat it as **~3 minutes, ±**. The *means* are robust: one-cycle `0.9667` vs full-span `0.9650`.

## R10. 🙋 FOR THE MANAGER

1. 🚨 **Ask (C) passes on the authoritative method: `B/G 0.9667 / sat 4.60` against `≥0.945 / ≤8.0`, and against the ideal `0.963 / 5.9`.** ⛔ No asset changed. ⛔ Only 🧑 he closes it on taste — the evidence is built for exactly that.
2. 🚨 **`FOG-§12.5`'s pinned band needs a companion clause, and this is the one measurement that should change a law:** *a single-frame `B/G` at `y ∈ [0.42,0.48]` cannot decide this band* — its range at fixed colours (`0.062`) is **twice the band's margin** (`0.032`). Either the metric averages over ≥1 breathing cycle, **or** it moves to the optically-thick band `y ∈ [0.05,0.25]`, which is **20.7× more stable** and reconciles 🧑 his own recording with the probe rig to `0.0008`. ⛔ **Re-pinning the vantage is yours, not mine** (`SC-§82`); I measured both and changed neither.
3. ⚖️ **Both prior rows are exonerated.** `TASK-1172` rev-1's `0.9772` and `TASK-1167` rev-2's `0.9372` are honest, correctly-acquired, correctly-analysed samples. ⛔ What was missing from **both** was a statement of *how many samples over how long*.
4. ⚠️ **The "in-place reads ~0.05 high" lead is REFUTED** (arm D) — recorded so it is not inherited as settled the way the pure-red null was (`SC-§114`).
5. ⚠️ **Possible ask-(B) residual, routed not ruled:** the fog visibly clears and re-thickens on a ~3-minute cycle at a FIXED camera (see the trough/peak composite — grass and ground legible at the trough, washed out at the peak). That is close to 🧑 his original *"some areas you cannot see, some areas it isn't even there"*, which `TASK-1158` re-read as *"one place at different moments"*. ⛔ I did not measure visibility distance and make **no** claim that ask (B) regressed.

## R11. ⭐ THE TRANSFERABLE LESSON

`TASK-1167` bought *verify the CONSUMER, not the WRITE*. Rev-1 of this row bought *a lever declared dead is a claim about an instrument*. **This adds the axis both of them held constant: WHEN.**

Every instrument in this lane was validated for *accuracy* — the analyser against six published figures, the acquirer against archived frames, the driver against four driven states. **Not one was ever validated for STABILITY.** Nobody sampled the same unchanged scene twice, minutes apart, and asked whether it gave the same answer. It does not — and the amount by which it does not is larger than the decision threshold the whole lane was arguing over.

⇒ 📌 ***A CALIBRATED INSTRUMENT POINTED AT A MOVING SUBJECT PRODUCES A PRECISE, REPRODUCIBLE, WELL-CONTROLLED NUMBER THAT MEANS NOTHING. `TASK-1172` REPLICATED ITS READING TO ΔB/G 0.0001 ACROSS TWO SESSIONS — AND THAT REPLICATION WAS EVIDENCE OF A STEADY RIG, NEVER OF A STEADY WORLD.***

## R12. EVIDENCE — promoted for 🧑 his eye

`.claude/pipeline/playtest-evidence/2026-09-08/`

| file | what |
|---|---|
| ⭐⭐ `TASK-1172rev2-the-fog-breathes-both-lanes-on-one-curve.png` | **the one to look at** — 128 frames, colour frozen, with `0.9772`, `0.9372` and the `0.945` floor drawn on it |
| ⭐ `TASK-1172rev2-SAME-COLOURS-53s-apart-trough-vs-peak.png` | same asset, same camera, 53 s apart — fails the band / passes the band |
| `TASK-1172rev2-phase-TROUGH-BG0.930-sat7.9.png` · `TASK-1172rev2-phase-PEAK-BG0.981-sat3.4.png` | the two singles |
| `TASK-1172rev2-freshspawn-first-frame-BG0.9373-reproduces-buildmaster.png` | the build-master's number, reproduced |
| `TASK-1172rev2-control-negative-no-fog-BG0.577-sat42.3.png` · `TASK-1172rev2-control-positive-beige-BG0.856-sat18.0.png` | live controls |
