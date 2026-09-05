# TASK-1024 — `NS_Spell_Fog` + `NS_Spell_BrightSun` (art-director)

**Status: ready-for-integration** · 2026-09-04 · Evidence: `.claude/pipeline/footage/VID-006-fog-brightsun-paid-and-resolved-but-silent.md`

Jonathan's report was right and the diagnosis was right: the cards were never broken. `Content/VFX/` simply had no system at the two paths the resolver composes, the spawn is null-safe, and a successful cast therefore spawned nothing. **This row was two assets and zero code.** Both now exist.

---

## 1. The `CardID`s were machine-read, never typed

Per `SC-§45`, and because a typo here is *silent* — a misspelled asset reproduces exactly the defect being fixed.

Read from `Docs/Data/cards.csv` with `csv.DictReader`, keyed on `list(fieldnames)[0]`:

- The first column header is the **empty string** `''`, as `FOG-§9.8e` warns.
- **Positive control:** all **34/34** rows returned a non-empty index-0 value (`Footman`, `Archer`, `Knight`, …). A blank table would have been caught here.
- The seven `CardType == Spell` rows are: `Fireball`, `FrostNova`, `Lightning`, `BattleCry`, `Pickpocket`, **`Fog`** (len 3), **`BrightSun`** (len 9).

Both expected IDs were **confirmed present, not assumed**.

## 2. The paths, verified character-for-character

`SpellLibrary.cpp:96-97` composes `/Game/VFX/NS_Spell_%s.%s`. I rebuilt that format string from the **CSV-derived** IDs and compared it to what the editor handed back from `load_asset` — a string comparison, not an eyeball:

| CardID (from CSV) | composed from CSV | `load_asset` refPath | class |
|---|---|---|---|
| `Fog` | `/Game/VFX/NS_Spell_Fog.NS_Spell_Fog` | `/Game/VFX/NS_Spell_Fog.NS_Spell_Fog` | `NiagaraSystem` |
| `BrightSun` | `/Game/VFX/NS_Spell_BrightSun.NS_Spell_BrightSun` | `/Game/VFX/NS_Spell_BrightSun.NS_Spell_BrightSun` | `NiagaraSystem` |

Identical. Baseline before the work: `exists` returned **false** for both — the measured defect.

**On disk** (`sha256`, per spec (4)):

- `Content/VFX/NS_Spell_Fog.uasset` — 2,566,686 B — `15dd122416700bcac8a52698b3526de920310992ca7f5824c886905a2ac5b542`
- `Content/VFX/NS_Spell_BrightSun.uasset` — 2,453,143 B — `bcf466c50ad546e3e353cf05f41dd4af0de8e3a1092321f2c210ac5b04ca744c`

⚠️ Note the standing caveat: for a Niagara asset a hash is proof of *bytes*, **not** of liveness — the compiled data is DDC-side. See §5.

## 3. Donors were chosen on pixels, and four were rejected

`CaptureAssetImage` does not support Niagara. What does work: the Niagara editor's own preview, captured via `CaptureEditorImage`. Since the inertness repair *requires* opening each system in that editor anyway, the repair step doubled as the instrument — I looked at every candidate before choosing.

**Rejected, each for a stated reason:**

| candidate | what the pixels showed | verdict |
|---|---|---|
| `NS_Ice_Magic_Snowstorm1` | compact white/cyan **crystal spray** | reads as FrostNova's family — a spell that looks like another spell is the failure mode |
| `NS_Ice_Magic_Snowstorm` | a faint wisp, barely visible | fails the §6 one-frame bar |
| `NS_Fire_Magic_Arena` | a few scattered embers — **sampled twice** to be fair to loop timing | too faint |
| `NS_Fire_Magic_Buff` | small rising ember column; right shape, too weak | too faint |
| `NS_AoeBuff_Lv2` (wizard set) | **cyan** | wrong temperature for a sun, and would collide with Fog |

**Chosen:**

- **`NS_Spell_Fog` ← `/Game/Ice_Magic/VFX_Niagara/NS_Ice_Magic_Arena`** — a wide, flat, pale band hugging the ground. Emitters `Plane, Smoke, Stone1, Stone2, Circle, Cylinder, Sparks`.
- **`NS_Spell_BrightSun` ← `/Game/Fire_Magic/VFX_Niagara/NS_Fire_Magic_Target`** — a tall, bright, warm gold-orange rising column. Emitters `Stone, Sparks1, Sparks2, Smoke, Trail, **Flash**`.

**How they read at the §6 bar, and why they read as opposites.** Both were legible in a single frame at first capture — which is precisely what the four rejects were not. They oppose on three independent axes, so the distinction survives even a glance:

| | Fog | Bright Sun |
|---|---|---|
| axis | **horizontal**, spread flat | **vertical**, rising column |
| temperature | cold, pale blue-white | warm, gold-orange |
| motion | settles / rolls in | breaks upward |

That opposition is the design answer to "it should be legible as the counter to Fog at a glance." Neither collides with the shipped set: Fireball is a spherical blast, FrostNova a crystal burst, Lightning a strike.

**Donor lane is legal:** both packs are the M7-sanctioned re-skin donors (`Fire_Magic` / `Ice_Magic`), both fully tracked in git (254 / 225 files), and both were **soft-referenced in place, never edited** — `git status` on both packs is empty and `is_dirty` is `false` for both donors.

⛔ **`Content/FogArea/` was deliberately NOT used as a donor.** It is fenced (vendor, `J-F11`) — and independently, `git ls-files` shows it is **untracked, 0 files**, so an asset depending on it would break a clean clone.

## 4. Scale and lifetime match the shipped five by construction

`SpawnSpellVFX` calls `SpawnSystemAtLocation(World, System, SpawnPoint, Rotation)` with **no scale override**, and both Fog and BrightSun fall through to the shared tail (`SpellLibrary.cpp:775`) spawning at `TargetPoint` — which for these two instant-global effects is, per the code's own comment, "only the VFX anchor." The shipped `NS_Spell_Fireball` / `FrostNova` are likewise un-rescaled donors from these same packs, PIE-verified at the gameplay camera (`handoffs/TASK-238.md`). Duplicating at donor scale is therefore the *matching* choice, not an unexamined one.

## 5. The Niagara inertness law was honoured — and these are not inert

`CONVENTIONS.md` "Material & Niagara lane laws": a Niagara system duplicated via MCP is **silently inert**, no hash or state check can detect it, and `is_active()`/`is_visible()` return `true` for a dead one. The repair is opening it **once** in the Niagara editor, then an explicit single-path save.

Done, in that order, for both:

1. `duplicate` donor → final path.
2. `OpenEditorForAsset` on the final path — the compile. Both showed the green check on **Compile**.
3. `save_assets` with an **explicit two-path list**. ⛔ Never the empty list.

**And the evidence that beats a readback:** after the compile, both duplicates **rendered visible particles in the Niagara preview at their final `/Game/VFX/` paths**. That is pixels, not `is_active()`. It rules out the inert-duplicate failure mode, which is exactly what `NS_CastleDebris` could not show.

⚠️ **The one residual, stated plainly rather than glossed:** the Niagara preview cannot tell you how the effect reads **at the gameplay camera**, nor whether it **ends** rather than persisting. I could not determine per-emitter loop behaviour — it lives in the emitter's state module, which is not reachable through the MCP property tools, and the byte-level token counts I tried are not a valid instrument (the shipped, known-good systems score the same). The timeline showed a single finite emitter bar for each, which is suggestive but **not proof**.

⇒ **For the integration pixel gate (`AS-§6 A(e)`), two things to look for:** (a) each effect is legible at the gameplay camera distance, and (b) **each effect ENDS and leaves nothing on the field.**

## 6. Blast radius

- `git status Content/` → exactly two additions: `Content/VFX/NS_Spell_BrightSun.uasset`, `Content/VFX/NS_Spell_Fog.uasset`. (`Content/FogArea/` was already untracked before I started — not mine.)
- **`L_Arena` NOT saved** — `Content/Maps/` is empty in `git status` and `L_Arena.umap` mtime is still **2026-08-27 15:05**. The level was loaded in the editor throughout; the explicit save list is what protected it.
- Donor packs `Ice_Magic` / `Fire_Magic` / `sA_StylizedWizardSet`: clean.
- `is_dirty` = `false` for both donors and both new assets — nothing left for a stray "Save All" to flush.
- ⛔ No `Source/` file, no `cards.csv`, no `DT_Cards`, no compile, no Git. `TASK-1025` and its test file untouched.

**Editor:** it was **not** running when I started (no process, no listener on `:8000`, machine had rebooted at 14:16). I booted it myself under the standing grant — PID **17916**, window title `GitClaudeUnrealTest - Unreal Editor`, **not** a `Restore Packages` modal (diagnosed by window-title enumeration, not the port). It is **still up** and MCP is live. Several donor Niagara editors remain open; all are clean.

**Note for build-master, not an action for me:** both new `.uasset`s show as **staged** (`A `) in `git status` without my having run any Git write command — I only ran `git status` / `git ls-files`. The same was true of `T_CardArt_*` at session start, so this looks like pre-existing repo behaviour rather than anything this task did. Flagging it so it is not mistaken for a stray `git add`.

---

## Deliverables

| what | where |
|---|---|
| Fog cast VFX | `/Game/VFX/NS_Spell_Fog.NS_Spell_Fog` → `Content/VFX/NS_Spell_Fog.uasset` |
| Bright Sun cast VFX | `/Game/VFX/NS_Spell_BrightSun.NS_Spell_BrightSun` → `Content/VFX/NS_Spell_BrightSun.uasset` |

No `Content/RawAssets/` source: these are in-engine donor duplicates (the M7 re-skin lane), not Blender exports.

⇒ **`TASK-1025`'s roster gate should now go green on these two rows.**
