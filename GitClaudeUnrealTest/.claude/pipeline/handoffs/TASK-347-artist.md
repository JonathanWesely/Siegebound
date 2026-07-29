# TASK-347 — [C3X-concept] Castle 3× HOLLOW concept regeneration (art-director handoff)

Status: **complete — ready for TASK-348**. Headless FLUX lane only: no editor, no MCP, no Meshy spend, no Git, no board edits.
Law followed: CONVENTIONS "Castle 3× HOLLOW (2026-07-28)" — SHELL law (open gate depicted), concept-backup law, naming law.

## Deliverables

| File | Role | sha256 |
|---|---|---|
| `Content/RawAssets/Concepts/Castle.png` | NEW accepted concept (same-path delivery) | `ccaa60525cdc5854abf2cb119e59d39fef80d2e8d7d6f51b51d817338ce8a7ad` |
| `Tools/ArtPipeline/Inbox/Castle.png` | Meshy `--mode image3d` input — **byte-identical** to the accepted concept (the TASK-329 dual-path convention) | `ccaa60525cdc5854abf2cb119e59d39fef80d2e8d7d6f51b51d817338ce8a7ad` |
| `Content/RawAssets/Concepts/Castle_pre3x.png` | Backup of the OLD approved concept (anchors all existing retention/chroma records) | `c711bca712440ef19d549f80d8fd87fb5e22c8f9db7d37eb45c32d75ef0cb76f` |

Backup was made FIRST, before any overwrite; its hash matches the pre-task `Castle.png` exactly (which was also byte-identical to the old Inbox copy — both paths rotated together, no drift).

New concept: **1024×1024, RGB (no alpha)** — FLUX lane standard; the alpha-mask law's **derived-mask fallback** applies downstream. Backdrop is a plain flat light-grey studio field → clean threshold mask.

## Generation record

- Tool: `Tools/ArtPipeline/concept_generate.py` (FLUX.1-dev, provider=auto, steps 40, guidance 3.5, 1024²). New `"Castle"` entry authored in `concept_prompts.json`; **entry seed pinned to the winner (73007)**, so `uv run concept_generate.py Castle --force` reproduces the accepted concept exactly.
- **TLS note (carried gotcha):** running bare FAILED with `CERTIFICATE_VERIFY_FAILED` — the concept lane's `provider=auto` route is NOT covered by Jonathan's two Norton HF exclusions (known `router.huggingface.co` gap). Fixed with the standing combined CA bundle `Tools/ArtPipeline/Cache/_certs/win-ca-bundle.pem` via `SSL_CERT_FILE`/`REQUESTS_CA_BUNDLE`/`CURL_CA_BUNDLE`. TASK-348 must use the same bundle for the Meshy call (`api.meshy.ai`, standing playbook). HF_TOKEN stayed env-only throughout — never echoed/logged/on-argv.
- **8 candidates, 3 prompt iterations**, all kept at `Tools/ArtPipeline/Cache/Castle/concept3x_candidates/`:

| Candidate | Prompt | Verdict |
|---|---|---|
| `seed73001.png` | v1 | Good gate, but heavy boulder clusters hug the walls (Stage-2 hollow/UCX noise) + irregular island base |
| `seed73002.png` | v1 | Gatehouse facade only; backdrop VISIBLE through the arch (would punch a hole through the model) — rejected |
| `seed73003.png` | v1 | Solid full-castle massing, gate a touch modest |
| `seed73004_v2prompt.png` | v2 | **Runner-up.** Biggest gate proportionally, passage floor visible through arch; puffier style, larger red pennants, irregular sandy base |
| `seed73005_v2prompt.png` | v2 | Fully frontal facade-keep; weak depth cue for image-to-3D |
| `seed73006_v2prompt.png` | v2 | Backdrop visible through arch + rocky base — rejected |
| **`seed73007_v3prompt.png`** | v3 | **WINNER** |
| `seed73008_v3prompt.png` | v3 | Frontal cathedral-front read; floats on backdrop |

- Prompt v1→v2: gate pushed from "huge… a fifth of the facade / two riders" to "colossal/enormous… nearly a quarter of the facade / four riders", doors removed ("portcullis fully raised, no doors"). v2→v3: "no flags or banners" (ignored by FLUX — guidance-distilled, no negatives) replaced with the positive "every spire topped with a plain stone finial", which shrank the flags to tiny weathervanes.
- **Winning prompt (v3, verbatim, as stored in `concept_prompts.json`; the tool appends its standard single-subject/grey-backdrop PROMPT_SUFFIX):**
  > a grand sprawling medieval fortress castle, warm tan honey sandstone masonry with blue-grey slate roofs, a gothic cathedral keep with tall bare spires, pointed arch windows and rose windows rising behind a crenellated sandstone curtain wall with round corner towers capped by conical slate roofs, the front wall dominated by a colossal fortified gatehouse with one enormous wide-open arched gateway nearly a quarter of the facade width, portcullis fully raised, no doors, a gaping dark shadowed hollow interior courtyard clearly visible through the huge open arch, the opening tall and wide enough for four mounted riders side by side, the entire castle sitting flush on flat level ground with a smooth level paved approach leading straight into the gate, no rocky outcrop and no moat, every spire topped with a plain stone finial, lightly weathered stonework with subtle moss accents at the wall base

## Why seed73007 won (acceptance eyeball)

- **Open gate unmistakable:** one huge round-arched gateway, pitch-dark hollow interior through the arch, no doors, wide paved approach running flush into the threshold — exactly the SHELL-law depiction TASK-348's gate cut must match.
- **Palette continuity vs `Castle_pre3x.png`:** warm tan/honey sandstone + blue-grey slate roofs + subtle moss at the base; gothic keep with lancet/rose windows behind a crenellated curtain wall with round corner towers — the closest structural sibling to the approved TASK-329 look of all 8 candidates.
- **Walk-in-ready base:** flat level ground, clean elliptical grass plate, no rocky crags (the old concept's outcrops are gone — they fought the retired plinth/flat-approach requirement).
- **Maskable:** plain flat grey backdrop, single centered subject, clean silhouette.

## Notes for TASK-348 (Meshy model build)

1. **Front face = the paved-approach side.** Cut the Stage-2 gate through the depicted arch; the LAW governs dimensions (clear ≥500w × 450h uu at final scale, threshold ≤40 uu) — the concept's arch is the visual anchor, not the measurement.
2. **Base plate:** the concept shows a thin grass ellipse + paved apron. Expect Meshy to mesh a thin base disc — flatten/trim it in Stage-2 so the entry stays ground-flush (threshold ≤40 uu; the plinth is RETIRED, do not let one reappear).
3. **Tiny red weathervane pennants** at spire tips may mesh as floating shards or bake as a few red pixels — scrub in Stage-2 if they survive; chroma impact negligible either way.
4. **All retention/chroma measurements now anchor on THIS concept** (alpha-masked via derived mask — RGB source). The dark gate void is part of the subject mask; expect it to pull masked-concept mean luma slightly down vs the old concept.
5. Runner-up `seed73004_v2prompt.png` is the fallback if Jonathan's eye wants an even bigger gate read — regenerate-free, already cached.
6. TLS: use `Cache/_certs/win-ca-bundle.pem` (env triplet) for the Meshy call; Meshy quota exit 3 ⇒ 🚨 Blockers + PAUSE per the board.

## Slack

Completion posted in 🎨 Art (standing thread `1783116278.693139`, channel `C0BF0QZP3CN`) — visibility only, NO approval gate (Jonathan waived; his eye stays final downstream at TASK-350).
