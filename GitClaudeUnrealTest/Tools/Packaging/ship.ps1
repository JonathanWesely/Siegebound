#Requires -Version 5.1
<#
================================================================================
 ship.ps1 - Siegebound's reproducible cook + zip.  THE RECIPE LIVES HERE.
================================================================================

 Authored TASK-701 (2026-08-30).  Law: CONVENTIONS.md sections SHIP-0..SHIP-8d
 and PKG-1..PKG-11.  The command file .claude/commands/ship.md is the authority
 on the PROCEDURE AND ITS REFUSALS; THIS FILE is the authority on the RECIPE.
 Neither may contradict PKG-*; where they seem to, PKG-* wins and the
 divergence is a defect.

 --------------------------------------------------------------------------
 ASCII-ONLY, DELIBERATELY.  Windows PowerShell 5.1 decodes a UTF-8 script
 WITHOUT a BOM as ANSI, which silently corrupts non-ASCII characters - and a
 corrupted character inside a match pattern is a gate that stops working
 without saying so.  Therefore this file contains NO section signs, arrows or
 emoji.  Law citations are written ASCII-only:
     PKG-5a  ==  PKG-<section>5a        SHIP-2  ==  SHIP-<section>2
 Read them against CONVENTIONS.md's PKG- and SHIP- namespaces.
 --------------------------------------------------------------------------

 WHAT THIS SCRIPT IS FOR, IN ONE SENTENCE:
   The last cook's pass 1 reported BUILD SUCCESSFUL, booted to a perfect main
   menu, and was UNPLAYABLE.  This script exists so that can never silently
   recur.  See the COOKDIR block below - it is the single most important thing
   in this file.

 --------------------------------------------------------------------------
 THE FLAGS THAT DO NOT EXIST AND MAY NEVER BE ADDED (SHIP-1)
 --------------------------------------------------------------------------
   -SkipTests   -SkipCook   -SkipSuite   -NoVerify   -Force   -IgnoreGates
   -AcceptWarnings   -FastShip   -YesReally   -PixelAdjudicated
 A one-word release command that can emit a broken zip is WORSE THAN NO
 COMMAND.  Every gate below is a STOP, not a warning.  A switch that lets a
 gate be skipped is exactly the switch that gets used at 2 a.m.  If a gate is
 wrong, FIX THE GATE - do not add a way around it.

 -PixelAdjudicated IS ON THAT LIST DELIBERATELY AND PERMANENTLY (SHIP-8b).
 The C3 pixel adjudication re-enters this script as a MEASURED ARTIFACT ON
 DISK that the next invocation READS AND RE-VALIDATES against the capture's
 own hash and the build's identity - never as a flag that says "trust me, it
 passed".  A flag is an assertion; a record is a measurement.  Collapsing one
 into the other is the single lazy change that would void this whole seam.

 --------------------------------------------------------------------------
 EVERY PARAMETER, AND WHETHER IT CAN WEAKEN A GATE (answer: none can)
 --------------------------------------------------------------------------
  -DryRun         Plan-only.  Runs PHASE A for real, prints B..G, and executes
                  NOTHING that compiles, launches the editor, cooks, zips,
                  prunes or commits.  Cannot weaken a gate: it produces no
                  artifact at all.  (SHIP-7; TASK-701 spec clause 4.)
  -Configuration  Shipping (default, PKG-2a) or Development.  Selects which
                  artifact is built; every gate still runs, and the config is
                  IN the zip name so the two can never overwrite each other.
  -ProjectPath    .uproject override.  Default: derived from this script's own
                  location.  Path resolution only.
  -EngineRoot     UE install override.  Default: derived from the uproject's
                  EngineAssociation via the registry, then a documented
                  fallback.  Path resolution only.
  -StagingDir     Staging folder override.  Default: probed candidates, first
                  that EXISTS.  The PKG-7a fence is MEASURED against whatever
                  this resolves to - overriding it cannot escape the fence.
  -BootEvidence   Log or Pixel.  The default is Log, but IT IS A REQUEST, NOT A
                  DEMAND: gate A6 measures the machine class and, on an
                  INSTALLED engine building Shipping, AUTO-SELECTS Pixel and
                  says so - because route Log cannot exist there (PKG-9a-1).
                  Passing -BootEvidence Log EXPLICITLY in that case is a STOP,
                  not a downgrade: you asked for an instrument that does not
                  exist.  Pixel does NOT auto-pass either: it captures evidence
                  and STOPS for caller adjudication.  Neither value can make
                  the boot gate easier to clear - PKG-6a's REAL ARENA bar is
                  untouched by this parameter.  Only the INSTRUMENT changes.
  -CommitPaths    Explicit repo-relative paths for PHASE F.  Empty (default)
                  means NO COMMIT.  Each path is validated: must exist, must be
                  inside the work tree, must NOT be ignored, must NOT be under
                  the staging dir.  There is no way to pass a directory sweep.
  -CommitTrailer  Extra literal lines appended to the commit message.  Cosmetic
                  only; touches no gate.
  -ShipLogDir     Where this run's logs/state go.  Default: <staging>/.ship.
                  Lives with the staged build, i.e. outside git by the same
                  fence.  Path resolution only.

 --------------------------------------------------------------------------
 RESUME-BY-MEASUREMENT (NOT a skip flag - read this before judging it)
 --------------------------------------------------------------------------
 SHIP-2 orders PHASE D (package) before PHASE E (document), but the README
 must be INSIDE the zip and its "WHAT WAS VERIFIED" section can only be
 written AFTER PHASE C's evidence exists.  Taken literally the law would seal
 the PREVIOUS ship's README into this ship's zip.  Resolution:

   * PHASE D refuses to zip a README that does not name THIS run's zip
     (gate D0 README-NOT-UPDATED).  So the first invocation of a ship runs
     A + B + C, proves the build, and STOPS handing the caller the evidence.
   * The caller writes packagedZIPofGame/README.md from that evidence and
     invokes this script AGAIN.  PHASE A re-runs (it is cheap).  PHASE B + C
     are REUSED - but ONLY when a state file proves they were run for a
     BYTE-IDENTICAL build input: same HEAD, same build-relevant working tree,
     same configuration, same recipe, same staged exe timestamp, all verdicts
     PASS.  Any difference re-runs the compile, the suite and the cook in full.

 This skips no gate.  It refuses to re-prove an input it has already proven
 identical, and it says so loudly in the summary when it does.

 --------------------------------------------------------------------------
 THE THIRD TERMINAL VERDICT: ADJUDICATE (SHIP-1 as amended, SHIP-8)
 --------------------------------------------------------------------------
 A PowerShell script cannot read a PNG.  It can take the capture; it cannot
 judge what is in it.  The CALLER of /ship is a Claude agent, and it CAN see.
 So under evidence route 2 (rendered pixels - the STANDING route on an
 installed engine, PKG-9a-1) the boot gate does not self-certify and does not
 fail: it SUSPENDS.  C3-BOOT-ARENA writes its state, prints the PKG-6a bar
 VERBATIM beside the capture path, and ends the run with

     SHIP RESULT: ADJUDICATE C3 - <capture path>

 ADJUDICATE IS NOT A PASS AND IS NOT A SHIP.  An unresolved suspension is
 exactly as unshipped as a STOP.  Nothing is skipped, no flag exists, and a
 boot that cannot be proven still cannot ship.  Note what the phase order buys
 for free: C3 precedes PHASE D, so at an adjudication stop THERE IS NO ZIP YET
 - the previous artifact sits untouched and there is nothing to clean up.

 The caller LOOKS at the capture, judges it against the printed bar, writes the
 verdict record (schema printed at the stop), and re-invokes.  The next run
 RE-MEASURES the record's bindings and either resumes at PHASE D or STOPS.

 WHICH PHASES THIS SCRIPT OWNS: A, B, C, D, F, G.  PHASE E (the documentation)
 IS THE CALLER'S - it happens BETWEEN the two invocations, and D0-README is the
 gate that refuses to zip without it.  So the resume reads
 "PHASE D (zip) -> F (commit) -> G (report)", with E already done by hand.

 ON A RESUME THAT ACTUALLY SHIPS, THE RECORD IS RETIRED (SHIP-8d).  A consumed
 verdict is ARCHIVED into that run's log dir - never deleted, SHIP-8b(6) keeps
 the verdict and its observations - and the state's boot advances from
 ADJUDICATE to PASS carrying the adjudication string.  Without that, the FIRST
 invocation of EVERY subsequent ship would stop at C3-VERDICT-BINDING on a HEAD
 mismatch until a human deleted a file, which is a chore standing exactly where
 Jonathan's "anytime we make any changes, I can say ship" is supposed to be.

 --------------------------------------------------------------------------
 HOW THE BOOT-VERIFY REACHES AN ARENA (PKG-9f) - IT IS A CLICK, NOT AN ARGUMENT
 --------------------------------------------------------------------------
 TASK-699 measured, as a controlled A/B, that a SHIPPING binary IGNORES the map
 argument AND -ExecCmds and boots to its menu, while the Development binary -
 same machine, same content, minutes apart - logged LoadMap: /Game/Maps/L_Arena.
 So under route 2 this script launches the shipped exe with NO ARGUMENTS, the
 way a player double-clicks it, and CLICKS Play in the game's own menu before
 capturing.  Passing the refuted argument would guarantee a MENU capture, and
 the only honest adjudication of a menu is FAIL - the ship could then never
 pass, on any build, forever.
 THAT IS A ROUTE CHANGE ONLY.  PKG-6a's bar is untouched: what must be ON SCREEN
 is exactly what it was.  "The arena is hard to reach in Shipping" is an
 argument about the INSTRUMENT, never about the BAR.
 ITS COST IS STATED, NOT ENGINEERED AROUND: the click needs an UNLOCKED DESKTOP,
 so a Shipping /ship is SCHEDULABLE, not unattended.  Gate A8-DESKTOP measures
 that in PHASE A, before the 30-minute cook.

 --------------------------------------------------------------------------
 SHIP-9 - THE INSTRUMENT-VALIDATION LAW, AND WHY THIS FILE LOOKS PARANOID
 --------------------------------------------------------------------------
 A GATE MUST BE VALIDATED AGAINST THE FAILURE IT EXISTS TO DETECT, NEVER
 MERELY AGAINST SUCCESS.  This lane bought that law with two measured
 instances on one day, and BOTH are live in this file:
   SHIP-9a  gates written against DEVELOPMENT-build names that only
            coincidentally match the project name.  Under Shipping the names
            DIVERGE and every such gate stops forever.  The forever-stop moved
            four times here; three shared exactly that root cause.
            => STAGED BINARIES COME FROM THE MANIFESTS.  RUNNING PROCESSES
               COME FROM THE PATH.  Never from $ProjectName.
   SHIP-9b  BOTH obvious desktop-availability probes LIE on a locked session:
            OpenInputDesktop() returns "Default" and SetCursorPos SUCCEEDS.
            => A8-DESKTOP uses the CLICK RIG'S OWN predicate instead.
 PREFER AN IDENTITY THAT MOVES WITH THE CONDITION - a path, a manifest-derived
 relative path, a hash, a per-run stamp - over a name that is stable only by
 coincidence.  And note SHIP-9c(5): every one of these hid behind a green dry
 run, because a -DryRun cannot reach the phases they live in.

 --------------------------------------------------------------------------
 THE TRUTH SURFACE (the exit-code-lie law, extended to this script)
 --------------------------------------------------------------------------
 Build.bat returns 0 on a FAILED build; UAT's ERRORLEVEL lies the same way.
 That law applies to THIS script's own callers too.  The authoritative result
 is the LAST LINE of stdout:

     SHIP RESULT: PASS | STOP | ADJUDICATE C3 | DRYRUN-OK | DRYRUN-WOULD-STOP

 Exit codes are provided as a convenience and are consistent with it
 (0 PASS/DRYRUN-OK, 2 STOP, 3 DRYRUN-WOULD-STOP, 4 ADJUDICATE, 1 unexpected
 error) - but the caller reads the summary line, not just the code.
 EXIT 4 IS NOT SUCCESS.  Treat it exactly as seriously as exit 2.
================================================================================
#>

[CmdletBinding()]
param(
    [switch] $DryRun,

    [ValidateSet('Shipping','Development')]
    [string] $Configuration = 'Shipping',

    [string] $ProjectPath,
    [string] $EngineRoot,
    [string] $StagingDir,

    [ValidateSet('Log','Pixel')]
    [string] $BootEvidence = 'Log',

    [string[]] $CommitPaths = @(),
    [string[]] $CommitTrailer = @(),

    [string] $ShipLogDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# ==============================================================================
# THE RECIPE - constants.  Change these only with a task and a QA pass.
# ==============================================================================

# --- The maps allowlist (PKG-5) ------------------------------------------------
# Sources of truth, re-verified at every run by gate A7:
#   /Game/Maps/L_MainMenu  <- Config/DefaultEngine.ini:10 GameDefaultMap
#   /Game/Maps/L_Arena     <- SiegeSessionSubsystem.cpp:20 ArenaMapPath
# These are the only two maps the shipped menu flow travels to.  The engine
# template and marketplace demo levels (ThirdPerson, Variant_*, the asset-pack
# showrooms) are deliberately NOT cooked - they are scaffolding, not the game.
$RECIPE_MAPS = @(
    '/Game/Maps/L_MainMenu',
    '/Game/Maps/L_Arena'
)

# ==============================================================================
# *** THE COOKDIR LIST - PKG-5a.  THE MOST IMPORTANT LINES IN THIS FILE. ***
# ==============================================================================
#
# WHY THIS EXISTS (measured live, TASK-696 pass 1, 2026-08-29):
#   A -map allowlist cooks each listed map plus its HARD references and NOTHING
#   ELSE.  UAT printed its own BUILD SUCCESSFUL, the exe launched, and the main
#   menu came up looking perfect.  The game was UNPLAYABLE:
#       DT_Cards        not found -> the deck was EMPTY, the hand dealt 6 blanks
#       WBP_HUD         not found -> no HUD at all
#       BP_HeroCharacter / BP_CommanderNpc / BP_Torch  unavailable
#       mines invisible, torches invisible
#   Nothing errored.  The cook was "successful".
#
# AND IT WILL RECUR WITHOUT THIS LIST, BY CONSTRUCTION.  Soft references are
# this project's HOUSE STYLE, not an accident: ASiegePlayerController's
# constructor resolves its ENTIRE content contract through TSoftObjectPtr /
# TSoftClassPtr on purpose -
#       Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp:194-215
#       ("content contract ... everything soft, resolved null-safe at runtime")
# The cooker cannot see that graph.  Every future feature that adds one more
# soft ref re-arms the trap for free.
#
# THE FIX, PINNED AS THE RECIPE AND NOT AS ADVICE: keep the maps allowlist AND
# add the game-owned content directories through the cook commandlet's
# supported -COOKDIR=, passed via UAT -AdditionalCookerOptions.  Measured
# effect of exactly this list: ucas 901.1 -> 1027.8 MB, archive 1.789 -> 1.91
# GB, all five missing assets resolve, "not found"/"unavailable" 0, errors
# 12 -> 1.
#
# NOTE WHAT IS *NOT* HERE, AND KEEP IT THAT WAY: the ~8 GB of unused
# marketplace / asset-pack directories (Realistic_Rocks, MedievalCastle...,
# Megaplant_Library, Fire_Magic, Ice_Magic, Prickly_Knight, Tree_Pack_1, the
# showrooms) are NOT cooked.  Only assets the two shipped maps genuinely
# reference ride along from those packs.  That is why the package is 1.9 GB
# and not 10 GB.
#
# ALSO NOT DONE, AND MUST NOT BE: no repo file is edited to achieve this.  No
# DirectoriesToAlwaysCook ini surgery, no plugin enable/disable surgery.
#
# PKG-9e: this is NOT optional in Shipping either.  Soft references do not
# become hard because the configuration changed, and in Shipping - where
# logging may be compiled out - the resulting empty artifact is HARDER to
# detect, not easier.
#
# A HAND-RUN COOK THAT OMITS -COOKDIR IS A DEFECT EVEN IF UAT SAYS
# "BUILD SUCCESSFUL".
# ==============================================================================
$RECIPE_COOKDIRS = @(
    'Data',              # DT_Cards and every data table the deck builds from
    'UI',                # WBP_HUD, victory screen, war map, menus
    'Blueprints',        # BP_HeroCharacter, BP_CommanderNpc, BP_Torch, mines
    'Input',             # IA_* / IMC_* - the whole Enhanced Input surface
    'Characters',        # skeletal meshes, anim BPs, montages
    'Meshes',            # SM_* referenced by soft path
    'Materials',         # M_Ghost, M_SpellReticle, circle materials
    'Textures',          # icons the widgets soft-load
    'VFX',
    'Audio',
    'LevelPrototyping'
)

# --- Gate constants ------------------------------------------------------------
# Suite baseline (PKG-9c).  Deliberately a CONSTANT and not a parameter: a
# lowerable baseline is a skip flag wearing a number.
$SUITE_BASELINE      = 143
$SUITE_TEST_FILTER   = 'Siegebound'
$RETENTION_KEEP      = 2          # PKG-7b: N=2 per configuration
$MIN_FREE_GB         = 4          # SHIP-2 A4
$COOK_TIMEOUT_MIN    = 90
$COMPILE_TIMEOUT_MIN = 45
$SUITE_TIMEOUT_MIN   = 45
$BOOT_TIMEOUT_SEC    = 300
$SIZE_ALARM_RATIO    = 0.60       # vs the newest zip of the SAME config
# Bumped 3 -> 4 for the SHIP-8d adjudication fields (boot may now be
# 'ADJUDICATE', and the state carries the pending capture + its hash).  A
# schema-3 state file is simply not reused; PHASE B and C re-run in full.
$STATE_SCHEMA        = 4

# --- SHIP-8b: THE PIXEL ADJUDICATION VERDICT RECORD ---------------------------
# The record is written by the CALLER (a Claude agent, or Jonathan) and READ by
# the next invocation of this script.  This script never writes it and never
# judges whether its observations are TRUE - it cannot see.  What it CAN do,
# and does, is require that the observations were MADE, that the verdict is one
# of exactly two values, and that the whole record still BINDS to the capture
# and the build it was made against.  That is what keeps a measured verdict
# from degenerating into a flag.
$ADJ_SCHEMA          = 1
$ADJ_FILE_NAME       = 'ship-adjudication.json'

# The four judgeable criteria, and the PKG-6a text each one answers.  Printed
# VERBATIM at the stop (SHIP-8b(5)): the adjudicator judges THE WRITTEN BAR,
# never its own idea of "looks right".  An adjudication performed without the
# criteria in front of it is not one.
$ADJ_CRITERIA = @(
    @{ Key = 'arena'
       Bar = 'The artifact boots to L_Arena and an ARENA IS ON SCREEN - terrain, castle, a match in progress. A MENU IS NOT A PASS (PKG-6a).' }
    @{ Key = 'deck'
       Bar = 'A REAL CARD DECK BUILT FROM DT_Cards ROWS is visible - cards with art and costs. NOT 6 BLANK SLOTS (PKG-6a; this is the exact defect PKG-5a shipped).' }
    @{ Key = 'hud'
       Bar = 'THE HUD IS PRESENT - the in-match readouts render (gold / stance / the card bar chrome), not an empty screen (PKG-6a).' }
    @{ Key = 'hero'
       Bar = 'THE HERO PAWN IS SPAWNED and visible in the world (PKG-6a).' }
)

# SHIP-8b(2): a record that says only "PASS" or "criteria met" is MALFORMED and
# is REJECTED.  These floors are the mechanical form of "NAME WHAT YOU SAW".
# Same doctrine as KBD-2a condition 3: prove survivors by naming the OBJECTS,
# never by a summary.
$ADJ_MIN_OBS_CHARS   = 24
$ADJ_MIN_OBS_WORDS   = 4
$ADJ_BANNED_OBS      = @('pass','passed','ok','okay','fine','good','yes','true','n/a','na','none',
                         'criteria met','all criteria met','met','present','verified','confirmed',
                         'looks right','looks good','looks fine','as expected','correct','checked')

# SHIP-8b(4): AMBIGUITY IS A FAIL, AND THE ASYMMETRY IS THE POINT.  A hedged
# observation on a PASS record is rejected - the adjudicator must either say
# what it actually saw or record FAIL.  Applied to PASS records ONLY, so this
# can only ever make a PASS HARDER to obtain, never easier.
$ADJ_HEDGES = @('probably','i think','maybe','not sure','unsure','unclear','hard to tell',
                "can't tell",'cannot tell','unreadable','might be','possibly','seems to be',
                'appears to be','i assume','presumably','looks like it','difficult to tell')

# --- Boot-verify evidence (PKG-6a) --------------------------------------------
# POSITIVE markers.  All must be present, and they double as proof that the log
# channel is ALIVE - which is what makes the negative sweep below meaningful.
# PKG-9a bans reading absence-of-errors as evidence in a log that cannot emit;
# these lines are how this script proves the log CAN emit before it trusts any
# absence.
# The arena marker must match LoadMap AND the arena path ON THE SAME LINE.  Two
# separate matches would be satisfied by a menu-only boot that merely mentions
# the arena path somewhere else in the log - which is exactly the false pass
# PKG-6a exists to forbid.  Composed from the recipe below.
$BOOT_MARK_ARENA = 'LoadMap'
$BOOT_MARK_DECK  = 'draw pile from'                   # DeckComponent.cpp:57,116
$BOOT_MARK_HERO  = 'Castle-relative hero start for'   # SiegeGameMode.cpp:850
# The deck line must report a REAL row count, not an empty hand (PKG-6a: "not
# 6 blank slots").  DeckComponent.cpp:116 -
#   "built a %d-card draw pile from %d card rows"
# DeckComponent.cpp:57 is the override-deck form; both are accepted.
$BOOT_RE_DECK    = 'built a (\d+)-card draw pile from (?:the pending OVERRIDE deck .* \((\d+) entries\)|(\d+) card rows)'

# NEGATIVE sweep.  Any hit is a STOP.
$BOOT_FAIL_PATTERNS = @('not found', 'unavailable', 'continuing without a HUD')

# The ONE benign exception, narrow and cited.  PKG-4: with no GGUF present the
# assistant reports itself unavailable and the code's own words are "THE MATCH
# IS FULLY PLAYABLE".  Source:
#   Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSubsystem.cpp:1994
# Matched on the full phrase so it can never widen into a blanket excuse.
$BOOT_BENIGN_PATTERNS = @('the in-match assistant is unavailable this session')

# ==============================================================================
# PKG-9f - THE SHIPPING BOOT-VERIFY IS A CLICK, NOT A LAUNCH ARGUMENT
# ==============================================================================
# MEASURED BY TASK-699 AS A CONTROLLED A/B, not inferred from one failure:
# /Game/Maps/L_Arena, bare L_Arena, AND -ExecCmds="open /Game/Maps/L_Arena" all
# three booted the SHIPPING exe to the MENU, while the Development exe - same
# machine, same content, minutes apart - logged LoadMap: /Game/Maps/L_Arena.
# ==> A SHIPPING ARTIFACT CANNOT SELF-DRIVE INTO AN ARENA.  Launching it with
# the map argument produces a MENU, and a menu is NEVER a pass (PKG-6a), so a
# boot-verify built on that argument could only ever hand the adjudicator a FAIL
# - on every build, forever.  The ruled route is to reach the arena THE WAY A
# PLAYER DOES: launch with NO ARGUMENTS and CLICK Play in the game's own menu.
#
# THE CLICK POINT IS A MEASURED RECIPE VALUE, AND ITS ERROR DIRECTION IS SAFE.
# Menu geometry measured at 1280x720 by TASK-669 (handoffs/TASK-669-buildmaster
# .md section 6): a CENTRED 7-entry VBox, pitch 49.7 px, with the 3rd entry
# ("Deck Builder") centred at client (639, 310).  With 7 entries the 4th sits on
# the window centre, so entry n is centred at H/2 + (n - 4) * pitch - and that
# reproduces the measurement exactly (360 - 49.7 = 310.3, measured 310).  The
# entries in order, from the same capture:
#     1 Play (vs Bot)  2 Sandbox (No Bot)  3 Deck Builder  4 Multiplayer
#     5 Settings       6 Login             7 Quit
# Held as FRACTIONS of the client rect so it survives a resolution change.
#
# ==> A MIS-AIMED CLICK CAN ONLY EVER PRODUCE A "FAIL", NEVER A FALSE "PASS".
# If this point is wrong the game simply stays on its menu, the capture shows a
# menu, and the adjudicator - judging the PKG-6a bar printed beside it, which
# says in as many words that A MENU IS NOT A PASS - records FAIL.  The value is
# still a measurement and not a certainty, which is exactly why the run also
# takes a PRE-CLICK menu capture: a miss must be DIAGNOSABLE, not mysterious.
$BOOT_MENU_ENTRY_COUNT = 7
$BOOT_MENU_PLAY_INDEX  = 1          # "Play (vs Bot)" is the 1st entry
$BOOT_MENU_PITCH_FRAC  = 0.069028   # 49.7 px / 720 px (TASK-669, measured)
$BOOT_MENU_CLICKS      = 2          # the t669 rig's own default: UE eats the first press often enough that TASK-669 pinned two
$BOOT_MENU_WAIT_SEC    = 120        # for the game's own window to exist and carry a title
$BOOT_TRAVEL_WAIT_SEC  = 90         # menu -> level travel -> HUD settle, AFTER the click
# Derived once from the two measured numbers above so the DRY RUN prints exactly
# the point the LIVE run clicks - a plan that prints one number and executes
# another is the divergence SHIP-0 calls a defect.
$BOOT_MENU_PLAY_XFRAC  = 0.5
$BOOT_MENU_PLAY_YFRAC  = 0.5 + ($BOOT_MENU_PLAY_INDEX - (($BOOT_MENU_ENTRY_COUNT + 1) / 2.0)) * $BOOT_MENU_PITCH_FRAC

# --- TASK-716: THE TWO OBVIOUS DESKTOP PROBES LIE ON A LOCKED SESSION ---------
# The click above needs an unlocked desktop (PKG-9f states that cost plainly;
# standing TASK-076 is that simulated input dies on a locked session).  TASK-716
# hit it live - 12/12 polls locked, no click deliverable, boot-verify BLOCKED -
# and measured that the two probes anyone would reach for are WRONG:
#     OpenInputDesktop()  returned "Default"   <- says UNLOCKED.  It was not.
#     SetCursorPos()      SUCCEEDED            <- says UNLOCKED.  It was not.
#     (screen capture SUCCEEDED too, capturing the LOCK SCREEN - which is why
#      A6's instrument probe cannot answer this question either)
# The Windows 11 lock screen renders on the *Default* desktop via LockApp plus
# explorer's LockScreenBackstopFrame; only the CREDENTIAL stage switches to
# Winlogon.  A gate built on either API PASSES ON A LOCKED MACHINE and then
# boot-verifies into a backstop: the SHIP-8c fake-gate family exactly.
# The two things that told the truth were the CLASS/OWNER of the window under
# the click point - i.e. the click rig's own abort predicate - and the pixels.
$LOCK_WINDOW_CLASSES = @('LockScreenBackstopFrame')
$LOCK_OWNER_PROCS    = @('LockApp','LogonUI')

# --- Build-relevant path filter for resume-by-measurement ----------------------
# A markdown or pipeline file cannot change a cooked binary; a Config or Source
# file can.  Only these fragments participate in the reuse hash.
# NAMED "FRAGMENTS", NOT "PREFIXES", BECAUSE THAT IS WHAT THE MATCH BELOW DOES:
# it is a -like "*<value>*" CONTAINS test, not a StartsWith.  Over-inclusive, so
# the error direction is MORE re-cooks and never fewer - but the name and the
# behaviour had disagreed, and a constant whose name lies is how the next reader
# gets it wrong (QA TASK-702 NIT-1).
$BUILD_RELEVANT_FRAGMENTS = @('Source/', 'Content/', 'Config/', 'Plugins/')
$BUILD_RELEVANT_SUFFIXES  = @('.uproject')

# --- Fallback engine location (documented, overridable, not a user path) -------
$ENGINE_FALLBACK = 'C:\Program Files\Epic Games\UE_5.8'

# ==============================================================================
# Machinery
# ==============================================================================

$script:Gates       = New-Object System.Collections.ArrayList
$script:Facts       = New-Object System.Collections.ArrayList
$script:StopId      = $null
$script:StopReason  = $null
$script:StopRemedy  = $null
$script:WouldStop   = New-Object System.Collections.ArrayList
$script:RunUtc      = (Get-Date).ToUniversalTime()

# SHIP-8: the third terminal verdict's carrier.  Set ONLY by Note-Adjudicate,
# read by Write-Summary and the outer catch.  Its non-emptiness is what makes a
# run an ADJUDICATE rather than a PASS or a STOP.
$script:AdjCapture   = $null
$script:AdjRecord    = $null
$script:AdjResumeCmd = $null

function Say([string]$Text) { Write-Host $Text }
function Head([string]$Text) {
    Write-Host ''
    Write-Host ('=' * 78)
    Write-Host $Text
    Write-Host ('=' * 78)
}

function Add-Fact([string]$Key, [string]$Value) {
    [void]$script:Facts.Add([pscustomobject]@{ Key = $Key; Value = $Value })
    Say ("    {0,-34} {1}" -f ($Key + ':'), $Value)
}

function Add-GateRecord([string]$Id, [string]$Status, [string]$Evidence) {
    [void]$script:Gates.Add([pscustomobject]@{ Id = $Id; Status = $Status; Evidence = $Evidence })
}

# The single gate primitive.  In a real run the FIRST failure stops the ship.
# In -DryRun every gate is still evaluated and recorded so the operator sees the
# WHOLE plan, and the summary reports DRYRUN-WOULD-STOP - a dry run writes no
# artifact, so continuing past a failed gate cannot ship anything.
function Assert-Gate {
    param(
        [Parameter(Mandatory=$true)][string] $Id,
        [Parameter(Mandatory=$true)][bool]   $Ok,
        [string] $Evidence = '',
        [string] $Remedy   = ''
    )
    if ($Ok) {
        Add-GateRecord $Id 'PASS' $Evidence
        Say ("  [PASS] {0}  {1}" -f $Id, $Evidence)
        return $true
    }
    Add-GateRecord $Id 'STOP' $Evidence
    Say ("  [STOP] {0}  {1}" -f $Id, $Evidence)
    if ($Remedy) { Say ("         REQUIRED TO CLEAR: {0}" -f $Remedy) }
    if ($DryRun) {
        [void]$script:WouldStop.Add($Id)
        return $false
    }
    $script:StopId     = $Id
    $script:StopReason = $Evidence
    $script:StopRemedy = $Remedy
    throw 'SHIP-STOP'
}

function Note-Skipped([string]$Id, [string]$Why) {
    Add-GateRecord $Id 'PLAN' $Why
    Say ("  [PLAN] {0}  {1}" -f $Id, $Why)
}

# SHIP-8c: THE MECHANICAL PRE-FILTER MAY ONLY EVER *FAIL*.
# It records CHECK - deliberately NOT 'PASS' - when its cheap checks hold,
# because nothing it measures is evidence that the frame shows an ARENA.  A
# "does it look like an arena" heuristic in PowerShell would pass a black
# screen with a loading spinner WHILE WEARING A GATE'S CLOTHES, and a fake gate
# is worse than no gate.  Fail-only is still strictly useful: it catches "the
# capture never happened" without pretending to judge content.
function Assert-PreFilter {
    param(
        [Parameter(Mandatory=$true)][string] $Id,
        [Parameter(Mandatory=$true)][bool]   $Ok,
        [string] $Evidence = '',
        [string] $Remedy   = ''
    )
    if ($Ok) {
        Add-GateRecord $Id 'CHECK' $Evidence
        Say ("  [CHECK] {0}  {1}" -f $Id, $Evidence)
        return
    }
    Assert-Gate -Id $Id -Ok $false -Evidence $Evidence -Remedy $Remedy | Out-Null
}

# StrictMode-safe property read.  The verdict record is authored by hand/by an
# agent, so a missing property is an EXPECTED input, not a bug - it must
# produce a clean "malformed record" STOP, never an unexpected-error exit 1.
function Get-JsonProp {
    param($Obj, [string] $Name)
    if ($null -eq $Obj) { return $null }
    $p = $Obj.PSObject.Properties[$Name]
    if ($null -eq $p) { return $null }
    return $p.Value
}

# SHIP-8b(5): THE BAR IS RE-STATED AT THE STOP, NOT REMEMBERED.  Also SHIP-7:
# a -DryRun can never REACH this gate, so the dry run PRINTS this contract
# instead - a gate a test cannot reach must at minimum ANNOUNCE itself in that
# test's output.
function Write-AdjudicationContract {
    param(
        [string] $CapturePath,
        [string] $RecordPath,
        [string] $ResumeCmd,
        [switch] $Prospective
    )
    Say ''
    Say ('-' * 78)
    if ($Prospective) {
        Say ' ADJUDICATION CONTRACT (SHIP-8) - PRINTED BECAUSE A DRY RUN CAN NEVER REACH IT'
    } else {
        Say ' ADJUDICATION REQUIRED (SHIP-8) - THE SHIP IS SUSPENDED, NOT PASSED'
    }
    Say ('-' * 78)
    Say ''
    Say '  WHY THIS EXISTS: this script cannot read a PNG and will never claim it'
    Say '  did.  You can.  Judgment moves to where sight exists; THE BAR DOES NOT'
    Say '  MOVE (PKG-6a is untouched).'
    Say ''
    if ($CapturePath) {
        Say ('  CAPTURE TO JUDGE:  {0}' -f $CapturePath)
    } else {
        Say  '  CAPTURE TO JUDGE:  <written by the live run into the run-log dir>'
    }
    Say ''
    Say '  THE BAR, VERBATIM (PKG-6a).  Judge THIS, not your own idea of "looks'
    Say '  right".  An adjudication performed without the criteria in front of it'
    Say '  is not one:'
    Say ''
    foreach ($c in $ADJ_CRITERIA) {
        Say ('    [{0}]' -f $c.Key)
        Say ('        {0}' -f $c.Bar)
    }
    Say ''
    Say '  THE VERDICT RULES (SHIP-8b) - each one is mechanically enforced on the'
    Say '  next invocation, so a record that breaks one is REJECTED, not warned:'
    Say '    1. verdict is PASS or FAIL.  There is no third value and no "probably".'
    Say '    2. AMBIGUITY IS A FAIL.  Unreadable, black, truncated, uncertain,'
    Say '       "I think it is probably fine" - all of these are FAIL, not pass.'
    Say '       The asymmetry is the point: pixels now carry the WHOLE boot-verify,'
    Say '       so the pixel verdict must be at least as unforgiving as the log'
    Say '       line it replaced.'
    Say '    3. NAME WHAT YOU SAW, PER CRITERION, IN THE AFFIRMATIVE.  "deck: six'
    Say '       slots populated with card art, bottom-left" - NOT "PASS", NOT'
    Say '       "criteria met", NOT an empty string.  This script cannot judge'
    Say '       whether your observations are TRUE, but it absolutely can require'
    Say '       that they were MADE.'
    Say ('    4. Every observation: at least {0} characters and {1} words.' -f $ADJ_MIN_OBS_CHARS, $ADJ_MIN_OBS_WORDS)
    Say '    5. The record BINDS to this capture and this build.  It is re-measured'
    Say '       on resume and ANY mismatch is a STOP - a verdict cannot be replayed'
    Say '       onto a different capture or a different cook.'
    Say '    6. Jonathan may discharge this himself at any time, and HIS EYE WINS.'
    Say ''
    Say ('  WRITE THE RECORD TO:  {0}' -f $(if ($RecordPath) { $RecordPath } else { '<ship log dir>\' + $ADJ_FILE_NAME }))
    Say ''
    Say '  EXACT SCHEMA (UTF-8 JSON; replace every <...> with what you SAW):'
    Say ''
    Say  '    {'
    Say ('      "schema": {0},' -f $ADJ_SCHEMA)
    Say  '      "gate": "C3-BOOT-ARENA",'
    Say  '      "verdict": "PASS",'
    Say  '      "adjudicatedBy": "<who looked - agent name, or Jonathan>",'
    Say  '      "adjudicatedUtc": "<ISO-8601 UTC>",'
    Say  '      "capturePath": "<the CAPTURE TO JUDGE path above, verbatim>",'
    Say  '      "captureSha256": "<sha256 of that file, lowercase hex>",'
    Say  '      "head": "<the HEAD fact from this run>",'
    Say  '      "config": "<the Configuration fact from this run>",'
    Say  '      "stageExeBytes": <the staged game exe size in bytes>,'
    Say  '      "stageExeUtc": "<the staged game exe LastWriteTimeUtc, round-trip o>",'
    Say  '      "observations": {'
    $last = $ADJ_CRITERIA.Count - 1
    for ($i = 0; $i -le $last; $i++) {
        $comma = ','
        if ($i -eq $last) { $comma = '' }
        Say ('        "{0}": "<what you SAW for {0}>"{1}' -f $ADJ_CRITERIA[$i].Key, $comma)
    }
    Say  '      }'
    Say  '    }'
    Say ''
    Say '    (On a FAIL you may add "reason": "<the named reason>". The four'
    Say '     observations are required either way - a FAIL also says what it saw.)'
    Say ''
    Say '    PowerShell will give you the two measured fields:'
    Say '      (Get-FileHash -LiteralPath <capture> -Algorithm SHA256).Hash.ToLower()'
    Say '      (Get-Item -LiteralPath <staged exe>).Length'
    Say '      (Get-Item -LiteralPath <staged exe>).LastWriteTimeUtc.ToString("o")'
    Say ''
    Say '  BEFORE YOU RE-INVOKE, WRITE THE README (ship.md step 3, PHASE E).'
    Say '  PHASE E IS YOURS, NOT THIS SCRIPT''S - it happens BETWEEN the two'
    Say '  invocations - and D0-README is the FIRST gate of the resume: it STOPS'
    Say '  unless packagedZIPofGame\README.md already names THIS ship''s zip.  A'
    Say '  resume that skips it does not fail late, it fails immediately.'
    Say ''
    Say '  THEN RE-INVOKE.  A PASS resumes at PHASE D (zip) -> F (commit) -> G'
    Say '  (report) against THE SAME STAGED BUILD, with NO SECOND COOK.  The'
    Say '  record is then RETIRED: archived into that run''s log dir (never'
    Say '  deleted - SHIP-8b(6) keeps the verdict) and the ship state advances'
    Say '  from ADJUDICATE to PASS, so the NEXT ship starts clean instead of'
    Say '  stopping on a record nobody remembered to remove.  A FAIL ends the'
    Say '  ship with your named reason and nothing is written:'
    Say ''
    Say ('    {0}' -f $(if ($ResumeCmd) { $ResumeCmd } else { '<the same ship.ps1 invocation you just ran>' }))
    Say ''
    Say ('-' * 78)
    Say ''
}

# SHIP-1 amended / SHIP-8a: the THIRD terminal verdict.  Called ONLY after the
# state write, so the resume is armed before the run suspends (SHIP-8d).  It
# throws unconditionally: the pixel branch is unreachable under -DryRun by
# construction (it lives inside the live-cook branch), and returning instead of
# throwing would let PHASE D run on an unadjudicated build - the one outcome
# this whole seam exists to prevent.
function Note-Adjudicate {
    param(
        [Parameter(Mandatory=$true)][string] $Id,
        [Parameter(Mandatory=$true)][string] $CapturePath,
        [Parameter(Mandatory=$true)][string] $RecordPath,
        [Parameter(Mandatory=$true)][string] $ResumeCmd
    )
    $script:AdjCapture   = $CapturePath
    $script:AdjRecord    = $RecordPath
    $script:AdjResumeCmd = $ResumeCmd
    Add-GateRecord $Id 'ADJUD' ("SUSPENDED for pixel adjudication (SHIP-8). Capture: {0}" -f $CapturePath)
    Say ("  [ADJUD] {0}  SUSPENDED for pixel adjudication - NOT a pass, NOT a ship" -f $Id)
    throw 'SHIP-ADJUDICATE'
}

# git plumbing.  NOTE ON $LASTEXITCODE, because a sweep will find it here:
# this is the ONLY place an exit code is read, and it is read for GIT QUERIES
# ONLY - rev-parse, status, check-ignore, diff --cached - where the exit code
# IS the documented answer ("is this path ignored", "did rev-parse resolve").
# NO BUILD, COOK, SUITE or BOOT verdict is ever taken from an exit code
# anywhere in this file; those are parsed out of the tools' own logs, because
# Build.bat and UAT both return 0 on failure (the exit-code-lie law).
#
# Every git call in this script is read-only EXCEPT two, both in PHASE F:
#   git add -- <one explicit path>   (never -A, never ., never a directory sweep)
#   git commit -F <message file>     (never --amend, and there is NO push)
function Invoke-Git {
    param([Parameter(Mandatory=$true)][string[]] $GitArgs, [string] $WorkDir)
    $dir = $WorkDir
    if (-not $dir) { $dir = $script:RepoRoot }
    $out = & git -C $dir @GitArgs 2>$null
    return @{ Ok = ($LASTEXITCODE -eq 0); Out = $out }
}

# Runs a native tool, captures stdout+stderr to files, returns the paths.  The
# exit code is captured but NEVER used as a verdict - callers parse the log.
function Invoke-Tool {
    param(
        [Parameter(Mandatory=$true)][string] $FilePath,
        [Parameter(Mandatory=$true)][string] $ArgumentString,
        [Parameter(Mandatory=$true)][string] $LogBase,
        [int] $TimeoutMinutes = 60
    )
    $outLog = "$LogBase.out.log"
    $errLog = "$LogBase.err.log"
    Say ("    > {0} {1}" -f $FilePath, $ArgumentString)
    $p = Start-Process -FilePath $FilePath -ArgumentList $ArgumentString `
                       -NoNewWindow -PassThru `
                       -RedirectStandardOutput $outLog -RedirectStandardError $errLog
    $done = $p.WaitForExit($TimeoutMinutes * 60 * 1000)
    if (-not $done) {
        try { $p.Kill() } catch { }
        return @{ TimedOut = $true; Out = $outLog; Err = $errLog; Code = -1 }
    }
    return @{ TimedOut = $false; Out = $outLog; Err = $errLog; Code = $p.ExitCode }
}

function Test-LogPattern {
    param(
        [Parameter(Mandatory=$true)][string[]] $Paths,
        [Parameter(Mandatory=$true)][string]   $Pattern,
        [switch] $Simple
    )
    foreach ($p in $Paths) {
        if (-not (Test-Path -LiteralPath $p)) { continue }
        if ($Simple) {
            $hit = Select-String -LiteralPath $p -Pattern $Pattern -SimpleMatch -List -ErrorAction SilentlyContinue
        } else {
            $hit = Select-String -LiteralPath $p -Pattern $Pattern -List -ErrorAction SilentlyContinue
        }
        if ($hit) { return $true }
    }
    return $false
}

function Get-LogMatches {
    param(
        [Parameter(Mandatory=$true)][string[]] $Paths,
        [Parameter(Mandatory=$true)][string]   $Pattern,
        [switch] $Simple
    )
    $res = @()
    foreach ($p in $Paths) {
        if (-not (Test-Path -LiteralPath $p)) { continue }
        if ($Simple) {
            $res += @(Select-String -LiteralPath $p -Pattern $Pattern -SimpleMatch -ErrorAction SilentlyContinue)
        } else {
            $res += @(Select-String -LiteralPath $p -Pattern $Pattern -ErrorAction SilentlyContinue)
        }
    }
    # TASK-1193: ALWAYS hand back an ARRAY.  A bare 'return $res' unrolls through
    # the pipeline: zero hits reach the caller as $null and ONE hit as a bare
    # MatchInfo, and under Set-StrictMode -Version Latest (:214) '.Count' THROWS
    # on both (measured, PS 5.1.26100.9168).  That is what turned a GREEN suite
    # into 'SHIP RESULT: STOP at UNEXPECTED-ERROR' at B2 - and 'Result={Fail}'
    # matching NOTHING is the NORMAL path of a green run.  The unary comma wraps
    # the array so it survives the pipeline whole: 0 / 1 / N hits => Object[] of
    # Count 0 / 1 / N, every time, for every caller ($perf in B2 and $deckHits in
    # C3 carried the same latent throw for a log with exactly one matching line).
    return ,$res
}

# ------------------------------------------------------------------------------
# TASK-1193: the B2-SUITE verdict, lifted out of the gate so it can be run against
# a LOG without a cook.  SHIP-9c: a gate is validated against the failure it
# exists to detect, on real artefacts - this one was, on the real green run's
# automation.log (555/0 => PASS), Tools/SuiteRunnerFixtures/red-suite.log (18/2
# => STOP) and result-absent.log (0/0 => STOP), by
# Tools/Packaging/Fixtures/b2_verdict_check.ps1, which lifts THIS text out of
# THIS file by AST and never retypes it (SHIP-0).  The gate prints exactly the
# strings this returns.
#
# Two rules, and they are the whole point:
#   1. The needles are LITERAL text matched with -Simple.  The original wrote
#      the regex-escaped 'Result=\{Success\}' under -SimpleMatch, which looks
#      for a BACKSLASH and matched 0 of 555 lines on every live run since the
#      file was born.  The pattern and the matcher must agree.
#   2. A ZERO IS A VERDICT - never a pass, never an exception.  0 Success AND
#      0 Fail is exactly what a suite that never ran, or a parser pointed at
#      the wrong file, produces (SC-114: a null is the one reading every broken
#      apparatus returns) => a NAMED STOP, ruled here and not left to the
#      baseline clause, so lowering the baseline can never turn it green.
# Counts are the MAX per log, not the SUM: -abslog and the redirected stdout
# carry the SAME lines, and a sum would report 1110 Success for a 555 suite.
# ------------------------------------------------------------------------------
function Get-SuiteVerdict {
    param(
        [Parameter(Mandatory=$true)][string[]] $Paths,
        [Parameter(Mandatory=$true)][int]      $Baseline
    )
    $SUCCESS_NEEDLE = 'Result={Success}'
    $FAIL_NEEDLE    = 'Result={Fail}'
    $existing  = @($Paths | Where-Object { Test-Path -LiteralPath $_ })
    $success   = 0
    $fail      = 0
    $performed = 0
    $failNames = @()
    # No @() around Get-LogMatches, deliberately: it already returns an array
    # (the ',' in its return), and @() around a command that emits ONE array
    # object NESTS it - zero hits would arrive as a 1-element array holding an
    # empty array (Count 1, no .Line), i.e. a green suite would read Fail=1.
    # Measured by b2_verdict_check.ps1 on this function's first run (rev 1).
    foreach ($p in $existing) {
        $s = Get-LogMatches -Paths @($p) -Pattern $SUCCESS_NEEDLE -Simple
        $f = Get-LogMatches -Paths @($p) -Pattern $FAIL_NEEDLE    -Simple
        if ($s.Count -gt $success) { $success = $s.Count }
        if ($f.Count -gt $fail)    { $fail    = $f.Count }
        foreach ($hit in $f) {
            if ($hit.Line -match 'Name=\{([^}]*)\}') { $failNames += $Matches[1] }
        }
    }
    $failNames = @($failNames | Select-Object -Unique)
    $perf = Get-LogMatches -Paths $Paths -Pattern '(\d+)\s+tests?\s+performed'
    if ($perf.Count -gt 0) { $performed = [int]$perf[$perf.Count - 1].Matches[0].Groups[1].Value }

    $noResults = (($success -eq 0) -and ($fail -eq 0))
    $ok = (-not $noResults) -and ($fail -eq 0) -and ($performed -ge $Baseline) -and ($success -ge $Baseline)
    $logsRead = ('{0} of {1} logs read' -f $existing.Count, $Paths.Count)
    $remedy = ''
    if ($noResults) {
        $evidence = ('NOT MEASURED - suite produced no results: 0 {0} and 0 {1} lines ({2} performed; {3})' -f `
                     $SUCCESS_NEEDLE, $FAIL_NEEDLE, $performed, $logsRead)
        $remedy   = 'A zero is not green. Either the suite never ran or this gate is reading the wrong file: check the -abslog path and read suite.out.log for a crash before the first test. There is no skip flag.'
    } else {
        $evidence = ('{0} performed / {1} Success / {2} Fail (baseline {3}; {4}; max per log)' -f `
                     $performed, $success, $fail, $Baseline, $logsRead)
        if ($failNames.Count -gt 0) { $evidence += (' - failing: ' + (@($failNames | Select-Object -First 8) -join ', ')) }
        if ($fail -gt 0) {
            $remedy = 'Any failing test STOPS the ship. Fix the test or the code - there is no skip flag.'
        } elseif (-not $ok) {
            $remedy = ('The suite ran below the {0}-test baseline. A shrunken suite is a STOP, not a pass; find the tests that went missing.' -f $Baseline)
        }
    }
    return @{
        Ok = $ok; NoResults = $noResults
        Performed = $performed; Success = $success; Fail = $fail; FailNames = $failNames
        LogsRead = $existing.Count; LogsGiven = $Paths.Count
        Evidence = $evidence; Remedy = $remedy
    }
}

function Get-Sha256OfString([string]$Text) {
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($Text)
        return ([System.BitConverter]::ToString($sha.ComputeHash($bytes))).Replace('-','').ToLowerInvariant()
    } finally { $sha.Dispose() }
}

function Format-Size([long]$Bytes) {
    if ($Bytes -ge 1073741824) { return ('{0:N2} GB ({1:N0} B)' -f ($Bytes / 1073741824), $Bytes) }
    if ($Bytes -ge 1048576)    { return ('{0:N1} MB ({1:N0} B)' -f ($Bytes / 1048576), $Bytes) }
    return ('{0:N0} B' -f $Bytes)
}

# ==============================================================================
# WIN32 - process and window MEASUREMENT for the PKG-9f boot-verify route
# ==============================================================================
# DECLARED LAZILY AND EXACTLY ONCE, for two reasons that are both about failing
# in the right voice:
#   * Add-Type on a type name that already exists THROWS, and this script may be
#     re-run or dot-sourced inside one PowerShell session.
#   * It is compiled ON FIRST USE, never at load, so a run that never needs to
#     drive a desktop (route Log) never pays for it - and a machine where the
#     compile fails yields a clean "the click could not be attempted" STOP at
#     C3-CAPTURE instead of an UNEXPECTED-ERROR before PHASE A even starts.
$script:ShipWin32Ready = $null
function Initialize-ShipWin32 {
    if ($null -ne $script:ShipWin32Ready) { return $script:ShipWin32Ready }
    if ('ShipWin32' -as [type]) { $script:ShipWin32Ready = $true; return $true }
    try {
        Add-Type @'
using System;
using System.Text;
using System.Runtime.InteropServices;
public class ShipWin32 {
    [DllImport("user32.dll")] public static extern IntPtr WindowFromPoint(POINT p);
    [DllImport("user32.dll")] public static extern IntPtr GetAncestor(IntPtr h, uint f);
    [DllImport("user32.dll")] public static extern int  GetWindowThreadProcessId(IntPtr h, out int procId);
    [DllImport("user32.dll")] public static extern int  GetClassName(IntPtr h, StringBuilder s, int m);
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int cx, int cy, uint flags);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern void mouse_event(uint flags, uint dx, uint dy, uint data, UIntPtr extra);
    public struct POINT { public int X, Y; }
    public struct RECT  { public int Left, Top, Right, Bottom; }
}
'@
        $script:ShipWin32Ready = $true
    } catch {
        $script:ShipWin32Ready = $false
    }
    return $script:ShipWin32Ready
}

# ------------------------------------------------------------------------------
# THE RUNNING GAME IS RESOLVED BY *PATH*, NEVER BY NAME.  (SHIP-9a INSTANCE 1)
# ------------------------------------------------------------------------------
# THIS IS THE PKG-10 DEFECT CLASS, ONE GATE TO THE LEFT, AND IT WAS MEASURED:
# Get-Process -Name applies NO implicit wildcard, and UE names the staged binary
# after the CONFIGURATION, not after the project -
#     Development : <Project>.exe
#     Shipping    : <Project>-Win64-Shipping.exe
# so 'Get-Process -Name <Project>' matches ONLY the title-less root shim and
# NEVER the process that owns the window.  TASK-715 section 6 launched this exact
# stage through the shim and listed both:
#     PID 28688  GitClaudeUnrealTest                 WS     7 MB  Title=''
#     PID  9104  GitClaudeUnrealTest-Win64-Shipping  WS 1,640 MB  Title='Siegebound'
# Under Development the two share a name, both match, and the title resolves -
# which is why the ONLY route anyone had exercised was the one that worked, while
# the STANDING route (Shipping + Pixel) stopped at C3-BOOT-TITLE on every run.
# Resolving by PATH is name-free: it catches the shim AND the game, it works when
# they are the same process, and it fixes the kill loop for free.
#
# TWO GUARDS THAT ARE NOT DECORATION:
#   1. Process.Path THROWS for protected and cross-bitness processes.  This
#      script runs under $ErrorActionPreference = 'Stop', so an unguarded read
#      inside a full enumeration would surface as UNEXPECTED-ERROR (exit 1) -
#      a gate failing in the wrong voice.  Every read is wrapped.
#   2. -ExcludePids is how a STALE process from an EARLIER run is kept out of
#      this run's evidence.  The stage path is stable across runs, so path
#      matching alone would happily adopt a leftover game as proof that THIS
#      cook booted.  The caller snapshots what is already running BEFORE it
#      launches anything and passes those PIDs here.
function Get-StageGameProcess {
    param(
        [Parameter(Mandatory=$true)][string] $StageRoot,
        [int[]] $ExcludePids = @()
    )
    $prefix = $StageRoot.TrimEnd('\','/') + '\'
    $out = New-Object System.Collections.ArrayList
    foreach ($p in @(Get-Process -ErrorAction SilentlyContinue)) {
        if ($ExcludePids -contains $p.Id) { continue }
        $path = $null
        try { $path = $p.Path } catch { $path = $null }
        if (-not $path) { continue }
        if ($path.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)) { [void]$out.Add($p) }
    }
    return @($out)
}

# Reads the MainWindowTitle of a process without letting a dead/protected handle
# turn into an UNEXPECTED-ERROR.  Refresh() first: a Process object caches the
# title from the moment it was fetched, and this one is polled in a loop.
function Get-ProcWindowTitle {
    param($Proc)
    try { $Proc.Refresh(); return [string]$Proc.MainWindowTitle } catch { return '' }
}

# ------------------------------------------------------------------------------
# IS THIS DESKTOP DRIVABLE?  The ONLY honest test (TASK-716; SHIP-9b INSTANCE 2).
# ------------------------------------------------------------------------------
# See the $LOCK_WINDOW_CLASSES comment for the measurement: OpenInputDesktop and
# SetCursorPos BOTH report a locked session as available, and screen capture
# happily captures the lock screen.  What told the truth was the CLASS and OWNER
# of the root window under the click point - the click rig's own abort predicate.
#
# DIRECTION OF FAILURE, DELIBERATELY CHOSEN: this is a POSITIVE LOCK DETECTOR.
# It reports Locked only when it can NAME a lock-screen class or owner; "I could
# not measure" is reported as a fact and is NOT a stop.  A check that demanded
# proof of an unlocked desktop would become forever-stop number five the first
# time Windows renamed a window class, and this lane has already moved a
# forever-stop four times.  A MISSED lock still cannot manufacture a pass: the
# rig aborts at the click, C3-CAPTURE records that no input was delivered, and
# the run STOPS.
function Test-DesktopLocked {
    param([int] $X = -1, [int] $Y = -1)
    $res = @{ Locked = $false; Measured = $false; Detail = '' }
    if (-not (Initialize-ShipWin32)) {
        $res.Detail = 'the user32 measurement shim could not be compiled on this machine'
        return $res
    }
    try {
        Add-Type -AssemblyName System.Windows.Forms
        $scr = [System.Windows.Forms.Screen]::PrimaryScreen
        if ($null -eq $scr) { $res.Detail = 'no primary screen (no interactive desktop session?)'; return $res }
        $b  = $scr.Bounds
        $px = $X
        $py = $Y
        if ($px -lt 0) { $px = $b.X + [int]($b.Width  / 2) }
        if ($py -lt 0) { $py = $b.Y + [int]($b.Height / 2) }
        $pt = New-Object ShipWin32+POINT
        $pt.X = $px
        $pt.Y = $py
        $root = [ShipWin32]::GetAncestor([ShipWin32]::WindowFromPoint($pt), 2)   # GA_ROOT
        $ownerId = 0
        [ShipWin32]::GetWindowThreadProcessId($root, [ref]$ownerId) | Out-Null
        $sb = New-Object System.Text.StringBuilder 256
        [ShipWin32]::GetClassName($root, $sb, 256) | Out-Null
        $cls   = $sb.ToString()
        $owner = ''
        $op = Get-Process -Id $ownerId -ErrorAction SilentlyContinue
        if ($op) { $owner = $op.ProcessName }
        $res.Measured = $true
        $res.Detail   = ("root window under ({0},{1}): class '{2}', owner '{3}'" -f $px, $py, $cls, $owner)
        if (($LOCK_WINDOW_CLASSES -contains $cls) -or ($LOCK_OWNER_PROCS -contains $owner)) { $res.Locked = $true }
    } catch {
        $res.Detail = ('could not measure: {0}' -f $_.Exception.Message)
    }
    return $res
}

# ------------------------------------------------------------------------------
# DRIVE THE SHIPPED GAME'S OWN MENU (PKG-9f's ruled route).
# ------------------------------------------------------------------------------
# This is t669_topclick.ps1's mechanism - topmost without activation, real
# cursor, the GetAncestor(WindowFromPoint) abort predicate, a double press,
# restore non-topmost - REPRODUCED IN THIS FILE rather than shelled out to.
# WHY IT IS NOT CALLED AS AN EXTERNAL SCRIPT, stated so it reads as a decision
# and not as an oversight: the rig lives in a SESSION SCRATCHPAD that does not
# survive the session.  A release procedure that depends on an ephemeral file is
# a forever-stop waiting to happen, and this lane has spent three tasks removing
# forever-stops.  The mechanism is 20 lines; the dependency was the risk.
#
# THE ABORT PREDICATE IS THE POINT, NOT THE CLICK.  If the window under the
# cursor is not the game - a lock-screen backstop, another topmost window, a
# stolen focus - NO INPUT IS INJECTED and the caller is told.  That refusal is
# what stops a "click" that lands on someone else's window from being reported
# as a drive into the arena.
function Invoke-MenuClick {
    param(
        [Parameter(Mandatory=$true)] $Hwnd,
        [Parameter(Mandatory=$true)][double] $XFrac,
        [Parameter(Mandatory=$true)][double] $YFrac,
        [int] $Clicks = 2
    )
    $r = @{ Ok = $false; Detail = ''; ScreenX = 0; ScreenY = 0 }
    if (-not (Initialize-ShipWin32)) {
        $r.Detail = 'the user32 click shim could not be compiled on this machine - NO INPUT WAS INJECTED'
        return $r
    }
    if (($null -eq $Hwnd) -or ($Hwnd -eq [IntPtr]::Zero)) {
        $r.Detail = 'the game exposed no main window handle to click'
        return $r
    }
    $rect = New-Object ShipWin32+RECT
    if (-not [ShipWin32]::GetClientRect($Hwnd, [ref]$rect)) { $r.Detail = 'GetClientRect failed on the game window'; return $r }
    $cw = $rect.Right  - $rect.Left
    $ch = $rect.Bottom - $rect.Top
    if (($cw -le 0) -or ($ch -le 0)) { $r.Detail = ('the game client rect is {0}x{1}' -f $cw, $ch); return $r }

    $TOPMOST   = [IntPtr](-1)
    $NOTOPMOST = [IntPtr](-2)
    $SWP_FLAGS = 0x13    # SWP_NOSIZE(1) | SWP_NOMOVE(2) | SWP_NOACTIVATE(0x10)
    [ShipWin32]::SetWindowPos($Hwnd, $TOPMOST, 0, 0, 0, 0, $SWP_FLAGS) | Out-Null
    Start-Sleep -Milliseconds 300

    $clientX = [int]($cw * $XFrac)
    $clientY = [int]($ch * $YFrac)
    $pt = New-Object ShipWin32+POINT
    $pt.X = $clientX
    $pt.Y = $clientY
    [ShipWin32]::ClientToScreen($Hwnd, [ref]$pt) | Out-Null
    [ShipWin32]::SetCursorPos($pt.X, $pt.Y) | Out-Null
    Start-Sleep -Milliseconds 250
    $r.ScreenX = $pt.X
    $r.ScreenY = $pt.Y

    # THE ABORT PREDICATE.  No click is injected unless the game itself owns the
    # pixel we are about to press.
    $probe = New-Object ShipWin32+POINT
    $probe.X = $pt.X
    $probe.Y = $pt.Y
    $root = [ShipWin32]::GetAncestor([ShipWin32]::WindowFromPoint($probe), 2)
    if ($root -ne $Hwnd) {
        [ShipWin32]::SetWindowPos($Hwnd, $NOTOPMOST, 0, 0, 0, 0, $SWP_FLAGS) | Out-Null
        $sb = New-Object System.Text.StringBuilder 256
        [ShipWin32]::GetClassName($root, $sb, 256) | Out-Null
        $r.Detail = ("ABORTED - the window under the click point is NOT the game (root class '{0}'). NO INPUT WAS INJECTED." -f $sb.ToString())
        return $r
    }
    [ShipWin32]::SetForegroundWindow($Hwnd) | Out-Null
    Start-Sleep -Milliseconds 200
    for ($i = 0; $i -lt $Clicks; $i++) {
        [ShipWin32]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)   # LEFTDOWN
        Start-Sleep -Milliseconds 90
        [ShipWin32]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)   # LEFTUP
        Start-Sleep -Milliseconds 350
    }
    [ShipWin32]::SetWindowPos($Hwnd, $NOTOPMOST, 0, 0, 0, 0, $SWP_FLAGS) | Out-Null
    $r.Ok = $true
    $r.Detail = ("delivered {0} press(es) at client ({1},{2}) of {3}x{4} -> screen ({5},{6})" -f `
                 $Clicks, $clientX, $clientY, $cw, $ch, $pt.X, $pt.Y)
    return $r
}

# Writes a full-primary-screen PNG.  Returns the path, or '' if the instrument
# threw - the caller NEVER converts "I could not measure" into a pass.
# (A6's own probe is deliberately left duplicated and untouched - QA TASK-702
# NIT-7 records the refactor as a contained follow-up AFTER this lane.)
function Save-ScreenCapture {
    param([Parameter(Mandatory=$true)][string] $Path)
    try {
        Add-Type -AssemblyName System.Drawing, System.Windows.Forms
        $b   = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
        $bmp = New-Object System.Drawing.Bitmap $b.Width, $b.Height
        $g   = [System.Drawing.Graphics]::FromImage($bmp)
        $g.CopyFromScreen($b.Location, [System.Drawing.Point]::Empty, $b.Size)
        $bmp.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
        $g.Dispose(); $bmp.Dispose()
        return $Path
    } catch { return '' }
}

# ==============================================================================
# PKG-10 STAGE HYGIENE - the manifests are the ONLY authority for what belongs
# ==============================================================================
#
# MEASURED BY TASK-699 (2026-08-30): UAT DOES NOT CLEAN THE STAGE.  714.5 MB of
# stale DEVELOPMENT binaries survived a SHIPPING cook -
#     GitClaudeUnrealTest/Binaries/Win64/GitClaudeUnrealTest.exe  347,694,080 B
#     GitClaudeUnrealTest/Binaries/Win64/GitClaudeUnrealTest.pdb  401,526,784 B
# - present in ZERO of the three manifests (UFSFiles, NonUFSFiles, DebugFiles),
# which name only ...-Win64-Shipping.exe and its companions.  Pure orphans of
# an earlier cook.
#
# THE HAZARD IS NOT THE DEAD WEIGHT - IT IS THAT THE WRONG EXE RUNS.  A curious
# player who opens Binaries\Win64\ and double-clicks GitClaudeUnrealTest.exe
# silently launches the OLD DEVELOPMENT BUILD - different config, different
# behaviour, stale code - WHILE BELIEVING THEY RAN SIEGEBOUND.  That is the
# PKG-5a family exactly: an artifact that works, reports nothing wrong, and is
# not the thing anyone meant to ship.
#
# THIS IS A GATE AND NOT A ONE-OFF CLEANUP, because UAT will do it again on
# every cook, forever.
#
# ORDER IS LOAD-BEARING: PRUNE -> MEASURE -> BOOT-VERIFY -> ZIP.  A boot-verify
# run against a tree that is then modified proves something about a tree that
# no longer exists.  VERIFY THE ARTIFACT YOU ACTUALLY SHIP.
#
# NEVER DELETE BY NAME-GUESS.  The binary's name is CONFIGURATION-DEPENDENT
# (UE names a Development binary <Project>.exe and a Shipping binary
# <Project>-Win64-Shipping.exe), so the very name that is an orphan under
# Shipping is the LEGITIMATE binary under Development.  Manifest membership is
# the only safe test, and it is the one the law names.
#
# ==============================================================================
# TWO TRAPS, BOTH HIT LIVE BY TASK-715 WHEN IT RAN THIS PRUNE BY HAND.
# READ THEM BEFORE YOU "SIMPLIFY" ANYTHING BELOW.
# ==============================================================================
#
# TRAP 1 - "DELETE EVERY NON-MANIFEST FILE" DELETES THE ENTIRE GAME.
#   The .ucas (1.078 GB) and the .utoc appear in NO manifest: the UFS manifest
#   lists the 3,446 assets packed INTO the container, not the container itself.
#   And it is worse than merely wrong - it LOOKS SAFE ON A SPOT-CHECK, because
#   the .pak IS listed while its own siblings are not.  An author who checks
#   one file concludes the rule is sound and ships a rule that deletes the
#   game's content.
#   MEASURED BY 715: 28 of 70 staged files are non-manifest; exactly 2 of them
#   were orphans.
#   => PKG-10's "game BINARY" scope is LOAD-BEARING, not loose wording.  THIS
#   GATE PRUNES ORPHANED GAME BINARIES (.exe / .pdb) AND NOTHING ELSE.  It must
#   NEVER be generalised to "non-manifest files".
#
# TRAP 2 - A BASENAME GREP EXONERATES THE ORPHAN.
#   Searching the manifests for "GitClaudeUnrealTest.exe" MATCHES - but it
#   matches the ROOT SHIM (<stage>\Windows\GitClaudeUnrealTest.exe), which is a
#   different, legitimate file: it is the player's click target, and 715 proved
#   by PID->path that it resolves to the Shipping exe.  An agent checking by
#   filename therefore finds a hit, declares compliance, AND KEEPS THE HAZARD.
#   => MEMBERSHIP IS TESTED ON THE FULL STAGE-RELATIVE PATH, NEVER A BASENAME.
#
# 715's measured stage, for whoever reads this next:
#   orphans   : GitClaudeUnrealTest/Binaries/Win64/GitClaudeUnrealTest.exe
#               + .pdb            (749,220,864 B together, 0 manifest hits)
#   survivor  : GitClaudeUnrealTest/Binaries/Win64/GitClaudeUnrealTest-Win64-
#               Shipping.exe      (Manifest_NonUFSFiles_Win64.txt line 4)
#   also legit: the root shim, and the two Engine vc_redist installers - which
#               is why the "exactly one runnable exe" assertion is scoped to
#               GAME exes and would otherwise fail on files that belong there.

# Reads every Manifest_*.txt at the stage root.  Each line is
# "<relative path><TAB><timestamp>"; the path is the first field.
function Get-StageManifestSet {
    param([Parameter(Mandatory=$true)][string] $StageRoot)
    $set   = @{}
    $paths = New-Object System.Collections.ArrayList
    $files = @()
    if (Test-Path -LiteralPath $StageRoot) {
        $files = @(Get-ChildItem -LiteralPath $StageRoot -Filter 'Manifest_*.txt' -File -ErrorAction SilentlyContinue)
    }
    foreach ($mf in $files) {
        foreach ($line in @(Get-Content -LiteralPath $mf.FullName -ErrorAction SilentlyContinue)) {
            if (-not $line) { continue }
            $rel = (($line -split "`t")[0]).Trim()
            if (-not $rel) { continue }
            $rel = $rel.Replace('\','/')
            if (-not $set.ContainsKey($rel.ToLowerInvariant())) {
                $set[$rel.ToLowerInvariant()] = $true
                [void]$paths.Add($rel)
            }
        }
    }
    return @{ Count = $files.Count; Names = @($files | ForEach-Object { $_.Name }); Set = $set; Paths = @($paths) }
}

function Get-StageRelPath {
    param([Parameter(Mandatory=$true)][string] $StageRoot,
          [Parameter(Mandatory=$true)][string] $FullPath)
    return $FullPath.Substring($StageRoot.Length).TrimStart('\','/').Replace('\','/')
}

# THE STAGED GAME BINARY'S NAME MUST NEVER BE GUESSED (PKG-10).  Guessing
# <Project>.exe under Shipping resolves to the ORPHAN this gate deletes.  The
# manifests are the authority whenever they exist; the UE naming convention is
# the fallback for a stage that has not been cooked yet (where nothing exists
# to resolve anyway, and the caller only uses the path with Test-Path).
$script:StageExeResolvedBy  = 'not resolved yet'
$script:StageShimResolvedBy = 'not resolved yet'

function Get-StageGameExeRel {
    param([Parameter(Mandatory=$true)][string] $StageRoot,
          [Parameter(Mandatory=$true)][string] $Project,
          [Parameter(Mandatory=$true)][string] $Config)
    $conv = ("{0}/Binaries/Win64/{1}.exe" -f $Project, $Project)
    if ($Config -ne 'Development') {
        $conv = ("{0}/Binaries/Win64/{1}-Win64-{2}.exe" -f $Project, $Project, $Config)
    }
    $m = Get-StageManifestSet $StageRoot
    if ($m.Count -ge 1) {
        $pre  = ("{0}/binaries/win64/" -f $Project.ToLowerInvariant())
        $hits = @($m.Paths | Where-Object {
            $l = $_.ToLowerInvariant()
            $l.StartsWith($pre) -and $l.EndsWith('.exe') -and ($l.Substring($pre.Length).IndexOf('/') -lt 0)
        })
        # QA TASK-702 NIT-2: an ambiguous manifest must be VISIBLE, not papered
        # over by a silent fallback.  The fallback is still correct (C2-STAGE-
        # PRESENT catches a wrong resolution), but "which authority answered" is
        # the fact that tells a reader whether to trust the name.
        if ($hits.Count -eq 1) {
            $script:StageExeResolvedBy = 'the manifests (1 matching game exe)'
            return $hits[0]
        }
        $script:StageExeResolvedBy = ("CONVENTION FALLBACK - the manifests matched {0} game exe(s), not exactly 1" -f $hits.Count)
        return $conv
    }
    $script:StageExeResolvedBy = 'CONVENTION FALLBACK - no manifests at the stage root (uncooked stage)'
    return $conv
}

# THE ROOT SHIM - the file a player double-clicks - IS DERIVED THE SAME WAY, AND
# THAT IS NOT SYMMETRY FOR ITS OWN SAKE.
# Hunting the defect class rather than its three instances: every gate in this
# script that names a staged binary from $ProjectName is the same bug waiting
# for the name to diverge.  The shim IS project-named today in BOTH
# configurations - TASK-715 section 4 measured <stage>\GitClaudeUnrealTest.exe
# surviving a SHIPPING cook, and Manifest_NonUFSFiles_Win64.txt names it - so
# this one is NOT presently broken.  It was a GUESS all the same, and it feeds
# the launch target, C2-STAGE-PRESENT and D2-ZIP-READBACK's click-target check.
# Deriving it costs one function and removes the guess; the convention fallback
# keeps it from ever becoming a forever-stop of its own.
# TRAP 2 APPLIES HERE IN REVERSE: the shim is selected because it sits AT THE
# STAGE ROOT (no '/' in the relative path), never because of its basename - the
# 347 MB orphan shares that basename and lives one directory down.
function Get-StageShimExeRel {
    param([Parameter(Mandatory=$true)][string] $StageRoot,
          [Parameter(Mandatory=$true)][string] $Project)
    $conv = ("{0}.exe" -f $Project)
    $m = Get-StageManifestSet $StageRoot
    if ($m.Count -ge 1) {
        $hits = @($m.Paths | Where-Object {
            $l = $_.ToLowerInvariant()
            $l.EndsWith('.exe') -and ($l.IndexOf('/') -lt 0)
        })
        if ($hits.Count -eq 1) {
            $script:StageShimResolvedBy = 'the manifests (1 root-level exe)'
            return $hits[0]
        }
        $script:StageShimResolvedBy = ("CONVENTION FALLBACK - the manifests matched {0} root-level exe(s), not exactly 1" -f $hits.Count)
        return $conv
    }
    $script:StageShimResolvedBy = 'CONVENTION FALLBACK - no manifests at the stage root (uncooked stage)'
    return $conv
}

# Prunes every non-manifest game .exe/.pdb, then asserts the PKG-10 invariant.
# IDEMPOTENT by construction, so it is safe - and correct - to run it both
# before the boot-verify (verify what you ship) and before the zip (ship what
# you verified, including on a REUSED stage that this run did not cook).
# Returns the stage-relative path of the one runnable game exe.
function Invoke-StageHygiene {
    param(
        [Parameter(Mandatory=$true)][string] $StageRoot,
        [Parameter(Mandatory=$true)][string] $Project,
        [Parameter(Mandatory=$true)][string] $Config,
        [Parameter(Mandatory=$true)][string] $When
    )
    Say ("  stage hygiene (PKG-10), {0}:" -f $When)

    $m = Get-StageManifestSet $StageRoot
    # No manifests => no authority => WE DO NOT GUESS.  A stage we cannot
    # adjudicate is a STOP, never a "prune what looks stale".
    Assert-Gate -Id 'C2-STAGE-MANIFESTS' -Ok ($m.Count -ge 1) `
        -Evidence ("{0} manifest file(s) at the stage root [{1}] listing {2} path(s)" -f $m.Count, ($m.Names -join ', '), $m.Set.Count) `
        -Remedy 'PKG-10 makes the manifests the ONLY authority for what belongs in the stage. Without them nothing may be pruned by name-guess. Re-cook so UAT writes them.' | Out-Null

    # SCOPE NOTE (QA TASK-702 WARN-4): this walk deliberately includes Engine/,
    # even though the "exactly one runnable exe" invariant below deliberately
    # EXCLUDES it.  The two are not in tension: PKG-10 makes the manifests the
    # authority EVERYWHERE, so an engine-side binary in no manifest is an orphan
    # exactly as a game-side one is - while the INVARIANT has to exclude Engine/
    # because UE's vc_redist INSTALLERS are manifest-listed exes that are not
    # game binaries and would fail a perfectly clean stage.  On 715's measured
    # stage this is inert: all 19 non-manifest Engine/ files are .dll/.json/
    # .html/.bat/.sh/.ini, and the .exe/.pdb filter below never reaches them.
    $orphans = @()
    foreach ($f in @(Get-ChildItem -LiteralPath $StageRoot -Recurse -File -ErrorAction SilentlyContinue)) {
        # TRAP 1 GUARD - DO NOT REMOVE THIS FILTER.  Widening the scope from
        # "game binaries" to "non-manifest files" deletes the .ucas (1.078 GB)
        # and the .utoc, which are in no manifest BY DESIGN, and destroys the
        # package.  The .pak's presence in the manifest makes that mistake look
        # safe on a spot-check.  .exe and .pdb ONLY.
        $ext = $f.Extension.ToLowerInvariant()
        if (($ext -ne '.exe') -and ($ext -ne '.pdb')) { continue }
        # TRAP 2 GUARD - FULL STAGE-RELATIVE PATH, NEVER A BASENAME.  The root
        # shim shares its basename with the orphan; a basename test declares the
        # orphan legitimate and keeps the hazard.
        $rel = Get-StageRelPath $StageRoot $f.FullName
        if (-not $m.Set.ContainsKey($rel.ToLowerInvariant())) {
            $orphans += [pscustomobject]@{ Rel = $rel; Full = $f.FullName; Bytes = $f.Length }
        }
    }

    $freed = 0
    foreach ($o in $orphans) {
        Say ("    ORPHAN (in NO manifest, runnable, DELETING): {0}  {1}" -f $o.Rel, (Format-Size $o.Bytes))
        Remove-Item -LiteralPath $o.Full -Force -ErrorAction SilentlyContinue
        $freed += $o.Bytes
    }
    $pruneEv = 'no orphans - every staged .exe/.pdb is manifest-listed'
    if ($orphans.Count -gt 0) {
        $pruneEv = ("pruned {0} non-manifest binary/ies, {1} freed: {2}" -f `
                    $orphans.Count, (Format-Size $freed), (($orphans | ForEach-Object { $_.Rel }) -join ', '))
    }
    Add-GateRecord 'C2-STAGE-PRUNE' 'PASS' $pruneEv
    Say ("  [PASS] C2-STAGE-PRUNE  {0}" -f $pruneEv)

    # THE INVARIANT: exactly ONE runnable game exe, plus the root shim.
    # SCOPED TO *GAME* EXES DELIBERATELY (715's measurement): Engine/ holds UE's
    # own redistributable installers (vc_redist.x64.exe, vc_redist.arm64.exe),
    # which are manifest-listed and belong in the package - counting them would
    # make this gate fail on a perfectly clean stage.  The ROOT SHIM is likewise
    # legitimate and must survive: it is the player's click target, and 715
    # proved by PID->path that it resolves to the Shipping exe.  So: exclude
    # Engine/, then split the remainder into "at the stage root" (the shim) and
    # "nested" (the runnable game binaries), and require exactly one of the
    # latter.
    $allExe = @(Get-ChildItem -LiteralPath $StageRoot -Recurse -File -Filter '*.exe' -ErrorAction SilentlyContinue |
                Where-Object { -not (Get-StageRelPath $StageRoot $_.FullName).ToLowerInvariant().StartsWith('engine/') })
    $shim     = @($allExe | Where-Object { (Get-StageRelPath $StageRoot $_.FullName).IndexOf('/') -lt 0 })
    $runnable = @($allExe | Where-Object { (Get-StageRelPath $StageRoot $_.FullName).IndexOf('/') -ge 0 })
    $runNames = @($runnable | ForEach-Object { Get-StageRelPath $StageRoot $_.FullName })

    # QA TASK-702 NIT-3: PKG-10's invariant reads "exactly ONE runnable game exe
    # (plus the root shim)" - SINGULAR.  The shim count was only ever REPORTED;
    # asserting it costs nothing and closes the gap.  '-le 1' rather than '-eq 1'
    # deliberately: a stage built without the bootstrap has zero shims and is not
    # a hazard, while TWO root-level exes is exactly the wrong-exe-to-click
    # ambiguity this gate exists to forbid.
    Assert-Gate -Id 'C2-STAGE-ONE-EXE' -Ok (($runnable.Count -eq 1) -and ($shim.Count -le 1)) `
        -Evidence ("{0} runnable game exe(s) [{1}] + {2} root shim(s) [{3}]" -f `
                   $runnable.Count, ($runNames -join ', '), $shim.Count, (@($shim | ForEach-Object { $_.Name }) -join ', ')) `
        -Remedy 'PKG-10: the zip contains EXACTLY ONE runnable game plus AT MOST ONE root shim, and the runnable one is the one the manifests name. A second exe means a player can double-click the WRONG BUILD and never know. Investigate every name listed above before shipping.' | Out-Null

    if ($runnable.Count -eq 1) { return $runNames[0] }
    return (Get-StageGameExeRel $StageRoot $Project $Config)
}

function Write-Summary {
    param([string] $Result)
    Head 'SHIP SUMMARY'
    Say 'GATES:'
    foreach ($g in $script:Gates) {
        Say ("  {0,-6} {1,-34} {2}" -f $g.Status, $g.Id, $g.Evidence)
    }
    Say ''
    Say 'FACTS:'
    foreach ($f in $script:Facts) {
        Say ("  {0,-34} {1}" -f $f.Key, $f.Value)
    }
    Say ''
    if ($script:StopId) {
        Say ("STOPPED AT GATE: {0}" -f $script:StopId)
        Say ("REASON:          {0}" -f $script:StopReason)
        if ($script:StopRemedy) { Say ("TO CLEAR:        {0}" -f $script:StopRemedy) }
        Say ''
        Say 'The previous zip has NOT been touched (PKG-7b write-new-then-prune).'
        Say ''
    }
    if ($script:WouldStop.Count -gt 0) {
        Say ("DRY RUN WOULD STOP AT: {0}" -f ($script:WouldStop -join ', '))
        Say ''
    }
    # SHIP-8: the suspension's contract is printed HERE, immediately above the
    # truth surface, so it is the last thing the caller reads.  A bar restated
    # 900 lines above the verdict is a bar that gets remembered instead of read.
    if ($script:AdjCapture) {
        Say 'THIS RUN DID NOT SHIP.  IT IS SUSPENDED AT C3-BOOT-ARENA (SHIP-8).'
        Say '  ADJUDICATE IS NOT A PASS AND IS NOT A SHIP.  An unresolved'
        Say '  suspension is exactly as unshipped as a STOP.'
        Say '  No zip was written - C3 precedes PHASE D, so the previous artifact'
        Say '  sits untouched and there is nothing to clean up.'
        Write-AdjudicationContract -CapturePath $script:AdjCapture `
                                   -RecordPath  $script:AdjRecord `
                                   -ResumeCmd   $script:AdjResumeCmd
    }
    Say 'NOT PROVEN BY THIS SCRIPT, EVER (SHIP-6):'
    Say '  - There is no input-injection lane.  Nothing here is a gameplay-feel'
    Say '    claim.  Human acceptance is Jonathan extracting the zip and clicking.'
    Say '  - The zip is a local artifact.  It was NOT pushed and never will be.'
    Say '  - This script cannot read pixels.  Every rendered-pixel verdict in the'
    Say '    record above was made by a CALLER that can see, and is retained as a'
    Say '    measured artifact - never self-certified here (SHIP-8).'
    if ($DryRun) {
        Say ''
        Say 'DRY RUN SCOPE (SHIP-7, AMENDED) - READ THIS BEFORE QUOTING THE RESULT:'
        Say '  - A -DryRun pass is NOT EVIDENCE ABOUT PHASES C-F.  A dry run never'
        Say '    compiles, never cooks, never boots, never zips and never commits.'
        Say '    DRYRUN-OK means PHASE A passed and the plan printed. Nothing more.'
        Say '  - THIS IS NOT HYPOTHETICAL: an unconditional stop at C3 once sat in'
        Say '    this script behind a green dry run, and it would have surfaced'
        Say '    only on the first real ship, after a 30-minute cook.'
        Say '  - The gate a dry run can never reach is C3-BOOT-ARENA under route 2.'
        Say '    Its contract is printed above so the seam is visible in this test'
        Say '    output even though this test cannot exercise it.'
    }
    Say ''
    # The truth surface.  MUST be the last line.
    Say ("SHIP RESULT: {0}" -f $Result)
}

# ==============================================================================
# MAIN
# ==============================================================================
try {

Head 'ship.ps1 - Siegebound cook + zip (law: SHIP-0..8d, PKG-1..11)'
Add-Fact 'Mode'          $(if ($DryRun) { 'DRY RUN - plans only, writes nothing' } else { 'LIVE' })
Add-Fact 'Configuration' $Configuration
Add-Fact 'Boot evidence (requested)' $BootEvidence   # RESOLVED route is decided at gate A6
Add-Fact 'Run (UTC)'     $script:RunUtc.ToString('yyyy-MM-dd HH:mm:ss')

# ------------------------------------------------------------------------------
# PHASE A - PRE-FLIGHT (SHIP-2 A; cheap, fails fast, every item a STOP)
# ------------------------------------------------------------------------------
Head 'PHASE A - PRE-FLIGHT'

# A1 - paths, all derived
if (-not $ProjectPath) {
    $projDir = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent   # <proj>/Tools/Packaging
    $found = @(Get-ChildItem -LiteralPath $projDir -Filter '*.uproject' -File -ErrorAction SilentlyContinue)
    Assert-Gate -Id 'A1-UPROJECT' -Ok ($found.Count -eq 1) `
        -Evidence ("{0} .uproject file(s) under {1}" -f $found.Count, $projDir) `
        -Remedy 'Pass -ProjectPath explicitly.' | Out-Null
    if ($found.Count -eq 1) { $ProjectPath = $found[0].FullName }
}
if (-not $ProjectPath) { $ProjectPath = '' }
$ProjectDir  = Split-Path $ProjectPath -Parent
$ProjectName = [System.IO.Path]::GetFileNameWithoutExtension($ProjectPath)
$ContentDir  = Join-Path $ProjectDir 'Content'
Add-Fact 'Project'      $ProjectPath
Add-Fact 'Project name' $ProjectName

$topRes = Invoke-Git -GitArgs @('rev-parse','--show-toplevel') -WorkDir $ProjectDir
Assert-Gate -Id 'A1-REPO' -Ok $topRes.Ok -Evidence 'git rev-parse --show-toplevel' `
    -Remedy 'Run from inside the work tree.' | Out-Null
$script:RepoRoot = (Resolve-Path ([string]$topRes.Out)).Path
Add-Fact 'Repo root' $script:RepoRoot

# Engine root: uproject EngineAssociation -> registry -> documented fallback.
if (-not $EngineRoot) {
    $assoc = ''
    try { $assoc = (Get-Content -LiteralPath $ProjectPath -Raw | ConvertFrom-Json).EngineAssociation } catch { $assoc = '' }
    if ($assoc) {
        foreach ($hive in @('HKLM:\SOFTWARE\EpicGames\Unreal Engine','HKCU:\SOFTWARE\Epic Games\Unreal Engine')) {
            $key = Join-Path $hive $assoc
            if (Test-Path $key) {
                $v = (Get-ItemProperty -Path $key -ErrorAction SilentlyContinue).InstalledDirectory
                if ($v) { $EngineRoot = $v; break }
            }
        }
    }
    if (-not $EngineRoot) { $EngineRoot = $ENGINE_FALLBACK }
}
$RunUAT    = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
$BuildBat  = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$EditorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
Assert-Gate -Id 'A1-ENGINE' -Ok ((Test-Path $RunUAT) -and (Test-Path $BuildBat) -and (Test-Path $EditorCmd)) `
    -Evidence $EngineRoot -Remedy 'Pass -EngineRoot pointing at the UE 5.8 install.' | Out-Null
Add-Fact 'Engine root' $EngineRoot

# Staging dir: probed candidates, first that EXISTS.  Never created silently -
# the folder is Jonathan's and a script that invents it also invents its fence.
if (-not $StagingDir) {
    $cands = @(
        (Join-Path $script:RepoRoot 'packagedZIPofGame'),
        (Join-Path $ProjectDir 'packagedZIPofGame'),
        (Join-Path (Split-Path $script:RepoRoot -Parent) 'packagedZIPofGame')
    )
    foreach ($c in $cands) { if (Test-Path -LiteralPath $c) { $StagingDir = $c; break } }
    Assert-Gate -Id 'A1-STAGING' -Ok ([bool]$StagingDir) `
        -Evidence ('probed: ' + ($cands -join ' | ')) `
        -Remedy 'Create the staging folder or pass -StagingDir.' | Out-Null
}
if ($StagingDir -and (Test-Path -LiteralPath $StagingDir)) {
    $StagingDir = (Resolve-Path -LiteralPath $StagingDir).Path
}
Add-Fact 'Staging dir' $StagingDir

if (-not $ShipLogDir) { $ShipLogDir = Join-Path $StagingDir '.ship' }
$RunStamp   = $script:RunUtc.ToString('yyyyMMdd-HHmmss')
$RunLogDir  = Join-Path $ShipLogDir $RunStamp
$StateFile  = Join-Path $ShipLogDir 'ship-state.json'
# SHIP-8b: the verdict record.  It lives BESIDE the state file at a STABLE
# path, deliberately NOT in the per-run stamped dir - a resume is a new run
# with a new stamp, and a record the next invocation cannot find is a record
# that does not exist.  This script only ever READS it.
$AdjFile    = Join-Path $ShipLogDir $ADJ_FILE_NAME
if (-not $DryRun) { New-Item -ItemType Directory -Path $RunLogDir -Force | Out-Null }
Add-Fact 'Run log dir' $RunLogDir

$ShipDate  = (Get-Date).ToString('yyyy-MM-dd')     # local date - PKG-7b names it
$ZipName   = "Siegebound-Win64-$Configuration-$ShipDate.zip"
$ZipPath   = Join-Path $StagingDir $ZipName
$StageWin  = Join-Path $StagingDir 'Windows'
$ReadmePath= Join-Path $StagingDir 'README.md'
Add-Fact 'Target zip' $ZipPath

# A2 - tree state.  A dirty tree does NOT stop the ship (Jonathan ships
# mid-work) but every dirty path is listed.  An unresolved merge/rebase DOES.
$headRes = Invoke-Git -GitArgs @('rev-parse','HEAD')
$HeadSha = ''
if ($headRes.Ok) { $HeadSha = ([string]$headRes.Out).Trim() }
$porcRes = Invoke-Git -GitArgs @('status','--porcelain')
$Porcelain = @()
if ($porcRes.Ok -and $porcRes.Out) { $Porcelain = @($porcRes.Out) }
Add-Fact 'HEAD' $HeadSha
Add-Fact 'Dirty paths' ("{0}" -f $Porcelain.Count)
foreach ($line in $Porcelain) { Say ("      dirty: {0}" -f $line) }

$gitDir = Join-Path $script:RepoRoot '.git'
$midMerge = @()
foreach ($m in @('MERGE_HEAD','CHERRY_PICK_HEAD','REVERT_HEAD','BISECT_LOG','rebase-merge','rebase-apply')) {
    if (Test-Path -LiteralPath (Join-Path $gitDir $m)) { $midMerge += $m }
}
Assert-Gate -Id 'A2-NO-MID-OPERATION' -Ok ($midMerge.Count -eq 0) `
    -Evidence ("unresolved git state markers: {0}" -f $(if ($midMerge.Count) { $midMerge -join ',' } else { 'none' })) `
    -Remedy 'Finish or abort the merge/rebase/cherry-pick, then ship.' | Out-Null

# A3 - QUIET-MODULE.  The cook is a serialized gate; it never runs beside a
# compile.  A running EDITOR is reported but is NOT a stop - the Shipping
# monolithic target links different binaries than the editor DLLs, so the cook
# may well build with the editor up.  A running UBT/UAT/AutomationTool IS a stop.
$busyNames = @('UnrealBuildTool','AutomationTool','UnrealPak','UnrealEditor-Cmd','UnrealHeaderTool','cl','link')
$busy = @()
foreach ($n in $busyNames) {
    $ps = @(Get-Process -Name $n -ErrorAction SilentlyContinue)
    if ($ps.Count -gt 0) { $busy += ("{0} x{1}" -f $n, $ps.Count) }
}
$editors = @(Get-Process -Name 'UnrealEditor' -ErrorAction SilentlyContinue)
Add-Fact 'Editor processes' ("{0}" -f $editors.Count)
Assert-Gate -Id 'A3-QUIET-MODULE' -Ok ($busy.Count -eq 0) `
    -Evidence ("build/cook processes live: {0}" -f $(if ($busy.Count) { $busy -join ', ' } else { 'none' })) `
    -Remedy 'Wait for the other compile/cook gate to finish (QUIET-MODULE, PKG-6).' | Out-Null

# A LEFTOVER *STAGED GAME* PROCESS IS DELIBERATELY NOT IN $busyNames ABOVE, AND
# THE A3 GATE IS UNCHANGED.  Stated rather than left implicit (QA TASK-702
# BLOCKER-1 blast radius, WARN-5):
#   * A running game does not contend for the serialized compile/cook gate PKG-6
#     protects, so stopping the ship for one would be a gate that fires on the
#     wrong thing.
#   * It could not have been named here anyway without repeating the very defect
#     this task closed: under Shipping the process is <Project>-Win64-Shipping,
#     not <Project>, so a name in this list would have matched nothing.
#   * What actually closes the hazard is downstream and is a MEASUREMENT: PHASE
#     C resolves game processes BY PATH and EXCLUDES every PID that was already
#     running, so a leftover can never supply this run's window title or be
#     mistaken for this cook's boot - and this run only ever kills what it
#     started itself.  A process that is Jonathan's stays Jonathan's.
$stagedProcs = @()
if ($StageWin -and (Test-Path -LiteralPath $StageWin)) { $stagedProcs = @(Get-StageGameProcess -StageRoot $StageWin) }
Add-Fact 'Staged-game processes' $(if ($stagedProcs.Count -eq 0) {
        'none running from the stage'
    } else {
        ("{0} already running from the stage (EXCLUDED from this run's boot evidence, and NOT killed by this run): {1}" -f `
         $stagedProcs.Count, (@($stagedProcs | ForEach-Object { ('pid {0} {1}' -f $_.Id, $_.ProcessName) }) -join ', '))
    })

# A4 - THE FENCE, MEASURED THIS RUN (PKG-7a).  Ruled as a PROPERTY, never as a
# path: the staging dir must be (i) outside this work tree, or (ii) matched by a
# live .gitignore rule.  A property survives someone moving the folder; a
# hardcoded path does not.
$fenceOk = $false
$fenceEv = ''
$stagingTop = Invoke-Git -GitArgs @('rev-parse','--show-toplevel') -WorkDir $StagingDir
if (-not $stagingTop.Ok) {
    $fenceOk = $true
    $fenceEv = 'route (i): staging dir is not inside any git work tree'
} else {
    $stagingTopPath = (Resolve-Path ([string]$stagingTop.Out)).Path
    if ($stagingTopPath -ne $script:RepoRoot) {
        $fenceOk = $true
        $fenceEv = "route (i): staging dir belongs to a different work tree ($stagingTopPath)"
    } else {
        $ci = Invoke-Git -GitArgs @('check-ignore','-v','--',$StagingDir)
        if ($ci.Ok -and $ci.Out) {
            $fenceOk = $true
            $fenceEv = "route (ii): ignored by " + (([string]@($ci.Out)[0]).Trim())
        } else {
            $fenceEv = 'staging dir is INSIDE the work tree and NOT ignored'
        }
    }
}
Assert-Gate -Id 'A4-FENCE' -Ok $fenceOk -Evidence $fenceEv `
    -Remedy 'Move the staging dir outside the work tree OR add a .gitignore rule covering it, then re-run. The multi-GB build never enters git in any form (PKG-3).' | Out-Null

# A5 - disk headroom
$driveLetter = (Split-Path -Qualifier $StagingDir)
$disk = Get-CimInstance -ClassName Win32_LogicalDisk -Filter ("DeviceID='{0}'" -f $driveLetter)
$freeGb = [math]::Round($disk.FreeSpace / 1GB, 1)
Assert-Gate -Id 'A5-DISK' -Ok ($freeGb -ge $MIN_FREE_GB) `
    -Evidence ("{0} GB free on {1} (need {2})" -f $freeGb, $driveLetter, $MIN_FREE_GB) `
    -Remedy 'Free disk space before cooking.' | Out-Null

# ==============================================================================
# A6 - PKG-9a EVIDENCE ROUTE.  Checked BEFORE the 30-minute cook.
# ==============================================================================
# THE GATE'S QUESTION HAS ONLY EVER BEEN THIS ONE:
#     "does this run have SOME valid way to PROVE the boot?"
# It is NOT "does Config/DefaultEngine.ini contain a particular key".
#
# REPAIRED under TASK-712, authorised by PKG-9a-2.  As first built (TASK-701)
# this gate required bUseLoggingInShipping=True in DefaultEngine.ini - correct
# against the law AS IT WAS THEN WRITTEN, and wrong in FACT.  That key can never
# be present, so as built every /ship would have STOPPED HERE FOREVER.
#
# MEASURED AT THE UE 5.8 SOURCE (PKG-9a-1), not inferred from a failed attempt:
#   * bUseLoggingInShipping is a UBT C# TARGET PROPERTY - TargetRules.cs:1610 -
#     NOT a config key.  It carries NO [ConfigFile] attribute (contrast
#     UEBuildWindows.cs:539, which does carry one), so nothing reads it out of
#     any .ini.  The contrast IS the proof: the attribute exists in that
#     codebase and is simply not on this property.
#   * No C++ reads it.  It appears only in UBT .cs and generated .xml docs.
#   * It carries [RequiresUniqueBuildEnvironment], while TargetRules.cs:2859
#     returns TargetBuildEnvironment.Shared whenever Unreal.IsEngineInstalled()
#     is true - and that is true here, because
#     <EngineRoot>\Engine\Build\InstalledBuild.txt EXISTS (a Launcher install).
#     Setting it anyway throws at UEBuildTarget.cs:1512 and THE COOK FAILS.
#
#   ==> SHIPPING BUILDS ON AN INSTALLED-ENGINE MACHINE ARE LOG-SILENT,
#       PERMANENTLY.  That is a property of the ENGINE INSTALL, not a config we
#       forgot to set.  The Log route DOES NOT EXIST here, and route 2
#       (rendered pixels) is the STANDING route - not a fallback, not a
#       degradation.  Stated honestly, because it is true: a rendered capture
#       of a real deck in a real arena is BETTER evidence than a log line.  It
#       is what the player sees, and it cannot be faked by a process that boots
#       and then does nothing.
#
# WHAT THIS GATE STILL STOPS, UNDIMINISHED (PKG-9a-2 clauses 2 and 3, SHIP-1):
#   1. -BootEvidence Log demanded EXPLICITLY on an installed engine + Shipping
#      -> STOP.  The operator asked for an instrument that cannot exist.
#   2. the PIXEL INSTRUMENT ITSELF UNAVAILABLE -> STOP.  A ship that cannot
#      verify its own boot must not ship.
#   3. IT NEVER PASSES WITH NO ROUTE, and it never switches SILENTLY.  The
#      selected route is named in the gate evidence, in the fact table and in
#      the banner below.  A silent switch is its own trap.
#
# WHAT THIS GATE DOES NOT TOUCH, AND MUST NOT:
#   the PKG-6a bar.  C3-BOOT-ARENA still has to reach a REAL ARENA with a REAL
#   DECK and a spawned hero.  ONLY THE INSTRUMENT CHANGED - never the bar.
#   Reading a PASS out of the absence of errors in a log that CANNOT EMIT stays
#   BANNED (PKG-9a): in Shipping here a silent log is the EXPECTED state, so
#   silence is worth exactly nothing as evidence.
# ------------------------------------------------------------------------------
$engineIni = Join-Path $ProjectDir 'Config\DefaultEngine.ini'

# The ini key is INERT.  It is read here for ONE reason: if someone re-adds it,
# SAY PLAINLY that it does nothing - rather than let the next reader grep our
# own file and conclude Shipping logging is ON (PKG-9a-3, the stale-symbol
# trap, with our own file as the false witness).  It is NEVER an input to the
# route decision below.
$inertLogKey = $false
if (Test-Path -LiteralPath $engineIni) {
    $inertLogKey = [bool](Select-String -LiteralPath $engineIni -Pattern 'bUseLoggingInShipping\s*=\s*True' -List -ErrorAction SilentlyContinue)
}
Add-Fact 'bUseLoggingInShipping' $(if ($inertLogKey) {
        'PRESENT and INERT - a UBT TargetRules property with no ini binding (PKG-9a-1). It does NOT enable Shipping logging. Strike it (PKG-9a-3).'
    } else {
        'absent - correct: it is not an ini key at all (PKG-9a-1)'
    })

# --- THE MACHINE CLASS, MEASURED THIS RUN (PKG-9a-1) --------------------------
# A property, measured, never a hardcoded assumption - the same discipline
# A4-FENCE uses.  It survives someone shipping from a source-built engine.
$InstalledMarker   = Join-Path $EngineRoot 'Engine\Build\InstalledBuild.txt'
$EngineIsInstalled = Test-Path -LiteralPath $InstalledMarker
Add-Fact 'Engine class' $(if ($EngineIsInstalled) {
        ("INSTALLED (Launcher): {0} exists ==> Shipping is LOG-SILENT (PKG-9a-1)" -f $InstalledMarker)
    } else {
        'source-built: IsEngineInstalled() is false, so the Log route is selectable via TargetRules (PKG-9a-1)'
    })

# The Log route exists when the build actually emits a log: ANY Development
# build, or a Shipping build on a SOURCE-BUILT engine.  Shipping + installed
# engine is the one combination where it CANNOT exist.
$LogRouteUsable = ($Configuration -ne 'Shipping') -or (-not $EngineIsInstalled)
$RouteRequested = $BootEvidence
$RouteExplicit  = $PSBoundParameters.ContainsKey('BootEvidence')
$RouteSelected  = $RouteRequested
$RouteAuto      = $false
$A6Ok           = $true
$A6Evidence     = ''
$A6Remedy       = ''

if (($RouteRequested -eq 'Log') -and (-not $LogRouteUsable)) {
    if ($RouteExplicit) {
        # PKG-9a-2 clause 2, first case: the operator DEMANDED an instrument
        # this machine class cannot provide.  There is no usable route -> STOP.
        $A6Ok       = $false
        $A6Evidence = 'NO USABLE EVIDENCE ROUTE: -BootEvidence Log was demanded EXPLICITLY, but this is an INSTALLED (Launcher) engine, so a Shipping cook here is log-silent permanently (PKG-9a-1)'
        $A6Remedy   = 'Re-run with -BootEvidence Pixel - route 2 is the STANDING route on this machine class (PKG-9a-1), not a fallback and not a degradation - or ship from a source-built engine. Do NOT edit .Target.cs to chase a log line: that trades a shipped-build property for a broken cook. Claiming a boot PASS from a log that cannot emit is BANNED.'
    } else {
        # PKG-9a-2 clause 1: route Log was only the DEFAULT, not a demand, and
        # it does not exist here.  Auto-select route 2 - and SAY SO, loudly.
        $RouteSelected = 'Pixel'
        $RouteAuto     = $true
    }
}

# --- IS THE SELECTED INSTRUMENT ACTUALLY AVAILABLE? (PKG-9a-2 clause 2) -------
$InstrumentDetail = 'n/a'
if ($A6Ok -and ($RouteSelected -eq 'Pixel')) {
    # Probe the pixel instrument NOW, not after a 30-minute cook.  This is the
    # same mechanism C3 uses to take the real capture; it is duplicated here
    # DELIBERATELY rather than refactored, so that this repair touches no other
    # gate's code.  The probe allocates a small IN-MEMORY bitmap and WRITES NO
    # FILE - the -DryRun "writes nothing at all" contract holds.
    try {
        Add-Type -AssemblyName System.Drawing, System.Windows.Forms
        $probeScreen = [System.Windows.Forms.Screen]::PrimaryScreen
        if ($null -eq $probeScreen) { throw 'no primary screen (no interactive desktop session?)' }
        $probeBounds = $probeScreen.Bounds
        if (($probeBounds.Width -le 0) -or ($probeBounds.Height -le 0)) {
            throw ('primary screen reports {0}x{1}' -f $probeBounds.Width, $probeBounds.Height)
        }
        $probeBmp = New-Object System.Drawing.Bitmap 8, 8
        $probeGfx = [System.Drawing.Graphics]::FromImage($probeBmp)
        $probeGfx.CopyFromScreen($probeBounds.Location, [System.Drawing.Point]::Empty, $probeBmp.Size)
        $probeGfx.Dispose(); $probeBmp.Dispose()
        $InstrumentDetail = ('screen capture OK, primary screen {0}x{1}' -f $probeBounds.Width, $probeBounds.Height)
    } catch {
        $A6Ok             = $false
        $InstrumentDetail = ('UNAVAILABLE - {0}' -f $_.Exception.Message)
        $A6Evidence       = ('NO USABLE EVIDENCE ROUTE: route Pixel is the route in force, and the pixel-capture instrument is unavailable ({0})' -f $InstrumentDetail)
        $A6Remedy         = 'Run the ship from an interactive desktop session where System.Drawing can capture the screen. A SHIP THAT CANNOT VERIFY ITS OWN BOOT MUST NOT SHIP (PKG-9a-2) - do not proceed on an unverifiable boot.'
    }
}

if ($A6Ok) {
    if ($RouteSelected -eq 'Pixel') {
        $A6Evidence = ('route PIXEL ({0}); instrument: {1}; PKG-6a arena bar UNCHANGED - only the instrument changed; ends in CALLER ADJUDICATION at C3-BOOT-ARENA, it never self-passes' -f `
            $(if ($RouteAuto) { 'AUTO-SELECTED by PKG-9a-1: installed engine, so the Log route does not exist here; requested was Log-by-default' } else { 'requested explicitly' }), `
            $InstrumentDetail)
    } else {
        $A6Evidence = ('route LOG, available because {0}. The log channel is proven ALIVE by C3''s POSITIVE markers (arena LoadMap + a non-zero deck + hero spawned); the negative sweep is only meaningful after them (PKG-9a)' -f `
            $(if ($Configuration -ne 'Shipping') { ("configuration is {0}, which compiles logging in" -f $Configuration) } else { 'this is a source-built engine, where TargetRules can carry bUseLoggingInShipping (PKG-9a-1)' }))
    }
}

# THE ANNOUNCEMENT.  PKG-9a-2 clause 1 requires the auto-selection to be SAID,
# not just done: a silent switch is its own trap.
if ($RouteAuto -and $A6Ok) {
    Say ''
    Say '  *** EVIDENCE ROUTE AUTO-SELECTED: PIXEL (route 2, PKG-9a-1) ***'
    Say '      This engine is an INSTALLED (Launcher) build, so a Shipping cook here is'
    Say '      LOG-SILENT PERMANENTLY.  The Log route does not exist on this machine'
    Say '      class - it was not "left unset", it CANNOT be set (PKG-9a-1).'
    Say '      Route 2 is therefore the STANDING route: not a fallback, not a degradation.'
    Say '      PKG-6a IS UNCHANGED - the boot must still reach a REAL ARENA with a REAL'
    Say '      DECK and a spawned hero.  ONLY THE INSTRUMENT CHANGED.'
    Say '      PLAN FOR THIS: route 2 ends in CALLER ADJUDICATION at C3-BOOT-ARENA - a'
    Say '      human or agent LOOKS at the capture.  This script never reads pixels and'
    Say '      will never claim it did.'
    Say ''
}

# The route is now DECIDED.  PHASE C reads $BootEvidence, so the DECISION - not
# the request - drives the boot-verify.  NOTE: $BootEvidence carries the param
# block's [ValidateSet('Log','Pixel')], which PowerShell re-enforces on every
# later assignment, so it is only ever assigned a value inside that set.  On the
# no-route STOP path it is deliberately left alone and the resolved-route fact
# below reports NONE.
if ($A6Ok) { $BootEvidence = $RouteSelected }
Add-Fact 'Evidence route (resolved)' $(if ($A6Ok) {
        ("{0}{1}" -f $RouteSelected, $(if ($RouteAuto) { ' - AUTO-SELECTED (PKG-9a-1)' } else { ' - as requested' }))
    } else {
        'NONE - no usable route; see gate A6-EVIDENCE-ROUTE'
    })

# A6 ALWAYS ASSERTS.  It is never Note-Skipped, in any configuration, because a
# gate that quietly declines to run IS the silent no-route pass PKG-9a-2 clause
# 3 forbids.  SHIP-1's posture is preserved exactly: the gate still stops a ship
# that cannot prove itself - it now stops for the REAL reason.
Assert-Gate -Id 'A6-EVIDENCE-ROUTE' -Ok $A6Ok -Evidence $A6Evidence -Remedy $A6Remedy | Out-Null

# A7 - RECIPE SANITY.  A renamed or deleted content directory would silently
# under-cook exactly the way PKG-5a describes, so the recipe is verified against
# the tree every run rather than trusted.
$missingDirs = @()
foreach ($d in $RECIPE_COOKDIRS) {
    if (-not (Test-Path -LiteralPath (Join-Path $ContentDir $d))) { $missingDirs += $d }
}
Assert-Gate -Id 'A7-COOKDIRS' -Ok ($missingDirs.Count -eq 0) `
    -Evidence ("{0} COOKDIR entries; missing: {1}" -f $RECIPE_COOKDIRS.Count, $(if ($missingDirs.Count) { $missingDirs -join ',' } else { 'none' })) `
    -Remedy 'A content directory in the PKG-5a recipe no longer exists. Do NOT drop it silently - fix the recipe under a task, because a missing COOKDIR is how the menu-only package came back.' | Out-Null

$missingMaps = @()
foreach ($m in $RECIPE_MAPS) {
    $leaf = $m.Substring($m.LastIndexOf('/') + 1)
    if (-not (Test-Path -LiteralPath (Join-Path $ContentDir ("Maps\{0}.umap" -f $leaf)))) { $missingMaps += $m }
}
Assert-Gate -Id 'A7-MAPS' -Ok ($missingMaps.Count -eq 0) `
    -Evidence ("maps: {0}; missing: {1}" -f ($RECIPE_MAPS -join '+'), $(if ($missingMaps.Count) { $missingMaps -join ',' } else { 'none' })) `
    -Remedy 'A map in the allowlist is not on disk.' | Out-Null

$bootMapOk = $false
if (Test-Path -LiteralPath $engineIni) {
    $gdm = Select-String -LiteralPath $engineIni -Pattern '^GameDefaultMap\s*=\s*(.+)$' -List -ErrorAction SilentlyContinue
    if ($gdm) {
        $gdmVal = $gdm.Matches[0].Groups[1].Value.Trim()
        $bootMapOk = $gdmVal.StartsWith($RECIPE_MAPS[0])
        Add-Fact 'GameDefaultMap' $gdmVal
    }
}
Assert-Gate -Id 'A7-BOOTMAP' -Ok $bootMapOk `
    -Evidence ("DefaultEngine.ini GameDefaultMap must start with {0}" -f $RECIPE_MAPS[0]) `
    -Remedy 'The boot map drifted from the recipe. Update the recipe under a task rather than cooking the wrong map.' | Out-Null

# ==============================================================================
# A8 - CAN THIS DESKTOP BE DRIVEN?  (PKG-9f's stated cost, TASK-716's measurement)
# ==============================================================================
# WHY IT IS IN PHASE A AND NOT AT C3: because at C3 the answer costs a 30-minute
# cook first.  TASK-716 ran the Shipping boot-verify by hand and was BLOCKED -
# 12/12 polls locked over 4 minutes, no click deliverable, zero of the four
# PKG-6a criteria observed.  PKG-9f already rules Shipping boot-verify
# SCHEDULABLE, not unattended; this gate is that ruling arriving before the
# expensive part instead of after it.
#
# ==> AND IT IS NOT BUILT ON THE TWO PROBES ANYONE WOULD REACH FOR, BECAUSE 716
# MEASURED BOTH OF THEM LYING ON A LOCKED MACHINE.  See the $LOCK_WINDOW_CLASSES
# comment: OpenInputDesktop() said "Default", SetCursorPos() succeeded, and
# screen capture succeeded (capturing the LOCK SCREEN - which is why A6's
# instrument probe cannot answer this question either).  A gate on either API
# PASSES ON A LOCKED MACHINE and then boot-verifies into a backstop: SHIP-8c's
# fake-gate family, a check wearing a gate's clothes.  The honest test is the
# click rig's OWN abort predicate, and that is what Test-DesktopLocked uses.
#
# IT IS A POSITIVE LOCK DETECTOR, NOT A PROOF-OF-UNLOCK.  "Could not measure" is
# reported and PASSES.  Demanding proof of an unlocked desktop would make this
# forever-stop number five the first time Windows renamed a class - and a missed
# lock still cannot manufacture a pass, because the rig refuses to inject input
# it cannot land and C3-CAPTURE records that the drive never happened.
#
# ROUTE LOG NEEDS NO DESKTOP AND THE GATE SAYS SO RATHER THAN SKIPPING: a gate
# that quietly declines to run is the silent no-route pass PKG-9a-2 clause 3
# forbids, and A6 was repaired for exactly that shape.
$deskRes = @{ Locked = $false; Measured = $false; Detail = '' }
$deskChecked = ($A6Ok -and ($BootEvidence -eq 'Pixel'))
if ($deskChecked) { $deskRes = Test-DesktopLocked }
Add-Fact 'Desktop input' $(if (-not $deskChecked) {
        'not applicable - route Log proves the boot from the build own log and drives no input'
    } elseif ($deskRes.Locked) {
        ('LOCKED - {0}' -f $deskRes.Detail)
    } elseif ($deskRes.Measured) {
        ('drivable as far as this can tell - {0}' -f $deskRes.Detail)
    } else {
        ('UNMEASURED (not a stop, by design) - {0}' -f $deskRes.Detail)
    })
Assert-Gate -Id 'A8-DESKTOP' -Ok (-not $deskRes.Locked) `
    -Evidence $(if (-not $deskChecked) {
            'route Log: the boot-verify reads the packaged build own log and injects no input, so no desktop is required'
        } elseif ($deskRes.Locked) {
            ('THE DESKTOP IS LOCKED: {0}. Route Pixel reaches the arena by CLICKING Play in the shipped menu (PKG-9f) and simulated input cannot land on a locked session (TASK-076). Checked here so it costs seconds, not a 30-minute cook.' -f $deskRes.Detail)
        } else {
            ('route Pixel: no lock-screen owner found at the primary-screen centre ({0}). POSITIVE LOCK DETECTOR ONLY - this is not a proof that the click will land; the rig re-tests the exact point before it injects anything.' -f $deskRes.Detail)
        }) `
    -Remedy 'Unlock the desktop and re-run, or schedule the ship for a time it is unlocked - PKG-9f rules a Shipping boot-verify SCHEDULABLE, not unattended, and states that cost rather than engineering around it. Jonathan may also discharge the adjudication himself at any time (SHIP-8b(7)): double-click the staged shim, click Play, and report the four criteria. Do NOT fall back to the map argument to dodge this: PKG-9f measured that a Shipping binary ignores it, so that route can only ever capture a menu, and a menu is never a pass.' | Out-Null

# ------------------------------------------------------------------------------
# RESUME-BY-MEASUREMENT - decide whether B and C must run
# ------------------------------------------------------------------------------
$relevant = @()
foreach ($line in $Porcelain) {
    if ($line.Length -lt 4) { continue }
    $p = $line.Substring(3).Trim().Trim('"')
    if ($p.Contains(' -> ')) { $p = $p.Substring($p.IndexOf(' -> ') + 4) }
    $keep = $false
    foreach ($frag in $BUILD_RELEVANT_FRAGMENTS) { if ($p -like "*$frag*") { $keep = $true } }
    foreach ($suf  in $BUILD_RELEVANT_SUFFIXES)  { if ($p.EndsWith($suf))  { $keep = $true } }
    if ($keep) { $relevant += $p }
}
$TreeHash = Get-Sha256OfString (($relevant | Sort-Object) -join "`n")
Add-Fact 'Build-relevant dirt' ("{0} path(s), hash {1}" -f $relevant.Count, $TreeHash.Substring(0,12))

# The ROOT SHIM - the player's click target and this script's launch target -
# is derived from the manifests too (see Get-StageShimExeRel).  It is NOT the
# configuration-dependent name and is not presently broken; deriving it removes
# the last guessed staged-binary name in this file.
$StageShimRel = Get-StageShimExeRel $StageWin $ProjectName
$StageExe     = Join-Path $StageWin ($StageShimRel -replace '/','\')
# PKG-10: THE GAME BINARY'S NAME IS CONFIGURATION-DEPENDENT AND IS DERIVED FROM
# THE MANIFESTS, NEVER GUESSED.  UE names a Development binary <Project>.exe
# and a Shipping binary <Project>-Win64-Shipping.exe, so the guessed name
# <Project>.exe resolves - under Shipping - to the 347 MB STALE DEVELOPMENT
# ORPHAN that stage hygiene deletes.  Binding this script's build identity to a
# file it is about to delete would make the ship self-defeating; binding a
# ZIP-READBACK gate to it would fail every clean Shipping package.
# Re-resolved after the cook, when fresh manifests exist.
$StageGameExeRel = Get-StageGameExeRel $StageWin $ProjectName $Configuration
$StageBinExe = Join-Path $StageWin ($StageGameExeRel -replace '/','\')

$RecipeHash = ''   # filled after the UAT line is composed (below)

# ------------------------------------------------------------------------------
# Compose the EXACT UAT command line now, so -DryRun can print it verbatim.
# ------------------------------------------------------------------------------
$cookDirArgs = @()
foreach ($d in $RECIPE_COOKDIRS) { $cookDirArgs += ("-COOKDIR={0}" -f (Join-Path $ContentDir $d)) }
$additionalCooker = ($cookDirArgs -join ' ')

$uatArgs = @(
    'BuildCookRun',
    ('-project="{0}"' -f $ProjectPath),
    '-nop4',
    '-utf8output',
    '-platform=Win64',
    ('-clientconfig={0}' -f $Configuration),
    '-build',
    '-cook',
    ('-map={0}' -f ($RECIPE_MAPS -join '+')),
    '-pak',
    '-stage',
    '-prereqs',
    '-archive',
    ('-archivedirectory="{0}"' -f $StagingDir),
    ('-AdditionalCookerOptions="{0}"' -f $additionalCooker)
)
$UatCommandLine = ($uatArgs -join ' ')
$RecipeHash = Get-Sha256OfString ($UatCommandLine + '|' + $Configuration)

$buildArgs = @(
    ('{0}Editor' -f $ProjectName),
    'Win64',
    'Development',
    ('-project="{0}"' -f $ProjectPath),
    '-waitmutex'
)
$BuildCommandLine = ($buildArgs -join ' ')

$suiteReport = Join-Path $RunLogDir 'automation-report'
$suiteLog    = Join-Path $RunLogDir 'automation.log'
$suiteArgs = @(
    ('"{0}"' -f $ProjectPath),
    ('-ExecCmds="Automation RunTests {0}"' -f $SUITE_TEST_FILTER),
    '-unattended', '-nopause', '-nullrhi', '-nosplash', '-NoSound',
    '-testexit="Automation Test Queue Empty"',
    ('-ReportExportPath="{0}"' -f $suiteReport),
    ('-abslog="{0}"' -f $suiteLog),
    '-stdout', '-FORCELOGFLUSH',
    '"-ini:Engine:[/Script/EngineSettings.GameMapsSettings]:EditorStartupMap=/Engine/Maps/Entry.Entry"'
)
$SuiteCommandLine = ($suiteArgs -join ' ')

# ------------------------------------------------------------------------------
# The EXACT invocation that resumes this ship after an adjudication.  Composed
# from what THIS run actually resolved, never typed from memory.
# ------------------------------------------------------------------------------
$resumeArgs = @(('-Configuration {0}' -f $Configuration), '-BootEvidence Pixel')
foreach ($rp in @('ProjectPath','EngineRoot','StagingDir','ShipLogDir')) {
    if ($PSBoundParameters.ContainsKey($rp)) {
        $resumeArgs += ('-{0} "{1}"' -f $rp, (Get-Variable -Name $rp -ValueOnly))
    }
}
$ResumeCommandLine = ('& "{0}" {1}' -f $PSCommandPath, ($resumeArgs -join ' '))

# ==============================================================================
# SHIP-8d - THE ADJUDICATION RESUME SEAM.  RESUME RE-VERIFIES, IT DOES NOT TRUST.
# ==============================================================================
#
# Evaluated HERE - before PHASE B - for one measured reason: a FAIL verdict
# must end the ship in seconds, not after another compile, another suite and
# another 30-minute cook.
#
# RESUMING ONTO A DIFFERENT BUILD THAN THE ONE ADJUDICATED IS THE SINGLE WORST
# FAILURE THIS SEAM COULD PRODUCE, and it is closed by MEASUREMENT, not by care:
# every binding in the record is re-measured against this run, and ANY mismatch
# is a STOP - never a warning, never a silent re-cook that leaves a stale PASS
# sitting on disk.
#
# Consulted ONLY under route Pixel.  Under route Log the boot proves itself from
# the packaged build's own log and this file is irrelevant - route 1's behaviour
# is byte-for-byte what it was before this seam existed.
$AdjPass       = $false
$AdjInstrument = ''
$AdjRec        = $null
$AdjObs        = @{}
$AdjWho        = ''
$AdjWhen       = ''

if ($DryRun) {
    Add-Fact 'Pixel verdict record' $(if (Test-Path -LiteralPath $AdjFile) {
        ("present at {0} - NOT evaluated: a dry run never reaches PHASE C and must not consume an adjudication" -f $AdjFile)
    } else {
        ("absent ({0}) - expected before the first adjudication" -f $AdjFile)
    })
} elseif ($BootEvidence -eq 'Pixel') {
    if (-not (Test-Path -LiteralPath $AdjFile)) {
        # DELIBERATELY NOT PREDICTIVE.  "Absent" is a fact; what happens next is
        # the RESUME DECISION's to state, and after a record has been consumed
        # and retired (SHIP-8d) the honest answer is "the state may already carry
        # this build's adjudicated PASS".  A line that promised a cook here would
        # be a confident stale literal in the class PKG-9a-3 names.
        Add-Fact 'Pixel verdict record' ("absent at {0} - nothing is pending adjudication at the stable path. The Resume decision below says whether this run cooks or resumes; a run with no proven boot cooks, captures and SUSPENDS at C3." -f $AdjFile)
    } else {
        Say ''
        Say 'Pixel adjudication record found - RE-MEASURING ITS BINDINGS (SHIP-8b/8d):'
        try { $AdjRec = Get-Content -LiteralPath $AdjFile -Raw | ConvertFrom-Json } catch { $AdjRec = $null }

        # ---- (1) IS IT WELL FORMED?  The script cannot judge whether the
        # observations are TRUE, but it can require that they were MADE - and
        # that alone defeats the reflexive rubber-stamp (SHIP-8b(2)).
        $mal = @()
        $verdict = ''
        if ($null -eq $AdjRec) {
            $mal += 'the file is not valid JSON'
        } else {
            $vSchema = Get-JsonProp $AdjRec 'schema'
            $vGate   = Get-JsonProp $AdjRec 'gate'
            $verdict = [string](Get-JsonProp $AdjRec 'verdict')
            $AdjWho  = [string](Get-JsonProp $AdjRec 'adjudicatedBy')
            $AdjWhen = [string](Get-JsonProp $AdjRec 'adjudicatedUtc')
            $obs     = Get-JsonProp $AdjRec 'observations'

            if ("$vSchema" -ne "$ADJ_SCHEMA") { $mal += ("schema is '{0}', expected {1}" -f $vSchema, $ADJ_SCHEMA) }
            if ("$vGate" -ne 'C3-BOOT-ARENA') { $mal += ("gate is '{0}', expected C3-BOOT-ARENA" -f $vGate) }
            # SHIP-8b(3): TWO VALUES ONLY.  No "probably", no third state.
            if (($verdict -cne 'PASS') -and ($verdict -cne 'FAIL')) {
                $mal += ("verdict is '{0}' - the ONLY accepted values are PASS and FAIL (SHIP-8b(3))" -f $verdict)
            }
            if (-not $AdjWho.Trim())  { $mal += 'adjudicatedBy is empty - a verdict has an author' }
            if (-not $AdjWhen.Trim()) { $mal += 'adjudicatedUtc is empty' }

            if ($null -eq $obs) {
                $mal += 'observations is missing entirely'
            } else {
                foreach ($c in $ADJ_CRITERIA) {
                    $v = [string](Get-JsonProp $obs $c.Key)
                    $t = $v.Trim()
                    if (-not $t) {
                        $mal += ("observations.{0} is empty - NAME WHAT YOU SAW (SHIP-8b(2))" -f $c.Key)
                        continue
                    }
                    $AdjObs[$c.Key] = $t
                    $lower = $t.ToLowerInvariant().TrimEnd('.','!')
                    if ($ADJ_BANNED_OBS -contains $lower) {
                        $mal += ("observations.{0} is '{1}' - a bare verdict word is NOT an observation (SHIP-8b(2))" -f $c.Key, $t)
                        continue
                    }
                    if ($t.Length -lt $ADJ_MIN_OBS_CHARS) {
                        $mal += ("observations.{0} is {1} chars, minimum {2} - describe what is ON SCREEN" -f $c.Key, $t.Length, $ADJ_MIN_OBS_CHARS)
                    }
                    if (@($t -split '\s+' | Where-Object { $_ }).Count -lt $ADJ_MIN_OBS_WORDS) {
                        $mal += ("observations.{0} is fewer than {1} words - describe what is ON SCREEN" -f $c.Key, $ADJ_MIN_OBS_WORDS)
                    }
                    # SHIP-8b(4): AMBIGUITY IS A FAIL.  Checked on PASS records
                    # ONLY, so it can only ever make a PASS harder to obtain.
                    if ($verdict -ceq 'PASS') {
                        foreach ($h in $ADJ_HEDGES) {
                            if ($lower.Contains($h)) {
                                $mal += ("observations.{0} hedges ('{1}') on a PASS record - SHIP-8b(4): ambiguity is a FAIL, not a pass. Either say what you actually saw, or record FAIL." -f $c.Key, $h)
                                break
                            }
                        }
                    }
                }
            }
        }
        Assert-Gate -Id 'C3-VERDICT-FORM' -Ok ($mal.Count -eq 0) `
            -Evidence ("verdict record at {0}: {1}" -f $AdjFile, $(if ($mal.Count -eq 0) { 'well formed' } else { ($mal -join ' | ') })) `
            -Remedy 'The record is MALFORMED and is rejected (SHIP-8b). Re-read the capture, re-write the record naming what you SAW per criterion, and re-run. There is no flag that bypasses this.' | Out-Null

        # ---- (2) DOES IT STILL BIND?  Re-measured, every time (SHIP-8b(1)).
        $mis = @()
        $st0 = $null
        if (Test-Path -LiteralPath $StateFile) {
            try { $st0 = Get-Content -LiteralPath $StateFile -Raw | ConvertFrom-Json } catch { $st0 = $null }
        }
        $recCap = [string](Get-JsonProp $AdjRec 'capturePath')
        $recSha = ([string](Get-JsonProp $AdjRec 'captureSha256')).ToLowerInvariant()
        $recHead= [string](Get-JsonProp $AdjRec 'head')
        $recCfg = [string](Get-JsonProp $AdjRec 'config')
        $recLen = Get-JsonProp $AdjRec 'stageExeBytes'
        $recUtc = [string](Get-JsonProp $AdjRec 'stageExeUtc')

        # The state file names WHICH capture this ship is pending on.  Without
        # this cross-check an OLDER capture that still exists and still hashes
        # would validate - a stale PASS replayed onto a fresh cook.
        $pendCap = ''
        $pendSha = ''
        if ($st0) {
            $pendCap = [string](Get-JsonProp $st0 'bootCapture')
            $pendSha = ([string](Get-JsonProp $st0 'bootCaptureSha')).ToLowerInvariant()
            $stBoot  = [string](Get-JsonProp $st0 'boot')
            if ($stBoot -ne 'ADJUDICATE') { $mis += ("the state file records boot='{0}', not a pending adjudication" -f $stBoot) }
        } else {
            $mis += 'there is no ship state file - nothing is pending adjudication'
        }
        if (-not $pendCap)            { $mis += 'the state file names no pending capture' }
        elseif ($recCap -ne $pendCap) { $mis += ("the record judges '{0}' but the pending capture is '{1}'" -f $recCap, $pendCap) }
        if ($pendSha -and $recSha -and ($recSha -ne $pendSha)) { $mis += 'the record hash does not match the hash recorded when the capture was taken' }

        if (-not $recCap) {
            $mis += 'capturePath is missing'
        } elseif (-not (Test-Path -LiteralPath $recCap)) {
            $mis += ("the adjudicated capture no longer exists on disk: {0}" -f $recCap)
        } else {
            $nowSha = (Get-FileHash -LiteralPath $recCap -Algorithm SHA256).Hash.ToLowerInvariant()
            if ($nowSha -ne $recSha) { $mis += 'the capture file has CHANGED since it was adjudicated (sha256 mismatch)' }
        }
        if ($recHead -ne $HeadSha)      { $mis += ("HEAD: adjudicated {0}, now {1}" -f $recHead, $HeadSha) }
        if ($recCfg  -ne $Configuration){ $mis += ("configuration: adjudicated {0}, now {1}" -f $recCfg, $Configuration) }
        if (-not (Test-Path -LiteralPath $StageBinExe)) {
            $mis += 'the staged game exe is gone'
        } else {
            $exeItem = Get-Item -LiteralPath $StageBinExe
            $recLenL = -1
            try { $recLenL = [long]$recLen } catch { $recLenL = -1 }
            if ($recLenL -ne $exeItem.Length)  { $mis += ("staged exe size: adjudicated {0}, now {1}" -f $recLen, $exeItem.Length) }
            if ($recUtc -ne $exeItem.LastWriteTimeUtc.ToString('o')) { $mis += 'staged exe timestamp changed - this is a DIFFERENT BUILD' }
        }

        Assert-Gate -Id 'C3-VERDICT-BINDING' -Ok ($mis.Count -eq 0) `
            -Evidence $(if ($mis.Count -eq 0) {
                ("re-measured OK: capture sha {0} / HEAD {1} / {2} / staged exe size+mtime all identical to the adjudicated build" -f $recSha.Substring(0,[math]::Min(12,$recSha.Length)), $(if ($recHead.Length -ge 8) { $recHead.Substring(0,8) } else { $recHead }), $recCfg)
            } else { ($mis -join ' | ') }) `
            -Remedy ("The verdict does NOT bind to this run - it was made against a different capture or a different build, and SHIP-8d closes that by measurement rather than by care. Delete {0} and re-run: the ship will cook, capture and suspend for a FRESH adjudication of the build you actually have." -f $AdjFile) | Out-Null

        # ---- (3) THE VERDICT.  A FAIL ends the ship with the named reason;
        # nothing was written (C3 precedes PHASE D), so the previous zip is
        # untouched and there is nothing to clean up.
        $failReason = [string](Get-JsonProp $AdjRec 'reason')
        if (-not $failReason.Trim()) {
            $failReason = (@($ADJ_CRITERIA | ForEach-Object { if ($AdjObs.ContainsKey($_.Key)) { ("{0}: {1}" -f $_.Key, $AdjObs[$_.Key]) } }) -join ' | ')
        }

        # ---- RETIRE A *FAIL* RECORD TOO, BEFORE THE STOP (SHIP-8d, the D11
        # repair applied to both terminal readings).
        # THE RULE, STATED ONCE: A RECORD IS RETIRED WHEN IT HAS BEEN READ TO A
        # VERDICT.  It is left exactly where it is when it could NOT be read - a
        # malformed record and a non-binding record must keep stopping until a
        # human looks at them, which is D2 and is correct.
        # WHY A FAIL RETIRES: the ship has ENDED on this verdict; the record is
        # spent.  Leaving it armed reproduces the defect one branch over - the
        # operator fixes the build, HEAD moves, and the very next /ship stops at
        # C3-VERDICT-BINDING on a stale-HEAD mismatch and demands a manual
        # delete.  The state is deliberately NOT advanced here: it still reads
        # boot=ADJUDICATE with no record, so the resume decision refuses to reuse
        # and the fixed build is cooked and captured afresh.  That is the right
        # outcome - a FAILED boot must never be resumed onto.
        # SHIP-8b(6) is satisfied: ARCHIVED, never deleted, and the archive path
        # is named in the STOP evidence below.
        $adjArchive = ''
        if ($verdict -cne 'PASS') {
            $adjArchive = Join-Path $RunLogDir ("ship-adjudication.{0}.consumed-FAIL.json" -f $RunStamp)
            try {
                Move-Item -LiteralPath $AdjFile -Destination $adjArchive -Force
                Add-Fact 'Adjudication retired' ("FAIL record ARCHIVED (not deleted) to {0}; the ship state still reads boot=ADJUDICATE, so the next run re-cooks and re-captures rather than resuming onto a failed boot" -f $adjArchive)
            } catch {
                $adjArchive = ''
                Say ("      NOTE: the FAIL record could not be archived ({0}); it is still at {1} and the next run will stop on it." -f $_.Exception.Message, $AdjFile)
            }
        }

        Assert-Gate -Id 'C3-VERDICT' -Ok ($verdict -ceq 'PASS') `
            -Evidence ("PIXEL VERDICT = {0}, adjudicated by {1} at {2}. {3}{4}" -f $verdict, $AdjWho, $AdjWhen, `
                       $(if ($verdict -ceq 'PASS') { 'Observations are carried into the report and the README (SHIP-8b(6)).' } else { ("Named reason: " + $failReason) }), `
                       $(if ($adjArchive) { (" The record has been ARCHIVED to {0} - retained, not deleted (SHIP-8b(6)) - so the next ship starts clean." -f $adjArchive) } else { '' })) `
            -Remedy 'A FAIL verdict ENDS the ship (PKG-6a: a menu is never a pass). Fix the build - the -COOKDIR recipe is the first suspect (PKG-5a) - and ship again; the next run cooks and captures afresh, because a FAILED boot is never resumed onto. Do NOT re-adjudicate the same capture hoping for a different answer.' | Out-Null

        $AdjPass = $true
        $AdjInstrument = ("rendered pixels (PKG-9a route 2) - ADJUDICATED PASS by {0} at {1}, capture {2}" -f $AdjWho, $AdjWhen, $recCap)
        Add-Fact 'Pixel verdict' ("PASS - {0} at {1}" -f $AdjWho, $AdjWhen)
        Add-Fact 'Pixel capture'  $recCap
        foreach ($c in $ADJ_CRITERIA) {
            if ($AdjObs.ContainsKey($c.Key)) { Add-Fact ("Adjudicated: " + $c.Key) $AdjObs[$c.Key] }
        }
    }
}

# Reuse decision
$reuse = $false
$reuseWhy = 'no prior state - PHASE B and C will run in full'
if (Test-Path -LiteralPath $StateFile) {
    $st = $null
    try { $st = Get-Content -LiteralPath $StateFile -Raw | ConvertFrom-Json } catch { $st = $null }
    if ($st) {
        # EVERY read of the state file goes through Get-JsonProp (QA TASK-702
        # WARN-3).  Under Set-StrictMode -Version Latest a bare $st.head on a
        # truncated, hand-edited or interrupted state file THROWS, and the ship
        # would surface a recoverable input problem as UNEXPECTED-ERROR (exit 1)
        # - a gate failing in the wrong voice.  A missing property now yields
        # $null, which fails its check and simply re-runs PHASE B and C: the
        # safe direction, and the same hardening the verdict record already had.
        $checks = @()
        $stBootV = [string](Get-JsonProp $st 'boot')
        if ((Get-JsonProp $st 'schema')     -ne $STATE_SCHEMA) { $checks += 'schema' }
        if ((Get-JsonProp $st 'head')       -ne $HeadSha)      { $checks += 'HEAD' }
        if ((Get-JsonProp $st 'treeHash')   -ne $TreeHash)     { $checks += 'working tree' }
        if ((Get-JsonProp $st 'config')     -ne $Configuration){ $checks += 'configuration' }
        if ((Get-JsonProp $st 'recipeHash') -ne $RecipeHash)   { $checks += 'recipe' }
        if ((Get-JsonProp $st 'compile')    -ne 'PASS')        { $checks += 'compile verdict' }
        if ((Get-JsonProp $st 'cook')       -ne 'PASS')        { $checks += 'cook verdict' }
        # SHIP-8d: the boot is proven either by the log route's own verdict, or
        # by a pixel adjudication whose bindings were RE-MEASURED at
        # C3-VERDICT-BINDING immediately above.  $AdjPass cannot be true unless
        # that gate passed - it throws otherwise - so this is not a second,
        # weaker door into the same room.
        # 'PASS' also covers a CONSUMED adjudication: once a verdict has been
        # spent the state carries boot=PASS with the adjudication in
        # bootInstrument, and every guard in this list is re-measured on top of
        # it exactly as it is for a log-route PASS.  Nothing is trusted twice.
        if ($stBootV -eq 'PASS') { }
        elseif (($stBootV -eq 'ADJUDICATE') -and $AdjPass) { }
        else { $checks += 'boot verdict' }
        if ([int](Get-JsonProp $st 'suiteTotal') -lt $SUITE_BASELINE) { $checks += 'suite total' }
        if (-not (Test-Path -LiteralPath $StageBinExe)) { $checks += 'staged exe missing' }
        elseif ((Get-JsonProp $st 'stageExeUtc') -ne (Get-Item -LiteralPath $StageBinExe).LastWriteTimeUtc.ToString('o')) { $checks += 'staged exe changed' }
        if ($checks.Count -eq 0) {
            $reuse = $true
            $reuseWhy = ("REUSING PHASE B+C proven at {0} for the IDENTICAL build input (HEAD {1}, tree {2}, recipe {3})" -f (Get-JsonProp $st 'timestampUtc'), $HeadSha.Substring(0,8), $TreeHash.Substring(0,12), $RecipeHash.Substring(0,12))
        } else {
            $reuseWhy = ("prior state exists but differs in: {0} - PHASE B and C RE-RUN IN FULL" -f ($checks -join ', '))
        }
    }
}
Add-Fact 'Resume decision' $reuseWhy

$compileVerdict = 'NOT-RUN'
$suiteTotal     = 0
$cookVerdict    = 'NOT-RUN'
$bootVerdict    = 'NOT-RUN'
$bootInstrument = 'none'
$windowTitle    = '(not measured)'

# ------------------------------------------------------------------------------
# PHASE B - PROVE THE BUILD (SHIP-2 B).  Prove, THEN cook: discovering after a
# 30-minute cook that the code never compiled is the most expensive ordering
# available.
# ------------------------------------------------------------------------------
Head 'PHASE B - PROVE THE BUILD (compile + suite, Development editor target)'
Say ''
Say 'Compile command line:'
Say ("  {0} {1}" -f $BuildBat, $BuildCommandLine)
Say ''
Say 'Suite command line (PKG-9c: the suite gates the CODE and always runs'
Say 'against the Development EDITOR target - a Shipping cook never proves code):'
Say ("  {0} {1}" -f $EditorCmd, $SuiteCommandLine)

if ($DryRun) {
    Note-Skipped 'B1-COMPILE' 'dry run: not executed'
    Note-Skipped 'B2-SUITE'   'dry run: not executed'
} elseif ($reuse) {
    Note-Skipped 'B1-COMPILE' 'reused - identical build input already proven'
    Note-Skipped 'B2-SUITE'   'reused - identical build input already proven'
    $compileVerdict = 'PASS (reused)'
    $suiteTotal = [int](Get-JsonProp (Get-Content -LiteralPath $StateFile -Raw | ConvertFrom-Json) 'suiteTotal')
} else {
    # B1 - compile.  Build.bat returns exit 0 on a FAILED build (Live Coding
    # mutex).  PARSE THE LOG.  Never trust $LASTEXITCODE.
    $r = Invoke-Tool -FilePath $BuildBat -ArgumentString $BuildCommandLine `
                     -LogBase (Join-Path $RunLogDir 'compile') -TimeoutMinutes $COMPILE_TIMEOUT_MIN
    $logs = @($r.Out, $r.Err)
    $sacHit = Test-LogPattern -Paths $logs -Pattern '0x800711C7' -Simple
    if ($sacHit) {
        Assert-Gate -Id 'B1-COMPILE' -Ok $false `
            -Evidence 'Smart App Control blocked the build (0x800711C7). This is a MACHINE STATE, not a code error.' `
            -Remedy 'Windows Security > Smart App Control > Off (Jonathan only), then re-run. Do NOT loop QA over this.' | Out-Null
    }
    $succeeded = Test-LogPattern -Paths $logs -Pattern 'Result:\s*Succeeded'
    $failed    = Test-LogPattern -Paths $logs -Pattern 'Result:\s*Failed'
    $ok = ($succeeded -and -not $failed -and -not $r.TimedOut)
    Assert-Gate -Id 'B1-COMPILE' -Ok $ok `
        -Evidence ("log-parsed verdict: Succeeded={0} Failed={1} timedOut={2} (exit code deliberately ignored)" -f $succeeded, $failed, $r.TimedOut) `
        -Remedy ("Read {0} and fix the compile before shipping." -f $r.Out) | Out-Null
    $compileVerdict = 'PASS'

    # B2 - the automation suite.
    $r2 = Invoke-Tool -FilePath $EditorCmd -ArgumentString $SuiteCommandLine `
                      -LogBase (Join-Path $RunLogDir 'suite') -TimeoutMinutes $SUITE_TIMEOUT_MIN
    $slogs = @($suiteLog, $r2.Out, $r2.Err)
    # TASK-1193: the parse lives in Get-SuiteVerdict (beside Get-LogMatches) so
    # it can be validated against a real log without a cook (SHIP-9c).  Literal
    # needles under -Simple; a zero is a NAMED STOP, never a pass and never an
    # UNEXPECTED-ERROR; counts are max-per-log because automation.log and
    # suite.out.log carry the same lines.  Nothing below retypes the verdict.
    $v2 = Get-SuiteVerdict -Paths $slogs -Baseline $SUITE_BASELINE
    $suiteTotal = $v2.Performed
    $ok2 = $v2.Ok -and (-not $r2.TimedOut)
    $remedy2 = $v2.Remedy
    if ($r2.TimedOut) {
        $remedy2 = ('The suite did not finish inside {0} minutes and was killed, so these counts are PARTIAL. ' -f $SUITE_TIMEOUT_MIN) + $v2.Remedy
    }
    Assert-Gate -Id 'B2-SUITE' -Ok $ok2 `
        -Evidence ("{0}; timedOut={1}" -f $v2.Evidence, $r2.TimedOut) `
        -Remedy $remedy2 | Out-Null
}

# ------------------------------------------------------------------------------
# PHASE C - COOK + VERIFY (PKG-2a, 5a, 6a, 9)
# ------------------------------------------------------------------------------
Head 'PHASE C - COOK + BOOT-VERIFY'
Say ''
Say '*** THE EXACT UAT COMMAND LINE (PKG-5a recipe; -COOKDIR is not optional) ***'
Say ''
Say ("  {0} {1}" -f $RunUAT, $UatCommandLine)
Say ''
Say 'COOKDIR entries, expanded:'
foreach ($a in $cookDirArgs) { Say ("  {0}" -f $a) }
Say ''

if ($DryRun) {
    Note-Skipped 'C1-COOK'         'dry run: not executed'
    Note-Skipped 'C2-UAT-LOG'      'dry run: not executed'
    Note-Skipped 'C3-BOOT-ARENA'   ("dry run: not executed (instrument would be: {0})" -f $BootEvidence)
    Note-Skipped 'C4-NO-MODELS'    'dry run: not executed'
    Note-Skipped 'C2-STAGE-MANIFESTS' 'dry run: not evaluated'
    Note-Skipped 'C2-STAGE-PRUNE'     'dry run: nothing deleted'
    Note-Skipped 'C2-STAGE-ONE-EXE'   'dry run: not evaluated'
    Say 'Stage hygiene plan (PKG-10 - prune BEFORE boot-verify, verify what you ship):'
    Say  '  every staged .exe/.pdb in NO manifest is an ORPHAN and is DELETED, then'
    Say  '  the invariant is asserted: EXACTLY ONE runnable game exe + the root shim.'
    Say ("  game binary resolved from the manifests: {0}" -f $StageGameExeRel)
    Say ("    (resolution authority: {0})" -f $script:StageExeResolvedBy)
    Say ("  root shim / click target:               {0}" -f $StageShimRel)
    Say ("    (resolution authority: {0})" -f $script:StageShimResolvedBy)
    Say ''
    Say 'Boot-verify plan (PKG-6a - a menu is NEVER a pass):'
    if ($BootEvidence -eq 'Pixel') {
        # THE PLAN AND THE LIVE RUN PRINT THE SAME NUMBERS FROM THE SAME
        # CONSTANTS.  SHIP-0: where the plan and the execution diverge, the
        # divergence is a defect - so neither is retyped.
        Say ("  launch:  {0}" -f $StageExe)
        Say  '           ...with NO ARGUMENTS AT ALL - exactly a player double-click.'
        Say  '  drive:   click "Play (vs Bot)" in the game OWN menu (PKG-9f), using the'
        Say  '           t669_topclick mechanism: topmost-without-activation, real'
        Say  '           cursor, the GetAncestor(WindowFromPoint) abort predicate, then'
        Say ("           {0} press(es).  NO INPUT IS INJECTED unless the game itself owns" -f $BOOT_MENU_CLICKS)
        Say  '           the pixel under the cursor.'
        Say ("           click point: client-relative ({0:N4}, {1:N4}) - entry {2} of {3}," -f `
             $BOOT_MENU_PLAY_XFRAC, $BOOT_MENU_PLAY_YFRAC, $BOOT_MENU_PLAY_INDEX, $BOOT_MENU_ENTRY_COUNT)
        Say ("           centred VBox, pitch {0} of client height (TASK-669, measured)." -f $BOOT_MENU_PITCH_FRAC)
        Say ("  waits:   up to {0}s for the game window, then {1}s after the click for" -f $BOOT_MENU_WAIT_SEC, $BOOT_TRAVEL_WAIT_SEC)
        Say  '           menu -> level travel -> HUD to settle.'
        Say  '  capture: a PRE-CLICK menu frame (DIAGNOSTIC ONLY - it is never bound as'
        Say  '           the pending capture and can never be adjudicated), then the'
        Say  '           adjudicated frame.'
        Say  '  judge:   the four PKG-6a criteria, by a caller that can see (SHIP-8).'
        Say ''
        Say  '  WHY NO MAP ARGUMENT (PKG-9f, measured by TASK-699 as a controlled A/B):'
        Say  '  /Game/Maps/L_Arena, bare L_Arena and -ExecCmds="open ..." ALL THREE'
        Say  '  booted the SHIPPING exe to the menu, while the Development exe - same'
        Say  '  machine, same content, minutes apart - logged LoadMap:/Game/Maps/L_Arena.'
        Say  '  A Shipping artifact CANNOT self-drive into an arena, so passing the'
        Say  '  argument would guarantee a MENU capture - and the only honest'
        Say  '  adjudication of a menu is FAIL, on every build, forever.'
        Say  '  THAT NEEDS AN UNLOCKED DESKTOP: a Shipping /ship is SCHEDULABLE, not'
        Say  '  unattended.  Gate A8-DESKTOP checks it in PHASE A, before the cook.'
        Say  '  It does NOT lower PKG-6a - "hard to reach" is an argument about the'
        Say  '  INSTRUMENT, never about the BAR.'
    } else {
        Say ("  launch: {0} {1} -windowed -ResX=1280 -ResY=720 -abslog=<runlog>\bootverify-arena.log" -f $StageExe, $RECIPE_MAPS[1])
        Say  '  the map argument IS honoured on this configuration / engine class:'
        Say  '  TASK-699 measured the Development binary logging LoadMap:/Game/Maps/'
        Say  '  L_Arena.  PKG-9f: a SHIPPING binary ignores it (and -ExecCmds), which'
        Say  '  is why route Pixel launches with no arguments and clicks the menu.'
        Say  '  require: arena LoadMap + a real deck built from card rows + hero spawned'
        Say ("  sweep:   {0} = 0 hits (benign exception: the SiegeLlama no-model line)" -f ($BOOT_FAIL_PATTERNS -join ' / '))
    }
    # SHIP-7 AMENDED: a dry run can never reach C3's adjudication, so it PRINTS
    # the contract it cannot exercise.  This is the exact hole that would
    # otherwise ship "green" and fail on first real use.
    Say ''
    if (-not $A6Ok) {
        # QA TASK-702 WARN-1.  On the no-route STOP path $BootEvidence is
        # deliberately left at its REQUESTED value (assigning a sentinel would
        # throw inside the gate and surface as exit 1), so printing it here as a
        # RESOLUTION would assert a resolution that did not happen - a confident
        # stale literal one screen below a fact table that correctly says NONE.
        Say  '  A6 found NO usable evidence route on this machine, so this run resolved'
        Say  '  NO route at all - the fact table above says NONE, and that is the truth.'
        Say  '  The contract below is printed anyway: a gate a test cannot reach must at'
        Say  '  minimum ANNOUNCE itself in that test output.'
    } elseif ($BootEvidence -eq 'Pixel') {
        Say '  This run resolved evidence route PIXEL, so a LIVE run of this exact'
        Say '  invocation WOULD end at: SHIP RESULT: ADJUDICATE C3 - <capture>.'
    } else {
        Say ('  This run resolved evidence route {0}. The contract below is printed' -f $BootEvidence)
        Say  '  anyway: a dry run is not evidence about phases C-F either way, and a'
        Say  '  gate a test cannot reach must at minimum ANNOUNCE itself here.'
    }
    Write-AdjudicationContract -CapturePath '' -RecordPath $AdjFile -ResumeCmd $ResumeCommandLine -Prospective
} elseif ($reuse) {
    Note-Skipped 'C1-COOK'       'reused - identical build input already cooked and proven'
    Note-Skipped 'C2-UAT-LOG'    'reused'
    $st = Get-Content -LiteralPath $StateFile -Raw | ConvertFrom-Json
    Note-Skipped 'C3-BOOT-ARENA' $(if ($AdjPass) {
        'reused - PIXEL-ADJUDICATED PASS, bindings RE-MEASURED this run at C3-VERDICT-BINDING'
    } else {
        # A boot proven by a CONSUMED adjudication reads boot=PASS in the state,
        # so name the instrument rather than printing a bare "reused" that hides
        # whether pixels or a log line carried the proof (SHIP-6 makes the
        # instrument material, PKG-9a makes it decisive).
        ("reused - state records boot=PASS, instrument: {0}" -f [string](Get-JsonProp $st 'bootInstrument'))
    })
    $cookVerdict = 'PASS (reused)'
    $bootVerdict = 'PASS (reused)'
    $bootInstrument = [string](Get-JsonProp $st 'bootInstrument')
    # SHIP-8b(6): the verdict travels WITH the ship, into the report and the
    # README's "what was verified" section.  A pixel verdict must be exactly as
    # auditable as the log line it replaced.
    if ($AdjPass) {
        $bootInstrument = $AdjInstrument

        # ======================================================================
        # RETIRE THE CONSUMED VERDICT AND ADVANCE THE STATE (SHIP-8d).
        # ======================================================================
        # THE DEFECT THIS CLOSES, TRACED END TO END: the record lived at a STABLE
        # path and was never retired, and the state's 'boot' never moved past
        # ADJUDICATE - because the resume takes THIS branch and the state write
        # lives in the cook branch.  So after a ship completed, the record still
        # sat on disk saying PASS for HEAD H1.  Jonathan then changes something
        # and says "ship": HEAD is H2, the record is found, C3-VERDICT-BINDING
        # re-measures, HEAD mismatches - and the FIRST INVOCATION OF EVERY
        # SUBSEQUENT SHIP STOPS until a human deletes a file.  That is a chore
        # standing exactly where "anytime we make any changes, I can say ship"
        # is supposed to be.
        #
        # WHAT MOVES, AND IT IS DELIBERATELY THE SMALLEST POSSIBLE EDIT: two
        # fields change value - boot ADJUDICATE -> PASS, and bootInstrument
        # gains the adjudication string - and one field is ADDED for the audit
        # trail.  EVERY OTHER FIELD IS CARRIED FORWARD VERBATIM, not recomputed:
        # the reuse decision immediately above already proved head, treeHash,
        # config, recipeHash, schema and stageExeUtc equal to this run's measured
        # values, so carrying them is provably identical to re-deriving them and
        # cannot silently launder a difference.
        #
        # WHY THIS IS NOT A WEAKENING, WHICH IS THE ONLY QUESTION THAT MATTERS:
        # boot=PASS is not a claim that is trusted later - it is re-guarded on
        # every future run by the SAME nine measurements a log-route PASS is
        # guarded by (HEAD, working tree, configuration, recipe, compile, cook,
        # suite total, staged exe presence and its mtime).  A stale state PASS
        # therefore cannot be replayed onto a different build any more than a
        # stale record could.  The anti-rubber-stamp property is untouched: the
        # only way to REACH this line is past C3-VERDICT-FORM, C3-VERDICT-BINDING
        # and C3-VERDICT, all three of which throw on failure in a live run.
        #
        # AND D2 IS PRESERVED EXACTLY - it is not the defect and is not reverted.
        # A genuinely stale record (one that binds to a different build) still
        # STOPS at C3-VERDICT-BINDING, because the only records that get retired
        # are the ones that were READ TO A VERDICT.
        #
        # ARCHIVED, NEVER DELETED (SHIP-8b(6)): moving-and-naming keeps the
        # verdict and its four observations auditable.  The operator's artifact
        # is never silently destroyed - which was D11's real point, and it
        # survives here intact.
        # A CONSUMED RECORD CANNOT RESURRECT: if the archive is copied back to
        # the stable path, the state now reads boot=PASS, so C3-VERDICT-BINDING
        # stops with "the state file records boot='PASS', not a pending
        # adjudication".
        $adjArchived = Join-Path $RunLogDir ("ship-adjudication.{0}.consumed.json" -f $RunStamp)
        $stateAdvanced = [pscustomobject]@{
            schema         = (Get-JsonProp $st 'schema')
            timestampUtc   = (Get-JsonProp $st 'timestampUtc')
            head           = (Get-JsonProp $st 'head')
            treeHash       = (Get-JsonProp $st 'treeHash')
            config         = (Get-JsonProp $st 'config')
            recipeHash     = (Get-JsonProp $st 'recipeHash')
            compile        = (Get-JsonProp $st 'compile')
            suiteTotal     = (Get-JsonProp $st 'suiteTotal')
            cook           = (Get-JsonProp $st 'cook')
            boot           = 'PASS'            # <== MOVED (was ADJUDICATE)
            bootInstrument = $AdjInstrument    # <== MOVED (carries the verdict)
            bootCapture    = (Get-JsonProp $st 'bootCapture')
            bootCaptureSha = (Get-JsonProp $st 'bootCaptureSha')
            stageExeRel    = (Get-JsonProp $st 'stageExeRel')
            stageExeBytes  = (Get-JsonProp $st 'stageExeBytes')
            stageExeUtc    = (Get-JsonProp $st 'stageExeUtc')
            adjudication   = $adjArchived      # <== ADDED (where the verdict went)
        }
        # ORDER IS LOAD-BEARING: ARCHIVE FIRST, ADVANCE THE STATE SECOND.
        # The two writes are not atomic together, so the order decides which
        # half-done state is survivable.  Archive-then-advance leaves, on a
        # failure, a record still armed beside a state still reading ADJUDICATE -
        # i.e. exactly the situation we started in, which the very next run
        # re-consumes idempotently.  The reverse order would leave a state
        # reading PASS beside an armed record, and THAT combination stops at
        # C3-VERDICT-BINDING with "not a pending adjudication": the chore back
        # again, produced by the repair meant to remove it.
        $retired = $false
        try {
            Move-Item -LiteralPath $AdjFile -Destination $adjArchived -Force
            $stateAdvanced | ConvertTo-Json | Out-File -LiteralPath $StateFile -Encoding utf8
            $retired = $true
        } catch {
            Say ("      NOTE: the adjudication could not be retired ({0}). The ship continues - the verdict was already proven - and the next run will simply re-consume the same record." -f $_.Exception.Message)
        }
        if ($retired) {
            Say ''
            Say '  ADJUDICATION CONSUMED AND RETIRED (SHIP-8d):'
            Say ('    record ARCHIVED (not deleted) to: {0}' -f $adjArchived)
            Say  '    ship state: boot ADJUDICATE -> PASS, carrying the adjudication in'
            Say  '    bootInstrument.  SHIP-8b(6) retention is satisfied; the next /ship'
            Say  '    starts clean instead of stopping on a record nobody removed, and a'
            Say  '    record that does NOT bind to its run still STOPS (D2 is untouched).'
            Add-Fact 'Adjudication retired' ("PASS consumed; record ARCHIVED (not deleted) to {0}; state boot ADJUDICATE -> PASS" -f $adjArchived)
        } else {
            Add-Fact 'Adjudication retired' 'NO - the archive/state write failed; the record is still armed and the next run re-consumes it (reported rather than hidden)'
        }
    }
} else {
    # C1/C2 - the cook.  UAT's ERRORLEVEL lies exactly as Build.bat's does;
    # the verdict comes from UAT's own lines.
    $r3 = Invoke-Tool -FilePath $RunUAT -ArgumentString $UatCommandLine `
                      -LogBase (Join-Path $RunLogDir 'cook') -TimeoutMinutes $COOK_TIMEOUT_MIN
    $clogs = @($r3.Out, $r3.Err)
    Assert-Gate -Id 'C1-COOK' -Ok (-not $r3.TimedOut) `
        -Evidence ("UAT finished within {0} min" -f $COOK_TIMEOUT_MIN) -Remedy 'Investigate the stalled cook.' | Out-Null
    $uatOk   = Test-LogPattern -Paths $clogs -Pattern 'BUILD SUCCESSFUL' -Simple
    $uatBad  = Test-LogPattern -Paths $clogs -Pattern 'BUILD FAILED' -Simple
    Assert-Gate -Id 'C2-UAT-LOG' -Ok ($uatOk -and -not $uatBad) `
        -Evidence ("UAT's own verdict lines: BUILD SUCCESSFUL={0} BUILD FAILED={1} (exit code deliberately ignored)" -f $uatOk, $uatBad) `
        -Remedy ("Read {0}." -f $r3.Out) | Out-Null
    $cookVerdict = 'PASS'

    # The cook has just written FRESH manifests.  Re-resolve the game binary
    # from them: its name is configuration-dependent (PKG-10) and the value
    # computed in PHASE A was derived from the PREVIOUS cook's manifests, which
    # may have been a different configuration entirely.
    $StageGameExeRel = Get-StageGameExeRel $StageWin $ProjectName $Configuration
    $StageBinExe     = Join-Path $StageWin ($StageGameExeRel -replace '/','\')
    $StageShimRel    = Get-StageShimExeRel $StageWin $ProjectName
    $StageExe        = Join-Path $StageWin ($StageShimRel -replace '/','\')
    Add-Fact 'Staged game binary' ("{0}  [resolved by: {1}]" -f $StageGameExeRel, $script:StageExeResolvedBy)
    Add-Fact 'Staged click target' ("{0}  [resolved by: {1}]" -f $StageShimRel, $script:StageShimResolvedBy)

    Assert-Gate -Id 'C2-STAGE-PRESENT' -Ok ((Test-Path -LiteralPath $StageExe) -and (Test-Path -LiteralPath $StageBinExe)) `
        -Evidence ("staged click target {0}; game binary {1}" -f $StageExe, $StageGameExeRel) `
        -Remedy 'The cook reported success but produced no staged exe.' | Out-Null

    # ---- PKG-10 STAGE HYGIENE, BEFORE THE BOOT-VERIFY.
    # ORDER IS THE WHOLE POINT: prune -> measure -> boot-verify -> zip.  A
    # boot-verify against a tree that is then modified proves something about a
    # tree that no longer exists.  VERIFY THE ARTIFACT YOU ACTUALLY SHIP.
    $StageGameExeRel = Invoke-StageHygiene -StageRoot $StageWin -Project $ProjectName `
                                           -Config $Configuration -When 'BEFORE boot-verify (PKG-10 order)'
    $StageBinExe     = Join-Path $StageWin ($StageGameExeRel -replace '/','\')

    # ==========================================================================
    # C3 BOOT-VERIFY TO A REAL ARENA (PKG-6a).  "BUILD SUCCESSFUL" is not
    # evidence of anything but the build.  A menu-only check PASSED a package
    # with no deck, no HUD and no hero.  That is why this gate is here.
    # ==========================================================================
    # THE LAUNCH IS ROUTE-DEPENDENT, AND PKG-9f IS THE WHOLE REASON:
    #   route Log   - Development, or Shipping on a source-built engine.  Launch
    #                 WITH the map argument.  TASK-699 measured the Development
    #                 binary honouring it (LoadMap: /Game/Maps/L_Arena).  This
    #                 path is byte-for-byte the behaviour this script has always
    #                 had; nothing about route 1 changed.
    #   route Pixel - THE STANDING ROUTE HERE.  Launch with NO ARGUMENTS, exactly
    #                 as a player double-clicks, and CLICK Play in the game's own
    #                 menu.  699 measured, as a controlled A/B, that a SHIPPING
    #                 binary ignores the map argument AND -ExecCmds and boots to
    #                 the menu.  Passing the refuted argument would guarantee a
    #                 MENU capture - and the only honest adjudication of a menu
    #                 is FAIL, so the ship could never pass on any build, ever.
    #
    # THIS IS A ROUTE CHANGE ONLY.  PKG-6a is untouched: what has to be ON SCREEN
    # is exactly what it was, and a caller that can see still judges it.  "The
    # arena is hard to reach in Shipping" is an argument about the INSTRUMENT and
    # never about the BAR.  The click route is also STRICTLY BETTER evidence than
    # the argument it replaces: it exercises the real menu -> level travel -> HUD
    # path a player takes, on the shipped binary, instead of a developer shortcut
    # that bypasses it.
    $bootLog = Join-Path $RunLogDir 'bootverify-arena.log'

    # ---- STALE-PROCESS INTERLOCK, TAKEN BEFORE ANYTHING IS LAUNCHED.
    # The stage path is stable across runs, so path-matching alone would happily
    # adopt a leftover game from an earlier ship as proof that THIS cook booted.
    # Snapshot what is already running and exclude those PIDs from every later
    # query.  They are NOT killed: a process that is Jonathan's stays Jonathan's,
    # and this run only ever kills what it started itself.
    $preExisting    = @(Get-StageGameProcess -StageRoot $StageWin)
    $preExistingIds = @($preExisting | ForEach-Object { $_.Id })
    if ($preExisting.Count -gt 0) {
        Say ("    NOTE: {0} process(es) were ALREADY running from this stage before the launch and are EXCLUDED from this run's evidence:" -f $preExisting.Count)
        foreach ($xp in $preExisting) { Say ("          pid {0}  {1}" -f $xp.Id, $xp.ProcessName) }
        Say  '          They are not killed - this run kills only what it started.'
    }

    $defaultLog = Join-Path $StageWin ("{0}\Saved\Logs\{1}.log" -f $ProjectName, $ProjectName)
    # (Checked against the PKG-10 defect class and CLEARED: UE names the staged
    # project folder and its log after the PROJECT, not after the target, so
    # neither of these two substitutions is configuration-dependent.  Under
    # Shipping the file does not exist at all - PKG-9a-1, log-silent - which is
    # why route Pixel does not read it.)
    $bootLogs   = @($bootLog, $defaultLog)
    $sawArena   = $false
    $arenaRe    = ('{0}.*{1}' -f $BOOT_MARK_ARENA, [regex]::Escape($RECIPE_MAPS[1]))
    $gameProcs  = @()
    $clickRes   = $null
    $menuShot   = ''
    $gameHwnd   = [IntPtr]::Zero

    if ($BootEvidence -eq 'Log') {
        $bootArgs = ('{0} -windowed -ResX=1280 -ResY=720 -abslog="{1}"' -f $RECIPE_MAPS[1], $bootLog)
        Say ("    launching (route Log - the map argument IS honoured on this configuration): {0} {1}" -f $StageExe, $bootArgs)
        $bp = Start-Process -FilePath $StageExe -ArgumentList $bootArgs -PassThru
        $deadline = (Get-Date).AddSeconds($BOOT_TIMEOUT_SEC)
        while ((Get-Date) -lt $deadline) {
            Start-Sleep -Seconds 5
            if (Test-LogPattern -Paths $bootLogs -Pattern $arenaRe) {
                if (Test-LogPattern -Paths $bootLogs -Pattern $BOOT_MARK_DECK -Simple) { $sawArena = $true; break }
            }
        }
    } else {
        # ---- (a) LAUNCH THE WAY A PLAYER DOES: NO ARGUMENTS AT ALL (PKG-9f).
        Say ("    launching (route Pixel - PKG-9f: NO ARGUMENTS, exactly a player's double-click): {0}" -f $StageExe)
        $bp = Start-Process -FilePath $StageExe -PassThru

        # ---- (b) WAIT FOR THE GAME'S OWN WINDOW.  Not for a log line: there is
        # no log (PKG-9a-1), and waiting on one that cannot be written is how a
        # gate spends 300 seconds proving nothing.  The click needs an hwnd, so
        # the honest wait condition is "a process launched from this stage owns a
        # titled window".
        $menuDeadline = (Get-Date).AddSeconds($BOOT_MENU_WAIT_SEC)
        while ((Get-Date) -lt $menuDeadline) {
            Start-Sleep -Seconds 5
            $gameProcs = @(Get-StageGameProcess -StageRoot $StageWin -ExcludePids $preExistingIds)
            foreach ($gp in $gameProcs) {
                $t = Get-ProcWindowTitle $gp
                $h = [IntPtr]::Zero
                try { $h = $gp.MainWindowHandle } catch { $h = [IntPtr]::Zero }
                if ($t -and ($h -ne [IntPtr]::Zero)) {
                    $windowTitle = $t
                    $gameHwnd    = $h
                    break
                }
            }
            if ($gameHwnd -ne [IntPtr]::Zero) { break }
        }
        Say ("    game window: {0}; title '{1}'" -f `
             $(if ($gameHwnd -ne [IntPtr]::Zero) { 'found' } else { 'NOT FOUND within the wait' }), $windowTitle)

        # ---- (c) THE PRE-CLICK MENU CAPTURE.  DIAGNOSTIC ONLY.
        # It exists so a mis-aimed click is DIAGNOSABLE instead of mysterious -
        # if the adjudicated frame turns out to be a menu, this frame says
        # whether the menu was even the thing we aimed at.
        # IT CAN NEVER BE ADJUDICATED BY ACCIDENT, and that is mechanical rather
        # than a matter of care: only $pixelShot is written into the state's
        # bootCapture, and SHIP-8b rule 9 rejects any record whose capturePath is
        # not the state's pending capture.  The filename says so as well, because
        # the next reader will meet the file before they meet this comment.
        $menuShot = Save-ScreenCapture (Join-Path $RunLogDir 'bootverify-menu-preclick-NOT-ADJUDICABLE.png')

        # ---- (d) DRIVE THE GAME'S OWN MENU (PKG-9f's ruled route).
        Say ("    clicking menu entry {0} of {1} (Play vs Bot) at client-relative ({2:N4}, {3:N4})" -f `
             $BOOT_MENU_PLAY_INDEX, $BOOT_MENU_ENTRY_COUNT, $BOOT_MENU_PLAY_XFRAC, $BOOT_MENU_PLAY_YFRAC)
        $clickRes = Invoke-MenuClick -Hwnd $gameHwnd -XFrac $BOOT_MENU_PLAY_XFRAC `
                                     -YFrac $BOOT_MENU_PLAY_YFRAC -Clicks $BOOT_MENU_CLICKS
        Say ("    menu click: {0} - {1}" -f $(if ($clickRes.Ok) { 'DELIVERED' } else { 'NOT DELIVERED' }), $clickRes.Detail)

        # ---- (e) LET menu -> level travel -> HUD SETTLE, then capture.
        if ($clickRes.Ok) {
            Say ("    waiting {0}s for level travel and the HUD to settle before the capture" -f $BOOT_TRAVEL_WAIT_SEC)
            Start-Sleep -Seconds $BOOT_TRAVEL_WAIT_SEC
        }
    }

    # ---- THE RUNNING GAME IS RESOLVED BY *PATH*, NEVER BY NAME.
    # This is BLOCKER-1 of QA TASK-702, and it is the PKG-10 defect class one
    # gate to the left: Get-Process -Name applies no wildcard, and under Shipping
    # the staged binary is <Project>-Win64-Shipping.exe, so a name query matched
    # ONLY the title-less root shim (715 section 6: pid 28688 Title='' vs pid
    # 9104 GitClaudeUnrealTest-Win64-Shipping Title='Siegebound').  C3-BOOT-TITLE
    # therefore stopped EVERY Shipping run - upstream of the state write, so
    # nothing ever armed the resume.  Under Development the two share a name and
    # it worked, which is precisely why the only route anyone exercised was the
    # one that was not broken.
    # Re-queried here, after the drive, so the title is read from the window that
    # actually exists at capture time.
    $gameProcs = @(Get-StageGameProcess -StageRoot $StageWin -ExcludePids $preExistingIds)
    $titleFrom = ''
    foreach ($gp in $gameProcs) {
        $t = Get-ProcWindowTitle $gp
        if ($t) { $windowTitle = $t; $titleFrom = ("pid {0} {1}" -f $gp.Id, $gp.ProcessName) }
    }
    Add-Fact 'Game processes (by path)' $(if ($gameProcs.Count -eq 0) {
            ("0 live under {0} - resolved by PATH, never by name (PKG-10 class)" -f $StageWin)
        } else {
            ("{0} live under the stage: {1}" -f $gameProcs.Count, (@($gameProcs | ForEach-Object { ('pid {0} {1}' -f $_.Id, $_.ProcessName) }) -join ', '))
        })
    Add-Fact 'Window title' $(if ($titleFrom) { ("{0}  [read from {1}]" -f $windowTitle, $titleFrom) } else { $windowTitle })

    $pixelShot = ''
    $pixelSha  = ''
    $pixelBytes = 0
    $pixelDistinct = 0
    $pixelReadable = $false
    if ($BootEvidence -eq 'Pixel') {
        # PKG-9a route 2: process liveness + window title + a RENDERED-PIXEL
        # capture.  This script can take the capture; it cannot read it.  So it
        # STOPS and hands the caller the file to adjudicate.  It never converts
        # "I could not measure" into a pass.
        $pixelShot = Save-ScreenCapture (Join-Path $RunLogDir 'bootverify-arena.png')
    }

    # Kill ONLY what this run started: $gameProcs already excludes every PID that
    # was running before the launch, and $bp is this run's own shim.  Resolving
    # by path is what makes this work at all - a name query never matched the
    # Shipping game, so it survived the ship holding Saved/ open (QA WARN-5).
    foreach ($gp in $gameProcs) { try { $gp.Kill() } catch { } }
    try { if (-not $bp.HasExited) { $bp.Kill() } } catch { }
    Start-Sleep -Seconds 3

    if ($BootEvidence -eq 'Log') {
        $bootInstrument = 'the packaged build''s own log'
        $deckHits = Get-LogMatches -Paths $bootLogs -Pattern $BOOT_RE_DECK
        $deckRows = 0
        $deckCards = 0
        if ($deckHits.Count -gt 0) {
            $m = $deckHits[$deckHits.Count - 1].Matches[0]
            $deckCards = [int]$m.Groups[1].Value
            if ($m.Groups[3].Success) { $deckRows = [int]$m.Groups[3].Value } elseif ($m.Groups[2].Success) { $deckRows = [int]$m.Groups[2].Value }
        }
        $heroOk = Test-LogPattern -Paths $bootLogs -Pattern $BOOT_MARK_HERO -Simple

        # The negative sweep is only meaningful because the positives above
        # proved the log channel is alive (PKG-9a).
        $bad = @()
        foreach ($pat in $BOOT_FAIL_PATTERNS) {
            foreach ($hit in (Get-LogMatches -Paths $bootLogs -Pattern $pat -Simple)) {
                $benign = $false
                foreach ($bp2 in $BOOT_BENIGN_PATTERNS) { if ($hit.Line -like ('*' + $bp2 + '*')) { $benign = $true } }
                if (-not $benign) { $bad += $hit.Line.Trim() }
            }
        }
        foreach ($b2 in ($bad | Select-Object -First 12)) { Say ("      content-resolution failure: {0}" -f $b2) }

        $bootOk = $sawArena -and ($deckCards -gt 0) -and ($deckRows -gt 0) -and $heroOk -and ($bad.Count -eq 0)
        Assert-Gate -Id 'C3-BOOT-ARENA' -Ok $bootOk `
            -Evidence ("arena={0} deck={1} cards from {2} rows hero={3} content-failures={4} [instrument: {5}]" -f $sawArena, $deckCards, $deckRows, $heroOk, $bad.Count, $bootInstrument) `
            -Remedy 'A menu is NEVER a pass (PKG-6a). If assets are missing, the -COOKDIR recipe is the first suspect (PKG-5a). Fix the cook - do not weaken the check.' | Out-Null
        $bootVerdict = 'PASS'
    } else {
        $bootInstrument = 'rendered pixels (PKG-9a route 2) - CALLER-ADJUDICATED'
        Assert-Gate -Id 'C3-BOOT-TITLE' -Ok ($windowTitle -like '*Siegebound*') `
            -Evidence ("window title '{0}' must read Siegebound (PKG-8)" -f $windowTitle) `
            -Remedy 'Set ProjectName/ProjectDisplayedTitle in Config/DefaultGame.ini (PKG-8) and re-cook.' | Out-Null

        # ---- SHIP-8c THE MECHANICAL PRE-FILTER.  IT MAY ONLY EVER *FAIL*.
        # Every check below is cheap, mechanical, and about whether a capture
        # HAPPENED - never about what is IN it.  There is deliberately no
        # "does it look like an arena" heuristic: in PowerShell that would pass
        # a black screen with a loading spinner while wearing a gate's clothes,
        # and a fake gate is worse than no gate.
        $preFail = @()
        if (-not $pixelShot) {
            $preFail += 'the screen capture never happened (System.Drawing threw)'
        } elseif (-not (Test-Path -LiteralPath $pixelShot)) {
            $preFail += ("no capture file at {0}" -f $pixelShot)
        } else {
            $pixelBytes = (Get-Item -LiteralPath $pixelShot).Length
            if ($pixelBytes -lt 1024) { $preFail += ("the capture is {0} B - truncated or empty" -f $pixelBytes) }
            # "Frame not uniformly blank" is a MECHANICAL property (are all the
            # sampled pixels the same value?), not a judgement about content.
            # Sampling a sparse grid keeps it cheap on a 4K frame.
            try {
                Add-Type -AssemblyName System.Drawing
                $chk = [System.Drawing.Bitmap]::FromFile($pixelShot)
                try {
                    $pixelReadable = $true
                    $seen = @{}
                    for ($sx = 0; $sx -lt 32; $sx++) {
                        for ($sy = 0; $sy -lt 18; $sy++) {
                            $px = [int](($chk.Width  - 1) * $sx / 31)
                            $py = [int](($chk.Height - 1) * $sy / 17)
                            $seen[$chk.GetPixel($px, $py).ToArgb()] = $true
                        }
                    }
                    $pixelDistinct = $seen.Count
                } finally { $chk.Dispose() }
            } catch { $pixelReadable = $false }
            if (-not $pixelReadable) {
                $preFail += 'the capture cannot be decoded as an image'
            } elseif ($pixelDistinct -lt 2) {
                $preFail += 'the captured frame is UNIFORMLY BLANK - one colour across the whole sample grid'
            }
        }
        if ($gameProcs.Count -lt 1) { $preFail += 'no live game process was found when the capture was taken' }
        if (($windowTitle -eq '(not measured)') -or (-not $windowTitle)) { $preFail += 'no window title could be read' }
        # DID THE DRIVE HAPPEN?  Under route Pixel the arena is reached by
        # CLICKING (PKG-9f); if no input was delivered the frame CANNOT show an
        # arena, only a menu or somebody else's window.  This is SHIP-8c's "the
        # capture never happened" family - a mechanical fact about whether the
        # DRIVE occurred, never a judgement about what is IN the frame - and it
        # keeps a guaranteed-FAIL menu from being handed to an adjudicator after
        # a 30-minute cook.  The cheap PHASE-A gate A8-DESKTOP is what stops the
        # commonest cause (a locked desktop) BEFORE the cook is spent.
        if ($null -eq $clickRes) {
            $preFail += 'the menu click was never attempted, so the capture cannot show an arena'
        } elseif (-not $clickRes.Ok) {
            $preFail += ("the menu click was NOT delivered: {0} - the capture cannot show an arena" -f $clickRes.Detail)
        }
        Add-Fact 'Menu capture (diagnostic)' $(if ($menuShot) {
                ("{0} - PRE-CLICK. NOT ADJUDICABLE and never the boot evidence: only the arena capture is bound as this run's pending capture (SHIP-8b rule 9)." -f $menuShot)
            } else { 'not taken' })

        Assert-PreFilter -Id 'C3-CAPTURE' -Ok ($preFail.Count -eq 0) `
            -Evidence ("MECHANICAL PRE-FILTER ONLY (SHIP-8c) - THIS IS NOT A PASS OF C3-BOOT-ARENA AND CAN NEVER BECOME ONE. {0}" -f `
                       $(if ($preFail.Count -eq 0) {
                            ("capture {0}, {1}, {2} distinct sampled colours, process live, title read" -f $pixelShot, (Format-Size $pixelBytes), $pixelDistinct)
                         } else { ($preFail -join ' | ') })) `
            -Remedy 'There is nothing to adjudicate. Re-run the boot-verify; if the capture keeps failing, the desktop may be locked (simulated input and screen capture both need an unlocked session).'

        $pixelSha = (Get-FileHash -LiteralPath $pixelShot -Algorithm SHA256).Hash.ToLowerInvariant()
        # NOT a pass.  A SUSPENSION - emitted below, AFTER the state write.
        $bootVerdict = 'ADJUDICATE'
    }

    # ---- State.  SHIP-8d: WRITTEN BEFORE THE ADJUDICATION STOP.
    # This is the whole repair.  The old code threw at C3 under route 2 and
    # never reached this write, so nothing ever armed resume-by-measurement and
    # every Shipping /ship re-cooked for ~30 minutes and stopped again, forever.
    # 'boot' now carries the honest third value: PASS (proven from the build's
    # own log) or ADJUDICATE (captured, pending the one judgement a script
    # cannot make).  ADJUDICATE is NOT a pass - the reuse decision refuses it
    # unless a verdict record binds to this exact capture and this exact build.
    $state = [pscustomobject]@{
        schema         = $STATE_SCHEMA
        timestampUtc   = $script:RunUtc.ToString('o')
        head           = $HeadSha
        treeHash       = $TreeHash
        config         = $Configuration
        recipeHash     = $RecipeHash
        compile        = 'PASS'
        suiteTotal     = $suiteTotal
        cook           = 'PASS'
        boot           = $bootVerdict
        bootInstrument = $bootInstrument
        bootCapture    = $pixelShot
        bootCaptureSha = $pixelSha
        stageExeRel    = $StageGameExeRel
        stageExeBytes  = (Get-Item -LiteralPath $StageBinExe).Length
        stageExeUtc    = (Get-Item -LiteralPath $StageBinExe).LastWriteTimeUtc.ToString('o')
    }
    $state | ConvertTo-Json | Out-File -LiteralPath $StateFile -Encoding utf8

    # ---- SHIP-1's THIRD TERMINAL VERDICT.  Reached only with the state on
    # disk, so the resume is armed before the run suspends.
    if ($bootVerdict -eq 'ADJUDICATE') {
        $exeNow = Get-Item -LiteralPath $StageBinExe
        Say ''
        Say  '  HOW THIS CAPTURE WAS REACHED (PKG-9f), so you know what you are judging:'
        Say  '    the shipped exe was launched with NO ARGUMENTS - a player double-click -'
        Say ('    and "Play (vs Bot)" was clicked in the game own menu: {0}' -f $(if ($clickRes) { $clickRes.Detail } else { 'no click was attempted' }))
        if ($menuShot) {
            Say ('    the PRE-CLICK menu frame is at {0}' -f $menuShot)
            Say  '    - DIAGNOSTIC ONLY.  It is not adjudicable and is not the boot evidence;'
            Say  '      it is there so that a mis-aimed click is diagnosable, not mysterious.'
        }
        Say ''
        Say '  The identity this verdict must bind to (copy these into the record):'
        Say ("    capturePath   : {0}" -f $pixelShot)
        Say ("    captureSha256 : {0}" -f $pixelSha)
        Say ("    head          : {0}" -f $HeadSha)
        Say ("    config        : {0}" -f $Configuration)
        Say ("    stageExeBytes : {0}" -f $exeNow.Length)
        Say ("    stageExeUtc   : {0}" -f $exeNow.LastWriteTimeUtc.ToString('o'))
        Note-Adjudicate -Id 'C3-BOOT-ARENA' -CapturePath $pixelShot `
                        -RecordPath $AdjFile -ResumeCmd $ResumeCommandLine
    }
}

# PKG-10 STAGE HYGIENE, SECOND CALL - deliberately OUTSIDE the cook branch, for
# the same reason C4 is: what matters is what is about to be ZIPPED.  A REUSED
# stage was pruned by the invocation that cooked it, but this run is the one
# writing the archive, so this run asserts the invariant.  The function is
# idempotent - on a stage already clean it deletes nothing and only asserts.
if (-not $DryRun) {
    $StageGameExeRel = Invoke-StageHygiene -StageRoot $StageWin -Project $ProjectName `
                                           -Config $Configuration -When 'BEFORE the zip (what you ship is what you verified)'
    $StageBinExe     = Join-Path $StageWin ($StageGameExeRel -replace '/','\')
}

# C4 - PKG-4 re-check on the STAGE.  Deliberately OUTSIDE the cook branch: it
# runs on a reused stage too, because what matters is what is about to be
# ZIPPED, not what was true when the cook finished.
if (-not $DryRun) {
    $ggufs     = @(Get-ChildItem -LiteralPath $StageWin -Recurse -File -Filter '*.gguf' -ErrorAction SilentlyContinue)
    $modelDirs = @(Get-ChildItem -LiteralPath $StageWin -Recurse -Directory -Filter 'Models' -ErrorAction SilentlyContinue)
    Assert-Gate -Id 'C4-NO-MODELS' -Ok (($ggufs.Count -eq 0) -and ($modelDirs.Count -eq 0)) `
        -Evidence ("staged .gguf={0} Models dirs={1}" -f $ggufs.Count, $modelDirs.Count) `
        -Remedy 'The zip ships WITHOUT the model weights by default (PKG-4).' | Out-Null
}

# Sizes (PKG-9d - a size that moves the wrong way is the cheapest smoke alarm)
if ((-not $DryRun) -and (Test-Path -LiteralPath $StageWin)) {
    if (Test-Path -LiteralPath $StageBinExe) { Add-Fact 'Staged exe' (Format-Size (Get-Item -LiteralPath $StageBinExe).Length) }
    # The pdb is named after the RESOLVED game binary, never guessed - same
    # PKG-10 reason as the exe: under Shipping the guessed name is the orphan.
    $pdb = [System.IO.Path]::ChangeExtension($StageBinExe, '.pdb')
    if (Test-Path -LiteralPath $pdb) { Add-Fact 'Staged pdb' (Format-Size (Get-Item -LiteralPath $pdb).Length) }
    else { Add-Fact 'Staged pdb' 'absent (record this against the Development baseline, PKG-9d)' }
    foreach ($ext in @('pak','ucas','utoc')) {
        $f = @(Get-ChildItem -LiteralPath $StageWin -Recurse -File -Filter ("*.{0}" -f $ext) -ErrorAction SilentlyContinue)
        if ($f.Count -gt 0) { Add-Fact ("Staged .{0}" -f $ext) (Format-Size ($f | Measure-Object -Property Length -Sum).Sum) }
    }
}

# ------------------------------------------------------------------------------
# PHASE D - PACKAGE (SHIP-2 D, SHIP-4, PKG-7b)
# ------------------------------------------------------------------------------
Head 'PHASE D - PACKAGE'
Say ("  zip target: {0}" -f $ZipPath)
Say  '  writer:     .NET ZipArchive (ZIP64-capable). Compress-Archive is BANNED'
Say  '              here: PS 5.1 truncates near 2 GB, and a silently-truncated'
Say  '              archive is the worst possible failure mode.'

# D0 - the README must be THIS ship's (see RESUME-BY-MEASUREMENT above).
$readmeFresh = $false
$readmeEv = 'README.md missing from the staging dir'
if (Test-Path -LiteralPath $ReadmePath) {
    $rtext = Get-Content -LiteralPath $ReadmePath -Raw
    $readmeFresh = $rtext.Contains($ZipName)
    $readmeEv = ("README.md {0} name this ship's zip ({1})" -f $(if ($readmeFresh) { 'does' } else { 'does NOT' }), $ZipName)
}
if ($DryRun) {
    Note-Skipped 'D0-README' ("dry run: {0}" -f $readmeEv)
} else {
    Assert-Gate -Id 'D0-README' -Ok $readmeFresh -Evidence $readmeEv `
        -Remedy ("Rewrite {0} for this ship (SHIP-3: zip name, date, config, sizes, WHAT CHANGED in player-facing language, WHAT WAS VERIFIED, the PKG-1 correction, PKG-4, PKG-8, the PKG-9b assistant finding, and the last shipped commit so the NEXT ship knows its diff base), then re-run. A zip written with a stale README is a PARTIAL SHIP and SHIP-1 forbids it." -f $ReadmePath) | Out-Null
}

if ($DryRun) {
    Note-Skipped 'D1-ZIP'          'dry run: nothing written'
    Note-Skipped 'D2-ZIP-READBACK' 'dry run: nothing written'
    Note-Skipped 'D2-SIZE-SANITY'  'dry run: nothing written'
    Note-Skipped 'D3-PRUNE'        'dry run: nothing deleted'
    $existing = @()
    if (Test-Path -LiteralPath $StagingDir) {
        $existing = @(Get-ChildItem -LiteralPath $StagingDir -Filter '*.zip' -File -ErrorAction SilentlyContinue)
    }
    Say  '  retention plan (PKG-7b, N=2 per configuration):'
    foreach ($z in $existing) {
        $sameCfg = $z.Name -match ("^Siegebound-Win64-{0}-\d{{4}}-\d{{2}}-\d{{2}}\.zip$" -f [regex]::Escape($Configuration))
        if ($sameCfg) { Say ("    candidate (same config): {0}" -f $z.Name) }
        else          { Say ("    PROTECTED (other config or foreign name, NEVER pruned): {0}" -f $z.Name) }
    }
} else {
    # D1 - write to a temp name first.  The previous zip's bytes are not
    # released until the replacement has been read back and verified
    # (PKG-7b: a failed zip plus a deleted predecessor leaves Jonathan nothing).
    $tmpZip = Join-Path $StagingDir ("{0}.partial" -f $ZipName)
    if (Test-Path -LiteralPath $tmpZip) { Remove-Item -LiteralPath $tmpZip -Force }

    # Purge boot-verify run output before zipping - test logs never ship.
    $savedDir = Join-Path $StageWin ("{0}\Saved" -f $ProjectName)
    if (Test-Path -LiteralPath $savedDir) {
        Remove-Item -LiteralPath $savedDir -Recurse -Force -ErrorAction SilentlyContinue
        Say '    purged the boot-verify Saved/ output from the stage'
    }

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $files = @(Get-ChildItem -LiteralPath $StageWin -Recurse -File)
    Say ("    zipping {0} staged file(s) + README.md ..." -f $files.Count)
    $fs = [System.IO.File]::Open($tmpZip, [System.IO.FileMode]::Create)
    try {
        $zip = New-Object System.IO.Compression.ZipArchive($fs, [System.IO.Compression.ZipArchiveMode]::Create)
        try {
            [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
                $zip, $ReadmePath, 'README.md', [System.IO.Compression.CompressionLevel]::Optimal) | Out-Null
            $n = 0
            foreach ($f in $files) {
                $rel = 'Windows/' + $f.FullName.Substring($StageWin.Length + 1).Replace('\','/')
                [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
                    $zip, $f.FullName, $rel, [System.IO.Compression.CompressionLevel]::Optimal) | Out-Null
                $n++
                if (($n % 250) -eq 0) { Say ("      {0}/{1}" -f $n, $files.Count) }
            }
        } finally { $zip.Dispose() }
    } finally { $fs.Dispose() }

    # D2 - VERIFY BY READING THE ARCHIVE BACK (SHIP-4).  The tool's success
    # report is about the tool, not about the artifact.
    $entries = @()
    $za = [System.IO.Compression.ZipFile]::OpenRead($tmpZip)
    try { $entries = @($za.Entries | ForEach-Object { $_.FullName }) } finally { $za.Dispose() }

    # PKG-10: BOTH staged binary names are DERIVED, neither is guessed.  The real
    # binary's name is configuration-dependent - the old hardcoded
    # "<Project>/Binaries/Win64/<Project>.exe" named the STALE DEVELOPMENT ORPHAN
    # under Shipping, so once stage hygiene deletes that orphan this gate would
    # have failed every clean package.  The shim is not configuration-dependent
    # today, but it was still a guess, and this file no longer guesses either.
    $clickTarget = ('Windows/' + $StageShimRel)
    $realBinary  = ('Windows/' + $StageGameExeRel)
    $hasClick  = $entries -contains $clickTarget
    $hasBinary = $entries -contains $realBinary
    $hasReadme = $entries -contains 'README.md'
    $hasPak    = @($entries | Where-Object { $_ -like '*.pak'  }).Count
    $hasUcas   = @($entries | Where-Object { $_ -like '*.ucas' }).Count
    $hasUtoc   = @($entries | Where-Object { $_ -like '*.utoc' }).Count
    $badModels = @($entries | Where-Object { ($_ -like '*.gguf') -or ($_ -like '*/Models/*') -or ($_ -like 'Models/*') }).Count
    $zipBytes  = (Get-Item -LiteralPath $tmpZip).Length

    Add-Fact 'Zip entries' ("{0}" -f $entries.Count)
    Add-Fact 'Zip size'    (Format-Size $zipBytes)

    $verifyOk = $hasClick -and $hasBinary -and $hasReadme -and ($hasPak -ge 1) -and ($hasUcas -ge 1) -and ($hasUtoc -ge 1) -and ($badModels -eq 0)
    Assert-Gate -Id 'D2-ZIP-READBACK' -Ok $verifyOk `
        -Evidence ("entries={0} clickTarget={1} binary={2} README={3} pak={4} ucas={5} utoc={6} models/gguf={7}" -f $entries.Count, $hasClick, $hasBinary, $hasReadme, $hasPak, $hasUcas, $hasUtoc, $badModels) `
        -Remedy 'A zip that cannot be read back is a STOP, and the previous zip is not pruned (SHIP-4).' | Out-Null

    # Size smoke alarm against the newest zip of the SAME configuration.  This
    # does NOT catch the PKG-5a soft-ref defect (the broken pass-1 package was
    # only 12% smaller) - the arena boot-verify is what catches that.  This is
    # a smoke alarm for a grossly mis-configured cook, nothing more.
    $prev = @(Get-ChildItem -LiteralPath $StagingDir -File -ErrorAction SilentlyContinue |
              Where-Object { $_.Name -match ("^Siegebound-Win64-{0}-\d{{4}}-\d{{2}}-\d{{2}}\.zip$" -f [regex]::Escape($Configuration)) -and $_.Name -ne $ZipName } |
              Sort-Object Name -Descending)
    if ($prev.Count -gt 0) {
        $ratio = $zipBytes / $prev[0].Length
        Assert-Gate -Id 'D2-SIZE-SANITY' -Ok ($ratio -ge $SIZE_ALARM_RATIO) `
            -Evidence ("{0:P0} of {1} ({2})" -f $ratio, $prev[0].Name, (Format-Size $prev[0].Length)) `
            -Remedy 'The package shrank sharply against the last zip of the same configuration. Re-check the cook recipe before shipping it.' | Out-Null
    } else {
        Note-Skipped 'D2-SIZE-SANITY' ("no previous {0} zip to compare against" -f $Configuration)
    }

    # Only now is the predecessor's name released.
    # QA TASK-702 NIT-4: -Force silently overwrites a SAME-DAY zip of the same
    # configuration (the name carries the date, not the time), and the destroyed
    # predecessor is invisible to D3-PRUNE's N=2 accounting.  Nothing unproven
    # ships - the replacement was read back first - but a file that vanished
    # should never do it silently, so it is named here.
    $replacedSameDay = $false
    $replacedBytes   = 0
    if (Test-Path -LiteralPath $ZipPath) {
        $replacedSameDay = $true
        $replacedBytes   = (Get-Item -LiteralPath $ZipPath).Length
    }
    Move-Item -LiteralPath $tmpZip -Destination $ZipPath -Force
    if ($replacedSameDay) {
        Add-Fact 'Replaced same-day zip' ("{0} ({1}) was OVERWRITTEN by this run - same configuration, same date, so it shares the PKG-7b name. It is NOT counted by D3-PRUNE's N=2 retention." -f $ZipName, (Format-Size $replacedBytes))
        Say ("    NOTE: an existing {0} ({1}) was replaced by this run." -f $ZipName, (Format-Size $replacedBytes))
    }
    Add-GateRecord 'D1-ZIP' 'PASS' $ZipPath
    Say ("  [PASS] D1-ZIP  {0}" -f $ZipPath)

    # D3 - PRUNE (PKG-7b).  Same configuration only, keep N=2, AFTER verify.
    # A zip of a DIFFERENT configuration is NEVER pruned without Jonathan's word
    # - the Development zip he has actually been handed is preserved
    # indefinitely.
    $sameCfg = @(Get-ChildItem -LiteralPath $StagingDir -File -ErrorAction SilentlyContinue |
                 Where-Object { $_.Name -match ("^Siegebound-Win64-{0}-\d{{4}}-\d{{2}}-\d{{2}}\.zip$" -f [regex]::Escape($Configuration)) } |
                 Sort-Object Name -Descending)
    $pruned = @()
    if ($sameCfg.Count -gt $RETENTION_KEEP) {
        foreach ($z in ($sameCfg | Select-Object -Skip $RETENTION_KEEP)) {
            Remove-Item -LiteralPath $z.FullName -Force
            $pruned += $z.Name
        }
    }
    Add-GateRecord 'D3-PRUNE' 'PASS' ("kept {0} {1} zip(s); pruned: {2}" -f [math]::Min($sameCfg.Count, $RETENTION_KEEP), $Configuration, $(if ($pruned.Count) { $pruned -join ', ' } else { 'none' }))
    Say ("  [PASS] D3-PRUNE  kept {0}; pruned: {1}" -f [math]::Min($sameCfg.Count, $RETENTION_KEEP), $(if ($pruned.Count) { $pruned -join ', ' } else { 'none' }))
    $others = @(Get-ChildItem -LiteralPath $StagingDir -Filter '*.zip' -File -ErrorAction SilentlyContinue |
                Where-Object { $_.Name -notmatch ("^Siegebound-Win64-{0}-" -f [regex]::Escape($Configuration)) })
    foreach ($o in $others) { Say ("         PROTECTED (other configuration, never pruned): {0}" -f $o.Name) }
}

# ------------------------------------------------------------------------------
# PHASE F - COMMIT (SHIP-5).  Three prohibitions, absolute:
#   1. NEVER PUSH.       Distribution is Jonathan's alone.
#   2. NEVER STAGE THE BUILD OR THE ZIP.  Explicit paths only - there is no
#      directory sweep in this script and there never will be.
#   3. NEVER AMEND OR REWRITE HISTORY.  Append-only.  Jonathan self-commits
#      milestones; the ship REPORTS what it finds and never tidies his commit.
# ------------------------------------------------------------------------------
Head 'PHASE F - COMMIT (explicit paths only)'
$CommitSha = '(none)'
if ($CommitPaths.Count -eq 0) {
    Note-Skipped 'F1-COMMIT' 'no -CommitPaths supplied: nothing is committed by this run'
} else {
    $resolved = @()
    $rejected = @()
    foreach ($cp in $CommitPaths) {
        $abs = $cp
        if (-not [System.IO.Path]::IsPathRooted($abs)) {
            $try1 = Join-Path $script:RepoRoot $cp
            $try2 = Join-Path $ProjectDir $cp
            if (Test-Path -LiteralPath $try1) { $abs = $try1 } elseif (Test-Path -LiteralPath $try2) { $abs = $try2 }
        }
        if (-not (Test-Path -LiteralPath $abs)) { $rejected += ("{0} (does not exist)" -f $cp); continue }
        $abs = (Resolve-Path -LiteralPath $abs).Path
        if ($abs.StartsWith($StagingDir, [System.StringComparison]::OrdinalIgnoreCase)) {
            $rejected += ("{0} (inside the staging dir - the build and the zip are NEVER staged)" -f $cp); continue
        }
        if (-not $abs.StartsWith($script:RepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
            $rejected += ("{0} (outside the work tree)" -f $cp); continue
        }
        $ign = Invoke-Git -GitArgs @('check-ignore','-q','--',$abs)
        if ($ign.Ok) { $rejected += ("{0} (git-ignored - it cannot be committed and must not be forced)" -f $cp); continue }
        $resolved += $abs
    }
    foreach ($rj in $rejected) { Say ("      rejected path: {0}" -f $rj) }
    Assert-Gate -Id 'F1-COMMIT-PATHS' -Ok ($rejected.Count -eq 0) `
        -Evidence ("{0} accepted, {1} rejected" -f $resolved.Count, $rejected.Count) `
        -Remedy 'Fix the -CommitPaths list. This script has no directory sweep and no force.' | Out-Null

    if ($DryRun) {
        Note-Skipped 'F2-COMMIT' 'dry run: not executed'
        foreach ($rp in $resolved) { Say ("      would stage: {0}" -f $rp) }
    } else {
        foreach ($rp in $resolved) {
            $add = Invoke-Git -GitArgs @('add','--',$rp)
            if (-not $add.Ok) {
                Assert-Gate -Id 'F2-COMMIT' -Ok $false -Evidence ("git add failed for {0}" -f $rp) -Remedy 'Inspect the path.' | Out-Null
            }
        }
        # Prove the staging dir appears NEITHER staged NOR untracked before the
        # commit.  The editor auto-stage trap fired live at TASK-683; the fence
        # is a property, not a hope.
        $stagedNames = @()
        $sn = Invoke-Git -GitArgs @('diff','--cached','--name-only')
        if ($sn.Ok -and $sn.Out) { $stagedNames = @($sn.Out) }
        $leak = @($stagedNames | Where-Object { $_ -like '*packagedZIPofGame*' -or $_ -like '*.zip' })
        $porc2 = Invoke-Git -GitArgs @('status','--porcelain')
        $leak2 = @()
        if ($porc2.Ok -and $porc2.Out) { $leak2 = @(@($porc2.Out) | Where-Object { $_ -like '*packagedZIPofGame*' }) }
        Assert-Gate -Id 'F2-INDEX-CLEAN' -Ok (($leak.Count -eq 0) -and ($leak2.Count -eq 0)) `
            -Evidence ("staged build/zip leaks={0}; porcelain staging-dir rows={1}" -f $leak.Count, $leak2.Count) `
            -Remedy 'Unstage the build/zip immediately. The multi-GB package never enters git in any form (PKG-3).' | Out-Null

        $msgFile = Join-Path $RunLogDir 'commit-message.txt'
        $msgLines = @()
        $msgLines += ("ship {0}: {1} ({2}) - {3}" -f $ShipDate, $ZipName, $Configuration, $(if ($script:StopId) { 'INCOMPLETE' } else { 'verified' }))
        $msgLines += ''
        $msgLines += ("Zip:        {0}" -f $ZipPath)
        $msgLines += ("Config:     {0}" -f $Configuration)
        $msgLines += ("HEAD:       {0}" -f $HeadSha)
        $msgLines += ("Suite:      {0} tests (baseline {1})" -f $suiteTotal, $SUITE_BASELINE)
        $msgLines += ("Boot-verify: real arena, instrument = {0}" -f $bootInstrument)
        $msgLines += 'Not pushed. The build and the zip are not in this commit and never are.'
        foreach ($t in $CommitTrailer) { $msgLines += $t }
        ($msgLines -join "`r`n") | Out-File -LiteralPath $msgFile -Encoding utf8

        $ci2 = Invoke-Git -GitArgs @('commit','-F',$msgFile)
        Assert-Gate -Id 'F2-COMMIT' -Ok $ci2.Ok -Evidence 'git commit (explicit paths, no amend, no push)' -Remedy 'Inspect git output.' | Out-Null
        $sha = Invoke-Git -GitArgs @('rev-parse','HEAD')
        if ($sha.Ok) { $CommitSha = ([string]$sha.Out).Trim() }
        Add-Fact 'Commit' $CommitSha
    }
}

# ------------------------------------------------------------------------------
# PHASE G - REPORT (SHIP-6)
# ------------------------------------------------------------------------------
if ($DryRun) {
    if ($script:WouldStop.Count -gt 0) { Write-Summary 'DRYRUN-WOULD-STOP'; exit 3 }
    Write-Summary 'DRYRUN-OK - every pre-flight gate passed; nothing was compiled, cooked, zipped or committed'
    exit 0
}
Add-Fact 'Zip (absolute path)' $ZipPath
Add-Fact 'Boot instrument'     $bootInstrument
Write-Summary 'PASS'
exit 0

} catch {
    if ($_.Exception.Message -eq 'SHIP-STOP') {
        Write-Summary ("STOP at {0} - {1}" -f $script:StopId, $script:StopReason)
        exit 2
    }
    # SHIP-1 amended: the THIRD terminal verdict.  Distinct from PASS and from
    # STOP, and NOT a success - exit 4 means the ship is SUSPENDED awaiting the
    # one judgement this script cannot make.  An unresolved suspension is
    # exactly as unshipped as a STOP.
    if ($_.Exception.Message -eq 'SHIP-ADJUDICATE') {
        Write-Summary ("ADJUDICATE C3 - {0}" -f $script:AdjCapture)
        exit 4
    }
    $script:StopId     = 'UNEXPECTED-ERROR'
    $script:StopReason = $_.Exception.Message
    $script:StopRemedy = 'This is a bug in ship.ps1 or an environment failure. The ship did NOT complete.'
    Say ''
    Say ($_.ScriptStackTrace)
    Write-Summary ("STOP at UNEXPECTED-ERROR - {0}" -f $_.Exception.Message)
    exit 1
}
