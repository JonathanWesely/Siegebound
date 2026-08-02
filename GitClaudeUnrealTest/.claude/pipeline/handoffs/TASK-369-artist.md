# TASK-369 — [AG-A2] Sorcerer concept (Stage 0) — LEY-WARDEN identity

**Agent:** art-director · **Date:** 2026-08-01 (RESUMED after Jonathan added HF credits)
**Status: NOT ACCEPTED — needs ONE ruling. 8 images generated, Gate B PASSES, TeamRegion ANSWERED, Gate A blocked by a model-level constraint conflict.**

---

## 0. Headline

✅ **The 402 is GONE.** Jonathan's credits work. **8 generations, 8 successes, zero 402s**, ~5–7 s each.
✅ **Gate B (Wizard distinctness): PASS**, decisively, on every roll.
✅ **The `TeamRegion` slab-mantle question is ANSWERED on real pixels** (§4) — it is **VIABLE**. TASK-370's manifest can be written.
⛔ **Gate A (antlers must not overshoot the skull): FAILS on every roll that renders the mandated antler crown.** This is **systematic, not seed noise** — 8 rolls, 6 distinct prompt formulations. It needs a ruling I do not own (§5).

**Nothing was archived to `Content/RawAssets/Concepts/Sorcerer.png`** — no roll passes both gates, and I will not record an accept I did not earn.

---

## 1. The CA-bundle triplet held (law confirmed again)

Ran straight to the bundled invocation as instructed. `SSL_CERT_FILE` / `REQUESTS_CA_BUNDLE` / `CURL_CA_BUNDLE`
→ `Tools/ArtPipeline/Cache/_certs/win-ca-bundle.pem`. **All 8 calls handshook clean.** Verification never disabled.
`--check` green: 19 entries, `HF_TOKEN` present (never read, logged, or placed on argv).

---

## 2. The 8 rolls

Contact sheet + all 8 full-res PNGs + ruled measurement crops are in the session scratchpad
(`…/scratchpad/Sorcerer_rolls_contactsheet.png`, `Sorcerer_roll<N>_seed<S>.png`).

| # | Seed | Result |
|---|---|---|
| 1 | 71018 | Antler rack **+143 px** over skull; invented a **2nd prop** — a slab-headed staff held aloft reading as a **war maul** (wrong for a unit that never attacks). Style + chunky proportions excellent. |
| 2 | 71019 | **Kneeling**, legs buried under a pooling cloak (unriggable). Rack **+115 px**. Mantle came back as **two split pauldrons**. |
| 3 | 71020 | Antlers best-ever **+23 px**. But **lost the standing stone AND the stone mantle** (soft draped cowl), and style/proportions drifted off-roster. |
| 4 | 71021 | Style recovered. *"like a lintel shelf"* taken **literally** → a giant architectural beam floating behind the neck, not worn. Pillar detached. Rack **+65 px**. |
| 5 | **71022** | ⭐ **RECOMMENDED.** Big **boxy granite shoulder plates with genuinely FLAT, up-facing tops** — the best TeamRegion target of all 8. Pillar well below shoulders. Cream sash present. Standing, legs visible. Rack **+88 px (9.2 %)**. |
| 6 | 71023 | Best *identity*: **both hands gripping the planted rune-stone**, exactly the spec pose. But shoulder plates are **sloped wedges** (weak up-facing) and rack **+117 px (11.7 %)**. |
| 7 | 71024 | Stone-crown experiment: the crown **ate the whole head** — antlers gone entirely (identity failure), mask became a plain box reading as a generic stone knight. |
| 8 | 71025 | **Antlers finally BELOW the skull** (tips y≈78 vs dome y≈45) — proves it is reachable. But the pillar migrated **in front of the torso, occluding the body centre** (fatal for image-to-3D), hands left the stone, style drifted. |

**Reroll count: 7** (seeds 71019–71025). Entry is now pinned to **seed 71022** and the exact 1,573-char text that produced roll 5, so `uv run concept_generate.py Sorcerer --force` reproduces the recommendation byte-for-byte.
`Inbox/Sorcerer.png` currently holds **roll 5**.

---

## 3. Two reusable prompt-craft findings (worth adding to CONVENTIONS)

**3a. On FLUX.1-dev, in-prompt negations SUMMON their tokens.** `negative_prompt` is dropped (guidance-distilled), so every defence must live in the positive prompt — and negating there backfires. Demonstrated: I wrote *"not rounded pauldrons"* → **got pauldrons** (roll 2); *"absolutely NO branching antler rack"* → **got a branching rack** (roll 2). Rolls 3+ were rewritten with **zero** negations (asserted in code) and immediately behaved better. The pre-existing `negative_prompt` field stays authored for a future CFG model swap, unused today.

**3b. Prompt length has a hard style cliff at ~1.6 k chars.** The shared style block ("Warcraft Rumble tier, 2.5–3 heads tall, hand-painted") sits at the **end** of every entry. At **2,007 chars** (roll 3) the tail was diluted and the roll lost the roster style, the chunky proportions **and** two identity props. Back at **~1,570** (rolls 4–6) the style returned. **Keep entries ≲1,600 chars.** The other 18 shipped concepts are ~700–900, which is why this never surfaced before.

---

## 4. ✅ The `TeamRegion` 4-point test — ANSWERED (real pixels, roll 5 / seed 71022)

This is the answer TASK-370's manifest is written against. Judged on `Sorcerer_roll5_seed71022.png`.

| § 6 criterion | Verdict | Evidence |
|---|---|---|
| **(1) A distinct mantle *plane*, not robe folds** | ✅ **PASS** | Two large **boxy granite plates**, unmistakably rigid stone with cracked-rock texture and rivets — plainly a different material from the cloth robe beneath. Zero risk of confusion with fabric. |
| **(2) Top face genuinely UP-facing** | ✅ **PASS** | The plates are **box-shaped with flat, level top faces** that catch the key light as horizontal ledges (see `roll5_head_ruled.png`, plate tops flat across y≈160–200). This is the criterion that failed on rolls 1 (sloped gorget), 3 (draped cowl) and 6 (sloped wedges) — roll 5 is the one that nails it. |
| **(3) Continuous across both shoulders, survives decimation** | ⚠️ **PARTIAL — acceptable** | It is **two plates split at the neck**, not the single unbroken slab I asked for. But each is **huge** (~170×150 px, ~17 % of image width). My §6 worry was *"a two-piece pauldron pair gives a split, fiddly region"* — the fiddliness is what mattered, and these are the opposite of fiddly. Expect a clean, symmetric **two-island** face set that decimates comfortably. |
| **(4) Separable from mask/antlers** | ✅ **PASS** | A clear vertical gap plus the scarf/cowl sits between plate and jaw; the antlers are far above and outboard with no contact. No creep onto the head. |

**Verdict: the `shoulder_caps` selector is VIABLE — no mantle reroll required.**
Per my own §6 law, only a failure of **(1) or (2)** forces a Stage-0 reroll, and **both pass**. (3) deviates but in the harmless direction. **TASK-370 should target two symmetric islands rather than one continuous band.**

---

## 5. ⛔ The one open decision — Gate A antlers (NOT mine to take)

**Gate A has two halves. The monolith half PASSES; the antler half does not.**

- ✅ **Monolith height — PASS** on rolls 5, 6 and 8: the planted stone's top sits **well below the shoulder line**. My shoulder-rather-than-head judgement call (approved) worked exactly as intended.
- ⛔ **Antlers — FAIL.** Overshoot by roll: **+143, +115, +23, +65, +88, +117, (none), 0** px. Every roll that renders the identity's mandated antler crown puts tines above the skull. The only two that didn't (7, 8) did so by **destroying the identity or the composition**.

**Why this matters, verified in code, not assumed.** I read `rig_character.py` and `rig_manifest.json`:
`"*_z are fractions of measured mesh HEIGHT (0=feet, 1=head-top)"`. So antlers do **not** merely shift one anchor — they skew **every** vertical anchor. At roll 5's 9.2 % inflation, `neck_top_z` 0.865 lands at ≈0.94 of true body height, i.e. the neck anchor ends up near the top of the skull. The gate's rationale is real.

**Why I stopped at 8 rolls instead of continuing.** The failure is systematic. I exhausted the levers: positive-form rewrite, size similes ("length of an ear"), explicit downward ram-horn framing with tips at the chin, and a stone crown intended to make head geometry the topmost point. FLUX's deer-antler prior overrode all of them. Further rolls spend Jonathan's newly-purchased credits on a constraint conflict that a seed cannot resolve.

### The ruling I need (one line back, please)

1. **ACCEPT the overshoot** and have the rig lane add a per-asset `proportions` override for Sorcerer (`rig_manifest.json` already supports this — *"per-asset keys override"*, and Ogre's `skeleton:"bespoke"` is precedent). Every `*_z` divides by the measured antler ratio, which Stage 2 can report exactly. **My recommendation**, with roll 5 (seed 71022) as the concept. Costs the rig lane ~10 numbers; costs art nothing.
2. **RELAX the identity** — drop or shrink the antler crown (a manager call, since the LEY-WARDEN identity mandates it). Then Stage 0 can pass clean.
3. **TRIM IN STAGE 2** — keep roll 5/6 and delete the antler geometry in Blender during refine. Feasible but leaves texture artifacts and is TASK-370 scope.

I did **not** swap `MODEL_ID` off FLUX.1-dev and will not unprompted — it is the shared style key for all 18 shipped concepts.

---

## 6. State on disk

- `Tools/ArtPipeline/concept_prompts.json` — `Sorcerer` entry rewritten (positive-form, 1,573 chars), **seed pinned to 71022**. 19 entries, JSON valid. **Only tracked file I changed.** No Git operations.
- `Tools/ArtPipeline/Inbox/Sorcerer.png` — **roll 5 (seed 71022)**, the recommended candidate, staged.
- `Content/RawAssets/Concepts/Sorcerer.png` — **deliberately ABSENT** (nothing accepted yet).
- `Tools/ArtPipeline/Cache/Sorcerer/` — absent. Meshy untouched (that is TASK-370).
- All 8 rolls + contact sheet + ruled crops: session scratchpad (not in the repo — say the word and I'll copy the chosen one in).

**TASK-370 remains blocked on the §5 ruling only.** Its TeamRegion input (§4) is now settled, so the moment the ruling lands, 370 can be dispatched without further Stage-0 work.
