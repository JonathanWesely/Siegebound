# TASK-185 — Concept-gen run: author 16 prompts + generate 16 concept PNGs — handoff

**Status:** blocked (prompts DONE + committed; generation 0/16 blocked on an HF Inference router outage)
**Assignee:** art-director
**Date:** 2026-07-16

## Summary
- **Prompts: COMPLETE.** All 16 M7 per-CardID art-direction prompts authored and committed to
  `Tools/ArtPipeline/concept_prompts.json`. `uv run concept_generate.py --check` → EXIT 0, validates all
  16 entries (ArrowTower, BallistaTower, Barracks, BombTower, Cavalry, Cleric, CrystalTower, DeepMine,
  GoldNode, Knight, Longbowman, MilitiaMob, Miner, Pikeman, Sapper, Wall), deps OK, HF_TOKEN present.
- **Generation: 0/16.** BLOCKED by a transient HF Inference **router outage** — `router.huggingface.co`
  returns `504 Gateway Time-out` for every model on every provider. NOT quota, NOT drift, NOT token.
- **A prerequisite TLS blocker was found and fixed** (see below) so that the moment HF recovers the run
  is turnkey.

## Prompts authored (source of truth: concept_prompts.json)
Sourced from GDD §4 roles, held to the §6 premium-stylized bar (polished stylized low-poly, Warcraft
Rumble / Fortnite tier; units ~2.5–3 heads tall, oversized hands/weapons; readable silhouette; team-neutral
muted stone/leather/wood/steel palette with ONE small neutral cream cloth accent per asset for the runtime
team-color slot — no baked red/blue). Each entry has a subject-specific `negative_prompt` (recorded for a
future SDXL swap; FLUX-schnell ignores negatives) and a distinct reproducibility `seed` (71001–71016).

- **Units (characters):** Knight (plate tank + longsword/shield), Miner (non-combat, oversized pickaxe +
  ore sack, lamp cap), Cavalry (lancer on chunky warhorse, single subject), Cleric (robed healer, holy
  staff, NO weapon), Longbowman (oversized longbow + quiver), MilitiaMob (single scrappy peasant levy,
  makeshift weapon), Pikeman (extra-long braced pike, kettle helm), Sapper (hugging a huge black bomb, lit
  fuse, leather apron).
- **Buildings (structures):** ArrowTower (round watchtower, arrow slits, crenellations), Wall (freestanding
  crenellated wall segment, no gate/towers), BombTower (squat stone tower + mortar + cannonballs),
  BallistaTower (timber tower + giant mounted ballista), Barracks (timber+stone hall, arched door, training
  yard), DeepMine (fortified mine entrance, headframe, minecart, faint gold-ore glints), CrystalTower
  (arcane spire + faceted violet-cyan crystal cluster, runes).
- **Prop (emissive, no team color):** GoldNode (glowing warm-gold ore/crystal cluster on grey rock, loose
  coins; the §6 gold-glow economy prop — the deliberate exception to the team-accent rule).

## Generation blocker — HF Inference router 504 (the reason 0/16)
- `uv run concept_generate.py --all` and single-card runs all fail: `router.huggingface.co` returns
  `504 Gateway Time-out` (AWS CloudFront) on every attempt (3 retries each, exponential backoff).
- **Proven router-wide, not model/provider/quota specific.** A throwaway probe (scratchpad, not committed)
  tried FLUX.1-schnell + SDXL + FLUX.1-dev across providers `hf-inference`, `fal-ai`, `together`,
  `replicate`, `nscale` — **all 504**. (`nebius` is no longer an accepted provider value — informational.)
- **General HF connectivity is healthy:** `model_info('black-forest-labs/FLUX.1-schnell')` → OK
  (pipeline_tag=text-to-image), `whoami` → OK, **PRO plan active**, token valid. So this is specifically the
  inference gateway (`router.huggingface.co`), which is a different subdomain from the healthy Hub.
- **Classification:** transient upstream outage. The tool would exit **1** (generic failure after retries)
  per card — NOT exit 3 (quota) and NOT the "expected quota pause." Do not fake; resume when HF recovers.

## Prerequisite TLS fix (required, applied, proven — record this)
- Before the 504, generation failed with `SSL: CERTIFICATE_VERIFY_FAILED (unable to get local issuer
  certificate)`. Root cause: **`router.huggingface.co` is NOT covered by Norton's TLS-scan exclusions**
  (`huggingface.co` + `*.hf.space` are excluded and proven — but `router.huggingface.co` is a different
  subdomain), so Norton MITMs it and Python's certifi bundle can't verify Norton's intercept cert. This is
  the FIRST pipeline tool to hit `router.huggingface.co` — `trellis_generate.py` uses a `*.hf.space` Space,
  which IS excluded, so it never needed this.
- **Fix (working stopgap):** built a combined CA bundle = certifi's `cacert.pem` + the Windows cert stores
  (which contain `CN=Norton Web/Mail Shield Root`), 179 certs, and pointed `SSL_CERT_FILE` /
  `REQUESTS_CA_BUNDLE` at it. Proven: TLS now verifies and the request reaches the router (it is the router
  that 504s, a separate issue). `UV_SYSTEM_CERTS=true` alone does NOT fix this — it governs `uv`'s own
  fetches, not the httpx SSL context inside the running Python process.
- Current session bundle path (ephemeral scratchpad):
  `C:\Users\wesel\AppData\Local\Temp\claude\C--GitProjects-GitHub-GitClaudeUnrealTesting-GitClaudeUnrealTest\81d71201-3b32-477b-aa6f-4d55aec85b00\scratchpad\ca-bundle-combined.pem`
- **Durable fix (recommended, Jonathan-owned — NOT applied by me):** add `router.huggingface.co` (or
  `*.huggingface.co`) to Norton's HTTPS-scan exclusion list alongside the existing `huggingface.co` /
  `*.hf.space`, so the tool "runs bare" like trellis. Alternatively persist an `SSL_CERT_FILE` HKCU user var
  pointing at a stable combined bundle. Either is a machine-config change (Jonathan's call).

## RESUME instructions (turnkey when HF Inference recovers)
1. Rebuild the CA bundle if the scratchpad was cleaned (certifi cacert.pem + exported Windows Root/CA stores;
   confirm `CN=Norton Web/Mail Shield Root` is present), OR reuse the path above if still present — UNLESS
   Jonathan has added the Norton exclusion / an HKCU `SSL_CERT_FILE`, in which case skip this step.
2. From `Tools/ArtPipeline/`:
   ```
   export SSL_CERT_FILE=<combined-bundle.pem>        # (skip if Norton exclusion added)
   export REQUESTS_CA_BUNDLE=$SSL_CERT_FILE
   uv run concept_generate.py --all
   ```
   `--all` is **resume-idempotent**: it skips any `Inbox/<CardID>.png` already present (no `--force`), so a
   partial run continues where it stopped.
3. Partial completion is useful: the 4 unit-wave-1 concepts unblock the matching TRELLIS production wave
   (TASK-168) without waiting for all 16.

## Downstream / integration notes
- No assets were imported into the editor (Stage 0 is headless; nothing to integrate yet).
- Once the 16 PNGs exist at `Tools/ArtPipeline/Inbox/<CardID>.png` they are the TRELLIS Stage-1 inputs
  (TASK-168..171). **TASK-167 (Jonathan's OPTIONAL concept review) becomes satisfiable the moment the PNGs
  exist** — he may review/replace any `Inbox/*.png` before its mesh wave; the tool skips existing files
  without `--force`, so a manual replacement is preserved.
- `Content/RawAssets/` was NOT touched (concepts get copied to `Content/RawAssets/Concepts/<AssetName>.png`
  only once accepted, downstream). Inbox/ is gitignored per the Textured-mesh law.
- Did NOT touch Git.

## Files
- `Tools/ArtPipeline/concept_prompts.json` — 16 prompts (committed content; build-master picks it up).
- `Tools/ArtPipeline/Inbox/` — 0/16 M7 targets generated. Contains only pre-existing, UNTOUCHED concepts
  for already-shipped assets (`Archer.png`, `Castle.png`, `Footman.png`, `Ogre.png`, `Ogre_original_4view.png`)
  — none are among the 16 M7 CardIDs, and `--all` iterates only the 16 in `concept_prompts.json`, so they are
  never touched. (Note: `Inbox/` is gitignored, so a `Glob` of it returns nothing — use `ls` to inspect.)
- Scratchpad (throwaway, NOT committed): `provider_probe.py`, `router_probe.py`, `winroot.pem`,
  `ca-bundle-combined.pem`.
