// Copyright Epic Games, Inc. All Rights Reserved.

#include "SiegeGraphicsMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CheckBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "CoreGlobals.h"        // GFrameCounter — the whole measurement (see FSiegeFrameRateSample)
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"   // FPlatformTime::Seconds — the other half of the window
#include "SiegeGraphicsSettingsSubsystem.h"
#include "SiegeSettingsSubsystem.h" // the PROFILE-SCOPED store that owns bShowFrameRateCounter (GFX-§3's named exception)
#include "TimerManager.h"

// ═════════════════════════════════════════════════════════════════════════════
//  ALL PLAYER-FACING TEXT ON THIS SCREEN LIVES HERE.
//
//  🧑 Jonathan's ask, verbatim, is the spec these strings answer: "the purpose
//  of all the graphic sliders will be for the player to decide what they want to
//  adjust to optimize for performance versus visual quality". ⇒ every hint below
//  says WHAT IT COSTS or WHEN IT APPLIES. ⛔ None of them is marketing copy and
//  none invents an adjective the project has not measured (GFX-§7's last clause).
// ═════════════════════════════════════════════════════════════════════════════
namespace SiegeGraphicsMenuText
{
	static const TCHAR* Title = TEXT("Graphics");

	static const TCHAR* Back = TEXT("Back");

	static const TCHAR* AutoDetect = TEXT("Auto-Detect Quality");

	/**
	 *  GFX-§6 rules Auto-Detect a BUTTON and never a first-boot action for one
	 *  reason: the benchmark stalls for a second or two, and an unannounced stall
	 *  reads as a hang. This line is that ruling's other half — the announcement.
	 */
	static const TCHAR* AutoDetectHint =
		TEXT("Measures this PC and picks a starting point. The game pauses for a moment while it runs.");

	static const TCHAR* OverallQuality = TEXT("Overall Quality");

	static const TCHAR* CustomLevel = TEXT("Custom");

	static const TCHAR* ResolutionScale = TEXT("Resolution Scale");

	/**
	 *  GFX-§5 calls this "the single biggest perf lever" and GFX-§8 agrees, so the
	 *  hint says what it actually does rather than calling it "quality".
	 */
	static const TCHAR* ResolutionScaleHint =
		TEXT("The biggest single performance win. Renders the 3D view below your screen resolution and upscales it — menus and text stay sharp.");

	static const TCHAR* ScreenResolution = TEXT("Screen Resolution");

	static const TCHAR* WindowMode = TEXT("Window Mode");

	/**
	 *  🚨 THIS SENTENCE IS DOING WORK, NOT DECORATING — AND TASK-1118 REWROTE IT,
	 *  BECAUSE THE MECHANIC IT DESCRIBES ARRIVED AND IS THE OPPOSITE WAY ROUND.
	 *
	 *  TASK-1115 shipped "applied when you confirm them", which was the honest
	 *  description of a panel that STAGED and applied nothing (gate WARN-2 ruled
	 *  that first sentence FALSE-ON-SCREEN until the countdown landed). With
	 *  TASK-1118 in the tree the truth inverts: the change is applied
	 *  IMMEDIATELY and PROVISIONALLY, and it is UNDONE unless the player
	 *  confirms. Leaving the old wording would have been a second lie in the
	 *  other direction — a player told to confirm before anything happens, who
	 *  then watches the screen change before they have confirmed anything.
	 *
	 *  ⛔ IT NAMES THE TEN SECONDS. That number is the player's assurance that a
	 *  mode they cannot see is survivable, and it is the one fact that turns a
	 *  black screen from a crash into a wait.
	 *
	 *  🚨 gate qa/TASK-1119.md NIT-1 — THE NUMBER IS HARDCODED HERE AND DERIVED
	 *  EVERYWHERE ELSE. ComposeVideoModeCountdownText() and the two log lines take
	 *  theirs from USiegeGraphicsMenuWidget::VideoModeConfirmSeconds; this prose
	 *  does not, because a runtime-composed hint would have to become a function
	 *  and this block is deliberately a table of plain literals. ⛔ SO: IF YOU EVER
	 *  MOVE VideoModeConfirmSeconds, MOVE THIS SENTENCE WITH IT — otherwise the
	 *  countdown says one number and the hint above it says ten, and nothing in
	 *  the suite or the compiler will tell you. GFX-§4 pins ten, so today they
	 *  agree; the constant's own declaration carries the matching ⛔ note.
	 */
	static const TCHAR* DisplayHint =
		TEXT("Display changes take effect straight away and then ask you to keep them. If you do not confirm within 10 seconds — or if you cannot see the screen at all — the old mode comes back by itself. Leaving Graphics also undoes an unconfirmed change.");

	/** GFX-§4's question, composed with the live count by ComposeVideoModeCountdownText. */
	static const TCHAR* VideoModeConfirmPrompt = TEXT("Keep these settings?");

	static const TCHAR* KeepSettings = TEXT("Keep");

	static const TCHAR* RevertSettings = TEXT("Revert");

	/** SecondsRemaining == 0: the revert is being issued in this same call, so the line says so rather than counting into the negatives. */
	static const TCHAR* VideoModeRevertingNow = TEXT("Keep these settings? Reverting now…");

	/**
	 *  ⛔ gate qa/TASK-1116.md WARN-3: the old refusal message set StatusText to
	 *  DisplayHint, which is about DISPLAY changes — while the player had just
	 *  pressed AUTO-DETECT. The mechanism (F-8) was right and is kept; the string
	 *  was wrong, and after TASK-1118 the ADVICE is wrong too: the way out is no
	 *  longer "press Back", it is the Keep/Revert prompt now on screen.
	 *
	 *  ⛔ gate qa/TASK-1119.md NIT-5: "wait for it to revert by itself" is false on
	 *  the TWO branches where a mode is staged with NO countdown running — no
	 *  timer manager (CountdownUnavailable) and a failed provisional apply. On
	 *  both, the only way out is to leave Graphics. Neither is reachable in a
	 *  running game, but the sentence has to be true on every path it can appear
	 *  on, so leaving is named as well. ⛔ Do not drop that clause.
	 */
	static const TCHAR* AutoDetectBlocked =
		TEXT("Auto-Detect cannot run while a display change is waiting to be confirmed. Choose Keep or Revert above — or wait for it to revert by itself, or leave Graphics — then try again.");

	/**
	 *  ⛔ The no-timer branch (CanArmVideoModeCountdown() false). Unreachable in a
	 *  running game — a viewport-added widget always has a world — but if it ever
	 *  IS reached, the panel must not silently do nothing: the player pressed a
	 *  button and the picture did not change, and SC-§94 says the screen may not
	 *  disagree with the state.
	 */
	// ⛔ gate qa/TASK-1119.md WARN-5: the old tail said "Nothing was changed" while
	// the stepper's own ValueText one line above already read the NEW mode —
	// StepWindowMode stages BEFORE this call and calls RefreshAllRows() AFTER it.
	// That is the same "the screen disagrees with the state" species SC-§94 cl. A
	// forbids, on the very branch that exists to avoid it. The sentence now says
	// what is true of BOTH halves: the picture did not change, the choice did.
	static const TCHAR* CountdownUnavailable =
		TEXT("This display change could not be applied safely, because the 10-second undo timer is unavailable here. Nothing on your screen was changed — your choice is staged, and leaving Graphics will discard it.");

	static const TCHAR* VSync = TEXT("V-Sync (tear-free, costs latency)");

	static const TCHAR* FrameRateLimit = TEXT("Frame Rate Limit");

	/**
	 *  GFX-§9's ruled consumer for ShouldEnableVolumetricFog(): a read-only line
	 *  beside the Shadows row. The engine — not this project — owns r.VolumetricFog
	 *  in [ShadowQuality@N] (0 at @0 and @1; on with a graduated froxel grid from
	 *  @2). Naming the coupling is the difference between "Shadows=Low killed my
	 *  fog" reading as a bug and reading as the setting doing its job.
	 *
	 *  ⛔⛔ RE-WORDED 2026-09-08 (TASK-1147, GFX-§12's honest-disclosure clause),
	 *  IN THE SAME COMMIT AS THE FLOOR ITSELF. The OLD text was, verbatim:
	 *      "Also drives volumetric fog, which the engine turns OFF at Low and
	 *       Medium. Currently: fog ON."   (and "… fog OFF." on the other branch)
	 *  ⛔⛔ CORRECTED 2026-09-08 (TASK-1161, from TASK-1148 WARN-7). THESE LINES USED
	 *  TO SAY that the old text "became FALSE the instant EnforceFogRenderFloor()
	 *  landed, because the Fog card's SIEGE fog is now floored from code at
	 *  ECVF_SetByCode and appears at EVERY Shadows level". ⛔ THAT WAS FALSE TWICE:
	 *    (a) ~~⛔ THE FLOOR DOES NOT REACH THE CARD'S FOG. It pins r.VolumetricFog and
	 *        the two froxel-grid cvars at ECVF_SetByCode and forces the height-fog
	 *        component's own bEnableVolumetricFog on (FogVolume.cpp:1004-1019) ⇒ what
	 *        it holds up is the world's AMBIENT ExponentialHeightFog. BP_SiegeFog is a
	 *        raymarched translucent mesh OUTSIDE the froxel grid, so no variable in
	 *        that set governs it — which is exactly what the "What IS established"
	 *        paragraph at :196-202 below says correctly, a few lines further down.~~
	 *        🚨⛔⛔⛔ REVERSED 2026-09-08 (TASK-1162). STRUCK IN PLACE, NEVER SILENTLY
	 *        DELETED: A COMMENT THAT RECORDS ITS OWN REVERSAL TEACHES THE NEXT READER
	 *        THAT THIS QUESTION IS HARD; A CLEAN ONE TEACHES HIM IT WAS OBVIOUS, AND HE
	 *        WILL MAKE THE SAME INFERENCE AGAIN. ⚠️ TASK-1161 WAS NOT SLOPPY — the
	 *        struck premise was this project's best-supported position and a gate ruled
	 *        it sound. Then TASK-1160 PUT A CAMERA ON IT, THE SAME DAY.
	 *        ⛔ THE MEASUREMENT (handoffs/TASK-1160-artist.md §3-§4, 5 promoted PNGs at
	 *        playtest-evidence/2026-09-08/): with a Fog card's BP_SiegeFog up and
	 *        Shadows at LOW, THE CARD'S FOG DOES NOT RENDER — the frame IS the no-fog
	 *        frame to within 0.2 % (far-field RGB 193→104, contrast 0.4-10 → 19.5,
	 *        featureless 70-100 % → 2.4 %), and ground visibility goes from ≤464 uu to
	 *        NO COLLAPSE ANYWHERE (enemy castle legible at ~43,000 uu) ⇒ a ≈93× lower
	 *        bound against the ≈650 uu he asked for.
	 *        ⭐ WHAT REPLACES THE STRUCK TEXT: (i) EnforceFogRenderFloor() pins
	 *        r.VolumetricFog plus the two froxel-grid axes at ECVF_SetByCode
	 *        (FogVolume.cpp:1004-1019); (ii) TASK-1160's 2×2 held Shadows FIXED IN BOTH
	 *        DIRECTIONS and isolated the effect to that ONE cvar — EPIC + forced
	 *        r.VolumetricFog 0 ⇒ the card's fog GONE; LOW + forced r.VolumetricFog 1 ⇒
	 *        the card's fog BACK ⇒ the effect tracks r.VolumetricFog and is INDEPENDENT
	 *        of sg.ShadowQuality; ⇒ (iii) THE FLOOR REACHES THE CARD'S FOG AND IS THE
	 *        FIX FOR THE DEMONSTRATED EXPLOIT — not defensive depth over a different
	 *        fog. TASK-1160's row D is a hand-simulation of this floor (Shadows at Low,
	 *        cvar held at 1) and it restores the wash completely.
	 *        ⚠️ (iv) AND THE HONEST BOUND: THE MECHANISM IS ISOLATED TO THE CVAR; THE
	 *        COUPLING HAS NOT YET BEEN READ AT THE MATERIAL. ⛔ Do NOT write "because
	 *        bUsedWithVolumetricFog" — that is a candidate NOBODY HAS OPENED, and
	 *        starting a NEW unmeasured claim inside the fix for an OLD one is the exact
	 *        defect this strike exists to remove (SC-§97).
	 *        ⛔ The struck cross-reference ":196-202" is kept VERBATIM as struck
	 *        history; that paragraph has since MOVED and now carries this same
	 *        reversal — find it by its opening words, "What IS established".
	 *    (b) ~~⛔ The old text never governed the card's fog in the first place, so
	 *        nothing the floor did could have falsified it.~~
	 *        ⚠️ ~~FLAGGED 2026-09-08 (TASK-1162), NOT REWRITTEN: (b) rests on the SAME
	 *        refuted premise as (a) — if r.VolumetricFog drives BOTH fogs, then the old
	 *        text's unqualified "volumetric fog" DID cover the card's, and the floor
	 *        DOES bear on it. ⛔ TASK-1162's row names only (a) and the two paragraphs
	 *        below, so I struck those and left (b) standing with this flag rather than
	 *        adjudicate an unnamed sentence. ⇒ ROUTED TO THE MANAGER (SC-§82).~~
	 *        🚨⛔⛔ STRUCK 2026-09-08 (TASK-1163) — THE FLAG WAS RIGHT AND THE MANAGER
	 *        ADJUDICATED IT: (b) falls for exactly the reason the flag gave. MEASURER
	 *        TASK-1160. ⭐ The flagging itself was CORRECT RESTRAINT (SC-§82/SC-§101),
	 *        not a miss — TASK-1162's row named only (a), so naming (b) was the
	 *        manager's to do, and this row is that naming.
	 *    (b-R) ✅ WHAT REPLACES IT: r.VolumetricFog drives BOTH fogs, so the old text's
	 *        unqualified "volumetric fog" DID cover the card's, and the floor DOES bear
	 *        on it. Nothing about the old sentence was out of scope.
	 *  ~~⭐ WHAT WAS ACTUALLY WRONG WITH THE OLD TEXT IS ITS NOUN: "volumetric fog",
	 *  unqualified, on a panel a player reaches having just watched a Fog card's wash
	 *  roll over the field — he reads it as the fog HE CAN SEE and concludes that
	 *  Shadows=Low will delete it. This is a SCOPING repair, not the repair of a lie.~~
	 *  🚨⛔⛔⭐⭐⭐ STRUCK 2026-09-08 (TASK-1163) — IT FELL WITH (b), WHICH WAS ITS ONLY
	 *  SUPPORT: "a SCOPING repair" only makes sense if the old noun was too WIDE, and it
	 *  was not. ⭐ THE REPLACEMENT, AND IT IS THE SHARPEST SENTENCE IN THIS LANE:
	 *  THE OLD MENU STRING WAS NOT A LIE. It said "drives volumetric fog, which the
	 *  engine turns OFF at Low and Medium", and that was ACCURATE — AND ITS ACCURACY WAS
	 *  THE BUG. It correctly told the player how to TURN THE FOG OFF.
	 *  ⇒ the repair was NEITHER a scoping fix NOR the correction of a lie: it REMOVED A
	 *  TRUE SENTENCE WHOSE TRUTH WAS THE DEFECT, and then EnforceFogRenderFloor() MADE
	 *  THE NEW SENTENCE TRUE. The string and the floor are one change in two files.
	 *  ⛔ THE SUITE ALREADY SAID THIS IN PLAIN WORDS AND IT IS CITED, NOT RE-DERIVED —
	 *  Tests/SiegeGraphicsMenuTest.cpp:1001: the pre-floor string "ADVERTISED an exploit
	 *  in the game's own menu, to the one population that would act on it".
	 *  ~~⛔⛔ AND THE CARD'S HALF IS STILL OPEN: no measured route deletes BP_SiegeFog
	 *  (FOG-§12.1 AS CORRECTED; TASK-1147's 28-package scan, ruled sound by TASK-1148)
	 *  and no frame of this game has ever been captured at Shadows=Low with a card up
	 *  (TASK-1160) ⇒ the reported exploit is UNEXPLAINED — ⛔ NOT confirmed, ⛔ NOT
	 *  refuted, ⛔ NOT CLOSED. ⛔ NO COMMIT MESSAGE, ROW, HANDOFF OR SLACK POST MAY
	 *  SOURCE A "the exploit is fixed" SENTENCE FROM THIS BLOCK.~~
	 *  🚨⛔⛔ SUPERSEDED 2026-09-08 (TASK-1162), MEASURER TASK-1160 — THE FRAME NOW
	 *  EXISTS AND THE VERDICT FLIPPED: the exploit is DEMONSTRATED, not unexplained
	 *  (FOG-§12.1 as corrected a SECOND time the same day; GFX-§9's reversal bullet).
	 *  ⭐ WHAT SURVIVES THE STRIKE, INTACT AND IMPORTANT: TASK-1147's 28-package scan
	 *  was NOT wrong. It enumerated DetailMode / QualitySwitch / draw-distance routes
	 *  and correctly found NONE — the route was a FOURTH KIND NOBODY ENUMERATED. ⇒ an
	 *  exhaustive search of an INCOMPLETE LIST is still an incomplete search, and its
	 *  rigour is exactly what makes it read as exhaustive.
	 *  🚨⛔ AND THE PROHIBITION IS NOT LIFTED, ONLY NARROWED — READ THE DIFFERENCE
	 *  BEFORE COPYING A SENTENCE OUT OF HERE. A commit MAY now say "the floor is the
	 *  fix for the demonstrated exploit". ⛔ NOTHING MAY SAY ASK (A) IS CLOSED. SEVEN
	 *  conditions remain UNTESTED: (1) r.SceneColorFormat and (2)
	 *  r.TranslucencyLightingVolume INDIVIDUALLY — the Effects GROUP is refuted only
	 *  because [EffectsQuality@0] never moved either one in this project's config, so
	 *  those two CVARS ARE NOT; (3) Shadows = MEDIUM (expected identical; expectation
	 *  is not measurement); (4) the shipped MENU path (MCP has no input lane — only he
	 *  can walk it, TASK-1159 cl. 1); (5) a PACKAGED build; (6) FRAME TIME, so nothing
	 *  here says what the floor COSTS; (7) the material-level WHY. ⇒ ASK (A) NARROWS,
	 *  IT DOES NOT CLOSE, AND HE CLOSES IT ON TASK-1159 — NOT FROM THIS FILE.
	 *  ⭐ THE OLD TEXT stays TRUE of the world's AMBIENT height fog, which nothing
	 *  floors outside a fog window ⇒ THE NEW STRING DISTINGUISHES THE TWO. A string that
	 *  simply dropped the fog clause would have been honest and useless; a string
	 *  left as it was would teach the player that the menu lies, which is SC-§94
	 *  pointed at him and exactly what GFX-§7's learnability clause forbids.
	 *  ⛔ ShouldEnableVolumetricFog() and its test VolumetricFogFollowsShadowNotEffects
	 *  STAY: they describe the ENGINE, and the engine is unchanged. What changed is
	 *  what that answer MEANS to a player, which is a sentence, not a predicate.
	 *
	 *  🚨⛔⛔ AND THE CLAIM IS SCOPED TO **SHADOWS**, DELIBERATELY AND NARROWLY —
	 *  read handoffs/TASK-1147-programmer.md §0 before widening it. An earlier
	 *  draft of this string said the siege fog "always appears at every setting",
	 *  and that is a claim about all TEN quality groups that NOBODY HAS MEASURED.
	 *  ⛔ Writing it would have replaced one false menu sentence with another —
	 *  the exact failure this correction exists to fix, one draft later.
	 *  ~~⭐ What IS established, and what this string therefore says: the Fog card's
	 *  visual is BP_SiegeFog, a raymarched TRANSLUCENT MESH and NOT a froxel
	 *  participant (TASK-1151, on pixels + a node census), so r.VolumetricFog —
	 *  the one cvar the Shadows group owns here — does not govern it; and the
	 *  AMBIENT half is floored from code while a fog is up
	 *  (AFogVolume::EnforceFogRenderFloor). Both halves of the Shadows row are
	 *  therefore covered, and no other row's is claimed.~~
	 *  🚨⛔⛔ SUPERSEDED 2026-09-08 (TASK-1162), MEASURER TASK-1160 — AND STRUCK EVEN
	 *  THOUGH NO GATE NAMED IT: TASK-1161 rewrote the block above to AGREE with this
	 *  paragraph, so repairing only the half a gate named would leave this block
	 *  SELF-CONTRADICTORY AGAIN, which is the very defect TASK-1161 existed to remove.
	 *  ⭐ WHAT IS STILL TRUE IN IT: TASK-1151's node census stands as a DESCRIPTION OF
	 *  THE ASSET — BP_SiegeFog is a raymarched translucent mesh and it is not a froxel
	 *  participant. ⛔ WHAT WAS FALSE IS THE INFERENCE DRAWN FROM IT: being outside the
	 *  froxel grid does NOT mean r.VolumetricFog leaves it alone. TASK-1160 measured
	 *  that r.VolumetricFog ALONE decides whether BP_SiegeFog renders, with
	 *  sg.ShadowQuality held fixed in BOTH directions.
	 *  ⇒ ⭐ WHAT IS ESTABLISHED NOW, AND WHAT THIS STRING THEREFORE SAYS: ONE cvar
	 *  governs BOTH fogs — the world's ambient froxel term AND the Fog card's mesh — so
	 *  ONE floor (AFogVolume::EnforceFogRenderFloor) covers both halves of the Shadows
	 *  row while a fog is up, and no other row's is claimed. ⚠️ THE MECHANISM IS
	 *  ISOLATED TO THE CVAR; THE COUPLING HAS NOT YET BEEN READ AT THE MATERIAL.
	 *  ⚖️ AND THE LESSON, WRITTEN WHERE THE MISTAKE WAS MADE: a CORRECT census of a
	 *  material graph did not license a conclusion about what DELETES the thing it
	 *  described. The census was right; the inference was not.
	 *
	 *  ⚠️⚠️ THE TWO FALSIFIERS OF THIS STRING, WRITTEN HERE RATHER THAN ONLY IN A
	 *  HANDOFF, BECAUSE THIS IS WHERE THE SENTENCE LIVES (TASK-1148 WARN-1/WARN-6):
	 *    (1) ~~⛔ "Lowering Shadows does not remove the Fog card's siege fog" is a
	 *        claim to the PLAYER about a RENDERED OUTCOME, and NO FRAME OF THIS GAME
	 *        HAS EVER BEEN CAPTURED AT SHADOWS=LOW WITH A FOG CARD UP. It rests on
	 *        TASK-1151's node census plus pixels of the AMBIENT system. ⇒ if the
	 *        capture owed to TASK-1149 shows the wash dying at Shadows=0, THIS
	 *        SENTENCE IS FALSE and must be corrected in the same action — one false
	 *        menu string replaced by another is this row's own failure mode.
	 *        ⛔ TAKING THAT CAPTURE REQUIRES DEFEATING THE FLOOR ON PURPOSE
	 *        (r.VolumetricFog 0 from the console outranks SetByCode) — otherwise the
	 *        wash survives whether or not the exploit exists. See FogVolume.cpp.~~
	 *        🚨⛔⛔⛔ THIS FALSIFIER FIRED 2026-09-08. TASK-1160 TOOK THE FRAME AND THE
	 *        WASH DID DIE AT Shadows=LOW. ⭐ THE STRING SURVIVES ANYWAY, AND WHY IT
	 *        SURVIVES IS THE MOST IMPORTANT SENTENCE BESIDE IT (TASK-1162 cl. 4):
	 *        ⛔⛔ THE STRING IS TRUE **POST-FLOOR**, AND TRUE **ONLY BECAUSE
	 *        EnforceFogRenderFloor() HOLDS r.VolumetricFog UP.** TASK-1160's row D is
	 *        the hand-simulation of exactly that: Shadows at LOW with the cvar pinned
	 *        at 1, and the card's fog renders in full.
	 *        ⇒ 🚨 IF EnforceFogRenderFloor() IS EVER REVERTED, DISABLED
	 *        (bEnableFogRenderFloor) OR OUT-RANKED (a console r.VolumetricFog 0 beats
	 *        ECVF_SetByCode), THIS USER-FACING STRING BECOMES A LIE TO THE PLAYER —
	 *        and that is MEASURED, not feared: pre-floor, at Shadows=Low, the card's
	 *        fog is GONE and the frame is the no-fog frame to within 0.2 %.
	 *        ⛔ SO THE STRING AND THE FLOOR SHIP TOGETHER AND REVERT TOGETHER. Anyone
	 *        deleting or disabling the floor MUST delete this sentence in the SAME
	 *        ACTION, exactly as this row deleted the premise it was justified on.
	 *        ⚠️ TASK-1161 §5 RULED THIS STRING DEFENSIBLE AND THE RULING STANDS — BUT
	 *        ITS REASON (3) IS REFUTED: it said the rule holds "because BP_SiegeFog is
	 *        NOT a froxel participant, NOT because the floor protects it". THAT IS
	 *        EXACTLY BACKWARDS. ⚖️ A CORRECT CONCLUSION FROM REFUTED PREMISES IS STILL
	 *        A DEFECT, BECAUSE THE PREMISE IS WHAT THE NEXT CHANGE WILL BE REASONED
	 *        FROM. ⛔ THE STRING ITSELF IS UNTOUCHED AND BYTE-IDENTICAL — cl. 4 is a
	 *        RULING, not an edit.
	 *        ⛔ ON THE STRUCK "REQUIRES DEFEATING THE FLOOR" LINE: it was FALSE FOR
	 *        TASK-1160 AND IT IS TRUE FROM NOW ON. That capture ran on a PRE-floor
	 *        binary (DLL built 09-08 01:32:13, FogVolume.cpp written 12 h 43 m later,
	 *        zero Live Coding patches — and sg.ShadowQuality 0 DID drive
	 *        r.VolumetricFog to 0, which a floored binary would have refused), so no
	 *        defeat was needed. ⛔ ANY RE-CAPTURE AFTER TASK-1149 SHIPS DOES NEED THE
	 *        DELIBERATE DEFEAT.
	 *    (2) ⛔ The FIRST clause ("which the engine turns OFF at Low and Medium") is
	 *        momentarily FALSE while a fog window is up, because the floor forces it
	 *        ON. That is harmless TODAY for a measured reason: the panel is
	 *        unreachable during a match (TASK-1146 cl. 3), so the clause is true at
	 *        every instant a player can read it. ⛔ It goes false the day an in-match
	 *        Settings entry ships — the same day named at FogVolume.cpp's scope
	 *        deletion — and this string must then say "except while siege fog is up".
	 */
	static const TCHAR* ShadowHintFogOn =
		TEXT("Also drives the world's ambient volumetric fog, which the engine turns OFF at Low and Medium. Currently: ambient fog ON. Lowering Shadows does not remove the Fog card's siege fog — its presence is a gameplay rule, not a graphics option.");

	static const TCHAR* ShadowHintFogOff =
		TEXT("Also drives the world's ambient volumetric fog, which the engine turns OFF at Low and Medium. Currently: ambient fog OFF. Lowering Shadows does not remove the Fog card's siege fog — its presence is a gameplay rule, not a graphics option.");

	/**
	 *  ⛔ board cl. (5) / GFX-§9's "CONSEQUENCE THE UI MUST STATE IN WORDS".
	 *  The battlefield scatter's cull band is set inside the build path
	 *  (ASiegeBattlefieldScatter::RunScatterPasses, BattlefieldScatter.cpp:363),
	 *  which runs ONCE PER MATCH. A control that silently does nothing until later
	 *  is worse than one that admits it.
	 */
	static const TCHAR* FoliageHintNextMatch =
		TEXT("Grass and trees on the battlefield are built when a match starts, so a change here appears at the NEXT match — not the one in progress.");

	/**
	 *  Shown in StatusText when the graphics subsystem cannot be resolved. Worded
	 *  to match the actual behaviour: the panel is inert, nothing was written, and
	 *  the game keeps whatever settings it already had.
	 */
	static const TCHAR* Unavailable =
		TEXT("Graphics settings are unavailable right now, so nothing on this screen can be changed. Your current settings are unaffected.");

	/** StatusText's normal content: the one sentence that frames the whole screen. */
	static const TCHAR* Purpose =
		TEXT("Lower settings buy frame rate; higher settings buy detail. Quality changes apply immediately.");

	// ── TASK-1120 (GFX-§7) ──────────────────────────────────────────────────

	/** The in-match-counter opt-in. The label says WHERE the number appears, because that is the entire value of the setting. */
	static const TCHAR* ShowFrameRateCounter = TEXT("Show FPS counter during a match");

	/**
	 *  ⛔ THIS HINT IS `GFX-§7`'s ARGUMENT, IN WORDS, ON SCREEN — not decoration.
	 *  Without it a player reasonably assumes the number at the top of this panel
	 *  is the number to tune against, and it is not: this menu level carries none
	 *  of the battlefield's cost. The sentence exists to stop a tuning session
	 *  being conducted against a flattering number.
	 */
	static const TCHAR* ShowFrameRateCounterHint =
		TEXT("The number above is measured HERE, in the menu — the battlefield costs far more. Turn this on to see the real frame rate in the top-right corner during a match, which is where these settings should be judged.");

	/** Shown while the in-match toggle cannot be read or written. Says what is true: the panel's own readout still works. */
	static const TCHAR* FrameRateCounterUnavailable =
		TEXT("Show FPS counter during a match (unavailable — your profile's settings could not be opened)");
}

// ═════════════════════════════════════════════════════════════════════════════
//  LAYOUT CONSTANTS — one place, so the 19 rows read as one family and so a
//  future WBP_GraphicsMenu has numbers to match. Geometry is lifted from the
//  shipped USettingsMenuWidget / WBP_MainMenu button idiom (font 28,
//  MakeMargin(24,12,24,12), HAlign_Fill) rather than invented.
// ═════════════════════════════════════════════════════════════════════════════
namespace SiegeGraphicsMenuLayout
{
	static constexpr float PanelWidth      = 1040.0f;
	static constexpr float TitleFontSize   = 36.0f;
	static constexpr float ButtonFontSize  = 28.0f;
	static constexpr float RowFontSize     = 22.0f;
	static constexpr float HintFontSize    = 16.0f;
	static constexpr float ValueMinWidth   = 190.0f;
	static constexpr float LabelMinWidth   = 320.0f;
	static constexpr float StepButtonWidth = 56.0f;

	static const FLinearColor HintColor(0.72f, 0.74f, 0.78f, 1.0f);
	static const FLinearColor ValueColor(1.0f, 0.92f, 0.70f, 1.0f);

	// ── TASK-1120: the in-match counter's own geometry ──────────────────────
	static constexpr float CounterFontSize = 20.0f;

	/** Kept clear of the top-centre castle health bars and of the bottom card bar. */
	static const FMargin CounterScreenMargin(0.f, 18.f, 22.f, 0.f);

	static const FMargin CounterPlatePadding(12.f, 5.f, 12.f, 5.f);

	/** Dark enough to read white text over snow AND over fire; not so dark it becomes a black box on the battlefield. */
	static const FLinearColor CounterPlateColor(0.f, 0.f, 0.f, 0.55f);

	static const FLinearColor CounterTextColor(0.85f, 1.0f, 0.85f, 1.0f);
}

// ═════════════════════════════════════════════════════════════════════════════
//  ⭐⭐ TASK-1120 — THE MEASUREMENT (GFX-§7)
//
//  Everything about the frame-rate instrument that is ARITHMETIC lives here, as
//  pure statics, so the whole of it is assertable by the suite without a world,
//  a widget, a clock or a rendered frame. The two widgets below own only the
//  PLUMBING — a timer, a text block and a visibility.
// ═════════════════════════════════════════════════════════════════════════════

bool FSiegeFrameRateSample::ComputeOverWindow(
	uint64 FramesElapsed, double SecondsElapsed, float& OutFramesPerSecond, float& OutMilliseconds)
{
	// ⛔ THE THREE REFUSALS, AND NONE OF THEM IS HYPOTHETICAL.
	//
	//  • SecondsElapsed <= 0: the clock did not advance across the window. Reading
	//    the same instant twice divides by zero.
	//  • !FMath::IsFinite: a corrupted or wrapped clock read. Guarded because the
	//    consequence is `nan` PAINTED ON THE HUD, which no later code can undo.
	//  • FramesElapsed == 0: ⛔ THE REAL ONE. A minimised window, an alt-tab, a
	//    loading hitch longer than the window, a frame-rate cap below 2 fps — the
	//    engine tick simply did not run. Dividing seconds by zero frames gives
	//    `inf` ms, and it would appear at exactly the moment the player is
	//    staring at the counter to find out what went wrong.
	//
	// ⛔ ON REFUSAL BOTH OUTPUTS ARE LEFT UNTOUCHED. The caller keeps its previous
	// text rather than being handed a zero it cannot distinguish from a reading —
	// "0 FPS" and "no reading" are different claims and only one of them is true.
	if (SecondsElapsed <= 0.0 || !FMath::IsFinite(SecondsElapsed) || FramesElapsed == 0)
	{
		return false;
	}

	const double Frames = static_cast<double>(FramesElapsed);

	// ⚠️ Both numbers come from the SAME division, so they cannot disagree: over
	// the window, ms is exactly 1000 / fps. That identity is what makes it safe to
	// show both — a player who tunes on ms and a player who tunes on FPS are
	// reading one measurement in two units, not two measurements.
	OutFramesPerSecond = static_cast<float>(Frames / SecondsElapsed);
	OutMilliseconds    = static_cast<float>((SecondsElapsed * 1000.0) / Frames);
	return true;
}

FString FSiegeFrameRateSample::ComposeReadoutText(float FramesPerSecond, float Milliseconds)
{
	// ⭐ ms IS SHOWN, AND IT IS SHOWN SECOND ON PURPOSE. FPS is the number players
	// name; ⛔ ms is the number that is LINEAR IN COST, and it is the only one of
	// the two you can reason about additively. Dropping from 60 to 50 fps and from
	// 30 to 25 fps are the same 3.3 ms of extra work — a fact the FPS column hides
	// and the ms column states. `GFX-§7` asks for both; this is why.
	return FString::Printf(TEXT("%d FPS  ·  %.1f ms"), FMath::RoundToInt(FramesPerSecond), Milliseconds);
}

FString FSiegeFrameRateSample::ComposePendingText()
{
	// ⛔ NOT "0 FPS" AND NOT AN EMPTY STRING. A zero reads as a measurement of a
	// frozen game; an empty line reads as a broken feature. Dashes read as
	// "waiting", which is what is true for the first half second.
	return FString(TEXT("-- FPS  ·  -- ms"));
}

void FSiegeFrameRateSample::ReadEngineNow(uint64& OutFrameCounter, double& OutSeconds)
{
	// ⛔ THE ONLY SITE IN THIS PROJECT THAT TOUCHES EITHER GLOBAL — one place to
	// audit, and one place a test-only substitute could ever be needed.
	//
	// GFrameCounter: CoreGlobals.h:532, `extern CORE_API uint64`. Incremented ONCE
	// PER ENGINE TICK, unconditionally, at the end of FEngineLoop::Tick
	// (LaunchEngineLoop.cpp:6130-6131) in EVERY build configuration — unlike the
	// `stat fps` machinery, which is compiled out of Shipping entirely.
	OutFrameCounter = GFrameCounter;
	OutSeconds      = FPlatformTime::Seconds();
}

bool FSiegeFrameRateSample::Advance(uint64 FrameCounterNow, double SecondsNow, FString& OutText)
{
	// ⚠️ THE FIRST CALL AFTER Reset() ALWAYS RETURNS false, AND THAT IS THE DESIGN:
	// it has nothing to subtract from, so it OPENS the first window rather than
	// inventing a reading for a window that has not happened yet. A version that
	// returned a number here would be reporting the interval since the program
	// started, or since whatever the members happened to hold.
	if (!bWindowOpen)
	{
		LastFrameCounter = FrameCounterNow;
		LastSeconds      = SecondsNow;
		bWindowOpen      = true;
		return false;
	}

	// ⛔ uint64 SUBTRACTION, GUARDED. GFrameCounter only ever increases, but a
	// caller could hand these in out of order (a test, or a future clock source),
	// and an unguarded `Now - Last` on unsigned types would wrap to ~1.8e19 frames
	// and print a spectacular lie rather than failing.
	const uint64 FramesElapsed = (FrameCounterNow >= LastFrameCounter)
		? (FrameCounterNow - LastFrameCounter)
		: 0;
	const double SecondsElapsed = SecondsNow - LastSeconds;

	// ⛔ THE WINDOW MOVES ON EVEN WHEN THE READING IS REFUSED. Otherwise a single
	// stall would leave a stale anchor behind and every later window would be
	// measured from it — one bad half-second would poison the readout forever.
	LastFrameCounter = FrameCounterNow;
	LastSeconds      = SecondsNow;

	float FramesPerSecond = 0.0f;
	float Milliseconds    = 0.0f;
	if (!ComputeOverWindow(FramesElapsed, SecondsElapsed, FramesPerSecond, Milliseconds))
	{
		return false;
	}

	OutText = ComposeReadoutText(FramesPerSecond, Milliseconds);
	return true;
}

void FSiegeFrameRateSample::Reset()
{
	bWindowOpen      = false;
	LastFrameCounter = 0;
	LastSeconds      = 0.0;
}

// ═════════════════════════════════════════════════════════════════════════════
//  ENTRY POINT
// ═════════════════════════════════════════════════════════════════════════════

USiegeGraphicsMenuWidget* USiegeGraphicsMenuWidget::CreateAndAddToViewport(
	APlayerController* OwningController,
	TSubclassOf<USiegeGraphicsMenuWidget> MenuClass,
	int32 ZOrder)
{
	if (!IsValid(OwningController))
	{
		UE_LOG(LogSiegeGraphics, Warning,
			TEXT("[GraphicsMenu] CreateAndAddToViewport: no owning player controller — no panel was created. Never fatal: the settings screen underneath is untouched."));
		return nullptr;
	}

	// ⚠️ `.Get()` ON BOTH ARMS IS LOAD-BEARING, NOT TIDYING — it is the fix for
	// C2445, carried verbatim from UWarMapWidget / USiegeControlsHelpWidget, which
	// paid for the diagnosis. TSubclassOf carries BOTH a non-explicit
	// TSubclassOf(UClass*) constructor AND a non-explicit operator UClass*(), so a
	// conditional whose arms are TSubclassOf<T> and UClass* has two equally good
	// common types and the compiler must refuse to choose.
	const TSubclassOf<USiegeGraphicsMenuWidget> ResolvedClass =
		MenuClass ? MenuClass.Get() : USiegeGraphicsMenuWidget::StaticClass();

	USiegeGraphicsMenuWidget* Panel = CreateWidget<USiegeGraphicsMenuWidget>(OwningController, ResolvedClass);
	if (Panel == nullptr)
	{
		UE_LOG(LogSiegeGraphics, Warning,
			TEXT("[GraphicsMenu] CreateAndAddToViewport: CreateWidget returned null for class '%s' — no panel. Never fatal."),
			*GetNameSafe(ResolvedClass));
		return nullptr;
	}

	Panel->AddToViewport(ZOrder);

	UE_LOG(LogSiegeGraphics, Log,
		TEXT("[GraphicsMenu] Opened (class '%s', ZOrder %d). %s"),
		*GetNameSafe(ResolvedClass), ZOrder,
		// ⚠️ `.Get()` AGAIN, AND FOR THE SAME REASON.
		(ResolvedClass.Get() == USiegeGraphicsMenuWidget::StaticClass())
			? TEXT("No WBP — the code-authored tree renders the panel (GFX-§2; /Game/UI/WBP_GraphicsMenu is RESERVED and unauthored).")
			: TEXT("An asset-authored tree is driving this panel (GFX-§2 condition (b))."));

	return Panel;
}

// ═════════════════════════════════════════════════════════════════════════════
//  TREE CONSTRUCTION
// ═════════════════════════════════════════════════════════════════════════════

TSharedRef<SWidget> USiegeGraphicsMenuWidget::RebuildWidget()
{
	// ⚠️⚠️ ORDER IS LOAD-BEARING — GFX-§2(c), AND IT IS THE WHOLE OF THAT
	// CONDITION. UUserWidget::RebuildWidget() reads WidgetTree->RootWidget AS IT
	// STANDS at the moment it is called and returns an SSpacer when it is null, so
	// a code-authored tree MUST be constructed BEFORE Super::RebuildWidget().
	// Building it afterwards and returning Super's result yields a silently EMPTY
	// widget that still passes every property read-back — the exact failure class
	// GFX-§2(e)'s "verification is a pixel check" exists for.
	//
	// ⚠️ Initialize() FIRST, and it closes the last route to a blank panel:
	// WidgetTree is allocated INSIDE Initialize() (UserWidget.cpp), and
	// Super::RebuildWidget()'s own self-heal (`if (!bInitialized) Initialize();`)
	// runs AFTER our tree-building would already have bailed out on a null
	// WidgetTree, leaving Super to find RootWidget null and hand back an SSpacer.
	// Initialize() is public and idempotent — it no-ops unless
	// (!bInitialized && !HasAnyFlags(RF_ClassDefaultObject)).
	//
	// ⛔ THE THREE LINES BELOW ARE THE SHIPPED USettingsMenuWidget SEQUENCE
	// (SettingsMenuWidget.cpp:92-94), CLONED CHARACTER-FOR-CHARACTER IN SHAPE.
	// Do not reorder them.
	Initialize();
	ConstructGraphicsTree();
	return Super::RebuildWidget();
}

void USiegeGraphicsMenuWidget::ConstructGraphicsTree()
{
	if (WidgetTree == nullptr)
	{
		UE_LOG(LogSiegeGraphics, Error,
			TEXT("[GraphicsMenu] No WidgetTree — the graphics panel cannot build its tree."));
		return;
	}

	// ------------------------------------------------------------------------
	// GFX-§2 CONDITION (b), THE ESCAPE HATCH. If an asset-authored tree exists (a
	// future /Game/UI/WBP_GraphicsMenu), it wins WHOLE: UMG has already resolved
	// every BindWidgetOptional member from it, so there is nothing to construct
	// and nothing to overwrite. Taking that fallback costs ONE art task and ZERO
	// C++ — which is what makes GFX-§2 a ruling and not a one-way door.
	// ------------------------------------------------------------------------
	if (WidgetTree->RootWidget != nullptr)
	{
		UE_LOG(LogSiegeGraphics, Log,
			TEXT("[GraphicsMenu] An asset-authored tree is present — the code-authored branch is skipped (GFX-§2 condition (b))."));
		return;
	}

	// ---- BackdropBorder: the modal plate, and the tree root -----------------
	if (BackdropBorder == nullptr)
	{
		BackdropBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackdropBorder"));
	}

	if (BackdropBorder == nullptr)
	{
		UE_LOG(LogSiegeGraphics, Error,
			TEXT("[GraphicsMenu] Could not construct BackdropBorder — the graphics panel has no root."));
		return;
	}

	// ⛔ GFX-§2(f). THIS LINE IS CORRECTNESS, NOT STYLING, AND IT IS THE OPPOSITE
	// OF WBP_SessionMenu's BACKDROP. This panel is added ON TOP of the LIVE
	// settings panel, which sits ON TOP of the LIVE main menu, and NEITHER is
	// removed. A HIT_TEST_INVISIBLE plate would let clicks fall straight through
	// to Play / Sandbox / Deck Builder / QUIT while the panel looks modal.
	// ESlateVisibility::Visible makes the border hit-testable, so it absorbs every
	// click that is not on a control.
	//
	// Note this does NOT depend on the brush drawing anything: Slate hit-tests on
	// VISIBILITY and geometry, not on whether pixels were painted. The dimming
	// below is appearance; the click blocking is this line.
	//
	// The owning UUserWidget stays at its UMG default of SelfHitTestInvisible —
	// "I do not hit-test, my children do" — which is exactly right: the border is
	// the child doing the absorbing.
	BackdropBorder->SetVisibility(ESlateVisibility::Visible);
	BackdropBorder->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.82f));
	BackdropBorder->SetPadding(FMargin(0.f, 24.f, 0.f, 24.f));
	BackdropBorder->SetHorizontalAlignment(HAlign_Center);
	BackdropBorder->SetVerticalAlignment(VAlign_Fill);

	WidgetTree->RootWidget = BackdropBorder;

	// ---- PanelSizeBox: caps the column width --------------------------------
	if (PanelSizeBox == nullptr)
	{
		PanelSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelSizeBox"));
		if (PanelSizeBox != nullptr)
		{
			PanelSizeBox->SetWidthOverride(SiegeGraphicsMenuLayout::PanelWidth);
		}
	}

	// ---- RootScrollBox: board cl. (4), scrolling is NOT optional ------------
	// Nineteen rows do not fit a fixed panel at 1080p, and a panel whose bottom
	// rows are off-screen is INDISTINGUISHABLE FROM MISSING FEATURES.
	if (RootScrollBox == nullptr)
	{
		RootScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RootScrollBox"));
	}

	// ---- RootPanel: the column ----------------------------------------------
	if (RootPanel == nullptr)
	{
		RootPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootPanel"));
	}

	if (RootPanel == nullptr)
	{
		UE_LOG(LogSiegeGraphics, Error,
			TEXT("[GraphicsMenu] Could not construct RootPanel — the graphics panel has no content column."));
		return;
	}

	// Assemble the spine. Every hop is optional-tolerant: a missing SizeBox or
	// ScrollBox degrades to a taller, wider panel rather than to no panel at all.
	if (PanelSizeBox != nullptr)
	{
		BackdropBorder->SetContent(PanelSizeBox);

		if (RootScrollBox != nullptr)
		{
			PanelSizeBox->SetContent(RootScrollBox);
			if (UScrollBoxSlot* ColumnSlot = Cast<UScrollBoxSlot>(RootScrollBox->AddChild(RootPanel)))
			{
				ColumnSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
				ColumnSlot->SetHorizontalAlignment(HAlign_Fill);
			}
		}
		else
		{
			PanelSizeBox->SetContent(RootPanel);
		}
	}
	else if (RootScrollBox != nullptr)
	{
		BackdropBorder->SetContent(RootScrollBox);
		if (UScrollBoxSlot* ColumnSlot = Cast<UScrollBoxSlot>(RootScrollBox->AddChild(RootPanel)))
		{
			ColumnSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
			ColumnSlot->SetHorizontalAlignment(HAlign_Fill);
		}
	}
	else
	{
		BackdropBorder->SetContent(RootPanel);
	}

	// ---- TitleText ----------------------------------------------------------
	if (TitleText == nullptr)
	{
		TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
		if (TitleText != nullptr)
		{
			TitleText->SetText(FText::FromString(FString(SiegeGraphicsMenuText::Title)));
			TitleText->SetFontSize(SiegeGraphicsMenuLayout::TitleFontSize);

			if (UVerticalBoxSlot* TitleSlot = RootPanel->AddChildToVerticalBox(TitleText))
			{
				TitleSlot->SetPadding(FMargin(24.f, 20.f, 24.f, 4.f));
				TitleSlot->SetHorizontalAlignment(HAlign_Center);
				TitleSlot->SetVerticalAlignment(VAlign_Top);
			}
		}
	}

	// ---- StatusText: the framing sentence, and the "why is this dead" surface
	if (StatusText == nullptr)
	{
		StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
		if (StatusText != nullptr)
		{
			StatusText->SetText(FText::FromString(FString(SiegeGraphicsMenuText::Purpose)));
			StatusText->SetFontSize(SiegeGraphicsMenuLayout::HintFontSize);
			StatusText->SetAutoWrapText(true);
			StatusText->SetColorAndOpacity(FSlateColor(SiegeGraphicsMenuLayout::HintColor));

			if (UVerticalBoxSlot* StatusSlot = RootPanel->AddChildToVerticalBox(StatusText))
			{
				StatusSlot->SetPadding(FMargin(24.f, 0.f, 24.f, 16.f));
				StatusSlot->SetHorizontalAlignment(HAlign_Fill);
				StatusSlot->SetVerticalAlignment(VAlign_Top);
			}
		}
	}

	// ---- TASK-1120: the readout + its in-match toggle (GFX-§7) ---------------
	// ⛔ FIRST, ABOVE AUTO-DETECT. The panel opens scrolled to the top, so the
	// number is the first thing on screen — which is the only placement that makes
	// "adjust for performance versus visual quality" a comparison rather than a
	// guess. It scrolls with the column; see the header for why that is acceptable
	// HERE and why the in-match counter is the half that actually matters.
	BuildFrameRateReadoutRows();

	// ---- Auto-Detect (GFX-§6) ------------------------------------------------
	BuildLabelledButtonRow(TEXT("AutoDetect"), AutoDetectButton, AutoDetectLabelText,
		FString(SiegeGraphicsMenuText::AutoDetect));
	BuildHintRow(TEXT("AutoDetectHintText"), AutoDetectHintText, FString(SiegeGraphicsMenuText::AutoDetectHint));

	// ---- Tier A: the overall preset -----------------------------------------
	BuildSliderRow(TEXT("OverallQuality"), OverallQualitySlider, OverallQualityLabelText, OverallQualityValueText,
		FString(SiegeGraphicsMenuText::OverallQuality),
		/*MinValue*/ 0.0f, /*MaxValue*/ 1.0f,
		/*StepSize*/ 1.0f / static_cast<float>(USiegeGraphicsSettingsSubsystem::MaxQualityLevel),
		/*bDetented*/ true);

	// ═════════════════════════════════════════════════════════════════════════
	//  ⭐ TIER B — TEN ROWS FROM ONE LOOP.
	//
	//  The list is the FACADE'S OWN GetQualityGroupNames() (TASK-1113 ships it
	//  precisely so this row loops instead of copy-pasting ten blocks), and it is
	//  in GFX-§8's Tier-B display order. ⛔ The panel never re-decides the list,
	//  never re-decides the spelling, and never adds an eleventh: a group added to
	//  the facade appears here for free, and a group removed there disappears here
	//  for free. That is the only shape in which the widget names and the ini
	//  section names cannot drift.
	// ═════════════════════════════════════════════════════════════════════════
	for (const FName& GroupName : USiegeGraphicsSettingsSubsystem::GetQualityGroupNames())
	{
		BuildQualityGroupRow(GroupName);
	}

	// ---- Tier C: the one genuinely continuous control ------------------------
	// GFX-§5: 50-100%, 1% steps, and ⛔ NOT detented — this is the control that
	// really is continuous, so faking detents here would be the mirror image of
	// faking continuity on the ten above.
	BuildSliderRow(TEXT("ResolutionScale"), ResolutionScaleSlider, ResolutionScaleLabelText, ResolutionScaleValueText,
		FString(SiegeGraphicsMenuText::ResolutionScale),
		USiegeGraphicsSettingsSubsystem::MinResolutionScalePercent,
		USiegeGraphicsSettingsSubsystem::MaxResolutionScalePercent,
		/*StepSize*/ 1.0f,
		/*bDetented*/ false);
	BuildHintRow(TEXT("ResolutionScaleHintText"), ResolutionScaleHintText, FString(SiegeGraphicsMenuText::ResolutionScaleHint));

	// ---- Tier C: the display steppers (GFX-§5: unordered sets, NEVER sliders) -
	BuildStepperRow(TEXT("ScreenResolution"), ScreenResolutionLabelText, ScreenResolutionValueText,
		ScreenResolutionPrevButton, ScreenResolutionNextButton, FString(SiegeGraphicsMenuText::ScreenResolution));

	BuildStepperRow(TEXT("WindowMode"), WindowModeLabelText, WindowModeValueText,
		WindowModePrevButton, WindowModeNextButton, FString(SiegeGraphicsMenuText::WindowMode));

	BuildHintRow(TEXT("DisplayHintText"), DisplayHintText, FString(SiegeGraphicsMenuText::DisplayHint));

	// ---- TASK-1118: GFX-§4's revert prompt, DIRECTLY under the two steppers ---
	// ⛔ THE POSITION IS A DECISION, NOT A LAYOUT ACCIDENT. The panel scrolls (19
	// rows do not fit at 1080p) and the display steppers are two thirds of the way
	// down, so a prompt at the top or bottom of the column would open OFF SCREEN
	// for the player who just pressed the stepper. Sited here it appears exactly
	// where their eyes already are — and ArmVideoModeCountdown() additionally
	// scrolls it into view, which covers the case where the player scrolled away
	// between the press and the prompt.
	BuildVideoModeConfirmRow();

	// ---- Tier C: VSync -------------------------------------------------------
	if (VSyncLabelText == nullptr)
	{
		VSyncLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("VSyncLabelText"));
		if (VSyncLabelText != nullptr)
		{
			VSyncLabelText->SetText(FText::FromString(FString(SiegeGraphicsMenuText::VSync)));
			VSyncLabelText->SetFontSize(SiegeGraphicsMenuLayout::RowFontSize);
			VSyncLabelText->SetAutoWrapText(true);
		}
	}

	if (VSyncCheckBox == nullptr)
	{
		VSyncCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("VSyncCheckBox"));
		if (VSyncCheckBox != nullptr)
		{
			// The label is the CHECK BOX'S CONTENT rather than a sibling — the
			// USettingsMenuWidget idiom. It makes the words part of the click
			// target, and disabling the box greys the label with it.
			if (VSyncLabelText != nullptr)
			{
				VSyncCheckBox->SetContent(VSyncLabelText);
			}

			if (UVerticalBoxSlot* VSyncSlot = RootPanel->AddChildToVerticalBox(VSyncCheckBox))
			{
				VSyncSlot->SetPadding(FMargin(24.f, 10.f, 24.f, 6.f));
				VSyncSlot->SetHorizontalAlignment(HAlign_Left);
				VSyncSlot->SetVerticalAlignment(VAlign_Top);
			}
		}
	}

	// ---- Tier C: the frame-rate ladder ---------------------------------------
	BuildStepperRow(TEXT("FrameRateLimit"), FrameRateLimitLabelText, FrameRateLimitValueText,
		FrameRateLimitPrevButton, FrameRateLimitNextButton, FString(SiegeGraphicsMenuText::FrameRateLimit));

	// ---- BackButton ----------------------------------------------------------
	BuildLabelledButtonRow(TEXT("Back"), BackButton, BackLabelText, FString(SiegeGraphicsMenuText::Back));

	if (BackButton == nullptr)
	{
		// This one IS worth an Error: without Back the only way out of a modal
		// backdrop is to quit the game.
		UE_LOG(LogSiegeGraphics, Error,
			TEXT("[GraphicsMenu] Could not construct BackButton — the graphics panel cannot be dismissed from itself."));
	}

	UE_LOG(LogSiegeGraphics, Log,
		TEXT("[GraphicsMenu] Code-authored tree built: %d quality-group rows + preset + resolution scale + 3 steppers + VSync + Auto-Detect + Back."),
		USiegeGraphicsSettingsSubsystem::GetQualityGroupNames().Num());
}

// ─────────────────────────────────────────────────────────────────────────────
//  ⭐ THE GROUP-ROW RESOLVER — the ONE place a canonical FName is paired with
//  its three UPROPERTY members.
//
//  ⚠️ THE MACRO IS DELIBERATE AND IT IS A CORRECTNESS DEVICE, NOT BREVITY. The
//  group constant and all three member names are produced by TOKEN-PASTING THE
//  SAME IDENTIFIER, so `ShadowQuality` cannot be paired with
//  `ReflectionQualityValueText` — the pairing is not written down anywhere to get
//  wrong. Ten hand-written five-line blocks would put thirty opportunities for a
//  silent mis-wire in a file nobody re-reads, and a mis-wired row is invisible:
//  the slider moves, a setting changes, and it is the WRONG setting.
//  ⛔ The macro is #undef'd immediately below; it does not escape this function.
// ─────────────────────────────────────────────────────────────────────────────
USiegeGraphicsMenuWidget::FGroupRowWidgets USiegeGraphicsMenuWidget::ResolveGroupRow(FName GroupName)
{
	FGroupRowWidgets Out;

#define SIEGE_GFX_GROUP_ROW(GroupToken)                                                       \
	if (GroupName == USiegeGraphicsSettingsSubsystem::GroupName_##GroupToken)                 \
	{                                                                                         \
		Out.Slider = &GroupToken##Slider;                                                     \
		Out.Label  = &GroupToken##LabelText;                                                  \
		Out.Value  = &GroupToken##ValueText;                                                  \
		return Out;                                                                           \
	}

	SIEGE_GFX_GROUP_ROW(ViewDistanceQuality)
	SIEGE_GFX_GROUP_ROW(AntiAliasingQuality)
	SIEGE_GFX_GROUP_ROW(ShadowQuality)
	SIEGE_GFX_GROUP_ROW(GlobalIlluminationQuality)
	SIEGE_GFX_GROUP_ROW(ReflectionQuality)
	SIEGE_GFX_GROUP_ROW(PostProcessQuality)
	SIEGE_GFX_GROUP_ROW(TextureQuality)
	SIEGE_GFX_GROUP_ROW(EffectsQuality)
	SIEGE_GFX_GROUP_ROW(FoliageQuality)
	SIEGE_GFX_GROUP_ROW(ShadingQuality)

#undef SIEGE_GFX_GROUP_ROW

	// An unknown name is a programming error, not a player action: the facade's
	// GetQualityGroupNames() is the only list this file ever iterates, so reaching
	// here means the two went out of step.
	UE_LOG(LogSiegeGraphics, Warning,
		TEXT("[GraphicsMenu] ResolveGroupRow('%s') found no row — the panel and USiegeGraphicsSettingsSubsystem::GetQualityGroupNames() disagree."),
		*GroupName.ToString());
	return Out;
}

void USiegeGraphicsMenuWidget::BuildQualityGroupRow(FName GroupName)
{
	if (WidgetTree == nullptr || RootPanel == nullptr)
	{
		return;
	}

	FGroupRowWidgets Row = ResolveGroupRow(GroupName);
	if (!Row.IsComplete())
	{
		return;
	}

	const FString GroupString = GroupName.ToString();

	// ⛔ THE WIDGET NAMES ARE DERIVED FROM THE CANONICAL GROUP NAME, never typed.
	// That is what makes GFX-§10's triple (`<Group>Slider` / `<Group>LabelText` /
	// `<Group>ValueText`) true BY CONSTRUCTION for all ten rows, and it is what an
	// asset-authored WBP_GraphicsMenu would have to match.
	BuildSliderRow(FName(*GroupString), *Row.Slider, *Row.Label, *Row.Value,
		USiegeGraphicsSettingsSubsystem::GetQualityGroupDisplayName(GroupName).ToString(),
		/*MinValue*/ 0.0f, /*MaxValue*/ 1.0f,
		/*StepSize*/ 1.0f / static_cast<float>(USiegeGraphicsSettingsSubsystem::MaxQualityLevel),
		/*bDetented*/ true);

	// The two hint lines this lane owes, both attached to the row they describe.
	// ComposeGroupHint returns an empty string for the other eight groups, so this
	// stays inside the loop instead of becoming two special cases outside it.
	if (GroupName == USiegeGraphicsSettingsSubsystem::GroupName_ShadowQuality)
	{
		BuildHintRow(TEXT("ShadowQualityHintText"), ShadowQualityHintText,
			ComposeGroupHint(GroupName, /*bVolumetricFogEnabled*/ true));
	}
	else if (GroupName == USiegeGraphicsSettingsSubsystem::GroupName_FoliageQuality)
	{
		BuildHintRow(TEXT("FoliageQualityHintText"), FoliageQualityHintText,
			ComposeGroupHint(GroupName, /*bVolumetricFogEnabled*/ true));
	}
}

void USiegeGraphicsMenuWidget::BuildSliderRow(
	FName BaseName,
	TObjectPtr<USlider>& OutSlider,
	TObjectPtr<UTextBlock>& OutLabel,
	TObjectPtr<UTextBlock>& OutValue,
	const FString& LabelText,
	float MinValue,
	float MaxValue,
	float StepSize,
	bool bDetented)
{
	if (WidgetTree == nullptr || RootPanel == nullptr)
	{
		return;
	}

	const FString Base = BaseName.ToString();

	UHorizontalBox* RowBox = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(), FName(*(Base + TEXT("Row"))));
	if (RowBox == nullptr)
	{
		return;
	}

	if (UVerticalBoxSlot* RowSlot = RootPanel->AddChildToVerticalBox(RowBox))
	{
		RowSlot->SetPadding(FMargin(24.f, 6.f, 24.f, 2.f));
		RowSlot->SetHorizontalAlignment(HAlign_Fill);
		RowSlot->SetVerticalAlignment(VAlign_Top);
	}

	// ---- label ----
	if (OutLabel == nullptr)
	{
		OutLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*(Base + TEXT("LabelText"))));
	}
	if (OutLabel != nullptr)
	{
		OutLabel->SetText(FText::FromString(LabelText));
		OutLabel->SetFontSize(SiegeGraphicsMenuLayout::RowFontSize);
		OutLabel->SetMinDesiredWidth(SiegeGraphicsMenuLayout::LabelMinWidth);

		if (UHorizontalBoxSlot* LabelSlot = RowBox->AddChildToHorizontalBox(OutLabel))
		{
			LabelSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
			LabelSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	// ---- slider ----
	if (OutSlider == nullptr)
	{
		OutSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), FName(*(Base + TEXT("Slider"))));
	}
	if (OutSlider != nullptr)
	{
		OutSlider->SetMinValue(MinValue);
		OutSlider->SetMaxValue(MaxValue);
		OutSlider->SetStepSize(StepSize);

		// ═════════════════════════════════════════════════════════════════════
		// 🚨 MEASURED AT ENGINE SOURCE, AND IT IS THE DIFFERENCE BETWEEN GFX-§5
		//    BEING TRUE AND GFX-§5 BEING A COMMENT.
		//
		//    USlider::MouseUsesStep DEFAULTS TO **false** (Slider.cpp:27), and
		//    SSlider::PositionToValue only snaps to StepSize INSIDE
		//    `if (bMouseUsesStep)` (SSlider.cpp:455-470). ⇒ StepSize ALONE affects
		//    keyboard / gamepad navigation ONLY. A mouse drag on a default USlider
		//    returns a CONTINUOUS value, so a "5-detent" quality slider built from
		//    StepSize 0.25 and nothing else would glide smoothly through 0.37 and
		//    0.61 — ⛔ EXACTLY the "fake continuity over 5 states" GFX-§5 forbids,
		//    and it would look correct in every code review and every property
		//    read-back.
		//
		//    ⛔ DO NOT DELETE THIS LINE. There is no public setter (MouseUsesStep
		//    is a plain public UPROPERTY with no Setter=), so this direct
		//    assignment is the whole mechanism. It is read by
		//    USlider::SynchronizeProperties (Slider.cpp:76) when the Slate widget
		//    is built, which happens AFTER this function — assigning here is in
		//    time.
		//
		//    ⚠️ FALSE for the resolution-scale bar ON PURPOSE: that one really is
		//    continuous (GFX-§5), so snapping it would be the same lie in reverse.
		// ═════════════════════════════════════════════════════════════════════
		OutSlider->MouseUsesStep = bDetented;

		if (UHorizontalBoxSlot* SliderSlot = RowBox->AddChildToHorizontalBox(OutSlider))
		{
			SliderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			SliderSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
			SliderSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	// ---- live value ----
	// ⛔ EVERY slider carries one. A detented slider with no label is unreadable:
	// the handle sits at "somewhere between the third and fourth notch" and the
	// player has no way to know that means "High".
	if (OutValue == nullptr)
	{
		OutValue = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*(Base + TEXT("ValueText"))));
	}
	if (OutValue != nullptr)
	{
		OutValue->SetFontSize(SiegeGraphicsMenuLayout::RowFontSize);
		OutValue->SetMinDesiredWidth(SiegeGraphicsMenuLayout::ValueMinWidth);
		OutValue->SetColorAndOpacity(FSlateColor(SiegeGraphicsMenuLayout::ValueColor));

		if (UHorizontalBoxSlot* ValueSlot = RowBox->AddChildToHorizontalBox(OutValue))
		{
			ValueSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
}

void USiegeGraphicsMenuWidget::BuildStepperRow(
	FName BaseName,
	TObjectPtr<UTextBlock>& OutLabel,
	TObjectPtr<UTextBlock>& OutValue,
	TObjectPtr<UButton>& OutPrev,
	TObjectPtr<UButton>& OutNext,
	const FString& LabelText)
{
	if (WidgetTree == nullptr || RootPanel == nullptr)
	{
		return;
	}

	const FString Base = BaseName.ToString();

	UHorizontalBox* RowBox = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(), FName(*(Base + TEXT("Row"))));
	if (RowBox == nullptr)
	{
		return;
	}

	if (UVerticalBoxSlot* RowSlot = RootPanel->AddChildToVerticalBox(RowBox))
	{
		RowSlot->SetPadding(FMargin(24.f, 6.f, 24.f, 2.f));
		RowSlot->SetHorizontalAlignment(HAlign_Fill);
		RowSlot->SetVerticalAlignment(VAlign_Top);
	}

	if (OutLabel == nullptr)
	{
		OutLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*(Base + TEXT("LabelText"))));
	}
	if (OutLabel != nullptr)
	{
		OutLabel->SetText(FText::FromString(LabelText));
		OutLabel->SetFontSize(SiegeGraphicsMenuLayout::RowFontSize);
		OutLabel->SetMinDesiredWidth(SiegeGraphicsMenuLayout::LabelMinWidth);

		if (UHorizontalBoxSlot* LabelSlot = RowBox->AddChildToHorizontalBox(OutLabel))
		{
			LabelSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
			LabelSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	// "<" — a stepper, not a slider: GFX-§5 rules resolution, window mode and the
	// frame-rate ladder UNORDERED or UNEVENLY SPACED sets, where a slider would
	// misrepresent the distance between two adjacent options.
	OutPrev = BuildStepButton(FName(*(Base + TEXT("PrevButton"))), OutPrev, TEXT("<"), RowBox);

	if (OutValue == nullptr)
	{
		OutValue = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*(Base + TEXT("ValueText"))));
	}
	if (OutValue != nullptr)
	{
		OutValue->SetFontSize(SiegeGraphicsMenuLayout::RowFontSize);
		OutValue->SetMinDesiredWidth(SiegeGraphicsMenuLayout::ValueMinWidth);
		OutValue->SetJustification(ETextJustify::Center);
		OutValue->SetColorAndOpacity(FSlateColor(SiegeGraphicsMenuLayout::ValueColor));

		if (UHorizontalBoxSlot* ValueSlot = RowBox->AddChildToHorizontalBox(OutValue))
		{
			ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			ValueSlot->SetVerticalAlignment(VAlign_Center);
			ValueSlot->SetHorizontalAlignment(HAlign_Center);
		}
	}

	OutNext = BuildStepButton(FName(*(Base + TEXT("NextButton"))), OutNext, TEXT(">"), RowBox);
}

UButton* USiegeGraphicsMenuWidget::BuildStepButton(FName ButtonName, UButton* ExistingButton, const TCHAR* Glyph, UHorizontalBox* RowBox)
{
	if (ExistingButton != nullptr || WidgetTree == nullptr || RowBox == nullptr)
	{
		// Condition (b): an asset-authored button wins whole and is never rebuilt.
		return ExistingButton;
	}

	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
	if (Button == nullptr)
	{
		return nullptr;
	}

	// The glyph is deliberately NOT a pinned member: it carries no state, is never
	// re-read, and adding two more BindWidgetOptional names per stepper would put
	// six unpinned names in GFX-§10's way for no gain.
	if (UTextBlock* Glyphs = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), FName(*(ButtonName.ToString() + TEXT("Glyph")))))
	{
		Glyphs->SetText(FText::FromString(FString(Glyph)));
		Glyphs->SetFontSize(SiegeGraphicsMenuLayout::RowFontSize);
		Glyphs->SetJustification(ETextJustify::Center);

		if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(Button->SetContent(Glyphs)))
		{
			ContentSlot->SetPadding(FMargin(14.f, 6.f, 14.f, 6.f));
			ContentSlot->SetHorizontalAlignment(HAlign_Center);
			ContentSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	if (UHorizontalBoxSlot* ButtonSlot = RowBox->AddChildToHorizontalBox(Button))
	{
		ButtonSlot->SetPadding(FMargin(4.f, 0.f, 4.f, 0.f));
		ButtonSlot->SetVerticalAlignment(VAlign_Center);
	}

	return Button;
}

void USiegeGraphicsMenuWidget::BuildLabelledButtonRow(
	FName BaseName,
	TObjectPtr<UButton>& OutButton,
	TObjectPtr<UTextBlock>& OutLabel,
	const FString& LabelText)
{
	if (WidgetTree == nullptr || RootPanel == nullptr)
	{
		return;
	}

	const FString Base = BaseName.ToString();

	if (OutLabel == nullptr)
	{
		OutLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*(Base + TEXT("LabelText"))));
		if (OutLabel != nullptr)
		{
			OutLabel->SetText(FText::FromString(LabelText));
			OutLabel->SetFontSize(SiegeGraphicsMenuLayout::ButtonFontSize);
		}
	}

	if (OutButton == nullptr)
	{
		OutButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(*(Base + TEXT("Button"))));
		if (OutButton != nullptr)
		{
			if (OutLabel != nullptr)
			{
				// Geometry lifted from the shipped WBP_MainMenu / USettingsMenuWidget
				// button idiom (font 28, MakeMargin(24,12,24,12), HAlign_Fill) rather
				// than invented, so the three screens read as one family.
				if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(OutButton->SetContent(OutLabel)))
				{
					ContentSlot->SetPadding(FMargin(24.f, 12.f, 24.f, 12.f));
					ContentSlot->SetHorizontalAlignment(HAlign_Center);
					ContentSlot->SetVerticalAlignment(VAlign_Center);
				}
			}

			if (UVerticalBoxSlot* ButtonSlot = RootPanel->AddChildToVerticalBox(OutButton))
			{
				ButtonSlot->SetPadding(FMargin(24.f, 12.f, 24.f, 8.f));
				ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
				ButtonSlot->SetVerticalAlignment(VAlign_Top);
			}
		}
	}
}

void USiegeGraphicsMenuWidget::BuildHintRow(FName HintName, TObjectPtr<UTextBlock>& OutHint, const FString& HintText)
{
	if (WidgetTree == nullptr || RootPanel == nullptr || HintText.IsEmpty())
	{
		return;
	}

	if (OutHint == nullptr)
	{
		OutHint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), HintName);
	}
	if (OutHint == nullptr)
	{
		return;
	}

	OutHint->SetText(FText::FromString(HintText));
	OutHint->SetFontSize(SiegeGraphicsMenuLayout::HintFontSize);
	OutHint->SetAutoWrapText(true);
	OutHint->SetColorAndOpacity(FSlateColor(SiegeGraphicsMenuLayout::HintColor));

	if (UVerticalBoxSlot* HintSlot = RootPanel->AddChildToVerticalBox(OutHint))
	{
		HintSlot->SetPadding(FMargin(40.f, 0.f, 24.f, 10.f));
		HintSlot->SetHorizontalAlignment(HAlign_Fill);
		HintSlot->SetVerticalAlignment(VAlign_Top);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  TASK-1120 — THE READOUT ROW + THE IN-MATCH TOGGLE
// ─────────────────────────────────────────────────────────────────────────────

void USiegeGraphicsMenuWidget::BuildFrameRateReadoutRows()
{
	if (WidgetTree == nullptr || RootPanel == nullptr)
	{
		return;
	}

	// ---- FrameRateReadoutText (GFX-§10 pinned) ------------------------------
	if (FrameRateReadoutText == nullptr)
	{
		FrameRateReadoutText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FrameRateReadoutText"));
		if (FrameRateReadoutText != nullptr)
		{
			// ⛔ SEEDED WITH THE PENDING TEXT, NEVER WITH A NUMBER. The first real
			// reading is half a second away (the window has to close before there is
			// anything to divide), and a fabricated starting value would be
			// indistinguishable from a measurement for that half second.
			FrameRateReadoutText->SetText(FText::FromString(FSiegeFrameRateSample::ComposePendingText()));
			FrameRateReadoutText->SetFontSize(SiegeGraphicsMenuLayout::ButtonFontSize);
			FrameRateReadoutText->SetColorAndOpacity(FSlateColor(SiegeGraphicsMenuLayout::ValueColor));

			if (UVerticalBoxSlot* ReadoutSlot = RootPanel->AddChildToVerticalBox(FrameRateReadoutText))
			{
				ReadoutSlot->SetPadding(FMargin(24.f, 0.f, 24.f, 2.f));
				ReadoutSlot->SetHorizontalAlignment(HAlign_Center);
				ReadoutSlot->SetVerticalAlignment(VAlign_Top);
			}
		}
	}

	// ---- The in-match opt-in ------------------------------------------------
	if (ShowFrameRateCounterLabelText == nullptr)
	{
		ShowFrameRateCounterLabelText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("ShowFrameRateCounterLabelText"));
		if (ShowFrameRateCounterLabelText != nullptr)
		{
			ShowFrameRateCounterLabelText->SetText(FText::FromString(FString(SiegeGraphicsMenuText::ShowFrameRateCounter)));
			ShowFrameRateCounterLabelText->SetFontSize(SiegeGraphicsMenuLayout::RowFontSize);
			ShowFrameRateCounterLabelText->SetAutoWrapText(true);
		}
	}

	if (ShowFrameRateCounterCheckBox == nullptr)
	{
		ShowFrameRateCounterCheckBox = WidgetTree->ConstructWidget<UCheckBox>(
			UCheckBox::StaticClass(), TEXT("ShowFrameRateCounterCheckBox"));
		if (ShowFrameRateCounterCheckBox != nullptr)
		{
			// The label is the check box's CONTENT, not a sibling — the VSync row's
			// idiom, cloned: the words join the click target and grey out with the box.
			if (ShowFrameRateCounterLabelText != nullptr)
			{
				ShowFrameRateCounterCheckBox->SetContent(ShowFrameRateCounterLabelText);
			}

			if (UVerticalBoxSlot* ToggleSlot = RootPanel->AddChildToVerticalBox(ShowFrameRateCounterCheckBox))
			{
				ToggleSlot->SetPadding(FMargin(24.f, 2.f, 24.f, 2.f));
				ToggleSlot->SetHorizontalAlignment(HAlign_Left);
				ToggleSlot->SetVerticalAlignment(VAlign_Top);
			}
		}
	}

	BuildHintRow(TEXT("ShowFrameRateCounterHintText"), ShowFrameRateCounterHintText,
		FString(SiegeGraphicsMenuText::ShowFrameRateCounterHint));
}

// ─────────────────────────────────────────────────────────────────────────────
//  TASK-1118 — THE REVERT PROMPT'S TREE
// ─────────────────────────────────────────────────────────────────────────────

void USiegeGraphicsMenuWidget::BuildVideoModeConfirmRow()
{
	if (WidgetTree == nullptr || RootPanel == nullptr)
	{
		return;
	}

	if (VideoModeConfirmBorder == nullptr)
	{
		VideoModeConfirmBorder = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("VideoModeConfirmBorder"));
	}

	if (VideoModeConfirmBorder == nullptr)
	{
		// ⛔ AN ERROR, NOT A WARNING, AND THEN THE PANEL CARRIES ON. The countdown
		// itself does not need this widget — the timer reverts with no prompt at
		// all (board cl. 2) — but a player who CAN see the screen would be given
		// no way to say "keep" and would lose a mode that works. Worth a loud line
		// and worth not being fatal.
		UE_LOG(LogSiegeGraphics, Error,
			TEXT("[GraphicsMenu] Could not construct VideoModeConfirmBorder — a display change will still auto-revert after %.0f seconds, but there is no Keep button to stop it."),
			VideoModeConfirmSeconds);
		return;
	}

	// ⛔ COLLAPSED, NOT HIDDEN. Collapsed takes no layout space, so the panel does
	// not carry a ten-line gap under the steppers for the 99 % of the time no
	// confirmation is running. WRITER 1 OF 4 (see the header).
	VideoModeConfirmBorder->SetVisibility(ESlateVisibility::Collapsed);
	VideoModeConfirmBorder->SetBrushColor(FLinearColor(0.16f, 0.10f, 0.02f, 0.96f));
	VideoModeConfirmBorder->SetPadding(FMargin(20.f, 14.f, 20.f, 14.f));
	VideoModeConfirmBorder->SetHorizontalAlignment(HAlign_Fill);

	if (UVerticalBoxSlot* BorderSlot = RootPanel->AddChildToVerticalBox(VideoModeConfirmBorder))
	{
		BorderSlot->SetPadding(FMargin(24.f, 4.f, 24.f, 10.f));
		BorderSlot->SetHorizontalAlignment(HAlign_Fill);
		BorderSlot->SetVerticalAlignment(VAlign_Top);
	}

	UVerticalBox* PromptColumn = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("VideoModeConfirmColumn"));
	if (PromptColumn == nullptr)
	{
		return;
	}
	VideoModeConfirmBorder->SetContent(PromptColumn);

	if (VideoModeConfirmText == nullptr)
	{
		VideoModeConfirmText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("VideoModeConfirmText"));
	}
	if (VideoModeConfirmText != nullptr)
	{
		// Seeded with the FULL count rather than an empty string: an asset-authored
		// tree that binds this name and never runs a countdown still reads as a
		// sentence, and the very first repaint has something to replace.
		VideoModeConfirmText->SetText(FText::FromString(
			ComposeVideoModeCountdownText(FMath::RoundToInt(VideoModeConfirmSeconds))));
		VideoModeConfirmText->SetFontSize(SiegeGraphicsMenuLayout::RowFontSize);
		VideoModeConfirmText->SetAutoWrapText(true);

		if (UVerticalBoxSlot* TextSlot = PromptColumn->AddChildToVerticalBox(VideoModeConfirmText))
		{
			TextSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
			TextSlot->SetHorizontalAlignment(HAlign_Fill);
		}
	}

	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(), TEXT("VideoModeConfirmButtonRow"));
	if (ButtonRow == nullptr)
	{
		return;
	}

	if (UVerticalBoxSlot* ButtonRowSlot = PromptColumn->AddChildToVerticalBox(ButtonRow))
	{
		ButtonRowSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	// ⛔ KEEP FIRST, REVERT SECOND. Revert is what happens if the player does
	// nothing at all, so it is the outcome that needs no encouragement; Keep is
	// the deliberate act and gets the reading position.
	BuildConfirmButton(TEXT("KeepSettings"), KeepSettingsButton, KeepSettingsLabelText,
		FString(SiegeGraphicsMenuText::KeepSettings), ButtonRow);
	BuildConfirmButton(TEXT("RevertSettings"), RevertSettingsButton, RevertSettingsLabelText,
		FString(SiegeGraphicsMenuText::RevertSettings), ButtonRow);
}

UButton* USiegeGraphicsMenuWidget::BuildConfirmButton(
	FName BaseName,
	TObjectPtr<UButton>& OutButton,
	TObjectPtr<UTextBlock>& OutLabel,
	const FString& LabelText,
	UHorizontalBox* RowBox)
{
	if (WidgetTree == nullptr || RowBox == nullptr)
	{
		return OutButton;
	}

	const FString Base = BaseName.ToString();

	if (OutLabel == nullptr)
	{
		OutLabel = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), FName(*(Base + TEXT("LabelText"))));
		if (OutLabel != nullptr)
		{
			OutLabel->SetText(FText::FromString(LabelText));
			OutLabel->SetFontSize(SiegeGraphicsMenuLayout::ButtonFontSize);
		}
	}

	if (OutButton != nullptr)
	{
		// Condition (b): an asset-authored button wins whole and is never rebuilt
		// or re-parented.
		return OutButton;
	}

	OutButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(*(Base + TEXT("Button"))));
	if (OutButton == nullptr)
	{
		return nullptr;
	}

	if (OutLabel != nullptr)
	{
		if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(OutButton->SetContent(OutLabel)))
		{
			ContentSlot->SetPadding(FMargin(24.f, 10.f, 24.f, 10.f));
			ContentSlot->SetHorizontalAlignment(HAlign_Center);
			ContentSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	if (UHorizontalBoxSlot* ButtonSlot = RowBox->AddChildToHorizontalBox(OutButton))
	{
		ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ButtonSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
		ButtonSlot->SetVerticalAlignment(VAlign_Center);
	}

	return OutButton;
}

// ═════════════════════════════════════════════════════════════════════════════
//  LIFECYCLE
// ═════════════════════════════════════════════════════════════════════════════

void USiegeGraphicsMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Everything is bound HERE and not in NativeOnInitialized, for the mechanical
	// reason USettingsMenuWidget records: the code-authored children do not exist
	// until RebuildWidget() runs, and the engine order is
	//   Initialize() -> NativeOnInitialized() -> RebuildWidget() -> NativeConstruct().
	// Binding earlier would silently bind NOTHING on the code-authored path while
	// working fine on a future WBP path — a difference that would only ever show
	// up as a dead panel in a build nobody could reproduce.
	SeedAndBind();

	// ⭐ TASK-1120: AFTER the seed, because ArmFrameRateReadout() opens the first
	// measurement window and the window should start when the panel is on screen,
	// not while it is still being wired. No world (automation) ⇒ no timer, and the
	// readout keeps its pending text — never a crash and never a fabricated number.
	ArmFrameRateReadout();
}

void USiegeGraphicsMenuWidget::NativeDestruct()
{
	UnbindAll();

	// ⭐ TASK-1120: stop the readout timer BEFORE anything else here, for the same
	// mechanical reason as the countdown's disarm below — a 0.5 s callback can
	// already be queued at a widget that is being torn down. It reverts nothing
	// and stages nothing; it only stops a SetText arriving at a dead text block.
	DisarmFrameRateReadout();

	// TASK-1118: stop the timer BEFORE the discard. This is not a revert and does
	// not affect the count below — it stops a callback from arriving at a widget
	// that is being torn down. ⛔ Order matters only in that the disarm must not
	// come after the object is gone; the revert arithmetic is the guard's, not
	// this line's.
	DisarmVideoModeCountdown();

	// 🚨 board cl. (3b-WIDENED). ANY teardown — Back, a level change, the owning
	// panel being removed — must close a staged video-mode window, or the facade
	// refuses every save AND Auto-Detect for the rest of the session with nothing
	// on screen to explain it. This is the SECOND of the two TEARDOWN call sites
	// and it is the catch-all; on the Back path it is a guarded no-op because
	// BackPressed() already closed the window. See DiscardStagedVideoMode.
	DiscardStagedVideoMode(ResolveGraphicsSubsystem());

	Super::NativeDestruct();
}

void USiegeGraphicsMenuWidget::SeedAndBind()
{
	// ------------------------------------------------------------------------
	// SEED, **THEN** BIND — and the order is the point (qa/TASK-005 major-2: a
	// control that is only BOUND, holding whatever value it was constructed with,
	// shows a stale value until something happens to change it).
	//
	// 🚨 AND HERE THE UNBIND-FIRST IS NOT BELT-AND-BRACES, IT IS REQUIRED.
	// MEASURED: USlider::SetValue(float) CALLS HandleOnValueChanged(InValue) and
	// therefore BROADCASTS OnValueChanged (Slider.cpp, USlider::SetValue) — unlike
	// UCheckBox::SetIsChecked, which does not. So a seed re-enters our own
	// handler. It would be harmless today (the OnValueChanged thunks only touch
	// labels) but it is one edit away from a seed that writes what it read, so the
	// delegates are off while we push.
	// ------------------------------------------------------------------------
	UnbindAll();

	// ⛔ WRITER 2 OF 4 on the prompt's visibility, and it is the ONLY ONE AN
	// ASSET-AUTHORED TREE EVER REACHES (gate qa/TASK-1119.md NIT-3). Writer 1
	// lives inside BuildVideoModeConfirmRow, which ConstructGraphicsTree's
	// condition-(b) guard skips WHOLESALE when WidgetTree->RootWidget is already
	// set — so a future WBP_GraphicsMenu binding the four GFX-§10 pinned names
	// would open with "Keep these settings? Reverting in 10 seconds." on screen
	// and no countdown running: SC-§94 cl. A, latent until the asset is authored.
	// SeedAndBind runs on BOTH tree paths and has exactly one caller
	// (NativeConstruct), so nothing live can be hidden by this line — a countdown
	// cannot already be running when the panel is being constructed.
	if (VideoModeConfirmBorder != nullptr)
	{
		VideoModeConfirmBorder->SetVisibility(ESlateVisibility::Collapsed);
	}

	USiegeGraphicsSettingsSubsystem* Graphics = ResolveGraphicsSubsystem();
	if (Graphics == nullptr)
	{
		ShowPanelUnavailable();
	}
	else
	{
		SetAllControlsEnabled(true);
		if (StatusText != nullptr)
		{
			StatusText->SetText(FText::FromString(FString(SiegeGraphicsMenuText::Purpose)));
		}

		RefreshAllRows();
		Graphics->OnGraphicsSettingsChanged.AddUniqueDynamic(this, &USiegeGraphicsMenuWidget::HandleGraphicsSettingsChanged);
	}

	// ---- bind AFTER the seed ------------------------------------------------
	for (const FName& GroupName : USiegeGraphicsSettingsSubsystem::GetQualityGroupNames())
	{
		if (USlider* Slider = FindGroupSlider(GroupName))
		{
			// ⛔ board cl. (3a): the VALUE-CHANGED delegate updates the LABEL and
			// nothing else; the write happens on capture end. Both capture-end
			// events are bound because a gamepad / keyboard user never produces a
			// MOUSE capture end — SSlider commits on OnFocusLost and then fires
			// OnControllerCaptureEnd (SSlider.cpp:280-289), so binding only the
			// mouse event would silently drop every pad-driven change.
			Slider->OnValueChanged.AddUniqueDynamic(this, &USiegeGraphicsMenuWidget::HandleGroupSliderValueChanged);
			Slider->OnMouseCaptureEnd.AddUniqueDynamic(this, &USiegeGraphicsMenuWidget::HandleGroupSliderCommitted);
			Slider->OnControllerCaptureEnd.AddUniqueDynamic(this, &USiegeGraphicsMenuWidget::HandleGroupSliderCommitted);
		}
	}

	if (OverallQualitySlider != nullptr)
	{
		OverallQualitySlider->OnValueChanged.AddUniqueDynamic(this, &USiegeGraphicsMenuWidget::HandleOverallSliderValueChanged);
		OverallQualitySlider->OnMouseCaptureEnd.AddUniqueDynamic(this, &USiegeGraphicsMenuWidget::HandleOverallSliderCommitted);
		OverallQualitySlider->OnControllerCaptureEnd.AddUniqueDynamic(this, &USiegeGraphicsMenuWidget::HandleOverallSliderCommitted);
	}

	if (ResolutionScaleSlider != nullptr)
	{
		ResolutionScaleSlider->OnValueChanged.AddUniqueDynamic(this, &USiegeGraphicsMenuWidget::HandleResolutionScaleValueChanged);
		ResolutionScaleSlider->OnMouseCaptureEnd.AddUniqueDynamic(this, &USiegeGraphicsMenuWidget::HandleResolutionScaleCommitted);
		ResolutionScaleSlider->OnControllerCaptureEnd.AddUniqueDynamic(this, &USiegeGraphicsMenuWidget::HandleResolutionScaleCommitted);
	}

	if (VSyncCheckBox != nullptr)
	{
		VSyncCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &USiegeGraphicsMenuWidget::HandleVSyncChanged);
	}

	// ⭐ TASK-1120. ⛔ SEEDED **BEFORE** IT IS BOUND, and by its own function rather
	// than by RefreshAllRows(): this row reads USiegeSettingsSubsystem, and
	// RefreshAllRows early-returns when the GRAPHICS facade is missing. Seeding it
	// there would mean one subsystem's absence silently blanked the other's row.
	RefreshFrameRateCounterRow();

	if (ShowFrameRateCounterCheckBox != nullptr)
	{
		ShowFrameRateCounterCheckBox->OnCheckStateChanged.AddUniqueDynamic(
			this, &USiegeGraphicsMenuWidget::HandleShowFrameRateCounterChanged);
	}

	// ⭐ TASK-1120: the OTHER subsystem's delegate. Subscribed so a value changed
	// elsewhere (another panel, a profile switch through ACC-§7's reload) reaches
	// this checkbox. Unlike its graphics sibling, HandleSettingsChanged FILTERS on
	// the token — that store carries a setting this panel does not own.
	if (USiegeSettingsSubsystem* Settings = ResolveSettingsSubsystem())
	{
		Settings->OnSettingsChanged.AddUniqueDynamic(this, &USiegeGraphicsMenuWidget::HandleSettingsChanged);
	}

	if (AutoDetectButton != nullptr)
	{
		AutoDetectButton->OnClicked.AddUniqueDynamic(this, &USiegeGraphicsMenuWidget::HandleAutoDetectClicked);
	}
	if (ScreenResolutionPrevButton != nullptr)
	{
		ScreenResolutionPrevButton->OnClicked.AddUniqueDynamic(this, &USiegeGraphicsMenuWidget::HandleScreenResolutionPrevClicked);
	}
	if (ScreenResolutionNextButton != nullptr)
	{
		ScreenResolutionNextButton->OnClicked.AddUniqueDynamic(this, &USiegeGraphicsMenuWidget::HandleScreenResolutionNextClicked);
	}
	if (WindowModePrevButton != nullptr)
	{
		WindowModePrevButton->OnClicked.AddUniqueDynamic(this, &USiegeGraphicsMenuWidget::HandleWindowModePrevClicked);
	}
	if (WindowModeNextButton != nullptr)
	{
		WindowModeNextButton->OnClicked.AddUniqueDynamic(this, &USiegeGraphicsMenuWidget::HandleWindowModeNextClicked);
	}
	if (FrameRateLimitPrevButton != nullptr)
	{
		FrameRateLimitPrevButton->OnClicked.AddUniqueDynamic(this, &USiegeGraphicsMenuWidget::HandleFrameRateLimitPrevClicked);
	}
	if (FrameRateLimitNextButton != nullptr)
	{
		FrameRateLimitNextButton->OnClicked.AddUniqueDynamic(this, &USiegeGraphicsMenuWidget::HandleFrameRateLimitNextClicked);
	}

	// TASK-1118's two prompt buttons. They are bound even though the prompt is
	// COLLAPSED: binding on arm and unbinding on disarm would put two more edges
	// on a state machine whose whole job is not to leak one, and a collapsed
	// button cannot be clicked anyway.
	if (KeepSettingsButton != nullptr)
	{
		KeepSettingsButton->OnClicked.AddUniqueDynamic(this, &USiegeGraphicsMenuWidget::HandleKeepSettingsClicked);
	}
	if (RevertSettingsButton != nullptr)
	{
		RevertSettingsButton->OnClicked.AddUniqueDynamic(this, &USiegeGraphicsMenuWidget::HandleRevertSettingsClicked);
	}

	// Back is bound unconditionally and LAST: it must work even when the graphics
	// subsystem is missing and every row above is dead. A panel you cannot leave
	// is worse than a panel that cannot change anything.
	if (BackButton != nullptr)
	{
		BackButton->OnClicked.AddUniqueDynamic(this, &USiegeGraphicsMenuWidget::HandleBackClicked);
	}
}

void USiegeGraphicsMenuWidget::UnbindAll()
{
	if (USiegeGraphicsSettingsSubsystem* Graphics = ResolveGraphicsSubsystem())
	{
		Graphics->OnGraphicsSettingsChanged.RemoveDynamic(this, &USiegeGraphicsMenuWidget::HandleGraphicsSettingsChanged);
	}

	for (const FName& GroupName : USiegeGraphicsSettingsSubsystem::GetQualityGroupNames())
	{
		if (USlider* Slider = FindGroupSlider(GroupName))
		{
			Slider->OnValueChanged.RemoveDynamic(this, &USiegeGraphicsMenuWidget::HandleGroupSliderValueChanged);
			Slider->OnMouseCaptureEnd.RemoveDynamic(this, &USiegeGraphicsMenuWidget::HandleGroupSliderCommitted);
			Slider->OnControllerCaptureEnd.RemoveDynamic(this, &USiegeGraphicsMenuWidget::HandleGroupSliderCommitted);
		}
	}

	if (OverallQualitySlider != nullptr)
	{
		OverallQualitySlider->OnValueChanged.RemoveDynamic(this, &USiegeGraphicsMenuWidget::HandleOverallSliderValueChanged);
		OverallQualitySlider->OnMouseCaptureEnd.RemoveDynamic(this, &USiegeGraphicsMenuWidget::HandleOverallSliderCommitted);
		OverallQualitySlider->OnControllerCaptureEnd.RemoveDynamic(this, &USiegeGraphicsMenuWidget::HandleOverallSliderCommitted);
	}

	if (ResolutionScaleSlider != nullptr)
	{
		ResolutionScaleSlider->OnValueChanged.RemoveDynamic(this, &USiegeGraphicsMenuWidget::HandleResolutionScaleValueChanged);
		ResolutionScaleSlider->OnMouseCaptureEnd.RemoveDynamic(this, &USiegeGraphicsMenuWidget::HandleResolutionScaleCommitted);
		ResolutionScaleSlider->OnControllerCaptureEnd.RemoveDynamic(this, &USiegeGraphicsMenuWidget::HandleResolutionScaleCommitted);
	}

	if (VSyncCheckBox != nullptr)
	{
		VSyncCheckBox->OnCheckStateChanged.RemoveDynamic(this, &USiegeGraphicsMenuWidget::HandleVSyncChanged);
	}

	// ⭐ TASK-1120 — both edges, symmetrically with SeedAndBind.
	if (ShowFrameRateCounterCheckBox != nullptr)
	{
		ShowFrameRateCounterCheckBox->OnCheckStateChanged.RemoveDynamic(
			this, &USiegeGraphicsMenuWidget::HandleShowFrameRateCounterChanged);
	}

	if (USiegeSettingsSubsystem* Settings = ResolveSettingsSubsystem())
	{
		Settings->OnSettingsChanged.RemoveDynamic(this, &USiegeGraphicsMenuWidget::HandleSettingsChanged);
	}

	if (AutoDetectButton != nullptr)
	{
		AutoDetectButton->OnClicked.RemoveDynamic(this, &USiegeGraphicsMenuWidget::HandleAutoDetectClicked);
	}
	if (ScreenResolutionPrevButton != nullptr)
	{
		ScreenResolutionPrevButton->OnClicked.RemoveDynamic(this, &USiegeGraphicsMenuWidget::HandleScreenResolutionPrevClicked);
	}
	if (ScreenResolutionNextButton != nullptr)
	{
		ScreenResolutionNextButton->OnClicked.RemoveDynamic(this, &USiegeGraphicsMenuWidget::HandleScreenResolutionNextClicked);
	}
	if (WindowModePrevButton != nullptr)
	{
		WindowModePrevButton->OnClicked.RemoveDynamic(this, &USiegeGraphicsMenuWidget::HandleWindowModePrevClicked);
	}
	if (WindowModeNextButton != nullptr)
	{
		WindowModeNextButton->OnClicked.RemoveDynamic(this, &USiegeGraphicsMenuWidget::HandleWindowModeNextClicked);
	}
	if (FrameRateLimitPrevButton != nullptr)
	{
		FrameRateLimitPrevButton->OnClicked.RemoveDynamic(this, &USiegeGraphicsMenuWidget::HandleFrameRateLimitPrevClicked);
	}
	if (FrameRateLimitNextButton != nullptr)
	{
		FrameRateLimitNextButton->OnClicked.RemoveDynamic(this, &USiegeGraphicsMenuWidget::HandleFrameRateLimitNextClicked);
	}
	if (KeepSettingsButton != nullptr)
	{
		KeepSettingsButton->OnClicked.RemoveDynamic(this, &USiegeGraphicsMenuWidget::HandleKeepSettingsClicked);
	}
	if (RevertSettingsButton != nullptr)
	{
		RevertSettingsButton->OnClicked.RemoveDynamic(this, &USiegeGraphicsMenuWidget::HandleRevertSettingsClicked);
	}
	if (BackButton != nullptr)
	{
		BackButton->OnClicked.RemoveDynamic(this, &USiegeGraphicsMenuWidget::HandleBackClicked);
	}
}

// ═════════════════════════════════════════════════════════════════════════════
//  SEEDING / REFRESH
// ═════════════════════════════════════════════════════════════════════════════

void USiegeGraphicsMenuWidget::RefreshAllRows()
{
	USiegeGraphicsSettingsSubsystem* Graphics = ResolveGraphicsSubsystem();
	if (Graphics == nullptr)
	{
		ShowPanelUnavailable();
		return;
	}

	// ⛔ THE ECHO GUARD. USlider::SetValue broadcasts OnValueChanged (measured —
	// see SeedAndBind), so every push below re-enters HandleGroupSliderValueChanged
	// et al. They early-out on this flag, which keeps a REFRESH from ever being
	// mistaken for a PLAYER ACTION.
	bSuppressRowEcho = true;

	// ---- Tier B: the ten groups, in the facade's own order -------------------
	for (const FName& GroupName : USiegeGraphicsSettingsSubsystem::GetQualityGroupNames())
	{
		ApplyGroupLevelToRow(GroupName, Graphics->GetQualityGroupLevel(GroupName));
	}

	// ---- Tier A: the preset, labelled from the VISIBLE groups (board cl. 3c) --
	{
		const bool bCustom = IsCustomForDisplay(Graphics);

		// When the visible ten agree, that shared level IS the preset the player
		// sees — and it is deliberately NOT GetOverallScalabilityLevel(), which can
		// read -1 because of the invisible eleventh group or because the resolution
		// scale drifted off the preset's canonical value. Either would label a
		// perfectly uniform panel "Custom" forever.
		int32 DisplayLevel = 0;
		if (bCustom)
		{
			TArray<int32> Levels;
			for (const FName& GroupName : USiegeGraphicsSettingsSubsystem::GetQualityGroupNames())
			{
				Levels.Add(Graphics->GetQualityGroupLevel(GroupName));
			}
			DisplayLevel = AveragedLevel(Levels);
		}
		else
		{
			const TArray<FName> Names = USiegeGraphicsSettingsSubsystem::GetQualityGroupNames();
			DisplayLevel = Names.Num() > 0 ? Graphics->GetQualityGroupLevel(Names[0]) : 0;
		}

		if (OverallQualitySlider != nullptr)
		{
			OverallQualitySlider->SetValue(LevelToSliderValue(DisplayLevel));
		}

		const FString OverallDisplay = ComposeLevelValueText(DisplayLevel, bCustom);
		if (OverallQualityValueText != nullptr)
		{
			OverallQualityValueText->SetText(FText::FromString(OverallDisplay));
		}

		OnGraphicsRowChanged(
			USiegeGraphicsSettingsSubsystem::SettingName_OverallQuality.ToString(),
			bCustom ? USiegeGraphicsSettingsSubsystem::CustomQualityLevel : DisplayLevel,
			OverallDisplay);
	}

	// ---- Tier C: the resolution scale ----------------------------------------
	// ⛔ SEEDED FROM GetResolutionScalePercentForSlider(), which is a DISPLAY
	// TRANSFORM THAT WRITES NOTHING: while the sg.ResolutionQuality=0 SENTINEL is
	// live it reports 100 %, and the sentinel survives untouched until the player
	// actually moves the handle. Seeding from the raw percent instead would show
	// "0%" on this machine's real ini — a lie about what is rendering — and
	// clamping on read would silently rewrite the sentinel to 50 % the first time
	// anyone OPENED this menu.
	{
		const float Percent = Graphics->GetResolutionScalePercentForSlider();
		if (ResolutionScaleSlider != nullptr)
		{
			ResolutionScaleSlider->SetValue(Percent);
		}

		const FString ScaleDisplay = FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Percent));
		if (ResolutionScaleValueText != nullptr)
		{
			ResolutionScaleValueText->SetText(FText::FromString(ScaleDisplay));
		}

		OnGraphicsRowChanged(
			USiegeGraphicsSettingsSubsystem::SettingName_ResolutionScale.ToString(),
			FMath::RoundToInt(Percent), ScaleDisplay);
	}

	// ---- Tier C: the display steppers ----------------------------------------
	// ⛔ board cl. (3e): the resolution crosses as an INDEX plus an already-
	// composed FString label. FIntPoint never reaches OnGraphicsRowChanged, which
	// would be a UHT COMPILE failure paid three rows later by someone else.
	{
		const int32 ResolutionIndex = Graphics->FindCurrentScreenResolutionIndex();
		const FString ResolutionLabel = Graphics->GetSupportedScreenResolutionLabel(ResolutionIndex);
		if (ScreenResolutionValueText != nullptr)
		{
			ScreenResolutionValueText->SetText(FText::FromString(ResolutionLabel));
		}
		OnGraphicsRowChanged(
			USiegeGraphicsSettingsSubsystem::SettingName_ScreenResolution.ToString(),
			ResolutionIndex, ResolutionLabel);
	}

	{
		const int32 Mode = Graphics->GetWindowMode();
		const FString ModeLabel = USiegeGraphicsSettingsSubsystem::GetWindowModeLabel(Mode);
		if (WindowModeValueText != nullptr)
		{
			WindowModeValueText->SetText(FText::FromString(ModeLabel));
		}
		OnGraphicsRowChanged(
			USiegeGraphicsSettingsSubsystem::SettingName_WindowMode.ToString(), Mode, ModeLabel);
	}

	// ---- Tier C: VSync + the frame-rate ladder -------------------------------
	{
		const bool bVSync = Graphics->IsVSyncEnabled();
		if (VSyncCheckBox != nullptr)
		{
			// UCheckBox::SetIsChecked does NOT re-enter OnCheckStateChanged (only
			// real user interaction broadcasts) — the opposite of USlider::SetValue.
			VSyncCheckBox->SetIsChecked(bVSync);
		}
		OnGraphicsRowChanged(
			USiegeGraphicsSettingsSubsystem::SettingName_VSync.ToString(),
			bVSync ? 1 : 0, bVSync ? TEXT("On") : TEXT("Off"));
	}

	{
		const int32 LimitIndex = Graphics->FindCurrentFrameRateLimitIndex();
		const FString LimitLabel = USiegeGraphicsSettingsSubsystem::GetFrameRateLimitLabel(Graphics->GetFrameRateLimit());
		if (FrameRateLimitValueText != nullptr)
		{
			FrameRateLimitValueText->SetText(FText::FromString(LimitLabel));
		}
		OnGraphicsRowChanged(
			USiegeGraphicsSettingsSubsystem::SettingName_FrameRateLimit.ToString(), LimitIndex, LimitLabel);
	}

	// ---- GFX-§9's read-only fog line, re-composed from the live Shadow level --
	// This is what gives TASK-1113's shipped ShouldEnableVolumetricFog() a REAL
	// caller (SC-§36.1): the getter is read here, on every refresh, and its answer
	// is on screen.
	if (ShadowQualityHintText != nullptr)
	{
		ShadowQualityHintText->SetText(FText::FromString(
			ComposeGroupHint(USiegeGraphicsSettingsSubsystem::GroupName_ShadowQuality,
				Graphics->ShouldEnableVolumetricFog())));
	}

	bSuppressRowEcho = false;
}

void USiegeGraphicsMenuWidget::ApplyGroupLevelToRow(FName GroupName, int32 Level)
{
	FGroupRowWidgets Row = ResolveGroupRow(GroupName);

	if (Row.Slider != nullptr && *Row.Slider != nullptr)
	{
		(*Row.Slider)->SetValue(LevelToSliderValue(Level));
	}

	const FString Display = ComposeLevelValueText(Level, /*bCustom*/ false);
	if (Row.Value != nullptr && *Row.Value != nullptr)
	{
		(*Row.Value)->SetText(FText::FromString(Display));
	}

	// The delegate law, cloned from USettingsMenuWidget::ApplyConfirmValueToRow:
	// notify on a real change AND on the first seed (so a future WBP_GraphicsMenu
	// never opens stale), ⛔ never on a no-op.
	const int32* Last = LastPushedGroupLevel.Find(GroupName);
	const bool bIsNew = (Last == nullptr) || (*Last != Level);
	LastPushedGroupLevel.Add(GroupName, Level);

	if (bIsNew)
	{
		OnGraphicsRowChanged(GroupName.ToString(), Level, Display);
	}
}

// ═════════════════════════════════════════════════════════════════════════════
//  PLAYER ACTIONS
// ═════════════════════════════════════════════════════════════════════════════

void USiegeGraphicsMenuWidget::BackPressed()
{
	// 🚨 board cl. (3b-WIDENED), CALL SITE 1 OF 2. Discard BEFORE the panel goes
	// away: a staged-but-unconfirmed video mode that outlives this panel makes the
	// facade refuse EVERY SAVE **and** AUTO-DETECT for the rest of the session,
	// silently. Two ordinary clicks — one stepper press, then Back — would
	// otherwise soft-lock the entire settings screen.
	//
	// ⚠️ It is done here as well as in NativeDestruct() ON PURPOSE, and it is
	// still EXACTLY ONE revert: DiscardStagedVideoMode is guarded on
	// IsVideoModeChangePending(), this call clears it, and NativeDestruct's call
	// then returns false without reverting. Doing it here rather than only in the
	// teardown means the window is closed SYNCHRONOUSLY on the click, instead of
	// whenever Slate gets round to destroying the SObjectWidget.
	//
	// ⭐⭐ TASK-1118'S RECONCILIATION, IN ONE LINE: the countdown is DISARMED here
	// and the revert below is LEFT EXACTLY AS TASK-1115 SHIPPED IT. Back still
	// reaches DiscardStagedVideoMode ONCE (NativeDestruct's later call finds the
	// window closed and returns false), and the disarm is not a revert — it stops
	// a timer and hides a prompt. ⛔ There is no third revert on this path and no
	// second call to the facade: the count is unchanged, and the grep that proves
	// it is in handoffs/TASK-1118-programmer.md.
	DisarmVideoModeCountdown();
	DiscardStagedVideoMode(ResolveGraphicsSubsystem());

	// ⛔ RemoveFromParent(self) AND NOTHING ELSE (GFX-§2(f), board cl. 2). The
	// settings panel underneath was never removed, is already alive and already
	// correct; re-creating it would put navigation state in the leaf.
	UE_LOG(LogSiegeGraphics, Log, TEXT("[GraphicsMenu] Back pressed — dismissing the graphics panel only."));
	RemoveFromParent();
}

void USiegeGraphicsMenuWidget::AutoDetectPressed()
{
	USiegeGraphicsSettingsSubsystem* Graphics = ResolveGraphicsSubsystem();
	if (Graphics == nullptr)
	{
		ShowPanelUnavailable();
		return;
	}

	// The facade refuses this while a video-mode change is unconfirmed, and that
	// refusal is CORRECT (the benchmark's own save would persist the mode). The
	// panel's job is to make sure that state cannot be reached by accident and to
	// SAY SO when it happens, rather than to leave a button that does nothing.
	if (!Graphics->AutoDetectQuality())
	{
		if (Graphics->IsVideoModeChangePending() && StatusText != nullptr)
		{
			// ⛔ gate qa/TASK-1116.md WARN-3: this used to show DisplayHint, which
			// talks about DISPLAY changes at a player who pressed AUTO-DETECT. The
			// message now names the thing that was refused AND the way out, and
			// after TASK-1118 that way out is the Keep/Revert prompt rather than
			// Back.
			StatusText->SetText(FText::FromString(FString(SiegeGraphicsMenuText::AutoDetectBlocked)));
		}
		UE_LOG(LogSiegeGraphics, Log,
			TEXT("[GraphicsMenu] Auto-Detect returned false (video-mode pending: %s)."),
			Graphics->IsVideoModeChangePending() ? TEXT("yes") : TEXT("no"));
		return;
	}

	// A benchmark moves ten groups AND the resolution scale, so the whole panel is
	// re-seeded rather than one row (board cl. 3d's rule, applied to Auto-Detect).
	RefreshAllRows();
}

// ═════════════════════════════════════════════════════════════════════════════
//  ⭐⭐ TASK-1118 — THE 10-SECOND "KEEP THESE SETTINGS?" (GFX-§4)
//
//  The state machine is drawn in the header. Five functions, and the split is
//  the point: BeginVideoModeConfirmation DECIDES, ArmVideoModeCountdown STARTS,
//  TickVideoModeCountdown COUNTS, and Keep/Revert END — with the revert going
//  through DiscardStagedVideoMode, which TASK-1115 shipped and which is still
//  the ONLY function in this class that calls the facade's revert.
// ═════════════════════════════════════════════════════════════════════════════

bool USiegeGraphicsMenuWidget::BeginVideoModeConfirmation(USiegeGraphicsSettingsSubsystem* Graphics)
{
	if (Graphics == nullptr)
	{
		ShowPanelUnavailable();
		return false;
	}

	// ── (1) IS THERE ANYTHING LEFT TO CONFIRM? ───────────────────────────────
	// A stepper press can land the player back on the mode they started in —
	// three presses of ">" on Window Mode wraps all the way round. Leaving the
	// prompt up over that would offer "Revert" for a change that no longer exists
	// and "Keep" for a mode that was never left.
	//
	// 🚨 THE PREDICATE HERE IS HasUnconfirmedVideoModeDifference(), ⛔ NOT
	// IsVideoModeChangePending(). The latter is `bVideoModeChangePending || <that
	// difference>` and the left-hand half is a LATCH: the first provisional apply
	// below raises it and only the facade's CloseVideoModeWindow lowers it.
	// Reading it here made this branch DEAD from the second press onward — the
	// wrap-around press would see "pending", re-apply the mode the player is
	// already on and RESTART the ten seconds instead of putting the question away
	// (qa/TASK-1119.md BLOCKER-1: a name read as if it were a pure comparison).
	//
	// ⛔ AND THE DISCARD IS NOT OPTIONAL — DISARMING ALONE WOULD BE A NEW SOFT-
	// LOCK. The latch would stay up with no prompt and no countdown left to lower
	// it, so the facade would refuse every save and Auto-Detect for the rest of
	// the session (cl. 8-WIDENED's failure, arriving through a new door).
	// DiscardStagedVideoMode reverts to the last CONFIRMED mode, closes the window
	// and flushes any save deferred while it was open.
	//
	// ⛔ CORRECTED BY TASK-1124 (board cl. 5a, from qa/TASK-1119.md's WARN against
	// its own loop-0 wording). This comment used to read "nothing the player can
	// see moves". ⛔ THAT WAS FALSE, AND IT MATTERED: the staged mode equals the
	// confirmed one by the time this branch fires, but the mode ON SCREEN is the
	// PREVIOUS press's provisional apply — so the revert here really does change
	// the picture, from that provisional mode back to the confirmed one. ⛔ IT MUST:
	// the wrapping press is the player saying "put it back", and a discard that
	// moved nothing would leave them looking at a mode they had just left.
	//
	// Order: disarm THEN discard, the same order Keep, Revert, Back and the
	// teardown all use. The countdown is dead before any state moves.
	if (!Graphics->HasUnconfirmedVideoModeDifference())
	{
		DisarmVideoModeCountdown();

		// ⛔ FOURTH CALLER of DiscardStagedVideoMode, and the Back path's count is
		// STILL ONE: Back does not reach this function at all. Its own guard makes
		// this a no-op when nothing was ever applied (a no-op stepper press on a
		// fresh panel), so the call is safe on both ways into this branch.
		DiscardStagedVideoMode(Graphics);

		UE_LOG(LogSiegeGraphics, Verbose,
			TEXT("[GraphicsMenu] The display change came back to the confirmed mode — the confirmation window is closed and any running countdown is cancelled."));
		return false;
	}

	// ── (2) CAN THE CHANGE BE UNDONE WITHOUT THE PLAYER? ─────────────────────
	// 🚨 THE ORDER OF THESE TWO CHECKS IS THE WHOLE SAFETY PROPERTY. The apply
	// below is what a player cannot survive without an auto-revert, so the ability
	// to auto-revert is established BEFORE anything reaches the display. Reversing
	// them would ship a mode change whose undo may not exist — which is the
	// permanent lockout GFX-§4 exists to prevent, arriving through a door nobody
	// was watching.
	if (!CanArmVideoModeCountdown())
	{
		UE_LOG(LogSiegeGraphics, Warning,
			TEXT("[GraphicsMenu] No timer manager is available, so a display change could not be auto-reverted — it was STAGED and NOT applied. Leaving Graphics will discard it."));

		if (StatusText != nullptr)
		{
			StatusText->SetText(FText::FromString(FString(SiegeGraphicsMenuText::CountdownUnavailable)));
		}
		return false;
	}

	// ── (3) APPLY — AND ⛔ NEVER SAVE ────────────────────────────────────────
	// ApplyVideoModeProvisional() is ApplyResolutionSettings(false) and nothing
	// else; the facade's own header records why it may never be ApplySettings()
	// (whose last line is SaveSettings()). GFX-§4 cl. 4: a saved-then-unviewable
	// mode is the permanent lockout itself.
	if (!Graphics->ApplyVideoModeProvisional())
	{
		UE_LOG(LogSiegeGraphics, Warning,
			TEXT("[GraphicsMenu] The provisional display apply failed — no countdown was armed. The change stays STAGED and Back will discard it."));
		DisarmVideoModeCountdown();
		return false;
	}

	// ── (4) ARM ──────────────────────────────────────────────────────────────
	ArmVideoModeCountdown();
	return true;
}

bool USiegeGraphicsMenuWidget::CanArmVideoModeCountdown() const
{
	// The automation seam FIRST: a bare NewObject widget has no world, and without
	// this the suite could only ever observe the refusal branch — the expiry path
	// this row exists to ship would have no coverage at all.
	if (bDriveVideoModeCountdownManuallyForAutomationTests)
	{
		return true;
	}

	return GetWorld() != nullptr;
}

void USiegeGraphicsMenuWidget::ArmVideoModeCountdown()
{
	// A second display change RESTARTS the ten seconds rather than inheriting the
	// remainder of the first — the player is being asked about the mode that is on
	// screen NOW, and giving them two seconds to answer because they pressed ">"
	// twice would be the countdown working against them.
	VideoModeCountdownSecondsRemaining = VideoModeConfirmSeconds;
	bVideoModeCountdownActive = true;

	RefreshVideoModeCountdownText();

	// ⭐ WRITER 3 OF 4 — and the ONLY writer in this file that can make the prompt
	// VISIBLE. The other three only ever Collapse it (see the header).
	if (VideoModeConfirmBorder != nullptr)
	{
		VideoModeConfirmBorder->SetVisibility(ESlateVisibility::Visible);

		// The panel scrolls and the display steppers are two thirds of the way
		// down, so the prompt can open below the fold if the player scrolled after
		// pressing. Headless-safe: UScrollBox::ScrollWidgetIntoView no-ops when its
		// Slate widget is invalid (ScrollBox.cpp), which is every automation run.
		if (RootScrollBox != nullptr)
		{
			RootScrollBox->ScrollWidgetIntoView(VideoModeConfirmBorder.Get(), /*AnimateScroll*/ false);
		}
	}

	if (bDriveVideoModeCountdownManuallyForAutomationTests)
	{
		// ⛔ The suite drives TickVideoModeCountdown() by hand. No timer is asked
		// for, because there is no world to ask.
		return;
	}

	if (UWorld* World = GetWorld())
	{
		FTimerManager& TimerManager = World->GetTimerManager();

		// ⛔ ClearTimer FIRST. Re-arming over a live handle would leave two loops
		// running and halve the visible countdown.
		TimerManager.ClearTimer(VideoModeCountdownTimerHandle);

		// ⛔ ONE REPEATING TIMER DOES BOTH JOBS — the repaint and the expiry. Two
		// timers would be two things to cancel on four different exits, and the one
		// that got missed would be the one that fires at a player who has already
		// left. The world's timer manager survives the mode switch: a resolution
		// change tears down no world.
		TimerManager.SetTimer(
			VideoModeCountdownTimerHandle, this, &USiegeGraphicsMenuWidget::TickVideoModeCountdown,
			VideoModeCountdownTickSeconds, /*bLoop*/ true);
	}

	UE_LOG(LogSiegeGraphics, Log,
		TEXT("[GraphicsMenu] Display mode applied PROVISIONALLY (nothing saved). %.0f-second confirmation armed — it auto-reverts with no input at all (GFX-§4)."),
		VideoModeConfirmSeconds);
}

void USiegeGraphicsMenuWidget::DisarmVideoModeCountdown()
{
	// ⛔ THE FLAG GOES DOWN FIRST, BEFORE THE TIMER IS TOUCHED. A tick can already
	// be queued when the player presses Keep, Revert or Back; clearing the handle
	// first and the flag second would leave a window in which that queued callback
	// runs against an "active" countdown and reverts a mode the player has just
	// chosen to keep. ⛔ Do not reorder these two lines.
	bVideoModeCountdownActive = false;
	VideoModeCountdownSecondsRemaining = 0.0f;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(VideoModeCountdownTimerHandle);
	}
	VideoModeCountdownTimerHandle.Invalidate();

	// WRITER 4 OF 4.
	if (VideoModeConfirmBorder != nullptr)
	{
		VideoModeConfirmBorder->SetVisibility(ESlateVisibility::Collapsed);
	}

	// ⛔ AND NOTHING ELSE. This function REVERTS NOTHING and must never become a
	// revert site: it is called from Keep (where a revert would undo the player's
	// choice), from Revert and from Back (where DiscardStagedVideoMode does the
	// one revert), and from the teardown. A revert here would be the second one
	// board cl. (8) forbids.
}

void USiegeGraphicsMenuWidget::RefreshVideoModeCountdownText()
{
	if (VideoModeConfirmText == nullptr)
	{
		return;
	}

	// CeilToInt, not RoundToInt: with a whole-second tick the two agree, but a
	// ceiling can never show "0 seconds" while time is still left — and the one
	// number a player reads off this line is the one they are deciding against.
	const int32 Seconds = FMath::Max(0, FMath::CeilToInt(VideoModeCountdownSecondsRemaining));
	VideoModeConfirmText->SetText(FText::FromString(ComposeVideoModeCountdownText(Seconds)));
}

void USiegeGraphicsMenuWidget::TickVideoModeCountdown()
{
	// ⛔ THE STRANDED-CALLBACK GUARD. A timer already in flight when the player
	// pressed Keep/Revert/Back arrives here with the countdown disarmed, and must
	// do NOTHING — otherwise it reverts a mode the player has since settled.
	if (!bVideoModeCountdownActive)
	{
		return;
	}

	VideoModeCountdownSecondsRemaining -= VideoModeCountdownTickSeconds;

	if (VideoModeCountdownSecondsRemaining > UE_KINDA_SMALL_NUMBER)
	{
		RefreshVideoModeCountdownText();
		return;
	}

	VideoModeCountdownSecondsRemaining = 0.0f;
	RefreshVideoModeCountdownText();

	// 🚨 THIS IS THE POINT OF THE WHOLE ROW. Nothing above required a click, a
	// hover or a visible widget — the player this protects cannot see the screen,
	// and the revert reaches them anyway.
	UE_LOG(LogSiegeGraphics, Warning,
		TEXT("[GraphicsMenu] The %.0f-second display confirmation EXPIRED with no answer — reverting to the last confirmed mode (GFX-§4)."),
		VideoModeConfirmSeconds);

	// ⛔ THE SAME BODY THE Revert BUTTON USES. A separate expiry path would be a
	// path written once, clicked never, and rotted unnoticed.
	RevertSettingsPressed();
}

void USiegeGraphicsMenuWidget::KeepSettingsPressed()
{
	// Disarm FIRST and unconditionally: whatever happens below, the prompt must
	// not survive the answer.
	DisarmVideoModeCountdown();

	USiegeGraphicsSettingsSubsystem* Graphics = ResolveGraphicsSubsystem();
	if (Graphics == nullptr)
	{
		ShowPanelUnavailable();
		return;
	}

	// ⛔ GUARDED, because this is BlueprintCallable and a future WBP_GraphicsMenu
	// could reach it with nothing pending. ConfirmVideoModeChange() would then
	// stamp a no-op AND release a save for a change nobody made.
	if (!Graphics->IsVideoModeChangePending())
	{
		UE_LOG(LogSiegeGraphics, Verbose,
			TEXT("[GraphicsMenu] Keep pressed with no display change pending — nothing to confirm."));
		return;
	}

	UE_LOG(LogSiegeGraphics, Log, TEXT("[GraphicsMenu] Keep pressed — confirming the display mode and releasing the save."));

	// ⛔ THE FACADE SAVES, NOT THIS FUNCTION. ConfirmVideoModeChange() stamps
	// LastConfirmed* and then flushes through the one guarded save site
	// (CloseVideoModeWindow -> RequestSaveSettings), which is also what flushes a
	// quality change that was refused while the confirmation was outstanding. A
	// save added here would be a second save site for a rule that is silent when
	// it is broken.
	Graphics->ConfirmVideoModeChange();

	// The facade broadcasts on close, so the bound HandleGraphicsSettingsChanged
	// re-seeds every row; no explicit refresh is added here, because two refreshes
	// would fire the presentation event twice for one player action.
}

void USiegeGraphicsMenuWidget::RevertSettingsPressed()
{
	DisarmVideoModeCountdown();

	USiegeGraphicsSettingsSubsystem* Graphics = ResolveGraphicsSubsystem();
	if (Graphics == nullptr)
	{
		ShowPanelUnavailable();
		return;
	}

	// ⛔ THROUGH DiscardStagedVideoMode, WHICH IS THE ONE REVERT IN THIS CLASS.
	// It is guarded on IsVideoModeChangePending(), so this is a no-op when there
	// is nothing to undo — and calling it here adds NO revert to the Back path,
	// because Back does not call this function. (Board cl. 8: exactly one revert
	// on the Back path, and the count is unchanged from TASK-1115.)
	//
	// ⛔ AND NOTHING AFTER IT. The facade's RevertVideoModeChange() already does
	// RevertVideoMode() + ApplyResolutionSettings(false); a second apply here
	// would fire a mode change at a player who may already be looking at a black
	// screen (TASK-1118 cl. 9, manager's RULING B).
	if (DiscardStagedVideoMode(Graphics))
	{
		UE_LOG(LogSiegeGraphics, Log, TEXT("[GraphicsMenu] The display mode was reverted to the last confirmed one."));
	}
}

// ═════════════════════════════════════════════════════════════════════════════
//  SLIDER HANDLERS — board cl. (3a): LABEL on change, WRITE on commit.
// ═════════════════════════════════════════════════════════════════════════════

void USiegeGraphicsMenuWidget::HandleGroupSliderValueChanged(float NewValue)
{
	if (bSuppressRowEcho)
	{
		return;
	}

	// ⚠️ USlider's delegates carry NO SENDER, so this one thunk drives all ten
	// rows and NewValue cannot be attributed to a slider on its own — every row is
	// re-read from its own handle below. The parameter is logged rather than
	// dropped so a Verbose trace still shows which value arrived.
	UE_LOG(LogSiegeGraphics, VeryVerbose, TEXT("[GraphicsMenu] Group slider moved to %.3f — refreshing level labels only."), NewValue);

	// ⛔ LABEL ONLY — no facade call, no apply, no save. The level name has to
	// track the handle live or a detented slider is unreadable mid-drag.
	for (const FName& GroupName : USiegeGraphicsSettingsSubsystem::GetQualityGroupNames())
	{
		FGroupRowWidgets Row = ResolveGroupRow(GroupName);
		if (!Row.IsComplete() || *Row.Slider == nullptr || *Row.Value == nullptr)
		{
			continue;
		}

		(*Row.Value)->SetText(FText::FromString(
			ComposeLevelValueText(SliderValueToLevel((*Row.Slider)->GetValue()), /*bCustom*/ false)));
	}
}

void USiegeGraphicsMenuWidget::HandleGroupSliderCommitted()
{
	if (bSuppressRowEcho)
	{
		return;
	}

	USiegeGraphicsSettingsSubsystem* Graphics = ResolveGraphicsSubsystem();
	if (Graphics == nullptr)
	{
		ShowPanelUnavailable();
		return;
	}

	// ⚠️ SNAPSHOT THE DISAGREEMENTS FIRST, THEN WRITE. The first write broadcasts,
	// which re-seeds every row through RefreshAllRows — so reading the sliders
	// while writing would compare a later group against a value the refresh had
	// already corrected. At most one group can disagree, but "at most one" is the
	// kind of invariant that stops being true quietly.
	TArray<TPair<FName, int32>> Pending;
	for (const FName& GroupName : USiegeGraphicsSettingsSubsystem::GetQualityGroupNames())
	{
		USlider* Slider = FindGroupSlider(GroupName);
		if (Slider == nullptr)
		{
			continue;
		}

		const int32 SliderLevel = SliderValueToLevel(Slider->GetValue());
		if (SliderLevel != Graphics->GetQualityGroupLevel(GroupName))
		{
			Pending.Emplace(GroupName, SliderLevel);
		}
	}

	for (const TPair<FName, int32>& Change : Pending)
	{
		UE_LOG(LogSiegeGraphics, Log, TEXT("[GraphicsMenu] Committing %s = %d."), *Change.Key.ToString(), Change.Value);

		// The facade owns the clamp, the no-op refusal, the apply and the save.
		Graphics->SetQualityGroupLevel(Change.Key, Change.Value);
	}
}

void USiegeGraphicsMenuWidget::HandleOverallSliderValueChanged(float NewValue)
{
	if (bSuppressRowEcho || OverallQualityValueText == nullptr)
	{
		return;
	}

	// ⛔ LABEL ONLY. Mid-drag the handle is the truth, so the label reads the level
	// under the handle and NOT "Custom" — the panel goes back to reporting the
	// real Custom state on the next refresh, which the commit below triggers.
	OverallQualityValueText->SetText(FText::FromString(
		ComposeLevelValueText(SliderValueToLevel(NewValue), /*bCustom*/ false)));
}

void USiegeGraphicsMenuWidget::HandleOverallSliderCommitted()
{
	if (bSuppressRowEcho || OverallQualitySlider == nullptr)
	{
		return;
	}

	USiegeGraphicsSettingsSubsystem* Graphics = ResolveGraphicsSubsystem();
	if (Graphics == nullptr)
	{
		ShowPanelUnavailable();
		return;
	}

	const int32 NewLevel = SliderValueToLevel(OverallQualitySlider->GetValue());
	UE_LOG(LogSiegeGraphics, Log, TEXT("[GraphicsMenu] Committing overall preset = %d."), NewLevel);

	// ⛔ board cl. (3d) — EXPECT THIS TO MOVE THE RESOLUTION-SCALE ROW, AND DO NOT
	// "FIX" IT. SetOverallScalabilityLevel forwards to
	// FQualityLevels::SetFromSingleQualityLevel, which ALSO writes ResolutionQuality
	// from PerfIndexValues_ResolutionQuality = 50 71 87 100 100
	// (Scalability.cpp:1047). One write, two player-visible values. The facade
	// fires a SECOND, conditional broadcast for exactly this, and the refresh
	// below (via HandleGraphicsSettingsChanged) re-seeds the scale row — ⛔ a STALE
	// resolution-scale row is the real defect here, not the engine's write.
	Graphics->SetOverallScalabilityLevel(NewLevel);

	// Belt and braces for the no-op case: if the preset did not actually change,
	// the facade broadcasts nothing, and the label would be left showing whatever
	// the drag put there instead of the real (possibly Custom) state.
	RefreshAllRows();
}

void USiegeGraphicsMenuWidget::HandleResolutionScaleValueChanged(float NewValue)
{
	if (bSuppressRowEcho || ResolutionScaleValueText == nullptr)
	{
		return;
	}

	// ═════════════════════════════════════════════════════════════════════════
	// 🚨 board cl. (3a) / gate F-5 — ⛔ THIS FUNCTION MUST NEVER WRITE.
	//
	// This is the ONE genuinely continuous slider (GFX-§5), 50→100 at 1% steps.
	// Every facade setter ends in ApplyQualitySettings() → RequestSaveSettings() →
	// UGameUserSettings::SaveSettings() → SaveConfig(CPF_Config,
	// GGameUserSettingsIni) (GameUserSettings.cpp:683) — a FILE WRITE. USlider
	// fires OnValueChanged once per frame of a drag, so writing here would turn a
	// single 50→100 drag into up to FIFTY ini writes, i.e. a disk write per frame.
	//
	// ⇒ label here, write in HandleResolutionScaleCommitted().
	// ═════════════════════════════════════════════════════════════════════════
	ResolutionScaleValueText->SetText(FText::FromString(
		FString::Printf(TEXT("%d%%"), FMath::RoundToInt(NewValue))));
}

void USiegeGraphicsMenuWidget::HandleResolutionScaleCommitted()
{
	if (bSuppressRowEcho || ResolutionScaleSlider == nullptr)
	{
		return;
	}

	USiegeGraphicsSettingsSubsystem* Graphics = ResolveGraphicsSubsystem();
	if (Graphics == nullptr)
	{
		ShowPanelUnavailable();
		return;
	}

	// Whole percents only: the slider is 1%-stepped by GFX-§5 and a stored 73.4184
	// would make the row's own label disagree with the value behind it.
	const float Percent = FMath::RoundToFloat(ResolutionScaleSlider->GetValue());

	UE_LOG(LogSiegeGraphics, Log, TEXT("[GraphicsMenu] Committing resolution scale = %d%%."), FMath::RoundToInt(Percent));

	// The facade clamps to [50, 100] ON WRITE (never on read — that is what keeps
	// the sg.ResolutionQuality=0 sentinel alive until the player really moves this
	// handle) and refuses a no-op.
	Graphics->SetResolutionScalePercent(Percent);
}

// ═════════════════════════════════════════════════════════════════════════════
//  BUTTON / CHECKBOX HANDLERS
// ═════════════════════════════════════════════════════════════════════════════

void USiegeGraphicsMenuWidget::HandleBackClicked()
{
	BackPressed();
}

void USiegeGraphicsMenuWidget::HandleAutoDetectClicked()
{
	AutoDetectPressed();
}

void USiegeGraphicsMenuWidget::HandleKeepSettingsClicked()
{
	KeepSettingsPressed();
}

void USiegeGraphicsMenuWidget::HandleRevertSettingsClicked()
{
	RevertSettingsPressed();
}

void USiegeGraphicsMenuWidget::HandleScreenResolutionPrevClicked()
{
	StepScreenResolution(-1);
}

void USiegeGraphicsMenuWidget::HandleScreenResolutionNextClicked()
{
	StepScreenResolution(+1);
}

void USiegeGraphicsMenuWidget::StepScreenResolution(int32 Delta)
{
	USiegeGraphicsSettingsSubsystem* Graphics = ResolveGraphicsSubsystem();
	if (Graphics == nullptr)
	{
		ShowPanelUnavailable();
		return;
	}

	const int32 Count = Graphics->GetSupportedScreenResolutionCount();
	const int32 NextIndex = StepIndex(Graphics->FindCurrentScreenResolutionIndex(), Delta, Count);

	// ⛔ STAGE FIRST. The facade's SetScreenResolutionByIndex applies nothing and
	// saves nothing; staging and applying are split precisely so the widget can
	// move BOTH the resolution and the window mode and pay for ONE screen flash.
	Graphics->SetScreenResolutionByIndex(NextIndex);

	// ✅ TASK-1115's SEAM, FILLED BY TASK-1118. The provisional apply lives behind
	// BeginVideoModeConfirmation() together with the countdown that undoes it,
	// because GFX-§4's whole point is that the two are ONE mechanism: applying
	// without the countdown is the permanent lockout, and arming the countdown
	// without applying is a prompt about nothing.
	BeginVideoModeConfirmation(Graphics);

	RefreshAllRows();
}

void USiegeGraphicsMenuWidget::HandleWindowModePrevClicked()
{
	StepWindowMode(-1);
}

void USiegeGraphicsMenuWidget::HandleWindowModeNextClicked()
{
	StepWindowMode(+1);
}

void USiegeGraphicsMenuWidget::StepWindowMode(int32 Delta)
{
	USiegeGraphicsSettingsSubsystem* Graphics = ResolveGraphicsSubsystem();
	if (Graphics == nullptr)
	{
		ShowPanelUnavailable();
		return;
	}

	// ⛔ int32 THROUGHOUT. EWindowMode::Type never crosses this boundary —
	// GFX-§2(d) forbids an enum in a BlueprintImplementableEvent parameter and the
	// facade deliberately exposes no UENUM at all, so the panel cannot get it
	// wrong even by accident.
	const int32 NextMode = StepIndex(Graphics->GetWindowMode(), Delta,
		USiegeGraphicsSettingsSubsystem::GetWindowModeCount());

	// STAGE, then confirm — see StepScreenResolution. ⛔ Window mode gets the SAME
	// countdown as resolution and for the same reason: Fullscreen at a mode the
	// monitor cannot show is exactly as unviewable as a bad resolution, and
	// GFX-§4 names both controls.
	Graphics->SetWindowMode(NextMode);

	BeginVideoModeConfirmation(Graphics);

	RefreshAllRows();
}

void USiegeGraphicsMenuWidget::HandleFrameRateLimitPrevClicked()
{
	StepFrameRateLimit(-1);
}

void USiegeGraphicsMenuWidget::HandleFrameRateLimitNextClicked()
{
	StepFrameRateLimit(+1);
}

void USiegeGraphicsMenuWidget::StepFrameRateLimit(int32 Delta)
{
	USiegeGraphicsSettingsSubsystem* Graphics = ResolveGraphicsSubsystem();
	if (Graphics == nullptr)
	{
		ShowPanelUnavailable();
		return;
	}

	// Unlike the two display steppers this one APPLIES AND SAVES immediately: a
	// frame-rate cap cannot make the screen unreadable, so GFX-§4's confirm-or-
	// revert dance does not apply to it.
	const int32 NextIndex = StepIndex(Graphics->FindCurrentFrameRateLimitIndex(), Delta,
		USiegeGraphicsSettingsSubsystem::GetFrameRateLimitOptionCount());

	Graphics->SetFrameRateLimitByIndex(NextIndex);

	RefreshAllRows();
}

void USiegeGraphicsMenuWidget::HandleVSyncChanged(bool bIsChecked)
{
	if (bSuppressRowEcho)
	{
		return;
	}

	USiegeGraphicsSettingsSubsystem* Graphics = ResolveGraphicsSubsystem();
	if (Graphics == nullptr)
	{
		ShowPanelUnavailable();
		return;
	}

	Graphics->SetVSyncEnabled(bIsChecked);
}

void USiegeGraphicsMenuWidget::HandleGraphicsSettingsChanged(FName SettingName)
{
	// ⚠️ THE TOKEN IS NOT FILTERED ON, and that is the USettingsMenuWidget rule
	// applied to a much wider panel: a preset press moves TWO values (the groups
	// AND the resolution scale) and Auto-Detect moves ELEVEN, so any per-token
	// refresh would have to reproduce the facade's own fan-out and would drift
	// from it. Re-reading everything costs a handful of in-memory getters and
	// cannot go stale.
	UE_LOG(LogSiegeGraphics, Verbose,
		TEXT("[GraphicsMenu] Graphics settings changed ('%s') — refreshing every row."), *SettingName.ToString());

	RefreshAllRows();
}

// ═════════════════════════════════════════════════════════════════════════════
//  NULL-SAFETY
// ═════════════════════════════════════════════════════════════════════════════

void USiegeGraphicsMenuWidget::ShowPanelUnavailable()
{
	// LOG ONCE per widget instance. This is reachable from the seed, from every
	// click and from a broadcast, and a settings screen that spams the log every
	// time the player pokes a dead control is a log nobody reads.
	if (!bLoggedSubsystemUnavailable)
	{
		bLoggedSubsystemUnavailable = true;
		UE_LOG(LogSiegeGraphics, Warning,
			TEXT("[GraphicsMenu] USiegeGraphicsSettingsSubsystem could not be resolved — every control is disabled. Nothing was written and the game's current settings are unaffected."));
	}

	if (StatusText != nullptr)
	{
		StatusText->SetText(FText::FromString(FString(SiegeGraphicsMenuText::Unavailable)));
	}

	SetAllControlsEnabled(false);

	// ⛔ Back stays alive. A panel you cannot leave is worse than a panel that
	// cannot change anything.
	if (BackButton != nullptr)
	{
		BackButton->SetIsEnabled(true);
	}
}

void USiegeGraphicsMenuWidget::SetAllControlsEnabled(bool bEnabled)
{
	for (const FName& GroupName : USiegeGraphicsSettingsSubsystem::GetQualityGroupNames())
	{
		if (USlider* Slider = FindGroupSlider(GroupName))
		{
			Slider->SetIsEnabled(bEnabled);
		}
	}

	// ⚠️ .Get() on every entry, deliberately: an aggregate initializer of UWidget*
	// from TObjectPtr<Derived> leans on a user-defined conversion followed by a
	// derived-to-base one, and this list is not worth finding out where that stops
	// being unambiguous.
	UWidget* const Controls[] =
	{
		OverallQualitySlider.Get(), ResolutionScaleSlider.Get(), VSyncCheckBox.Get(), AutoDetectButton.Get(),
		ScreenResolutionPrevButton.Get(), ScreenResolutionNextButton.Get(),
		WindowModePrevButton.Get(), WindowModeNextButton.Get(),
		FrameRateLimitPrevButton.Get(), FrameRateLimitNextButton.Get(),
		// TASK-1118: a dead panel has no live buttons. Reachable only if the
		// subsystem vanished while a countdown was live — in which case the timer
		// still reverts, which is the safe outcome, and Keep would have failed at
		// the facade's own null check anyway.
		KeepSettingsButton.Get(), RevertSettingsButton.Get()
		// 🚨⭐ TASK-1120: ShowFrameRateCounterCheckBox is DELIBERATELY ABSENT, and
		// this is a ruling rather than an oversight. Every control in this list
		// writes through the GRAPHICS FACADE, and this function's one caller is
		// "the facade did not resolve". The FPS-counter toggle writes through
		// USiegeSettingsSubsystem — a different store, `GFX-§3`'s named exception —
		// so a dead facade says nothing about whether that toggle can be read or
		// written, and greying it would be this panel disabling a control it has no
		// authority over. Its enabled state has exactly ONE writer,
		// RefreshFrameRateCounterRow(), which drives it off ITS OWN subsystem.
		// ⇒ the invariant this list upholds is "every control the FACADE owns, in
		// one call" — which is what it always meant; TASK-1120 is the row that made
		// the distinction visible. ⛔ Do not "fix" this by adding the checkbox: it
		// would be re-enabled two lines later by the seed anyway, leaving a list
		// entry that lies about what it does.
	};

	for (UWidget* Control : Controls)
	{
		if (Control != nullptr)
		{
			Control->SetIsEnabled(bEnabled);
		}
	}
}

USiegeGraphicsSettingsSubsystem* USiegeGraphicsMenuWidget::ResolveGraphicsSubsystem() const
{
	// ⛔ Automation override first — a bare NewObject widget has no world, so this
	// is the only way the suite can drive the panel at all.
	if (GraphicsOverrideForAutomationTests != nullptr)
	{
		return GraphicsOverrideForAutomationTests;
	}

	// Null-safe at every hop — the USettingsMenuWidget::ResolveSettingsSubsystem
	// shape, cloned. The subsystem lives on the GAME INSTANCE so the panel and the
	// in-match consumers read the same object.
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<USiegeGraphicsSettingsSubsystem>() : nullptr;
}

USiegeSettingsSubsystem* USiegeGraphicsMenuWidget::ResolveSettingsSubsystem() const
{
	// ⛔ Automation override first — same reason as its sibling above.
	if (SettingsOverrideForAutomationTests != nullptr)
	{
		return SettingsOverrideForAutomationTests;
	}

	// ⚠️ THE SAME GAME INSTANCE, A DIFFERENT SUBSYSTEM. This one is profile-scoped
	// (`GFX-§3`'s named exception) and it lives on the game instance for exactly
	// the reason its own header records: the value is WRITTEN here in L_MainMenu
	// and READ in L_Arena, and nothing widget-owned survives OpenLevel.
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<USiegeSettingsSubsystem>() : nullptr;
}

void USiegeGraphicsMenuWidget::SetSettingsSubsystemForAutomationTests(USiegeSettingsSubsystem* InSettings)
{
	SettingsOverrideForAutomationTests = InSettings;
}

// ═════════════════════════════════════════════════════════════════════════════
//  ⭐⭐ TASK-1120 — THE PANEL'S READOUT + ITS TOGGLE ROW
// ═════════════════════════════════════════════════════════════════════════════

void USiegeGraphicsMenuWidget::ArmFrameRateReadout()
{
	// A fresh window every time the panel opens: the previous one belonged to a
	// previous visit and could be arbitrarily old.
	FrameRateSample.Reset();

	if (FrameRateReadoutText != nullptr)
	{
		FrameRateReadoutText->SetText(FText::FromString(FSiegeFrameRateSample::ComposePendingText()));
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		// ⛔ NOT AN ERROR AND NOT EVEN A WARNING. A bare NewObject widget (the
		// suite) has no world; a real panel always does. The readout simply stays
		// at its pending text, which is honest — no timer ran, so no window closed.
		return;
	}

	FTimerManager& TimerManager = World->GetTimerManager();

	// ⛔ ClearTimer FIRST. NativeConstruct can run again on a re-added widget, and
	// arming over a live handle would leave two loops sampling the same member —
	// each closing the other's window and halving every reading.
	TimerManager.ClearTimer(FrameRateReadoutTimerHandle);

	// ⛔ A TIMER, NOT NativeTick — the board's rule and the reason for it: a
	// per-frame callback that exists only to print a frame-rate number is a cost
	// the number then reports. The first firing is one interval away and it only
	// OPENS the window (Advance returns false with nothing to subtract from), so
	// the first real reading lands at 2 × the interval. That is a second of dashes
	// on a screen the player has just opened, and it is the correct trade against
	// showing them a number measured over an unknown span.
	TimerManager.SetTimer(
		FrameRateReadoutTimerHandle, this, &USiegeGraphicsMenuWidget::RefreshFrameRateReadout,
		FrameRateReadoutIntervalSeconds, /*bLoop*/ true);
}

void USiegeGraphicsMenuWidget::DisarmFrameRateReadout()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FrameRateReadoutTimerHandle);
	}
	FrameRateReadoutTimerHandle.Invalidate();
	FrameRateSample.Reset();
}

void USiegeGraphicsMenuWidget::RefreshFrameRateReadout()
{
	if (FrameRateReadoutText == nullptr)
	{
		return;
	}

	uint64 FrameCounterNow = 0;
	double SecondsNow      = 0.0;
	FSiegeFrameRateSample::ReadEngineNow(FrameCounterNow, SecondsNow);

	FString ReadoutText;
	if (FrameRateSample.Advance(FrameCounterNow, SecondsNow, ReadoutText))
	{
		FrameRateReadoutText->SetText(FText::FromString(ReadoutText));
	}

	// ⛔ AND ON false: NOTHING. The previous text stays. That covers both the
	// window-opening call and a refused reading, and in neither case is there an
	// honest number to show — overwriting with a zero or a dash would turn a
	// missing sample into a claim about the machine.
}

void USiegeGraphicsMenuWidget::RefreshFrameRateCounterRow()
{
	if (ShowFrameRateCounterCheckBox == nullptr && ShowFrameRateCounterLabelText == nullptr)
	{
		return;
	}

	USiegeSettingsSubsystem* Settings = ResolveSettingsSubsystem();

	// ⛔ THE FALLBACK IS false, NOT the checkbox's current state: with no store to
	// read, the honest thing to show is the state the game will actually be in,
	// and with no store there is no persisted opt-in, so the counter will not
	// appear.
	//
	// ⭐ AND IT IS THE COUNTER'S OWN PREDICATE, DELIBERATELY — ⛔ ONE polarity site
	// for the whole feature. A checkbox that said "on" while the in-match counter
	// decided "off" would be the panel lying about the game, which is precisely
	// the class of divergence a shared predicate makes impossible.
	const bool bEnabled = USiegeFrameRateCounterWidget::ShouldShowFrameRateCounter(Settings);

	if (ShowFrameRateCounterCheckBox != nullptr)
	{
		// UCheckBox::SetIsChecked does NOT re-enter OnCheckStateChanged (only real
		// user interaction broadcasts) — measured for the VSync row and true here
		// for the same reason. The bSuppressRowEcho flag is therefore not needed on
		// this path; it is still honoured in the HANDLER, where a future asset tree
		// could route a synthetic click.
		ShowFrameRateCounterCheckBox->SetIsChecked(bEnabled);
		ShowFrameRateCounterCheckBox->SetIsEnabled(Settings != nullptr);
	}

	if (ShowFrameRateCounterLabelText != nullptr)
	{
		// The label itself says why the row is dead when it is dead — a greyed
		// checkbox with unchanged words is indistinguishable from a checkbox the
		// player simply has not ticked.
		ShowFrameRateCounterLabelText->SetText(FText::FromString(
			(Settings != nullptr)
				? FString(SiegeGraphicsMenuText::ShowFrameRateCounter)
				: FString(SiegeGraphicsMenuText::FrameRateCounterUnavailable)));
	}
}

void USiegeGraphicsMenuWidget::HandleShowFrameRateCounterChanged(bool bIsChecked)
{
	if (bSuppressRowEcho)
	{
		return;
	}

	USiegeSettingsSubsystem* Settings = ResolveSettingsSubsystem();
	if (Settings == nullptr)
	{
		// ⛔ DELIBERATELY *NOT* ShowPanelUnavailable(). That function declares the
		// whole GRAPHICS panel dead and disables every control on it; this row's
		// store is a different subsystem, and its absence says nothing about the
		// other eighteen rows. Re-seed this row alone — which also snaps the
		// checkbox back, because nothing was written.
		UE_LOG(LogSiegeGraphics, Warning,
			TEXT("[GraphicsMenu] The in-match FPS counter preference could not be written — USiegeSettingsSubsystem did not resolve. Nothing else on this panel is affected."));
		RefreshFrameRateCounterRow();
		return;
	}

	// ⛔ THE ONLY WRITE. The subsystem owns the no-op check, the disk write and the
	// broadcast; this line must never grow a second responsibility.
	Settings->SetFrameRateCounterEnabled(bIsChecked);
}

void USiegeGraphicsMenuWidget::HandleSettingsChanged(FName SettingName)
{
	// ⚠️ FILTERED, AND THAT IS THE OPPOSITE OF HandleGraphicsSettingsChanged'S
	// DOCUMENTED RULE — for a stated reason. The graphics facade fans one press
	// out across many values, so filtering there would mean reproducing its
	// fan-out. This store is not like that: it holds settings this panel does not
	// own (the assistant confirm toggle), and re-seeding on those would push this
	// checkbox for an unrelated reason. The filter is also what makes the payload
	// NAME load-bearing — a setter broadcasting the wrong token leaves this row
	// stale, which a test asserts by NAME rather than by broadcast count.
	if (SettingName != USiegeSettingsSubsystem::SettingName_ShowFrameRateCounter)
	{
		return;
	}

	RefreshFrameRateCounterRow();
}

void USiegeGraphicsMenuWidget::SetGraphicsSubsystemForAutomationTests(USiegeGraphicsSettingsSubsystem* InGraphics)
{
	GraphicsOverrideForAutomationTests = InGraphics;
}

void USiegeGraphicsMenuWidget::SetVideoModeCountdownDrivenManuallyForAutomationTests(bool bInDrivenManually)
{
	bDriveVideoModeCountdownManuallyForAutomationTests = bInDrivenManually;
}

USlider* USiegeGraphicsMenuWidget::FindGroupSlider(FName GroupName) const
{
	FGroupRowWidgets Row = const_cast<USiegeGraphicsMenuWidget*>(this)->ResolveGroupRow(GroupName);
	return (Row.Slider != nullptr) ? Row.Slider->Get() : nullptr;
}

UTextBlock* USiegeGraphicsMenuWidget::FindGroupValueText(FName GroupName) const
{
	FGroupRowWidgets Row = const_cast<USiegeGraphicsMenuWidget*>(this)->ResolveGroupRow(GroupName);
	return (Row.Value != nullptr) ? Row.Value->Get() : nullptr;
}

UTextBlock* USiegeGraphicsMenuWidget::FindGroupLabelText(FName GroupName) const
{
	FGroupRowWidgets Row = const_cast<USiegeGraphicsMenuWidget*>(this)->ResolveGroupRow(GroupName);
	return (Row.Label != nullptr) ? Row.Label->Get() : nullptr;
}

// ═════════════════════════════════════════════════════════════════════════════
//  PURE STATICS
// ═════════════════════════════════════════════════════════════════════════════

bool USiegeGraphicsMenuWidget::DiscardStagedVideoMode(USiegeGraphicsSettingsSubsystem* Graphics)
{
	if (Graphics == nullptr)
	{
		return false;
	}

	// ⛔ THE GUARD IS WHAT MAKES FOUR CALL SITES SAFE. BackPressed() calls this,
	// then RemoveFromParent() reaches NativeDestruct() which calls it again — and
	// the second call finds the window already closed and returns WITHOUT
	// reverting. ⇒ EXACTLY ONE RevertVideoModeChange() on the Back path (board
	// cl. 3b: "never two"; a double revert re-applies a mode nobody asked for).
	//
	// ⛔ AND THE GUARD MUST STAY ON IsVideoModeChangePending(), THE LATCHED ONE.
	// HasUnconfirmedVideoModeDifference() would be wrong here: after a wrap-around
	// there is no difference left while a provisional apply IS still open, and
	// returning early would strand the facade's window with nothing able to close
	// it — every save refused for the rest of the session (qa/TASK-1119.md
	// BLOCKER-1's second half). The two predicates are not interchangeable in
	// either direction.
	if (!Graphics->IsVideoModeChangePending())
	{
		return false;
	}

	UE_LOG(LogSiegeGraphics, Log,
		TEXT("[GraphicsMenu] A staged video mode was never confirmed and the panel is closing — reverting it. Without this the facade would refuse EVERY save and Auto-Detect for the rest of the session (qa/TASK-1114.md WARN-3 / WARN-9)."));

	// ⛔ THE FACADE'S revert AND NOTHING ELSE. Do NOT unwrap it: the engine's own
	// RevertVideoMode() restores five member fields and pushes NOTHING to the
	// display (GameUserSettings.cpp:276-285), which is why the facade follows it
	// with ApplyResolutionSettings(false). ⛔ And do NOT add a second apply after
	// this call — verified by reading RevertVideoModeChange
	// (SiegeGraphicsSettingsSubsystem.cpp:900-925), which still does both, in that
	// order, after TASK-1113's loop-1 rewrite (TASK-1118 cl. 9's read-to-the-
	// terminator duty, discharged here).
	Graphics->RevertVideoModeChange();
	return true;
}

bool USiegeGraphicsMenuWidget::IsCustomForDisplay(const USiegeGraphicsSettingsSubsystem* Graphics)
{
	// ⛔ board cl. (3c). NOT GetOverallScalabilityLevel() == -1 and NOT
	// IsOverallQualityCustom(): the engine answers over ELEVEN groups (including
	// the LandscapeQuality this panel does not show) AND requires ResolutionQuality
	// to match the preset's canonical scale, so Auto-Detect stranding the invisible
	// eleventh — or the player nudging the resolution-scale bar — would make the
	// preset read "Custom" FOREVER with all ten visible sliders in agreement and
	// nothing on screen able to fix it.
	return Graphics != nullptr && Graphics->IsOverallQualityCustomAcrossVisibleGroups();
}

float USiegeGraphicsMenuWidget::LevelToSliderValue(int32 Level)
{
	const int32 Clamped = FMath::Clamp(Level,
		USiegeGraphicsSettingsSubsystem::MinQualityLevel,
		USiegeGraphicsSettingsSubsystem::MaxQualityLevel);

	// Exact in float at every rung: 0, 0.25, 0.5, 0.75, 1.0 — which is why GFX-§5's
	// StepSize 0.25 lands ON a level and never between two.
	return static_cast<float>(Clamped) / static_cast<float>(USiegeGraphicsSettingsSubsystem::MaxQualityLevel);
}

int32 USiegeGraphicsMenuWidget::SliderValueToLevel(float SliderValue)
{
	// ⛔ ROUND, NEVER TRUNCATE. FMath::TruncToInt(0.9999f * 4) is 3, so a handle
	// parked on Cinematic by a float-imprecise drag would read as Epic and the
	// panel would silently write the wrong level back on commit.
	const int32 Level = FMath::RoundToInt(SliderValue * static_cast<float>(USiegeGraphicsSettingsSubsystem::MaxQualityLevel));
	return FMath::Clamp(Level,
		USiegeGraphicsSettingsSubsystem::MinQualityLevel,
		USiegeGraphicsSettingsSubsystem::MaxQualityLevel);
}

int32 USiegeGraphicsMenuWidget::StepIndex(int32 CurrentIndex, int32 Delta, int32 Count)
{
	if (Count <= 0)
	{
		// A stepper over an empty set is a programming error upstream; 0 keeps the
		// row inert instead of dividing by zero. The facade guarantees >= 1 for the
		// resolution list, so this is a floor, not an expected path.
		return 0;
	}

	// Wraps in BOTH directions: (-1 % 8) is -1 in C++, so the +Count is required.
	return ((CurrentIndex + Delta) % Count + Count) % Count;
}

bool USiegeGraphicsMenuWidget::DoesGroupApplyAtNextMatchStart(FName GroupName)
{
	// ⚠️ MEASURED, NOT COPIED FROM THE BOARD (SC-§101 — a prescribed remedy is a
	// claim). Board cl. (5) names "Foliage, View Distance and Effects"; that list
	// predates the GFX-§9 correction of 2026-09-07, which STRUCK the View-Distance
	// and fog levers. Today:
	//   • ViewDistanceQuality — the ENGINE's [ViewDistanceQuality@N] sets
	//     r.ViewDistanceScale (0.4 @0 → 1.0 @3), which scales HISM cull distances
	//     IMMEDIATELY. GetViewDistanceScale() is 1.0 at every level BY MEASUREMENT,
	//     so there is no project-side deferral to announce, and claiming one would
	//     UNDERSELL a control that already works this frame.
	//   • EffectsQuality — stock scalability CVars, immediate. The fog lever that
	//     GFX-§9 once bound to this group was struck for being the wrong group AND
	//     the wrong symbol; there is no scatter-driven Effects lever at all.
	//   • FoliageQuality — the ONE surviving Tier-D lever. The battlefield scatter's
	//     cull band is set inside RunScatterPasses (BattlefieldScatter.cpp:363),
	//     which runs once per match.
	// ⇒ ONE member. Flagged for TASK-1116 as a deliberate deviation from cl. (5)'s
	// three-row wording, with the measurement above as the grounds.
	return GroupName == USiegeGraphicsSettingsSubsystem::GroupName_FoliageQuality;
}

FString USiegeGraphicsMenuWidget::ComposeGroupHint(FName GroupName, bool bVolumetricFogEnabled)
{
	if (GroupName == USiegeGraphicsSettingsSubsystem::GroupName_ShadowQuality)
	{
		return bVolumetricFogEnabled
			? FString(SiegeGraphicsMenuText::ShadowHintFogOn)
			: FString(SiegeGraphicsMenuText::ShadowHintFogOff);
	}

	if (DoesGroupApplyAtNextMatchStart(GroupName))
	{
		return FString(SiegeGraphicsMenuText::FoliageHintNextMatch);
	}

	// The other eight need no hint: their engine CVars apply this frame and say so
	// by simply working. A hint on every row would make the two that matter
	// invisible.
	return FString();
}

FString USiegeGraphicsMenuWidget::ComposeLevelValueText(int32 Level, bool bCustom)
{
	if (bCustom)
	{
		return FString(SiegeGraphicsMenuText::CustomLevel);
	}

	// ⛔ THE ENGINE ALREADY LOCALISES Low / Medium / High / Epic / Cinematic
	// (Scalability::GetScalabilityNameFromQualityLevel, wrapped by the facade), so
	// there is no hand-written array here to drift from the ini.
	return USiegeGraphicsSettingsSubsystem::GetQualityLevelDisplayName(Level).ToString();
}

FString USiegeGraphicsMenuWidget::ComposeVideoModeCountdownText(int32 SecondsRemaining)
{
	if (SecondsRemaining <= 0)
	{
		// ⛔ NOT "0 seconds". At zero the revert is being issued in this same call
		// stack, so the line reports what is happening rather than counting into
		// the negatives — and a player who glances at it sees a menu doing
		// something, not a menu stuck.
		return FString(SiegeGraphicsMenuText::VideoModeRevertingNow);
	}

	// ⛔ SINGULAR AT ONE. It is one word, and it is the last thing a player reads
	// before the screen changes under them; "1 seconds" is the tell that nobody
	// looked at this screen.
	return FString::Printf(TEXT("%s Reverting in %d %s."),
		SiegeGraphicsMenuText::VideoModeConfirmPrompt,
		SecondsRemaining,
		SecondsRemaining == 1 ? TEXT("second") : TEXT("seconds"));
}

int32 USiegeGraphicsMenuWidget::AveragedLevel(const TArray<int32>& Levels)
{
	if (Levels.Num() == 0)
	{
		return 0;
	}

	int32 Sum = 0;
	for (const int32 Level : Levels)
	{
		Sum += Level;
	}

	const int32 Mean = FMath::RoundToInt(static_cast<float>(Sum) / static_cast<float>(Levels.Num()));
	return FMath::Clamp(Mean,
		USiegeGraphicsSettingsSubsystem::MinQualityLevel,
		USiegeGraphicsSettingsSubsystem::MaxQualityLevel);
}

// ═════════════════════════════════════════════════════════════════════════════
//  ⭐⭐⭐ TASK-1120 — THE IN-MATCH COUNTER (GFX-§7's load-bearing clause)
//
//  ⛔ THIS IS THE HALF THAT MAKES THE FEATURE REAL. The panel readout above is
//  measured in L_MainMenu, which has no ~340 scatter trees, no ~24k grass
//  instances, no volumetric fog and no Lumen-lit battlefield. This widget draws
//  in L_Arena, where the cost actually is.
// ═════════════════════════════════════════════════════════════════════════════

// The pinned readout name, in this tree too — GFX-§10's `FrameRateReadoutText`.
// ⚠️ THE SAME STRING AS THE PANEL'S CHILD, ON PURPOSE AND WITHOUT A COLLISION:
// they live in two different UWidgetTrees, and FName uniqueness is per tree. One
// word for one thing beats two words for one thing.
const TCHAR* USiegeFrameRateCounterWidget::FrameRateReadoutWidgetName = TEXT("FrameRateReadoutText");

USiegeFrameRateCounterWidget* USiegeFrameRateCounterWidget::CreateAndAddToViewport(
	APlayerController* OwningController,
	TSubclassOf<USiegeFrameRateCounterWidget> CounterClass,
	int32 ZOrder)
{
	if (!IsValid(OwningController))
	{
		UE_LOG(LogSiegeGraphics, Warning,
			TEXT("[FrameRateCounter] CreateAndAddToViewport: no owning player controller - no counter. NEVER FATAL: the match is unaffected, the player simply has no readout."));
		return nullptr;
	}

	// ⚠️ `.Get()` ON BOTH ARMS — the C2445 fix carried from
	// USiegeGraphicsMenuWidget::CreateAndAddToViewport, which records the
	// diagnosis: TSubclassOf has both a non-explicit TSubclassOf(UClass*) ctor and
	// a non-explicit operator UClass*(), so a conditional with one arm of each has
	// two equally good common types and the compiler must refuse to choose.
	const TSubclassOf<USiegeFrameRateCounterWidget> ResolvedClass =
		CounterClass ? CounterClass.Get() : USiegeFrameRateCounterWidget::StaticClass();

	USiegeFrameRateCounterWidget* Counter = CreateWidget<USiegeFrameRateCounterWidget>(OwningController, ResolvedClass);
	if (Counter == nullptr)
	{
		UE_LOG(LogSiegeGraphics, Warning,
			TEXT("[FrameRateCounter] CreateAndAddToViewport: CreateWidget returned null for class '%s' - no counter. NEVER FATAL."),
			*GetNameSafe(ResolvedClass));
		return nullptr;
	}

	// ⛔ ADDED TO THE VIEWPORT REGARDLESS OF THE PREFERENCE, AND COLLAPSED BY
	// NativeConstruct WHEN IT IS OFF. The alternative — create it only when the
	// pref is on — needs a second creation path for the mid-match toggle, and a
	// second creation path is where the "works on a fresh match, silently dead
	// after Play Again" class of defect lives (TASK-1122's own finding, and
	// SC-§94). One creation site, one visibility writer.
	Counter->AddToViewport(ZOrder);

	return Counter;
}

TSharedRef<SWidget> USiegeFrameRateCounterWidget::RebuildWidget()
{
	// ⚠️⚠️ ORDER IS LOAD-BEARING — the same three lines, in the same order, as
	// USiegeGraphicsMenuWidget::RebuildWidget(), and for the identical mechanical
	// reason: UUserWidget::RebuildWidget() reads WidgetTree->RootWidget AS IT
	// STANDS and returns an SSpacer when it is null, and WidgetTree itself is
	// allocated inside Initialize(). Building after Super would ship a silently
	// EMPTY widget that still passes every property read-back. ⛔ Do not reorder.
	Initialize();
	ConstructCounterTree();
	return Super::RebuildWidget();
}

void USiegeFrameRateCounterWidget::ConstructCounterTree()
{
	if (WidgetTree == nullptr)
	{
		UE_LOG(LogSiegeGraphics, Error,
			TEXT("[FrameRateCounter] No WidgetTree - the counter cannot build its tree."));
		return;
	}

	// The GFX-§2(b) escape hatch, in the same shape: an asset-authored tree wins
	// whole, because UMG has already resolved every BindWidgetOptional member from
	// it. (No WidgetBlueprint exists for this widget and none is reserved; the
	// guard costs one comparison and closes the door anyway.)
	if (WidgetTree->RootWidget != nullptr)
	{
		return;
	}

	if (FrameRateCounterRoot == nullptr)
	{
		FrameRateCounterRoot = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FrameRateCounterRoot"));
	}
	if (FrameRateCounterRoot == nullptr)
	{
		UE_LOG(LogSiegeGraphics, Error,
			TEXT("[FrameRateCounter] Could not construct FrameRateCounterRoot - the counter has no root."));
		return;
	}

	// 🚨⛔⛔ THE SINGLE MOST IMPORTANT LINE IN THIS FILE FOR A PLAYER.
	//
	// This Border FILLS THE VIEWPORT (that is how it aligns its content to the
	// top-right corner), so if it were hit-testable it would swallow EVERY click
	// in the match: card placement, unit orders, the card bar, the war map, all of
	// it. `HitTestInvisible` means "neither I nor my children are hit-testable",
	// which is exactly the contract.
	//
	// ⚠️ NOTE IT IS THE *OPPOSITE* OF `GFX-§2(f)`'s ruling for the menu backdrop,
	// which is deliberately `Visible` so it ABSORBS clicks and they cannot fall
	// through to QUIT. Same enum, opposite requirement — ⛔ do not copy one site to
	// the other. ApplyFrameRateCounterPreference() is the only writer of this
	// value after construction and it uses HitTestInvisible / Collapsed and
	// nothing else.
	FrameRateCounterRoot->SetVisibility(ESlateVisibility::HitTestInvisible);

	// Fully transparent: this border exists for ALIGNMENT, not for appearance.
	FrameRateCounterRoot->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.f));
	FrameRateCounterRoot->SetPadding(SiegeGraphicsMenuLayout::CounterScreenMargin);
	FrameRateCounterRoot->SetHorizontalAlignment(HAlign_Right);
	FrameRateCounterRoot->SetVerticalAlignment(VAlign_Top);

	WidgetTree->RootWidget = FrameRateCounterRoot;

	if (FrameRateCounterBorder == nullptr)
	{
		FrameRateCounterBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FrameRateCounterBorder"));
		if (FrameRateCounterBorder != nullptr)
		{
			FrameRateCounterBorder->SetBrushColor(SiegeGraphicsMenuLayout::CounterPlateColor);
			FrameRateCounterBorder->SetPadding(SiegeGraphicsMenuLayout::CounterPlatePadding);
			FrameRateCounterBorder->SetHorizontalAlignment(HAlign_Center);
			FrameRateCounterBorder->SetVerticalAlignment(VAlign_Center);
			FrameRateCounterRoot->SetContent(FrameRateCounterBorder);
		}
	}

	if (FrameRateReadoutText == nullptr)
	{
		FrameRateReadoutText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), FName(FrameRateReadoutWidgetName));
		if (FrameRateReadoutText != nullptr)
		{
			// Pending, never a fabricated number — see the panel's readout.
			FrameRateReadoutText->SetText(FText::FromString(FSiegeFrameRateSample::ComposePendingText()));
			FrameRateReadoutText->SetFontSize(SiegeGraphicsMenuLayout::CounterFontSize);
			FrameRateReadoutText->SetColorAndOpacity(FSlateColor(SiegeGraphicsMenuLayout::CounterTextColor));

			if (FrameRateCounterBorder != nullptr)
			{
				FrameRateCounterBorder->SetContent(FrameRateReadoutText);
			}
			else
			{
				// Degrade to a plate-less number rather than to no number at all.
				FrameRateCounterRoot->SetContent(FrameRateReadoutText);
			}
		}
	}
}

void USiegeFrameRateCounterWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// ⛔ SEED, **THEN** BIND (qa/TASK-005 major-2). ApplyFrameRateCounterPreference()
	// READS the store; binding first and waiting for a broadcast would leave the
	// counter in whatever state it was constructed in until the player next
	// toggled it — i.e. permanently, for the player who set it last session.
	ApplyFrameRateCounterPreference();

	if (USiegeSettingsSubsystem* Settings = ResolveSettingsSubsystem())
	{
		Settings->OnSettingsChanged.AddUniqueDynamic(this, &USiegeFrameRateCounterWidget::HandleSettingsChanged);
	}
}

void USiegeFrameRateCounterWidget::NativeDestruct()
{
	if (USiegeSettingsSubsystem* Settings = ResolveSettingsSubsystem())
	{
		Settings->OnSettingsChanged.RemoveDynamic(this, &USiegeFrameRateCounterWidget::HandleSettingsChanged);
	}

	// ⛔ STOP THE TIMER. A 0.5 s callback can already be queued at a widget being
	// torn down; it does nothing dangerous, but a SetText on a dead text block is
	// a callback that should not have been able to arrive.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CounterTimerHandle);
	}
	CounterTimerHandle.Invalidate();
	bCounterTimerArmed = false;
	FrameRateSample.Reset();

	Super::NativeDestruct();
}

bool USiegeFrameRateCounterWidget::ShouldShowFrameRateCounter(const USiegeSettingsSubsystem* Settings)
{
	// ⛔ THE FAIL-SAFE DIRECTION IS OFF, AND IT IS WRITTEN EXACTLY ONCE. An
	// unresolvable settings store must never put a diagnostic overlay on a shipped
	// player's battlefield — the opposite polarity to
	// IsAssistantConfirmEnabled()'s documented `true` fallback, and for the
	// mirror-image reason (there, a lookup failure must buy MORE human review;
	// here, LESS debug UI).
	return (Settings != nullptr) && Settings->IsFrameRateCounterEnabled();
}

void USiegeFrameRateCounterWidget::ApplyFrameRateCounterPreference()
{
	const USiegeSettingsSubsystem* Settings = ResolveSettingsSubsystem();
	const bool bShow = ShouldShowFrameRateCounter(Settings);

	// ⛔ THE ONE WRITER OF THIS WIDGET'S VISIBILITY. Two writers is how a counter
	// ends up visible with a dead timer (a frozen number, which a player reads as
	// a frozen GAME) or collapsed with a live one (invisible cost). Both halves,
	// here, always.
	if (FrameRateCounterRoot != nullptr)
	{
		FrameRateCounterRoot->SetVisibility(
			bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	UWorld* World = GetWorld();
	FTimerManager* TimerManager = World ? &World->GetTimerManager() : nullptr;

	if (!bShow)
	{
		// ⛔ COLLAPSED **AND** UNARMED. Slate skips a collapsed widget in layout and
		// paint, and with the timer cleared there is nothing left running at all —
		// which is what makes "the counter a player did not ask for costs nothing"
		// a structural fact rather than a claim.
		if (TimerManager != nullptr)
		{
			TimerManager->ClearTimer(CounterTimerHandle);
		}
		CounterTimerHandle.Invalidate();
		bCounterTimerArmed = false;

		// Forget the open window: when the player turns the counter back on, the
		// first reading must be measured from THEN, not across the invisible span.
		FrameRateSample.Reset();

		if (FrameRateReadoutText != nullptr)
		{
			// So a re-show never flashes the number from before it was hidden.
			FrameRateReadoutText->SetText(FText::FromString(FSiegeFrameRateSample::ComposePendingText()));
		}
		return;
	}

	if (TimerManager == nullptr)
	{
		// No world — the suite's bare NewObject counter. Visibility is already
		// correct; there is simply no timer manager to ask, and IsCounterTimerArmed()
		// reports false, which is the truth.
		bCounterTimerArmed = false;
		return;
	}

	FrameRateSample.Reset();

	// ⛔ ClearTimer FIRST — arming over a live handle would leave two loops
	// sampling one member, each closing the other's window.
	TimerManager->ClearTimer(CounterTimerHandle);
	TimerManager->SetTimer(
		CounterTimerHandle, this, &USiegeFrameRateCounterWidget::RefreshCounterText,
		CounterIntervalSeconds, /*bLoop*/ true);
	bCounterTimerArmed = true;

	UE_LOG(LogSiegeGraphics, Log,
		TEXT("[FrameRateCounter] In-match FPS/frame-time counter ON - sampling every %.2f s from GFrameCounter (GFX-7). No per-frame work: the widget does not tick."),
		CounterIntervalSeconds);
}

void USiegeFrameRateCounterWidget::RefreshCounterText()
{
	if (FrameRateReadoutText == nullptr)
	{
		return;
	}

	uint64 FrameCounterNow = 0;
	double SecondsNow      = 0.0;
	FSiegeFrameRateSample::ReadEngineNow(FrameCounterNow, SecondsNow);

	FString ReadoutText;
	if (FrameRateSample.Advance(FrameCounterNow, SecondsNow, ReadoutText))
	{
		FrameRateReadoutText->SetText(FText::FromString(ReadoutText));
	}
	// On false the previous text stays — see the panel's RefreshFrameRateReadout.
}

void USiegeFrameRateCounterWidget::HandleSettingsChanged(FName SettingName)
{
	// FILTERED: this store also carries the assistant confirm toggle, and a
	// confirm-toggle change must not re-arm a frame-rate timer. Asserting the NAME
	// rather than a broadcast count is what makes a swapped payload detectable
	// (qa/TASK-1119.md § LOOP 1, the M21 coincidence).
	if (SettingName != USiegeSettingsSubsystem::SettingName_ShowFrameRateCounter)
	{
		return;
	}

	ApplyFrameRateCounterPreference();
}

USiegeSettingsSubsystem* USiegeFrameRateCounterWidget::ResolveSettingsSubsystem() const
{
	if (SettingsOverrideForAutomationTests != nullptr)
	{
		return SettingsOverrideForAutomationTests;
	}

	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<USiegeSettingsSubsystem>() : nullptr;
}

void USiegeFrameRateCounterWidget::SetSettingsSubsystemForAutomationTests(USiegeSettingsSubsystem* InSettings)
{
	SettingsOverrideForAutomationTests = InSettings;
}
