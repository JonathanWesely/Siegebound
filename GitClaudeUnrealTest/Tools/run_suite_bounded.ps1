<#
.SYNOPSIS
    THE ONE SANCTIONED EXECUTOR of the Siegebound automation suite and of arbitrary
    editor console commands (TL-6). Bounded, self-instrumenting, and it can say NO.

    *** SANCTIONED IS EXERCISED ONLY IN PART. *** The LAUNCH path is measured and the
    OVERALL bound has killed a real editor; the BOOT and STALL bounds have not. Read
    the EXERCISED IN PART block in .NOTES before you quote this file's authority
    anywhere.

.DESCRIPTION
    This script is the EXECUTABLE FORM of SC-116's pinned invocation recipe. It exists
    because that recipe previously lived only in prose, and prose gets re-typed. Three
    separate defects on ONE engine flag (-ExecCmds) have silently run NOTHING in this
    project, each time producing output indistinguishable from success:

      1. PowerShell splitting the -ExecCmds value on SPACES        (SC-95 cl. 1)
      2. The engine splitting -ExecCmds on COMMA and NOT on ';'    (SC-116 cl. 1)
         (Engine/Source/Runtime/Engine/Private/ParseExecCommands.cpp:29)
      3. 'Quit' not quitting an EDITOR commandlet -> it must be QUIT_EDITOR
         (EditorServer.cpp:5993); the process hangs until someone kills it.

    ------------------------------------------------------------------------------
    THE TWO LANES, AND WHY THEIR SEPARATORS DIFFER. READ THIS BEFORE "FIXING" ONE.
    ------------------------------------------------------------------------------
    SUITE lane   :  -ExecCmds="Automation RunTests <Filter>;Quit"
                    The ';' is CORRECT and MUST NOT be changed to a comma. The whole
                    string is ONE console command whose name is 'Automation'. The
                    Automation handler splits its OWN argument list on ';'. That ';'
                    is therefore never seen by the -ExecCmds parser at all.

    COMMAND lane :  -ExecCmds="CmdA,CmdB,CmdC,QUIT_EDITOR"
                    COMMAS, because here the -ExecCmds parser itself is doing the
                    splitting, and it splits only on ','. QUIT_EDITOR is appended by
                    THIS SCRIPT, never by the caller, because 'Quit' leaves an editor
                    commandlet running forever.

    The rule is NOT "commas". The rule is "ASK WHOSE PARSER SEES THE SEPARATOR".

    ------------------------------------------------------------------------------
    THE TERMINATOR IS NEVER A VERDICT. SC-116 cl. 4(c), as corrected 2026-09-09.
    ------------------------------------------------------------------------------
    A 'Cmd:' echo is DEMANDED for every NON-TERMINATOR command. It is NEVER demanded
    for the terminator (';Quit' / 'QUIT_EDITOR'):
      * in the SUITE lane the Automation handler consumes ';Quit' before the engine
        echoes anything, so the terminator CANNOT echo (MEASURED: the string 'quit'
        appears ZERO times in the real 554-test log Saved/Logs/suite-1167rev2.log);
      * in the COMMAND lane it usually DOES echo, but it is the last line written
        before the process dies -- the single line most exposed to a flush race, a
        hard exit, or this script's own Stop-Process.
    The terminator's evidence is SELF-TERMINATION + WALL CLOCK, both of which this
    script measures. Its echo is reported as CORROBORATION ONLY and can never
    contribute to exit 2.

.NOTES
    NEVER TRUST $LASTEXITCODE. Build.bat returns 0 on a failed build, and the editor
    returns 0 on a run that executed nothing. This script derives its verdict ONLY by
    parsing the log. $LASTEXITCODE is recorded for the record and never consulted for
    the verdict.

    ---------------------------------------------------------------------------
    !!! EXERCISED IN PART  --  WHICH HALF IS MEASURED, AND WHICH HALF IS NOT
    ---------------------------------------------------------------------------
    CORRECTED 2026-09-19 by TASK-1310. What stood here from 2026-09-09 until today was,
    verbatim:

        "As of 2026-09-09 this script has NEVER launched UnrealEditor-Cmd.exe.
         Start-Process, the .cmdline sidecar, the polling loop, boot detection on a
         growing log, Stop-Process and the survivor check have been executed ONLY
         against a synthetic stand-in process (TASK-1181 rev-2, which is how exit 6 was
         proven reachable) -- NOT against the engine. Bound arithmetic is proven against
         synthetic clocks. No real editor has ever been killed by this file.
         FIRST REAL OUTING: TASK-1183."

    That was FALSE ON THE DAY IT WAS WRITTEN, not merely stale: four of the launches
    counted in (a) below are dated 2026-09-09 itself, and TASK-1183 -- named above as the
    outing that had not happened yet -- had already happened. TL-6's matching law bullet
    was struck by the manager on 2026-09-18 (SC-82 reserves that strike to him); this
    block is the code-side half of the same correction.

    (a) LAUNCH -- MEASURED. Start-Process has run against the real UnrealEditor-Cmd.exe,
        and the receipt is the .cmdline sidecar, written one per launch immediately
        BEFORE the launch it records. FIND BOTH SITES BY GREP, NOT BY LINE NUMBER --
        EDITING THIS BLOCK MOVES THEM, which is how the previous revision went stale:
            grep -n "Start-Process -FilePath"   -- the launch
            grep -n "+ '.cmdline'"              -- the receipt
        The two addresses this paragraph used to carry were DELETED, not refreshed
        (TASK-1327): they had been RE-GREPPED against the finished file, never offset
        from a diff -- an earlier revision derived them by arithmetic (+69 where the
        header's growth was +70) and both were wrong by one -- and they went stale
        ANYWAY, displaced by this block's own next edit. A citation that every later
        edit above it moves has no correct value to iterate toward; the two greps are
        the entire remedy (SC-126 cl. 7).
        RE-DERIVE THE NUMBER, DO NOT INHERIT IT (SC-91):

            Get-ChildItem Saved/Logs/*.cmdline | Measure-Object

        Census taken 2026-09-19 for TASK-1310: 22 sidecars -- 21 suite lane + 1 command
        lane -- spanning 20260909-045216 to 20260918-234424.
        Earlier counts are THE SAME EVIDENCE AT EARLIER INSTANTS, not a dispute:
            15 (TASK-1294: the 09-14..09-18 window only, omitting the four 09-09 runs)
          +  4 (those four 09-09 runs)                                          = 19
            19 (TASK-1295 and the manager, globbed 2026-09-18 16:xx)
          +  3 (the 09-18 23:42..23:44 runs, which post-date that glob)          = 22
        This count only ever GROWS. A larger one is not a contradiction; a SMALLER one
        means somebody deleted evidence.

    (b) COMMAND LANE -- MEASURED, exactly once, and the receipt is named so you can open
        it: Saved/Logs/run_suite_bounded_command_20260909-045326.log.cmdline. The other
        21 sidecars are suite lane.

    (c) BOUNDS -- SPLIT, AND THE SPLIT IS THE POINT. TWO CLAIMS, TWO VERDICTS.

        (c1) The OVERALL bound HAS killed a real editor -- MEASURED 2026-09-09 by
             TASK-1183 section 6, which ran this script with -OverallSeconds 20 against a
             suite that needs ~44 s. It printed
                 BOUND TRIPPED: OVERALL bound 20s exceeded (elapsed 21s)
             RUNNER_EXIT = 6, and UnrealEditor-Cmd.exe PID 13816 was confirmed gone three
             independent ways (HasExited, Win32_Process, a fresh name census).
             THE CORPSE IS STILL ON DISK -- re-read it without rerunning anything:
             Saved/Logs/run_suite_bounded_suite_20260909-045537.log opens at 04:55:38,
             its last line is 04:55:58 (20 s), it stops MID-TEST, and it holds 134 started
             / 133 completed -- the exact partial counts TASK-1183 reports. The run beside
             it, ...-045412.log, is a clean 555/555.

        (c2) The BOOT and STALL bounds are NOT MEASURED — NULL WITH NO POSITIVE CONTROL.
             -BootSeconds and -StallSeconds have tripped ONLY against the synthetic
             stand-in of TASK-1181 rev-2; no real editor has ever been killed by either.
             Of the 22 real runs in (a), 21 SELF-terminated (19 suite logs end at
             "LogCore: Engine exit requested (reason: Win RequestExit)";
             ...-20260917-211827.log ends a few lines earlier inside the same orderly
             shutdown; the command log ends at "Log file closed") and the 22nd is (c1)'s
             OVERALL kill. A LAUNCH HAPPENING IS NOT A BOUND FIRING, and one bound firing
             is not three.

    !! RESOLVED 2026-09-19 -- THE LAW AND THIS BLOCK NOW AGREE. NOT A DIVERGENCE.
    An earlier revision of this paragraph reported a live divergence: that TL-6 "still
    reads, unstruck, NO BOUND HAS EVER KILLED A REAL PROCESS". That sentence was TRUE
    when it was written and FALSE a few hours later, because the manager struck both of
    the law's false sentences that same day (SC-82 reserves that strike to him; TASK-1310
    flagged, and did not touch, CONVENTIONS.md). Nothing in this file changed; the file it
    described did.

    ANCHOR TO QUOTED TEXT, NEVER TO A LINE NUMBER OR TO "THE LAW STILL READS Y"
    (SC-126 cl. 7, minted off exactly this event). This block has already cited
    CONVENTIONS.md at :5966-5969 and then at :6172-6175, and BOTH rotted with nothing
    going red -- a clean diff, a green parse and a false sentence. The strings below are
    ASCII substrings that are IN the law today; grep for the substring, not for a whole
    sentence (the law interleaves its own emphasis markers mid-phrase):

        grep -n "FALSE SENTENCE 1" .claude/pipeline/CONVENTIONS.md

        "STRUCK 2026-09-19 BY THE MANAGER" -- the strike, made as FALSE, not SUPERSEDED
        "FALSE SENTENCE 1"  -- "NO BOUND HAS EVER KILLED A REAL PROCESS", false since
                               2026-09-09; the evidence is (c1) above
        "FALSE SENTENCE 2"  -- "every one of those 19 runs terminated normally",
                               corrected to 21 of 22
        "WHAT REPLACES THEM" -- the replacement, split by bound

    What replaces them in the law is the SAME three-way split this block publishes, and
    the law writes the two null verdicts in the words (c2) uses, verbatim:

        -OverallSeconds = MEASURED  (TASK-1183: a real process, a real kill)
        -BootSeconds    = NOT MEASURED — NULL WITH NO POSITIVE CONTROL
        -StallSeconds   = NOT MEASURED — NULL WITH NO POSITIVE CONTROL

    The law cites this header back by name, so the two are now coupled in both
    directions. If a later edit makes them disagree again, report it where you find it --
    do not quietly pick a side, and do not resolve it here: only the manager edits the
    law.

    DO NOT "TIDY" (c2) AWAY WHILE FIXING (a). Deleting the true sentence next to a false
    one is how this block got wrong in the first place: "the file exists" was read as "the
    tool runs", and the correction that caught that then quietly welded "runs" to "kills".
    That law sentence is cited here the way everything else in this block is cited -- by
    SUBSTRING, never by address. It earned that twice over: the address this line used to
    carry was wrong on arrival (it pointed at a different section entirely), and the true
    address then moved AGAIN, the same day, between the gate that measured it and the fix
    that deleted it. A fourth number would have rotted too.

        grep -n "THE FILE EXISTS" .claude/pipeline/CONVENTIONS.md
            -- every hit is that sentence, or a later entry quoting it

    THE PORCELAIN CONTRACT
      RUNNER_EXIT is the PROCESS EXIT CODE. They are the same variable by construction
      (one 'exit' statement, at the bottom of the file, of the value Show-Verdict
      printed). If a bound killed the run, RUNNER_EXIT is 6 and the log's own opinion is
      reported separately as RUNNER_LOG_VERDICT -- a fragment, never a verdict.

    EXIT CODES
      0   OK                    suite green / all commands dispatched
      2   DISPATCH_ECHO_FAILED  the invocation was mangled; nothing (or the wrong
                                thing) ran. This is the SC-116 W9 / SC-95 cl. 1 catch.
      3   ZERO_STARTED          echo was fine but N == 0 (SC-95 cl. 1)
      4   RESULT_ABSENT         tests started but no Result={} lines: unreadable
                                instrument, NOT a green suite
      5   TESTS_FAILED          a genuine red suite (M > 0), or -SelfTest failed
      6   BOUND_EXCEEDED        overall / boot / stall bound tripped (SC-87). The run
                                was FORCE-KILLED; nothing it left behind is a result.
      7   LOG_UNREADABLE        no log, empty log, or log could not be read
      8   COUNT_MISMATCH        Started != Completed != sum of Result={} states
      64  USAGE                 bad arguments, unsafe -LogPath, mangled -Filter
      70  INTERNAL_ERROR        this script threw. A documented code, so a caller never
                                sees PowerShell's undocumented 1.
      (1) NOT ours. PowerShell's own PARAMETER-BINDING failure, which happens BEFORE
          any line of this script runs, so no try/catch inside the file can reach it.
          MEASURED 2026-09-09. If you see 1, you mistyped the command line -- read the
          binding error above it.

    INVOCATION, MEASURED 2026-09-09 -- HOW YOU PASS -Commands MATTERS
      WORKS:     .\Tools\run_suite_bounded.ps1 -Mode Command -Commands A,B,C
      WORKS:     powershell -Command "& .\Tools\run_suite_bounded.ps1 -Mode Command -Commands A,B,C"
      DOES NOT:  powershell -File .\Tools\run_suite_bounded.ps1 -Mode Command -Commands A,B,C
                 -File hands the whole token over as the SINGLE string 'A,B,C'. Both
                 halves of this script now REFUSE that with exit 64 and a message naming
                 the working forms; before rev-2 the -VerifyLog half accepted it and
                 returned exit 2 on a good log.

.EXAMPLE
    # The suite. Semicolon lane. This is the default.
    .\Tools\run_suite_bounded.ps1

.EXAMPLE
    # Arbitrary console commands. Comma lane. QUIT_EDITOR is appended for you.
    .\Tools\run_suite_bounded.ps1 -Mode Command -Commands Siege.Fog.Raise,Siege.Fog.Status

.EXAMPLE
    # Print the exact command line without launching anything.
    .\Tools\run_suite_bounded.ps1 -DryRun

.EXAMPLE
    # Re-judge an existing log. Launches no engine. This is how the guards are tested.
    .\Tools\run_suite_bounded.ps1 -VerifyLog Saved\Logs\suite-1167rev2.log

.EXAMPLE
    # Prove the guards still catch their failures. Launches no engine, no process.
    .\Tools\run_suite_bounded.ps1 -SelfTest
#>

[CmdletBinding(DefaultParameterSetName = 'Run')]
param(
    [ValidateSet('Suite', 'Command')]
    [string] $Mode = 'Suite',

    # SUITE lane: the automation filter. 'Siegebound' is this project's root group.
    [string] $Filter = 'Siegebound',

    # COMMAND lane: the console commands to dispatch. Do NOT include Quit/QUIT_EDITOR;
    # this script appends QUIT_EDITOR itself (see SC-116 cl. 1, second defect).
    [string[]] $Commands = @(),

    # Bounds. TL-6's proven numbers. Do not re-derive them casually.
    [int] $OverallSeconds = 1500,
    [int] $BootSeconds    = 420,
    [int] $StallSeconds   = 180,

    [string] $ProjectPath = '',
    [string] $EditorCmd   = '',
    [string] $LogPath     = '',

    # Print the resolved command line and exit. Launches nothing.
    [switch] $DryRun,

    # Judge an existing log instead of producing one. Launches nothing.
    [string] $VerifyLog = '',

    # Run the fixture corpus through the guards. Launches nothing, no process at all.
    [switch] $SelfTest,

    # Emit machine-readable summary lines (KEY=VALUE) for a calling agent to parse.
    [switch] $Porcelain
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# ---------------------------------------------------------------------------
# Exit codes
# ---------------------------------------------------------------------------
$EXIT_OK                   = 0
$EXIT_DISPATCH_ECHO_FAILED = 2
$EXIT_ZERO_STARTED         = 3
$EXIT_RESULT_ABSENT        = 4
$EXIT_TESTS_FAILED         = 5
$EXIT_BOUND_EXCEEDED       = 6
$EXIT_LOG_UNREADABLE       = 7
$EXIT_COUNT_MISMATCH       = 8
$EXIT_USAGE                = 64
$EXIT_INTERNAL_ERROR       = 70

# The PROJECT root (the folder holding the .uproject). NOT the git root -- that is one
# level further up, and SC-102 records that a mis-anchored pathspec answers with SILENCE.
# Never hand this to a git pathspec.
$script:ProjectRoot = Split-Path -Parent $PSScriptRoot

# The terminator, in one place. 'Quit' does NOT quit an editor commandlet
# (EditorServer.cpp:5993). Changing this literal is caught by -SelfTest.
$script:Terminator = 'QUIT_EDITOR'

function Write-Head([string] $Text) {
    Write-Host ''
    Write-Host "=== $Text ===" -ForegroundColor Cyan
}

function Write-Verdict([string] $Label, [string] $Text, [string] $Colour) {
    Write-Host ("{0,-24}{1}" -f $Label, $Text) -ForegroundColor $Colour
}

# ===========================================================================
# LANE CONSTRUCTION  --  the whole point of the tracked tool.
# ===========================================================================

<#
    INPUT VALIDATION, IN ONE PLACE, USED BY BOTH THE EMITTING AND THE JUDGING HALF.

    MEASURED 2026-09-09 (TASK-1181 rev-2, by running it): when this validation lived
    ONLY in New-ExecCmdsValue, the -VerifyLog path -- which never calls that function --
    accepted a caller-supplied separator and produced 'dispatch echo 0 / 1' on a
    PERFECTLY GOOD command log. A FALSE RED, reachable from the most natural CLI form.
    Asymmetric sanitisation is how one half of a tool contradicts the other, so there is
    now exactly one implementation and both halves call it.
#>
function Assert-SafeFilter {
    param([Parameter(Mandatory)][AllowEmptyString()][string] $ForFilter)
    # The filter is interpolated into a quoted -ExecCmds value AND into a verbatim
    # command line. A ',' would make -ExecCmds split; a ';' would make the Automation
    # handler split; a '"' would escape the quoting entirely. Same guard the Command
    # lane gets -- a parameter is not safe merely because nobody has abused it yet.
    if ([string]::IsNullOrWhiteSpace($ForFilter)) {
        throw 'SUITE mode requires a non-empty -Filter.'
    }
    if ($ForFilter -match '[,;"]') {
        throw ("Filter '{0}' contains a separator or a quote. The filter is interpolated into a quoted -ExecCmds value; ',' ';' and a double quote would all mangle the invocation. (SC-116)" -f $ForFilter)
    }
}

function Get-CleanCommandList {
    param([Parameter(Mandatory)][AllowEmptyCollection()][AllowEmptyString()][string[]] $ForCommands)

    $clean = @()
    foreach ($c in $ForCommands) {
        if ($null -eq $c) { continue }
        $t = $c.Trim()
        if ($t.Length -eq 0) { continue }
        if ($t -match '[,;]') {
            # PARENTHESISE THE WHOLE CONCATENATION. -f binds TIGHTER than '+', so
            # ("a{0}" + "b" -f $x) formats only the LAST fragment and prints a literal
            # '{0}' to the reader. That is defect 5b, and it RECURRED here on
            # 2026-09-09 because only verdict messages were placeholder-checked.
            # -SelfTest now placeholder-checks THROW messages too.
            throw ((
                   "Command '{0}' contains a separator. ONE command per array element; " +
                   "this script does the joining (SC-116).`n" +
                   "        WORKS:      .\Tools\run_suite_bounded.ps1 -Mode Command -Commands A,B,C`n" +
                   "        WORKS:      powershell -Command `"& .\Tools\run_suite_bounded.ps1 -Mode Command -Commands A,B,C`"`n" +
                   "        DOES NOT:   powershell -File .\Tools\run_suite_bounded.ps1 -Mode Command -Commands A,B,C`n" +
                   "                    MEASURED 2026-09-09: -File delivers that as the SINGLE string 'A,B,C'."
                   ) -f $t)
        }
        # QUIT_EDITOR is ours to append; drop a caller-supplied terminator of either spelling.
        if ($t -match '^(?i)(quit|exit|quit_editor)$') { continue }
        $clean += $t
    }
    # PowerShell UNROLLS a one-element array on return, so EVERY caller wraps this in
    # @(). MEASURED 2026-09-09: without that, $clean.Count threw under StrictMode on a
    # list that had exactly one command left after the terminator was dropped -- and the
    # 'fix' of returning ,$clean instead produced '-ExecCmds="System.Object[],QUIT_EDITOR"'.
    # The wrapping belongs at the CALL SITE, where the shape is actually consumed.
    return $clean
}

<#
    Builds the VALUE that goes inside -ExecCmds="...".

    SUITE   -> 'Automation RunTests <Filter>;Quit'   (semicolon: the Automation
                handler's own parser splits it; -ExecCmds never sees it)
    COMMAND -> 'A,B,C,QUIT_EDITOR'                   (comma: the -ExecCmds parser
                itself splits, and it splits ONLY on comma)

    Throws on any caller input that would smuggle a separator into either lane. The
    caller of this function maps that throw to exit 64.
#>
function New-ExecCmdsValue {
    param(
        [Parameter(Mandatory)][ValidateSet('Suite', 'Command')][string] $ForMode,
        [AllowEmptyString()][string] $ForFilter = 'Siegebound',
        [AllowEmptyCollection()][AllowEmptyString()][string[]] $ForCommands = @()
    )

    if ($ForMode -eq 'Suite') {
        Assert-SafeFilter -ForFilter $ForFilter
        # DO NOT "FIX" THIS SEMICOLON. See the header block.
        return ('Automation RunTests {0};Quit' -f $ForFilter)
    }

    $clean = @(Get-CleanCommandList -ForCommands $ForCommands)
    if ($clean.Count -eq 0) {
        throw 'COMMAND mode requires at least one command via -Commands.'
    }
    # COMMAS, and the terminator appended by the SCRIPT, never by the caller.
    $clean += $script:Terminator
    return ($clean -join ',')
}

<#
    The list of console commands whose 'Cmd: <text>' echo is REQUIRED in the log.
    This is what the dispatch-echo guard compares against.

    It is deliberately NOT "count the Cmd: lines": the engine emits its own (MAP LOAD,
    MAP CHECKDEP, OBJ SAVEPACKAGE, HighResShot ...). MEASURED: the real green suite log
    Saved/Logs/suite-1167rev2.log carries THREE 'Cmd:' lines for ONE command of ours.

    !! IT ALSO DELIBERATELY EXCLUDES THE TERMINATOR. SC-116 cl. 4(c) as corrected
    2026-09-09: a per-command echo may NEVER be demanded for the terminator. Adding
    'QUIT_EDITOR' to this list would red a good run whose last line was lost to a flush
    race or to our own Stop-Process. -SelfTest asserts this list's exact contents; a
    re-added terminator is a self-test failure, not a silent tightening.
#>
function Get-ExpectedEchoes {
    param(
        [Parameter(Mandatory)][ValidateSet('Suite', 'Command')][string] $ForMode,
        [AllowEmptyString()][string] $ForFilter = 'Siegebound',
        [AllowEmptyCollection()][AllowEmptyString()][string[]] $ForCommands = @()
    )

    if ($ForMode -eq 'Suite') {
        # SAME validation the emitting half applies. Judging a log against an input the
        # run path would have REFUSED is how a tool produces a false red.
        Assert-SafeFilter -ForFilter $ForFilter
        return @(('Automation RunTests {0}' -f $ForFilter))
    }

    return (Get-CleanCommandList -ForCommands $ForCommands)
}

function New-EditorCommandLine {
    param(
        [Parameter(Mandatory)][string] $Uproject,
        [Parameter(Mandatory)][string] $ExecValue,
        [Parameter(Mandatory)][string] $AbsLog
    )

    # SC-95 cl. 1 / SC-116: -ExecCmds MUST reach the engine as ONE argument with its
    # quotes intact. We therefore build the entire command line as a SINGLE verbatim
    # string and hand that one string to Start-Process. An ARRAY -ArgumentList lets
    # PowerShell re-quote elements, which is the exact mechanism that produced
    # 'Cmd: Automation' and a zero-started, zero-failed, perfectly green-looking run.
    # -SelfTest asserts BOTH that the return is [string] (not [string[]]) and that the
    # -ExecCmds token survives as one quoted unit.
    #
    # ---------------------------------------------------------------------------
    # -DisablePlugins=Aura  --  TASK-1294. THIS IS THE FIX, NOT A FALLBACK.
    # ---------------------------------------------------------------------------
    # THE DEFECT. The Aura editor plugin starts its own client on editor boot and
    # calls its indexing endpoint ~19-24 s later. Under -nullrhi there is no signed-in
    # user, so the call comes back
    #     LogAura: Error: Response code: 401
    #     LogAura: Error: Response content: {"message":"User not authenticated",...}
    #     LogAura: Warning: Indexing failed: Authentication required
    # and the automation controller SOMETIMES attributes that Error line to whichever
    # test's capture window happens to be open ~31 ms earlier. Measured landing in a
    # DIFFERENT test on run after run while the tests themselves did not change: that
    # is the signature of a TIME-BASED EXTERNAL fault, not a code fault.
    #
    # !! AND IT IS NASTIER THAN "IT REDS WHATEVER IS OPEN" (measured 2026-09-18,
    # !! TASK-1306's 5a): the 401 landed squarely inside Deck.UncapFiftyOfOneCardLegal
    # !! and that test STILL returned Success. PRESENCE and ABILITY-TO-RED are
    # !! SEPARABLE. A run therefore has THREE states, not two:
    # !!   (i) no 401 · (ii) 401 fired, no victim · (iii) 401 fired, a test reds.
    # !! => a bare `grep -c 'Response code: 401'` CANNOT tell (i)-because-fixed from
    # !! (i)-because-quiet-this-run, and it has returned 0 with NO fix applied (runs
    # !! 20260917-212139 / -212229 / 20260918-161724). THE STRUCTURAL PROOF THAT THIS
    # !! FLAG WORKED IS `grep -c LogAura <log>` == 0 -- TOTAL, not merely the 401 --
    # !! PLUS the absence of `LogPluginManager: Mounting Engine plugin Aura`. Those
    # !! same greps return 18-31 on every pre-change suite log, so they can fire.
    #
    # WHY THIS LEVER AND NOT ANOTHER (measured in UE 5.8's own sources, not memory):
    #   * FPluginManager::ConfigureEnabledPlugins calls FindCommandLinePlugins FIRST
    #     (PluginManager.cpp:2043), BEFORE FindTargetPlugins (:2049).
    #   * FindCommandLinePlugins parses -DisablePlugins= (:1587), configures the name
    #     as a DISABLED reference (:1592) and records it in ConfiguredPluginNames
    #     (:1597). A disabled reference short-circuits at
    #     FPluginReferenceDescriptor::IsEnabledForPlatform (PluginReferenceDescriptor
    #     .cpp:41-47, "if(!bEnabled) return false"), so it never enters EnabledPlugins.
    #   * EVERY later source is gated on !ConfiguredPluginNames.Contains(name): the
    #     target receipt (:1636), the .uproject "Plugins" array (:1709, source
    #     "Enabled plugins in .uproject for ...") and .uplugin EnabledByDefault (:1748).
    #     => the command line OVERRIDES GitClaudeUnrealTest.uproject's Aura entry
    #     (:55-56) for THIS PROCESS ONLY.
    #   * That is the whole reason this lever was chosen: GitClaudeUnrealTest.uproject
    #     is Jonathan's file (ruling R11) and no agent edits it, and editing it -- or
    #     the engine-side Aura.uplugin -- would ALSO kill Aura for the GUI editor,
    #     which VER-7's verifier lane and Aura's own MCP tooling depend on. This flag
    #     is typed only here, so the GUI editor is untouched by construction.
    #
    # SCOPE: unconditional, BOTH lanes. The Command lane boots the same -nullrhi
    # editor and inherits the same race, and nothing either lane runs needs Aura --
    # the automation corpus references it exactly ONCE, in a COMMENT
    # (SiegeMenuInputTest.cpp:35), across 47 files and 561 tests.
    #
    # DO NOT "simplify" this to an expected-error / log-suppression pin for the 401
    # string. That fixes exactly one string and leaves the next LogAura Error line
    # free to red a random test; it is the declared FALLBACK for this row and it was
    # NOT taken, because this lever was reachable.
    $parts = @(
        ('"{0}"' -f $Uproject),
        ('-ExecCmds="{0}"' -f $ExecValue),
        '-nullrhi',
        '-unattended',
        '-nopause',
        '-nosplash',
        '-NoLiveCoding',
        '-DisablePlugins=Aura',
        '-log',
        ('-abslog="{0}"' -f $AbsLog)
    )
    return [string] ($parts -join ' ')
}

<#
    W-3. The ONE place this script can damage the repo is the Remove-Item that clears a
    stale log before launch. -LogPath is caller-supplied; confine it to the project's
    own Saved\ tree and to a .log extension, and reject everything else with exit 64.
    Pure string math -- no disk access -- so -SelfTest covers it.
#>
function Resolve-SafeLogPath {
    param(
        [Parameter(Mandatory)][AllowEmptyString()][string] $Candidate,
        [Parameter(Mandatory)][string] $ProjectRootPath
    )

    $r = @{ Ok = $false; Path = ''; Reason = '' }

    if ([string]::IsNullOrWhiteSpace($Candidate)) {
        $r.Reason = '-LogPath is empty.'
        return $r
    }

    $full = $Candidate
    if (-not [System.IO.Path]::IsPathRooted($full)) {
        $full = Join-Path $ProjectRootPath $full
    }
    try {
        $full = [System.IO.Path]::GetFullPath($full)
    } catch {
        $r.Reason = ("-LogPath '{0}' is not a resolvable path." -f $Candidate)
        return $r
    }

    $savedRoot = [System.IO.Path]::GetFullPath((Join-Path $ProjectRootPath 'Saved'))
    if (-not $savedRoot.EndsWith('\')) { $savedRoot = $savedRoot + '\' }

    if (-not $full.StartsWith($savedRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        $r.Reason = (("-LogPath '{0}' resolves to '{1}', which is OUTSIDE '{2}'. This script " +
                      "DELETES the log path before launching; it will not delete anything " +
                      "outside the project's own Saved\ tree.") -f $Candidate, $full, $savedRoot)
        return $r
    }
    if (-not $full.EndsWith('.log', [System.StringComparison]::OrdinalIgnoreCase)) {
        $r.Reason = (("-LogPath '{0}' does not end in '.log'. This script DELETES the log path " +
                      "before launching; it will not delete a non-log file.") -f $Candidate)
        return $r
    }

    $r.Ok = $true
    $r.Path = $full
    return $r
}

# ===========================================================================
# GUARDS  --  every one of these exists to return NO.
# ===========================================================================

function Read-LogLines {
    param([Parameter(Mandatory)][string] $Path)
    if (-not (Test-Path -LiteralPath $Path)) { return $null }
    try {
        $lines = Get-Content -LiteralPath $Path -ErrorAction Stop
    } catch {
        return $null
    }
    if ($null -eq $lines) { return @() }
    return @($lines)
}

<#
    W-10. The boot peek runs every 2 seconds for up to BootSeconds. Re-reading a whole
    growing log each time costs more the longer the exact failure it exists to catch
    lasts. Read only the bytes added since the last poll, and never consume a partial
    trailing line (a 'Cmd:' echo split across two polls would otherwise never match).

    Returns @{ Lines; Offset }. Offset is the byte position of the start of the first
    INCOMPLETE line, i.e. what to pass in next time.
#>
function Read-LogSince {
    param(
        [Parameter(Mandatory)][string] $Path,
        [Parameter(Mandatory)][long] $Offset
    )

    $r = @{ Lines = @(); Offset = $Offset }
    if (-not (Test-Path -LiteralPath $Path)) { return $r }

    $text = ''
    $start = $Offset
    try {
        $fs = [System.IO.File]::Open($Path, [System.IO.FileMode]::Open,
                                     [System.IO.FileAccess]::Read,
                                     [System.IO.FileShare]::ReadWrite)
        try {
            # The file was truncated or replaced under us: start again from the top.
            if ($fs.Length -lt $start) { $start = 0 }
            $available = $fs.Length - $start
            if ($available -le 0) {
                $r.Offset = $start
                return $r
            }
            if ($available -gt 4194304) { $available = 4194304 }
            [void] $fs.Seek($start, [System.IO.SeekOrigin]::Begin)
            $buf = New-Object byte[] ([int] $available)
            $read = $fs.Read($buf, 0, [int] $available)
            if ($read -gt 0) {
                $text = [System.Text.Encoding]::UTF8.GetString($buf, 0, $read)
            }
        } finally {
            $fs.Dispose()
        }
    } catch {
        return $r
    }

    if ($text.Length -eq 0) {
        $r.Offset = $start
        return $r
    }

    $lastNl = $text.LastIndexOf("`n")
    if ($lastNl -lt 0) {
        # No complete line yet. Consume nothing; try again next poll.
        $r.Offset = $start
        return $r
    }
    $consumed = $text.Substring(0, $lastNl + 1)
    $r.Offset = $start + [System.Text.Encoding]::UTF8.GetByteCount($consumed)
    $r.Lines  = @($consumed -split "`r`n|`n|`r")
    return $r
}

<#
    Extract the text of every 'Cmd: ' echo. A log line looks like
        [2026.09.09-05.48.32:329][  0]Cmd: Automation RunTests Siegebound
    so we take everything after the first 'Cmd: '.
#>
function Test-ContainsLiteral {
    param([AllowEmptyString()][string] $Haystack, [AllowEmptyString()][string] $Needle)
    if ([string]::IsNullOrEmpty($Needle)) { return $false }
    if ([string]::IsNullOrEmpty($Haystack)) { return $false }
    return ($Haystack.IndexOf($Needle, [System.StringComparison]::OrdinalIgnoreCase) -ge 0)
}

# [AllowEmptyString()] IS LOAD-BEARING, NOT DECORATION. Get-Content on a real log yields
# BLANK LINES, and a Mandatory [string[]] rejects an empty-string ELEMENT: without this,
# 8 synthetic fixtures pass while the script cannot read a single real log. The fixture
# corpus now contains blank lines specifically so deleting this attribute REDS -SelfTest.
function Get-CmdEchoes {
    param([Parameter(Mandatory)][AllowEmptyCollection()][AllowEmptyString()][string[]] $Lines)
    $out = @()
    foreach ($line in $Lines) {
        if ($line -match 'Cmd:\s*(.+?)\s*$') {
            $out += $Matches[1]
        }
    }
    return $out
}

<#
    GUARD (b) -- THE DISPATCH ECHO. SC-116 cl. 4(c), as corrected 2026-09-09.

    This runs BEFORE a single result is read, because it answers a question no result
    count can: "did the thing I asked for actually get dispatched?" Three fixtures in
    this repo (w9-cmd-semicolon, zero-started, zero-started-filtered) have IDENTICAL,
    all-zero result counts and THREE different root causes. Only this guard separates
    the first two from the third.

    $Expected carries NON-TERMINATOR commands only. The terminator is reported in
    TerminatorEcho as corroboration and can never red the run.

    Returns a hashtable: Ok, Reason, Matched, Expected, Echoes, TerminatorEcho.
#>
function Test-DispatchEcho {
    param(
        [Parameter(Mandatory)][AllowEmptyCollection()][AllowEmptyString()][string[]] $Echoes,
        [Parameter(Mandatory)][AllowEmptyCollection()][AllowEmptyString()][string[]] $Expected,
        [Parameter(Mandatory)][ValidateSet('Suite', 'Command')][string] $ForMode
    )

    $result = @{
        Ok             = $false
        Reason         = ''
        Matched        = 0
        Expected       = $Expected.Count
        Echoes         = $Echoes
        TerminatorEcho = 'n/a'
    }

    # --- Terminator corroboration. NEVER a verdict. --------------------------
    if ($ForMode -eq 'Suite') {
        $result.TerminatorEcho = "n/a (suite lane: ';Quit' is consumed by the Automation handler and never echoes)"
    } else {
        $seen = $false
        foreach ($e in $Echoes) {
            if ($e.Trim() -ieq $script:Terminator) { $seen = $true; break }
        }
        if ($seen) {
            $result.TerminatorEcho = ("{0} seen (corroboration only, not a verdict)" -f $script:Terminator)
        } else {
            $result.TerminatorEcho = ("{0} NOT SEEN (corroboration only, NOT a verdict -- the terminator's evidence is self-termination + wall clock, SC-116 cl. 4b)" -f $script:Terminator)
        }
    }

    if ($Expected.Count -eq 0) {
        $result.Reason = ('no non-terminator command was expected in the {0} lane, so there is nothing this guard could confirm was dispatched. Pass at least one real command.' -f $ForMode)
        return $result
    }

    # --- The W9 signature: ONE echo carrying ALL our separators. -------------
    # In the working SUITE lane the engine echoes 'Automation RunTests Siegebound'
    # with the ';Quit' already consumed by the Automation handler (MEASURED on
    # Saved/Logs/suite-1167rev2.log; the string 'quit' appears nowhere in that log).
    # So an echo that still carries a ';' joining two commands is the defect.
    foreach ($e in $Echoes) {
        if ($ForMode -eq 'Suite') {
            # allow the benign 'Automation RunTests X;Quit' shape in case an engine
            # build echoes before the Automation handler strips its own tail.
            if ($e -match '^\s*Automation\s+RunTests\s+') { continue }
        }
        if ($e -match ';') {
            $hits = 0
            foreach ($x in $Expected) {
                if (Test-ContainsLiteral -Haystack $e -Needle $x) { $hits++ }
            }
            if ($hits -ge 2 -or $e -match ';\s*(?i)quit\s*$') {
                $result.Reason = (("SC-116 W9: a single 'Cmd:' echo carries the separators -- " +
                                  "'{0}'. -ExecCmds splits on COMMA, never ';' " +
                                  "(ParseExecCommands.cpp:29). Nothing was dispatched.") -f $e)
                return $result
            }
        }
    }

    # --- The SC-95 cl. 1 signature: PowerShell split the value on spaces. ----
    if ($ForMode -eq 'Suite') {
        foreach ($e in $Echoes) {
            if ($e -match '^\s*Automation\s*$') {
                $result.Reason = ("SC-95 cl. 1: the echo is a bare 'Cmd: Automation' -- the " +
                                  "-ExecCmds value was split on its SPACES before it reached the " +
                                  "engine. Zero tests were asked to run; a 0-fail count here is " +
                                  "meaningless.")
                return $result
            }
        }
    }

    # --- Positive control: one echo per expected NON-TERMINATOR command. -----
    $matched = 0
    $missing = @()
    foreach ($x in $Expected) {
        $found = $false
        foreach ($e in $Echoes) {
            if ($ForMode -eq 'Suite') {
                if ($e -match ('^\s*Automation\s+RunTests\s+{0}\s*(;.*)?$' -f [regex]::Escape(($x -replace '^Automation\s+RunTests\s+', '')))) {
                    $found = $true; break
                }
            } else {
                if ($e.Trim() -ieq $x.Trim()) { $found = $true; break }
            }
        }
        if ($found) { $matched++ } else { $missing += $x }
    }

    $result.Matched = $matched
    if ($matched -ne $Expected.Count) {
        $result.Reason = (("dispatch echo {0} / {1} -- never echoed: {2}. Grep the log for 'Cmd: ' " +
                          "and read it yourself; a command that was never dispatched cannot have " +
                          "produced the result you are about to believe.") -f
                          $matched, $Expected.Count, ($missing -join ', '))
        return $result
    }

    $result.Ok = $true
    # SC-116 cl. 4(c) as corrected: matched by CONTENT. The engine emits its own 'Cmd:'
    # lines, so "one line per command" was never true of a real log.
    $result.Reason = ('dispatch echo {0} / {1} -- matched by CONTENT; extra engine Cmd: lines ignored, terminator exempt' -f $matched, $Expected.Count)
    return $result
}

<#
    GUARD (c) -- N IS READ FIRST AND MUST BE NON-ZERO. SC-95 cl. 1.

    W-2: Result states are tallied GENERICALLY. UE's EAutomationState is not just
    {Success} and {Fail}: {Skipped} and {NotRun} exist and DO appear. Counting only two
    of them made one skipped test read as 'the run is incomplete or the log is
    truncated' -- a WRONG CAUSE for a healthy run, on a 554-test suite, as a matter of
    time. Completeness is now Started == Completed == (all Result={} lines).

    Returns a hashtable: Code, Reason, Started, Completed, Success, Fail, Skipped,
    NotRun, Resulted, OtherStates, Discovered.
#>
function Test-SuiteCounts {
    param([Parameter(Mandatory)][AllowEmptyCollection()][AllowEmptyString()][string[]] $Lines)

    $started    = 0
    $completed  = 0
    $success    = 0
    $fail       = 0
    $skipped    = 0
    $notrun     = 0
    $resulted   = 0
    $other      = @()
    $discovered = -1

    foreach ($line in $Lines) {
        if ($line -match 'Test Started\.')   { $started++ }
        if ($line -match 'Test Completed\.') { $completed++ }
        if ($line -match 'Result=\{(\w+)\}') {
            $state = $Matches[1]
            $resulted++
            if     ($state -ieq 'Success') { $success++ }
            elseif ($state -ieq 'Fail')    { $fail++ }
            elseif ($state -ieq 'Failed')  { $fail++ }
            elseif ($state -ieq 'Skipped') { $skipped++ }
            elseif ($state -ieq 'NotRun')  { $notrun++ }
            else                           { $other += $state }
        }
        if ($line -match "Found\s+(\d+)\s+automation tests based on") {
            $discovered = [int] $Matches[1]
        }
    }

    $r = @{
        Started = $started; Completed = $completed
        Success = $success; Fail = $fail
        Skipped = $skipped; NotRun = $notrun
        Resulted = $resulted; OtherStates = $other
        Discovered = $discovered
        Code = $EXIT_OK; Reason = ''
    }

    # N FIRST. Not M. A 0 fail count is identical on a perfect run and on a run that
    # never happened, and those are the two cases a gate most needs to tell apart.
    if ($started -eq 0) {
        $r.Code = $EXIT_ZERO_STARTED
        $r.Reason = ("SC-95 cl. 1: N = 0. ZERO tests started. This is NOT a green suite -- " +
                     "it is a suite that was never asked to run" +
                     $(if ($discovered -eq 0) { " (the filter matched 0 tests)" } else { "" }) + ".")
        return $r
    }

    if ($completed -eq 0 -or $resulted -eq 0) {
        $r.Code = $EXIT_RESULT_ABSENT
        $r.Reason = (("{0} tests started but NO 'Result={{}}' line was readable. An absent Result " +
                     "line is an UNREADABLE INSTRUMENT, not a green suite (log truncated, run " +
                     "killed, or the controller never reported).") -f $started)
        return $r
    }

    if ($started -ne $completed -or $completed -ne $resulted) {
        $r.Code = $EXIT_COUNT_MISMATCH
        $r.Reason = (("count mismatch: Started={0} Completed={1} Result-lines={2}. These must be " +
                     "equal; they are not, so the run is incomplete or the log is truncated.") -f
                     $started, $completed, $resulted)
        return $r
    }

    $extra = ''
    if ($skipped -gt 0 -or $notrun -gt 0 -or $other.Count -gt 0) {
        $extra = (' [{0} skipped, {1} not-run{2}]' -f $skipped, $notrun,
                  $(if ($other.Count -gt 0) { (', other states: ' + (($other | Select-Object -Unique) -join '/')) } else { '' }))
    }

    if ($fail -gt 0) {
        $r.Code = $EXIT_TESTS_FAILED
        $r.Reason = ('{0} / {1} -- the suite is RED.{2}' -f $success, $fail, $extra)
        return $r
    }

    $r.Reason = ('{0} / {1} -- green.{2}' -f $success, $fail, $extra)
    return $r
}

<#
    The full verdict over a log. Used identically by a real run and by -VerifyLog /
    -SelfTest, so the fixtures exercise the SAME code path a live run does.

    EVERY key this hashtable will ever be read through is initialised here. Under
    Set-StrictMode -Version Latest, reading an absent hashtable key THROWS.
#>
function Get-LogVerdict {
    param(
        [Parameter(Mandatory)][string] $Path,
        [Parameter(Mandatory)][ValidateSet('Suite', 'Command')][string] $ForMode,
        [AllowEmptyString()][string] $ForFilter = 'Siegebound',
        [AllowEmptyCollection()][AllowEmptyString()][string[]] $ForCommands = @(),
        [switch] $Quiet
    )

    $verdict = @{
        Code = $EXIT_OK; Reason = ''; Echo = $null; Counts = $null; LogPath = $Path
        Mode = $ForMode
        BoundMessage = ''; LogVerdictCode = -1; LogVerdictReason = ''
    }

    $lines = Read-LogLines -Path $Path
    if ($null -eq $lines -or $lines.Count -eq 0) {
        $verdict.Code = $EXIT_LOG_UNREADABLE
        $verdict.Reason = (("no readable log at '{0}'. SC-114: a null from an instrument that " +
                           "never spoke is NOT a result.") -f $Path)
        return $verdict
    }

    $expected = @(Get-ExpectedEchoes -ForMode $ForMode -ForFilter $ForFilter -ForCommands $ForCommands)
    $echoes   = Get-CmdEchoes -Lines $lines

    # ORDER IS DELIBERATE: the dispatch echo is judged BEFORE any result is read
    # (SC-116 cl. 4c). It names the CAUSE (the invocation was mangled) rather than
    # the SYMPTOM (no tests ran), and it is the only guard that can tell a mangled
    # invocation apart from an empty filter.
    $echo = Test-DispatchEcho -Echoes $echoes -Expected $expected -ForMode $ForMode
    $verdict.Echo = $echo
    if (-not $echo.Ok) {
        $verdict.Code = $EXIT_DISPATCH_ECHO_FAILED
        $verdict.Reason = $echo.Reason
        return $verdict
    }

    if ($ForMode -eq 'Command') {
        # W-9: for a command list the dispatch echo IS the result, and 'dispatched' is
        # NOT 'succeeded'. SC-104: assert STATE, not tallies -- if you need to know what
        # a command DID, dispatch a status command and read the state back out of the log.
        $verdict.Reason = ($echo.Reason + ' -- COMMAND LANE: this means DISPATCHED, not SUCCEEDED. Nothing here asserts what the commands did.')
        return $verdict
    }

    $counts = Test-SuiteCounts -Lines $lines
    $verdict.Counts = $counts
    $verdict.Code   = $counts.Code
    $verdict.Reason = $counts.Reason
    return $verdict
}

<#
    B-2. A run that was FORCE-KILLED is not described by the log it left behind.

    Before this existed the bound path printed the LOG's verdict -- 'OK 554 / 0 green,
    exit 0, RUNNER_EXIT=0' -- and THEN exited 6. A human skimming read green; an agent
    parsing the documented porcelain read RUNNER_EXIT=0. The instrument built to be
    unable to lie lied in exactly the case it exists to detect.

    The exit code and the porcelain line are now the SAME VARIABLE by construction:
    this function moves the bound into Verdict.Code, the log's opinion is demoted to
    LogVerdictCode, and the file has exactly ONE 'exit' statement.

    Pure. -SelfTest drives it with no process.
#>
function Merge-BoundIntoVerdict {
    param(
        [Parameter(Mandatory)][hashtable] $Verdict,
        [Parameter(Mandatory)][AllowEmptyString()][string] $BoundMessage
    )

    if ($BoundMessage -eq '') { return $Verdict }

    $Verdict.LogVerdictCode   = $Verdict.Code
    $Verdict.LogVerdictReason = $Verdict.Reason
    $Verdict.BoundMessage     = $BoundMessage
    $Verdict.Code             = $EXIT_BOUND_EXCEEDED
    $Verdict.Reason = (("BOUND EXCEEDED -- {0}. The process was FORCE-KILLED, so nothing it left " +
                        "behind is a result (SC-114). For information only, the partial log would " +
                        "have judged as exit {1}: {2}") -f
                        $BoundMessage, $Verdict.LogVerdictCode, $Verdict.LogVerdictReason)
    return $Verdict
}

# ===========================================================================
# BOUNDS  --  SC-87: a hang lies by OMISSION, so it must announce itself.
# Factored as a PURE function so its arithmetic is testable with no process.
# ===========================================================================
function Test-BoundExceeded {
    param(
        [Parameter(Mandatory)][double] $ElapsedSeconds,
        [Parameter(Mandatory)][double] $SecondsSinceLogGrowth,
        [Parameter(Mandatory)][bool]   $BootObserved,
        [Parameter(Mandatory)][int]    $Overall,
        [Parameter(Mandatory)][int]    $Boot,
        [Parameter(Mandatory)][int]    $Stall
    )
    if ($ElapsedSeconds -ge $Overall) {
        return ('OVERALL bound {0}s exceeded (elapsed {1:N0}s)' -f $Overall, $ElapsedSeconds)
    }
    if ((-not $BootObserved) -and $ElapsedSeconds -ge $Boot) {
        return ('BOOT bound {0}s exceeded: the command was never echoed (elapsed {1:N0}s)' -f $Boot, $ElapsedSeconds)
    }
    if ($BootObserved -and $SecondsSinceLogGrowth -ge $Stall) {
        return ('STALL bound {0}s exceeded: the log has not grown for {1:N0}s' -f $Stall, $SecondsSinceLogGrowth)
    }
    return ''
}

# ===========================================================================
# SELF TEST  --  SHIP-9: validate the guard against the FAILURE it detects.
# Launches NO engine and NO process. Pure file parsing plus pure arithmetic.
#
# SC-116 cl. 6: A POSITIVE CONTROL IS ITSELF AN INSTRUMENT. The first version of this
# self-test could not fail on ANY of the three things the tool exists for -- the suite
# ';', the terminator, and the single-string -ArgumentList -- because it only ever
# called the JUDGING half. Sections 3-8 below exist so that mutating the EMITTING half,
# the LAUNCHING half or the INPUT tolerance REDS. Every mutation named in
# qa/TASK-1182-report.md 8(b) rows 1-6 is covered here; rows 7-8 are covered by row 8
# (Result={Skipped}) and by a live process (the kill path) respectively.
# ===========================================================================

$script:StPass  = 0
$script:StTotal = 0
$script:StFails = @()

function Reset-SelfTestLedger {
    $script:StPass  = 0
    $script:StTotal = 0
    $script:StFails = @()
}

<#
    N-3: ONE ledger entry per case, however many sub-assertions the case makes, so a
    reader counting FAILURES lines cannot over-count failures.
#>
function Add-SelfTestCase {
    param(
        [Parameter(Mandatory)][string] $Name,
        [Parameter(Mandatory)][bool] $Ok,
        [AllowEmptyString()][string] $Detail = '',
        [AllowEmptyString()][string] $Row = ''
    )
    $script:StTotal++
    if ($Ok) { $script:StPass++ } else { $script:StFails += ('{0}: {1}' -f $Name, $Detail) }

    $colour = 'Red'
    $okText = 'NO'
    if ($Ok) { $colour = 'Green'; $okText = 'yes' }

    if ($Row -ne '') {
        Write-Host $Row -ForegroundColor $colour
    } else {
        Write-Host ('{0,-62} {1,-5}{2}' -f $Name, $okText, $Detail) -ForegroundColor $colour
    }
}

<#
    A refusal is only useful if the reader can act on it. This asserts BOTH that the
    body threw AND that the sentence a human gets is intact.

    MEASURED 2026-09-09: defect 5b (-f binds tighter than '+', so a multi-fragment
    message formats only its LAST fragment) RECURRED in a NEW throw message, because the
    self-test placeholder-checked verdict messages and bound messages but not throws.
    Every message this tool can print is now checked by the same rule.
#>
function Test-ThrowsOn {
    param([Parameter(Mandatory)][scriptblock] $Body)
    $threw = $false
    $msg = ''
    try { & $Body | Out-Null } catch { $threw = $true; $msg = $_.Exception.Message }
    if (-not $threw) { return @{ Ok = $false; Detail = 'did NOT throw -- this input was ACCEPTED' } }
    if ([string]::IsNullOrWhiteSpace($msg)) { return @{ Ok = $false; Detail = 'threw with an empty message' } }
    if ($msg -match '\{\d+\}') { return @{ Ok = $false; Detail = ('threw, but the message carries an unsubstituted placeholder -> ' + $msg) } }
    return @{ Ok = $true; Detail = '' }
}

function Add-ThrowCase {
    param(
        [Parameter(Mandatory)][string] $Name,
        [Parameter(Mandatory)][scriptblock] $Body,
        [AllowEmptyString()][string] $Note = ''
    )
    $r = Test-ThrowsOn -Body $Body
    $detail = $Note
    if (-not $r.Ok) { $detail = $r.Detail }
    Add-SelfTestCase -Name $Name -Ok $r.Ok -Detail $detail
}

<#
    SC-126 cl. 7/9 -- THE HEADER MUST NOT GROW A ROT-PRONE ADDRESS.

    Six consecutive rows each DELETED one line-number citation from this file's header BY
    HAND. None of those citations was residue: every one was written FRESH by a careful
    row and was false within days, because an edit ANYWHERE ABOVE a citation moves the
    thing it points at while the citation itself stays put -- a clean diff, a green parse
    and a false sentence. Fixing the last instance does not close a GENERATIVE failure
    mode, so this case closes it by construction instead.

    THE BAN LIST IS THE TWO SHAPES MEASURED TO HAVE ACTUALLY ROTTED IN THAT CHAIN, AND
    NOTHING ELSE. Each is quoted from the diff that deleted it, by commit, because a
    commit hash is the one reference in this comment that cannot rot:

      (a) an address into a PIPELINE MARKDOWN file, in EITHER spelling. The second
          spelling is not a variant -- it is the one that defeated a sweep once already:
              f5697f3 (TASK-1324) deleted   tool runs" (CONVENTIONS.md:5093), and the ...
              the header's own exhibit records  ... at :5966-5969 and then at :6172-6175
      (b) a BARE self-address into this .ps1:
              9d80505 (TASK-1327) deleted   they stood at :1680 (launch) and
              9d80505 (TASK-1327) deleted   :1677 (receipt), both RE-GREPPED ...

    SHAPE (a) IS CASE-INSENSITIVE, SHAPE (b) IS NOT, AND THE ASYMMETRY IS DELIBERATE.
    [regex]::Matches is CASE-SENSITIVE in .NET -- unlike PowerShell's own -match, and that
    exact asymmetry defeated this row's FIRST control harness -- so a lowercase spelling of
    the filename would have slipped shape (a) in silence. Shape (a) therefore carries (?i).
    Shape (b) matches no letter at all outside its lookbehind, so case cannot reach it, and
    an (?i) there would only be noise pretending to be rigour.

    AND SHAPE (a) DEMANDS THE COLON TOUCH THE DIGIT. "see CONVENTIONS.md: 3 rows apply" is
    ordinary English, not an address, and an earlier '\s*:\s*\d+' spelling would have RED
    on it. A false red on this header is the failure with teeth: it invites the very
    hand-edit this case exists to end, so the pattern errs toward letting prose through
    (SHIP-9 -- validate against the failure that costs, not against success).

    DELIBERATELY OUT OF SCOPE, DECLARED SO A SKIPPED CLASS CANNOT READ AS A FORGOTTEN ONE:

      * ENGINE-SOURCE addresses ('ParseExecCommands.cpp:29', 'EditorServer.cpp:5993').
        They move on an ENGINE UPGRADE, not on an agent's edit, and banning them would gut
        the -ExecCmds explanation this header exists to carry. NOTE THE TWO DIGITS in
        ':29' -- a ':[0-9]{3,4}' ban would MISS A REAL ADDRESS. That is why shape (b) uses
        '\d+' and discriminates on WHAT PRECEDES THE COLON, never on how long the number is.
      * CLOCK TIMES ('23:42', '04:55:38', '04:55:58'). Not addresses at all. Shape (b)
        refuses a colon preceded by a digit, so they are excluded BY CONSTRUCTION rather
        than by a subtracted exception.

    A third shape may be added ONLY by naming an instance that rotted. Speculative bans
    are out: sweeping for "an address" and subtracting exceptions is the exact move that
    produced both of the errors above.

    THE EXHIBIT IS EXCLUDED BY SUBSTRING, NEVER BY AN ADDRESS. The header block introduced
    by 'ANCHOR TO QUOTED TEXT' carries two dead addresses ON PURPOSE: they are there
    BECAUSE they rotted. They are EVIDENCE, not citations, and a check that prescribed
    their removal would destroy the thing they demonstrate. Those lines are removed from
    the corpus BEFORE any matching runs -- and if that substring is ever deleted or
    reworded the function REFUSES TO SCAN AT ALL rather than scanning without it, so no
    failure this case can emit is capable of naming those two addresses UNDER ANY INPUT
    THAT DELETES OR REWORDS THE ANCHOR, not merely under the happy one. A REFLOW THAT
    SPLITS THAT PARAGRAPH IS A DIFFERENT INPUT CLASS AND IS NOT COVERED: the downward walk
    stops at the inserted blank, the address line falls outside the exemption and IS
    scanned -- visibly, because the excluded-line count in the same ledger line drops.
    Narrowing the exempt block to the marker line would make that case UNCONDITIONAL, so
    it is ruled out, not overlooked. Hard-coding that block's line number instead would
    carry the exact defect this case guards: the block moved +3 under TASK-1327 (measured
    across that commit, not quoted from a ledger), and any edit that CHANGES THE LINE
    COUNT above it moves it again.

    THE HEADER BOUNDARY IS DERIVED, NEVER HARDCODED. PowerShell's own parser is asked for
    the first block-comment token in the file, so nesting and same-line delimiters are the
    language's problem and not this function's guess. THE DERIVED BOUNDARY IS NOWHERE
    WRITTEN DOWN HERE, AND NO ADDRESS INTO THIS FILE SURVIVES IN THIS COMMENT AS A LIVE
    CITATION: the only ones pointing into this file are quoted FROM THE DIFFS THAT DELETED
    THEM, and the boundary itself is printed in the ledger line on EVERY RUN THAT DERIVES
    ONE, pass or fail, which is the one place it cannot go stale. It moves with any edit
    that CHANGES THE LINE COUNT above it -- not with every edit to this file, and a live
    address recorded here would rot exactly as the hand-deleted citations above it did. A
    count in a comment is an address wearing different clothes. (This paragraph may NOT
    quote a block comment's own opener and closer literally: a doc comment that spells its
    own closer CLOSES ITSELF there, and the rest is then parsed as code. Measured while
    writing this function.)

    IT FAILS CLOSED AT ALL THREE OF ITS ENTRY CONDITIONS -- the parse, the header boundary
    and the exhibit anchor. A parse error, a missing leading block comment, or a missing
    exhibit anchor each RED the case with a FAIL-CLOSED sentence instead of scanning a
    corpus this function could not bound. A guard that reports "clean" when it could not
    find its subject is worse than no guard (SC-39) -- and a guard that fails closed at one
    anchor while failing open at the next has only moved the hole.
#>
function Get-HeaderRotProneAddress {
    param([Parameter(Mandatory)][string] $Path)

    $tokens = $null
    $errors = $null
    $null = [System.Management.Automation.Language.Parser]::ParseFile($Path, [ref] $tokens, [ref] $errors)

    if ($errors -and $errors.Count -gt 0) {
        return @{ Ok = $false; First = 0; Last = 0; Exempt = 0
                  Why = @(('FAIL-CLOSED: the parser reported {0} error(s) on this file, so its token stream cannot be trusted to bound the header' -f $errors.Count)) }
    }

    $header = $null
    foreach ($t in $tokens) {
        if ($t.Kind -eq [System.Management.Automation.Language.TokenKind]::Comment -and $t.Text.StartsWith('<#')) {
            $header = $t
            break
        }
    }
    if ($null -eq $header) {
        return @{ Ok = $false; First = 0; Last = 0; Exempt = 0
                  Why = @('FAIL-CLOSED: no leading block comment found by the parser, so the header could not be bounded') }
    }

    $first = $header.Extent.StartLineNumber
    $last  = $header.Extent.EndLineNumber
    $lines = [System.IO.File]::ReadAllLines($Path)

    # The exhibit. Found by SUBSTRING and grown outward to its blank-line boundaries, so
    # the exclusion survives every reflow of the paragraph that carries it. No address.
    $exempt = @{}
    for ($i = $first; $i -le $last; $i++) {
        if ($lines[$i - 1] -notlike '*ANCHOR TO QUOTED TEXT*') { continue }
        $a = $i
        while ($a -gt $first -and $lines[$a - 2].Trim() -ne '') { $a-- }
        $b = $i
        while ($b -lt $last  -and $lines[$b].Trim()     -ne '') { $b++ }
        for ($k = $a; $k -le $b; $k++) { $exempt[$k] = $true }
    }
    if ($exempt.Count -eq 0) {
        return @{ Ok = $false; First = $first; Last = $last; Exempt = 0
                  Why = @('FAIL-CLOSED: exhibit anchor not found inside the derived header, so the exhibit could not be excluded and NOTHING was scanned') }
    }

    $shapes = @(
        @{ What = 'pipeline-markdown address, either spelling (TASK-1324 shape)'
           Rx   = '(?i)(?:CONVENTIONS|TASKBOARD)\.md(?:\s+at\s*)?:\d+' },
        @{ What = 'bare self-address into this .ps1 (TASK-1327 shape)'
           Rx   = '(?<![0-9A-Za-z._/\\-]):\d+' }
    )

    $why = @()
    for ($i = $first; $i -le $last; $i++) {
        if ($exempt.ContainsKey($i)) { continue }
        foreach ($s in $shapes) {
            foreach ($m in [regex]::Matches($lines[$i - 1], $s.Rx)) {
                $why += ('line {0} grew a {1}: "{2}" in >>{3}<< -- delete it or replace it with a grep anchor (SC-126 cl. 7)' `
                         -f $i, $s.What, $m.Value, $lines[$i - 1].Trim())
            }
        }
    }

    return @{ Ok = ($why.Count -eq 0); First = $first; Last = $last; Exempt = $exempt.Count; Why = $why }
}

function Invoke-SelfTest {
    Reset-SelfTestLedger

    $fixtureDir = Join-Path $PSScriptRoot 'SuiteRunnerFixtures'
    if (-not (Test-Path -LiteralPath $fixtureDir)) {
        Write-Host "FIXTURES MISSING: $fixtureDir" -ForegroundColor Red
        return $EXIT_LOG_UNREADABLE
    }

    # =======================================================================
    # 1. THE FIXTURE CORPUS -- the judging half.
    # =======================================================================
    # ReasonLike is asserted as well as Expect. MEASURED 2026-09-09: a mutation that
    # DISABLED the W9 detector still produced exit 2, because the generic k/n echo check
    # catches the same fixture. Detection is redundant (good); what the W9 branch uniquely
    # supplies is the NAMED CAUSE. Assert the code only, and that branch is untested.
    $cases = @(
        @{ File = 'green-suite.log';            Mode = 'Suite';   Filter = 'Siegebound'; Commands = @();
           Expect = $EXIT_OK;                   ReasonLike = 'green';
           What = 'a genuinely green suite (and BLANK LINES, mid-log + EOF)' }
        @{ File = 'red-suite.log';              Mode = 'Suite';   Filter = 'Siegebound'; Commands = @();
           Expect = $EXIT_TESTS_FAILED;         ReasonLike = 'the suite is RED';
           What = 'real test failures (M > 0)' }
        @{ File = 'skipped-suite.log';          Mode = 'Suite';   Filter = 'Siegebound'; Commands = @();
           Expect = $EXIT_OK;                   ReasonLike = '2 skipped';
           What = 'W-2: Result={Skipped} is NOT a truncated run' }
        @{ File = 'zero-started.log';           Mode = 'Suite';   Filter = 'Siegebound'; Commands = @();
           Expect = $EXIT_DISPATCH_ECHO_FAILED; ReasonLike = "bare 'Cmd: Automation'";
           What = 'SC-95 cl.1 space split -> bare "Cmd: Automation"' }
        @{ File = 'zero-started-filtered.log';  Mode = 'Suite';   Filter = 'Siegebound'; Commands = @();
           Expect = $EXIT_ZERO_STARTED;         ReasonLike = 'ZERO tests started';
           What = 'correct echo but N = 0' }
        @{ File = 'result-absent.log';          Mode = 'Suite';   Filter = 'Siegebound'; Commands = @();
           Expect = $EXIT_RESULT_ABSENT;        ReasonLike = 'UNREADABLE INSTRUMENT';
           What = 'started, but no Result={} line' }
        @{ File = 'count-mismatch.log';         Mode = 'Suite';   Filter = 'Siegebound'; Commands = @();
           Expect = $EXIT_COUNT_MISMATCH;       ReasonLike = 'count mismatch';
           What = 'Started 20 != Completed 18' }
        @{ File = 'w9-cmd-semicolon.log';       Mode = 'Command'; Filter = 'Siegebound';
           Commands = @('Siege.Fog.Raise', 'Siege.Fog.Status');
           Expect = $EXIT_DISPATCH_ECHO_FAILED; ReasonLike = 'SC-116 W9';
           What = 'SC-116 W9 semicolon soup, named by cause' }
        @{ File = 'green-commands.log';         Mode = 'Command'; Filter = 'Siegebound';
           Commands = @('Siege.Fog.Raise', 'Siege.Fog.Clear', 'Siege.Fog.Status');
           Expect = $EXIT_OK;                   ReasonLike = 'DISPATCHED, not SUCCEEDED';
           What = 'comma lane, 3 non-terminator commands (blank line)' }
        @{ File = 'no-terminator-echo.log';     Mode = 'Command'; Filter = 'Siegebound';
           Commands = @('Siege.Fog.Raise', 'Siege.Fog.Clear', 'Siege.Fog.Status');
           Expect = $EXIT_OK;                   ReasonLike = 'DISPATCHED, not SUCCEEDED';
           What = 'B-1: terminator echo LOST -> still GREEN (never a verdict)' }
        @{ File = '__does_not_exist__.log';     Mode = 'Suite';   Filter = 'Siegebound'; Commands = @();
           Expect = $EXIT_LOG_UNREADABLE;       ReasonLike = 'no readable log';
           What = 'absent log is not a pass' }
    )

    Write-Head '1. FIXTURE CORPUS -- guards driven against the FAILURES they must catch'
    Write-Host 'No engine launched. No process started. File parsing and arithmetic only.'
    Write-Host ''
    Write-Host ('{0,-30}{1,-9}{2,-9}{3,-7}{4}' -f 'fixture', 'expect', 'got', 'ok', 'what it proves')
    Write-Host ('-' * 122)

    foreach ($c in $cases) {
        $p = Join-Path $fixtureDir $c.File
        $ok = $true
        $why = @()
        $got = 'THREW'
        $v = $null

        try {
            $v = Get-LogVerdict -Path $p -ForMode $c.Mode -ForFilter $c.Filter -ForCommands $c.Commands
        } catch {
            # A parameter-binding failure (e.g. [AllowEmptyString()] deleted, which a real
            # log's BLANK LINES trip instantly) must RED this case, not abort the run.
            $ok = $false
            $why += ('THREW: {0}' -f $_.Exception.Message)
        }

        if ($null -ne $v) {
            $got = $v.Code
            if ($got -ne $c.Expect) {
                $ok = $false
                $why += ('expected {0}, got {1} ({2})' -f $c.Expect, $got, $v.Reason)
            }
            # GUARD ON THE GUARD. A verdict whose REASON still carries an unsubstituted
            # '{0}' is an unreadable explanation, and this self-test originally asserted the
            # exit CODE only -- so a broken message passed it. MEASURED 2026-09-09: in
            # PowerShell the -f operator binds TIGHTER than '+', so a message built as
            # ("a{0}" + "b" -f $x) formats only the LAST fragment and prints "a{0}b".
            # The code was right; the sentence a human reads was garbage.
            if ($v.Reason -match '\{\d+\}') {
                $ok = $false
                $why += ('verdict message carries an unsubstituted placeholder -> {0}' -f $v.Reason)
            }
            if ($v.Reason -notlike ('*{0}*' -f $c.ReasonLike)) {
                $ok = $false
                $why += ('verdict message never named the cause (wanted "{0}") -> {1}' -f $c.ReasonLike, $v.Reason)
            }
        }

        $okText = 'NO'
        if ($ok) { $okText = 'yes' }
        Add-SelfTestCase -Name ('fixture/' + $c.File) -Ok $ok -Detail ($why -join '; ') `
            -Row ('{0,-30}{1,-9}{2,-9}{3,-7}{4}' -f $c.File, $c.Expect, $got, $okText, $c.What)
    }

    # =======================================================================
    # 2. BOUND ARITHMETIC -- synthetic clocks, no process.
    #    W-5: the MESSAGE is asserted too. Defect 5b (-f binds tighter than +) was found
    #    in the verdict messages and fixed there; the bound messages had the identical
    #    shape and only ever asserted tripped/not-tripped.
    # =======================================================================
    Write-Head '2. BOUND ARITHMETIC (synthetic clocks, no process) -- code AND message'
    $boundCases = @(
        @{ El = 1500.0; Grow = 0.0;   Boot = $true;  Want = $true;  Like = 'OVERALL bound 1500s exceeded'; What = 'overall bound tripped' }
        @{ El = 420.0;  Grow = 5.0;   Boot = $false; Want = $true;  Like = 'BOOT bound 420s exceeded';     What = 'boot bound: command never echoed' }
        @{ El = 600.0;  Grow = 180.0; Boot = $true;  Want = $true;  Like = 'STALL bound 180s exceeded';    What = 'stall bound: log stopped growing' }
        @{ El = 40.0;   Grow = 1.0;   Boot = $true;  Want = $false; Like = '';                             What = 'healthy run is NOT killed' }
        @{ El = 300.0;  Grow = 5.0;   Boot = $false; Want = $false; Like = '';                             What = 'slow boot inside the bound survives' }
    )
    Write-Host ('{0,-12}{1,-12}{2,-8}{3,-9}{4,-7}{5}' -f 'elapsed', 'sinceGrow', 'booted', 'expect', 'ok', 'what it proves')
    Write-Host ('-' * 108)
    foreach ($b in $boundCases) {
        $msg = Test-BoundExceeded -ElapsedSeconds $b.El -SecondsSinceLogGrowth $b.Grow `
                                  -BootObserved $b.Boot -Overall 1500 -Boot 420 -Stall 180
        $tripped = ($msg -ne '')
        $ok = ($tripped -eq $b.Want)
        $why = @()
        if (-not $ok) { $why += ('expected tripped={0}, got {1}' -f $b.Want, $tripped) }
        if ($b.Want) {
            if ($msg -match '\{\d+\}') {
                $ok = $false
                $why += ('bound message carries an unsubstituted placeholder -> {0}' -f $msg)
            }
            if ($msg -notlike ('*{0}*' -f $b.Like)) {
                $ok = $false
                $why += ('bound message never named the bound (wanted "{0}") -> {1}' -f $b.Like, $msg)
            }
        }
        $okText = 'NO'
        if ($ok) { $okText = 'yes' }
        Add-SelfTestCase -Name ('bound/' + $b.What) -Ok $ok -Detail ($why -join '; ') `
            -Row ('{0,-12}{1,-12}{2,-8}{3,-9}{4,-7}{5}' -f $b.El, $b.Grow, $b.Boot, $b.Want, $okText, $b.What)
    }

    # =======================================================================
    # 3. LANE CONSTRUCTION -- the EMITTING half. THE REASON THIS TOOL EXISTS.
    #    Mutate the ';' at New-ExecCmdsValue, or the terminator, and these die.
    # =======================================================================
    Write-Head '3. LANE CONSTRUCTION -- the exact -ExecCmds VALUE (string equality)'
    $suiteVal = ''
    $ok = $true; $why = @()
    try { $suiteVal = New-ExecCmdsValue -ForMode Suite -ForFilter 'Siegebound' } catch { $ok = $false; $why += ('THREW: ' + $_.Exception.Message) }
    if ($ok -and $suiteVal -cne 'Automation RunTests Siegebound;Quit') { $ok = $false; $why += ("got '$suiteVal'") }
    Add-SelfTestCase -Name "suite value is 'Automation RunTests Siegebound;Quit'" -Ok $ok -Detail ($why -join '; ')

    $ok = $true; $why = @(); $v2 = ''
    try { $v2 = New-ExecCmdsValue -ForMode Suite -ForFilter 'Siegebound.Account' } catch { $ok = $false; $why += ('THREW: ' + $_.Exception.Message) }
    if ($ok -and $v2 -cne 'Automation RunTests Siegebound.Account;Quit') { $ok = $false; $why += ("got '$v2'") }
    Add-SelfTestCase -Name 'suite value honours a sub-group filter' -Ok $ok -Detail ($why -join '; ')

    $ok = $true; $why = @(); $v3 = ''
    try { $v3 = New-ExecCmdsValue -ForMode Command -ForCommands @('A', 'B') } catch { $ok = $false; $why += ('THREW: ' + $_.Exception.Message) }
    if ($ok -and $v3 -cne 'A,B,QUIT_EDITOR') { $ok = $false; $why += ("got '$v3'") }
    Add-SelfTestCase -Name "command value is 'A,B,QUIT_EDITOR' (COMMAS + terminator)" -Ok $ok -Detail ($why -join '; ')

    $ok = $true; $why = @(); $v4 = ''
    try { $v4 = New-ExecCmdsValue -ForMode Command -ForCommands @('A', 'Quit') } catch { $ok = $false; $why += ('THREW: ' + $_.Exception.Message) }
    if ($ok -and $v4 -cne 'A,QUIT_EDITOR') { $ok = $false; $why += ("got '$v4'") }
    Add-SelfTestCase -Name "a caller-supplied 'Quit' is DROPPED, QUIT_EDITOR appended" -Ok $ok -Detail ($why -join '; ')

    Add-ThrowCase -Name "caller separator ';' inside a command is REFUSED (exit 64)" `
        -Body { New-ExecCmdsValue -ForMode Command -ForCommands @('A;B') } `
        -Note 'this guard defends the ~53 prose ";Quit" sites from a sweep'
    Add-ThrowCase -Name "caller separator ',' inside a command is REFUSED (exit 64)" `
        -Body { New-ExecCmdsValue -ForMode Command -ForCommands @('A,B') }
    Add-ThrowCase -Name 'an empty command list is REFUSED (exit 64)' `
        -Body { New-ExecCmdsValue -ForMode Command -ForCommands @() }
    Add-ThrowCase -Name 'a command list of ONLY terminators is REFUSED (exit 64)' `
        -Body { New-ExecCmdsValue -ForMode Command -ForCommands @('Quit', 'Exit') }
    Add-ThrowCase -Name "W-4: a -Filter carrying ';' is REFUSED (exit 64)" `
        -Body { New-ExecCmdsValue -ForMode Suite -ForFilter 'A;B' }
    Add-ThrowCase -Name "W-4: a -Filter carrying ',' is REFUSED (exit 64)" `
        -Body { New-ExecCmdsValue -ForMode Suite -ForFilter 'A,B' }
    Add-ThrowCase -Name 'W-4: a -Filter carrying a quote is REFUSED (exit 64)' `
        -Body { New-ExecCmdsValue -ForMode Suite -ForFilter 'A"B' }
    Add-ThrowCase -Name 'W-4: an empty -Filter is REFUSED (exit 64)' `
        -Body { New-ExecCmdsValue -ForMode Suite -ForFilter '  ' }

    # =======================================================================
    # 4. THE EXPECTED-ECHO LIST -- B-1. The terminator is EXEMPT.
    # =======================================================================
    Write-Head '4. EXPECTED ECHOES -- B-1: the terminator is NEVER in the required list'
    $ok = $true; $why = @(); $e1 = @()
    try { $e1 = @(Get-ExpectedEchoes -ForMode Suite -ForFilter 'Siegebound') } catch { $ok = $false; $why += ('THREW: ' + $_.Exception.Message) }
    if ($ok -and ($e1.Count -ne 1 -or $e1[0] -cne 'Automation RunTests Siegebound')) { $ok = $false; $why += ('got [' + ($e1 -join '|') + ']') }
    Add-SelfTestCase -Name 'suite expects exactly 1 echo, without ";Quit"' -Ok $ok -Detail ($why -join '; ')

    $ok = $true; $why = @(); $e2 = @()
    try { $e2 = @(Get-ExpectedEchoes -ForMode Command -ForCommands @('A', 'B')) } catch { $ok = $false; $why += ('THREW: ' + $_.Exception.Message) }
    if ($ok) {
        if ($e2.Count -ne 2) { $ok = $false; $why += ('expected 2 entries, got ' + $e2.Count + ' [' + ($e2 -join '|') + ']') }
        foreach ($x in $e2) { if ($x -ieq $script:Terminator) { $ok = $false; $why += 'QUIT_EDITOR is in the REQUIRED list -- SC-116 cl. 4(c) FORBIDS demanding the terminator echo' } }
    }
    Add-SelfTestCase -Name 'B-1: command lane requires 2 echoes and NOT the terminator' -Ok $ok -Detail ($why -join '; ')

    # SYMMETRY. MEASURED 2026-09-09: with the separator guard only in the EMITTING half,
    # -VerifyLog judged 'A,B,C' as ONE expected command and returned exit 2 -- a FALSE RED
    # on a good log, from the most natural CLI form. Both halves must refuse the same input.
    Add-ThrowCase -Name "symmetry: Get-ExpectedEchoes refuses ';' like New-ExecCmdsValue" `
        -Body { Get-ExpectedEchoes -ForMode Command -ForCommands @('A;B') } `
        -Note 'the JUDGING half must refuse what the EMITTING half refuses'
    Add-ThrowCase -Name "symmetry: Get-ExpectedEchoes refuses ',' like New-ExecCmdsValue" `
        -Body { Get-ExpectedEchoes -ForMode Command -ForCommands @('A,B,C') } `
        -Note 'else -VerifyLog reds a good log with "dispatch echo 0 / 1"'
    Add-ThrowCase -Name 'symmetry: Get-ExpectedEchoes refuses a mangled -Filter' `
        -Body { Get-ExpectedEchoes -ForMode Suite -ForFilter 'A;B' }

    # =======================================================================
    # 5. THE COMMAND LINE -- the LAUNCHING half. Defect #1's home.
    # =======================================================================
    Write-Head '5. COMMAND LINE -- ONE verbatim string, -ExecCmds quoted as ONE token'
    $cl = $null
    $ok = $true; $why = @()
    try {
        $cl = New-EditorCommandLine -Uproject 'C:\p\X.uproject' `
                                    -ExecValue (New-ExecCmdsValue -ForMode Suite -ForFilter 'Siegebound') `
                                    -AbsLog 'C:\p\Saved\Logs\x.log'
    } catch { $ok = $false; $why += ('THREW: ' + $_.Exception.Message) }
    $isString = ($cl -is [string])
    if (-not $isString) { $ok = $false; $why += ('returned [{0}], NOT [string] -- an ARRAY -ArgumentList lets PowerShell re-quote elements: that is SC-95 cl. 1, the historical false green' -f $(if ($null -eq $cl) { 'null' } else { $cl.GetType().Name })) }
    Add-SelfTestCase -Name 'New-EditorCommandLine returns [string], not [string[]]' -Ok $ok -Detail ($why -join '; ')

    $ok = $isString; $why = @()
    if (-not $isString) { $why += 'not a string, cannot inspect tokens' }
    else {
        if ($cl -cnotmatch ([regex]::Escape('-ExecCmds="Automation RunTests Siegebound;Quit"'))) {
            $ok = $false; $why += ("the -ExecCmds token is not present verbatim -> " + $cl)
        }
    }
    Add-SelfTestCase -Name '-ExecCmds="Automation RunTests Siegebound;Quit" survives verbatim' -Ok $ok -Detail ($why -join '; ')

    $ok = $isString; $why = @()
    if ($isString) {
        if ($cl -cnotmatch ([regex]::Escape('"C:\p\X.uproject"')))      { $ok = $false; $why += 'uproject is not quoted' }
        if ($cl -cnotmatch ([regex]::Escape('-abslog="C:\p\Saved\Logs\x.log"'))) { $ok = $false; $why += 'abslog is not quoted' }
        if ($cl -cnotmatch ([regex]::Escape('-unattended')))            { $ok = $false; $why += '-unattended missing' }
        if ($cl -cnotmatch ([regex]::Escape('-nullrhi')))               { $ok = $false; $why += '-nullrhi missing' }
    } else { $why += 'not a string' }
    Add-SelfTestCase -Name 'paths quoted; -nullrhi/-unattended present' -Ok $ok -Detail ($why -join '; ')

    $ok = $true; $why = @(); $cl2 = $null
    try {
        $cl2 = New-EditorCommandLine -Uproject 'C:\p\X.uproject' `
                                     -ExecValue (New-ExecCmdsValue -ForMode Command -ForCommands @('Siege.Fog.Raise', 'Siege.Fog.Status')) `
                                     -AbsLog 'C:\p\Saved\Logs\x.log'
    } catch { $ok = $false; $why += ('THREW: ' + $_.Exception.Message) }
    if ($ok -and ($cl2 -cnotmatch ([regex]::Escape('-ExecCmds="Siege.Fog.Raise,Siege.Fog.Status,QUIT_EDITOR"')))) {
        $ok = $false; $why += ('command lane token wrong -> ' + $cl2)
    }
    Add-SelfTestCase -Name 'command lane emits the COMMA form with QUIT_EDITOR' -Ok $ok -Detail ($why -join '; ')

    # =======================================================================
    # 5b. TASK-1294 -- the Aura plugin must be EXCLUDED from BOTH editor lanes.
    #
    # SC-39 / SHIP-9: a guard only ever seen PASSING is indistinguishable from no
    # guard -- and this flag's absence is SILENT. The suite still boots, still runs
    # 561 tests and still prints a total; it just goes back to rolling dice on the
    # Aura 401. Nothing downstream would notice. So the SAME predicate that judges
    # the real command line is run against synthetic degenerate ones and MUST refuse
    # every one of them. The controls go THROUGH the predicate; none is asserted
    # beside it.
    # =======================================================================
    Write-Head '5b. AURA EXCLUSION -- -DisablePlugins=Aura, both lanes (TASK-1294)'

    # FParse::Value(FCommandLine::Get(), TEXT("DisablePlugins="), ...) takes the token
    # immediately after '=' with NO space, then ParseIntoArray splits it on ','
    # (PluginManager.cpp:1478-1479). Any other spelling silently yields an EMPTY
    # plugin list -- and Aura loads exactly as before, with no warning anywhere.
    $hasAuraFlag = { param([string] $Line) ($null -ne $Line) -and ($Line -cmatch '(^|\s)-DisablePlugins=Aura(\s|$)') }

    $ok = $true; $why = @()
    if (-not (& $hasAuraFlag $cl))  { $ok = $false; $why += ('SUITE lane is missing the flag -> ' + $cl) }
    if (-not (& $hasAuraFlag $cl2)) { $ok = $false; $why += ('COMMAND lane is missing the flag -> ' + $cl2) }
    Add-SelfTestCase -Name '-DisablePlugins=Aura present verbatim in BOTH lanes' -Ok $ok -Detail ($why -join '; ')

    # THE FIRING CONTROL. Each entry is a real way this flag dies in a future edit.
    $ok = $true; $why = @()
    $degenerates = @(
        @{ Why = 'flag deleted outright';                        Line = ($cl -creplace '\s-DisablePlugins=Aura', '') }
        @{ Why = 'space after = (FParse reads an EMPTY list)';   Line = ($cl -creplace '-DisablePlugins=Aura', '-DisablePlugins= Aura') }
        @{ Why = 'value emptied';                                Line = ($cl -creplace '-DisablePlugins=Aura', '-DisablePlugins=') }
        @{ Why = 'plugin name mistyped';                         Line = ($cl -creplace '-DisablePlugins=Aura', '-DisablePlugins=AuraModelGenerator') }
    )
    foreach ($d in $degenerates) {
        if (& $hasAuraFlag $d.Line) { $ok = $false; $why += ('ACCEPTED a degenerate line [' + $d.Why + '] -> ' + $d.Line) }
    }
    # ...and the predicate must still be able to say YES, or the four NOs above are
    # just the answer of a needle that matches nothing (SC-39).
    if (-not (& $hasAuraFlag $cl)) { $ok = $false; $why += 'the predicate no longer matches the REAL line -- the 4 refusals above prove NOTHING' }
    Add-SelfTestCase -Name 'the flag guard REFUSES 4 degenerate lines, still accepts the real one' -Ok $ok -Detail ($why -join '; ')

    # =======================================================================
    # 6. THE BOUND MERGE -- B-2. A force-killed run may not read green.
    # =======================================================================
    Write-Head '6. BOUND MERGE -- B-2: a killed run reports 6, never the log verdict'
    $ok = $true; $why = @()
    $fake = @{ Code = $EXIT_OK; Reason = '554 / 0 -- green.'; Echo = $null; Counts = $null
               LogPath = 'x.log'; Mode = 'Suite'; BoundMessage = ''; LogVerdictCode = -1; LogVerdictReason = '' }
    $merged = Merge-BoundIntoVerdict -Verdict $fake -BoundMessage 'STALL bound 180s exceeded: the log has not grown for 180s'
    if ($merged.Code -ne $EXIT_BOUND_EXCEEDED) { $ok = $false; $why += ('Code is {0}, MUST be {1} -- a killed run reported success' -f $merged.Code, $EXIT_BOUND_EXCEEDED) }
    if ($merged.LogVerdictCode -ne $EXIT_OK)   { $ok = $false; $why += 'the log verdict was not preserved as LogVerdictCode' }
    if ($merged.Reason -notlike '*BOUND EXCEEDED*') { $ok = $false; $why += ('reason does not lead with the bound -> ' + $merged.Reason) }
    if ($merged.Reason -notlike '*FORCE-KILLED*')   { $ok = $false; $why += 'reason does not say the process was killed' }
    if ($merged.Reason -match '\{\d+\}')            { $ok = $false; $why += ('reason carries an unsubstituted placeholder -> ' + $merged.Reason) }
    Add-SelfTestCase -Name 'B-2: green log + tripped bound  ->  exit 6, NOT 0' -Ok $ok -Detail ($why -join '; ')

    $ok = $true; $why = @()
    $fake2 = @{ Code = $EXIT_TESTS_FAILED; Reason = '18 / 2 -- the suite is RED.'; Echo = $null; Counts = $null
                LogPath = 'x.log'; Mode = 'Suite'; BoundMessage = ''; LogVerdictCode = -1; LogVerdictReason = '' }
    $merged2 = Merge-BoundIntoVerdict -Verdict $fake2 -BoundMessage ''
    if ($merged2.Code -ne $EXIT_TESTS_FAILED) { $ok = $false; $why += 'an untripped bound must not touch the verdict' }
    if ($merged2.BoundMessage -ne '')         { $ok = $false; $why += 'an untripped bound must leave BoundMessage empty' }
    Add-SelfTestCase -Name 'no bound tripped  ->  the log verdict is untouched' -Ok $ok -Detail ($why -join '; ')

    # =======================================================================
    # 7. -LogPath SAFETY -- W-3. The one place this script can damage the repo.
    # =======================================================================
    Write-Head '7. -LogPath SAFETY -- the Remove-Item is confined to Saved\*.log'
    $root = 'C:\proj'
    $pathCases = @(
        @{ In = 'Saved\Logs\run.log';                 Want = $true;  What = 'the default shape is allowed' }
        @{ In = 'C:\proj\Saved\Logs\run.log';         Want = $true;  What = 'an absolute path inside Saved is allowed' }
        @{ In = 'Content\Meshes\SM_Rock_01.uasset';   Want = $false; What = 'a .uasset is REFUSED' }
        @{ In = 'Saved\Logs\run.txt';                 Want = $false; What = 'a non-.log extension is REFUSED' }
        @{ In = 'C:\Windows\Temp\run.log';            Want = $false; What = 'outside the project is REFUSED' }
        @{ In = 'Saved\..\Content\run.log';           Want = $false; What = 'traversal out of Saved is REFUSED' }
        @{ In = '';                                   Want = $false; What = 'an empty path is REFUSED' }
    )
    foreach ($pc in $pathCases) {
        $res = Resolve-SafeLogPath -Candidate $pc.In -ProjectRootPath $root
        $ok = ($res.Ok -eq $pc.Want)
        $why = @()
        if (-not $ok) { $why += ('expected Ok={0}, got Ok={1} ({2})' -f $pc.Want, $res.Ok, $res.Reason) }
        Add-SelfTestCase -Name ("logpath/'" + $pc.In + "'") -Ok $ok -Detail (($why + $pc.What) -join '; ')
    }

    # =======================================================================
    # 8. INPUT TOLERANCE -- B-3(a). A synthetic corpus tests the LOGIC, not the INPUT.
    #    These call the parsers DIRECTLY with the blank lines Get-Content really yields.
    # =======================================================================
    Write-Head '8. INPUT TOLERANCE -- blank lines, empty arrays, the shapes a REAL log has'
    $realish = @('', '[..][ 0]Cmd: Siege.Fog.Raise', '', '   ', '[..][ 1]Cmd: QUIT_EDITOR', '')

    $ok = $true; $why = @(); $ech = @()
    try { $ech = @(Get-CmdEchoes -Lines $realish) } catch { $ok = $false; $why += ('THREW on blank lines: ' + $_.Exception.Message) }
    if ($ok -and $ech.Count -ne 2) { $ok = $false; $why += ('expected 2 echoes, got ' + $ech.Count) }
    Add-SelfTestCase -Name 'Get-CmdEchoes survives BLANK LINES ([AllowEmptyString])' -Ok $ok -Detail ($why -join '; ')

    $ok = $true; $why = @()
    try { [void] (Get-CmdEchoes -Lines @()) } catch { $ok = $false; $why += ('THREW on an empty array: ' + $_.Exception.Message) }
    Add-SelfTestCase -Name 'Get-CmdEchoes survives an EMPTY array ([AllowEmptyCollection])' -Ok $ok -Detail ($why -join '; ')

    $ok = $true; $why = @(); $sc = $null
    $counted = @('', '[..]Test Started. Name={A}', '', '[..]Test Completed. Result={Success} Name={A}', '')
    try { $sc = Test-SuiteCounts -Lines $counted } catch { $ok = $false; $why += ('THREW on blank lines: ' + $_.Exception.Message) }
    if ($null -ne $sc) {
        if ($sc.Started -ne 1 -or $sc.Completed -ne 1 -or $sc.Success -ne 1) { $ok = $false; $why += ('counts wrong: {0}/{1}/{2}' -f $sc.Started, $sc.Completed, $sc.Success) }
        if ($sc.Code -ne $EXIT_OK) { $ok = $false; $why += ('code {0}' -f $sc.Code) }
    }
    Add-SelfTestCase -Name 'Test-SuiteCounts survives BLANK LINES and still counts' -Ok $ok -Detail ($why -join '; ')

    $ok = $true; $why = @(); $de = $null
    try { $de = Test-DispatchEcho -Echoes @() -Expected @('A') -ForMode Command } catch { $ok = $false; $why += ('THREW: ' + $_.Exception.Message) }
    if ($null -ne $de) {
        if ($de.Ok) { $ok = $false; $why += 'a log with ZERO echoes was accepted' }
        if ($de.TerminatorEcho -notlike '*NOT SEEN*') { $ok = $false; $why += 'a missing terminator was not reported as corroboration' }
    }
    Add-SelfTestCase -Name 'zero echoes is NOT a pass; missing terminator is reported' -Ok $ok -Detail ($why -join '; ')

    $ok = $true; $why = @(); $de2 = $null
    try { $de2 = Test-DispatchEcho -Echoes @('A', 'B', 'QUIT_EDITOR') -Expected @('A', 'B') -ForMode Command } catch { $ok = $false; $why += ('THREW: ' + $_.Exception.Message) }
    if ($null -ne $de2) {
        if (-not $de2.Ok) { $ok = $false; $why += ('a good command log was rejected: ' + $de2.Reason) }
        if ($de2.Expected -ne 2) { $ok = $false; $why += ('Expected count is {0}, must be 2 (terminator exempt)' -f $de2.Expected) }
        if ($de2.TerminatorEcho -notlike '*seen*') { $ok = $false; $why += 'a present terminator was not corroborated' }
    }
    Add-SelfTestCase -Name 'B-1: terminator echo is corroboration, never a required match' -Ok $ok -Detail ($why -join '; ')

    # W-10's incremental reader, driven against a real fixture file.
    $ok = $true; $why = @()
    $peekFile = Join-Path $fixtureDir 'green-commands.log'
    try {
        $first = Read-LogSince -Path $peekFile -Offset 0
        $firstEch = @(Get-CmdEchoes -Lines $first.Lines)
        if ($firstEch.Count -lt 3) { $ok = $false; $why += ('first read saw {0} echoes, expected >= 3' -f $firstEch.Count) }
        $second = Read-LogSince -Path $peekFile -Offset $first.Offset
        if ($second.Lines.Count -ne 0) { $ok = $false; $why += ('a second read of an unchanged file returned {0} lines; it must return 0' -f $second.Lines.Count) }
        $rewound = Read-LogSince -Path $peekFile -Offset 99999999
        if ($rewound.Lines.Count -eq 0) { $ok = $false; $why += 'an offset past EOF must rewind and re-read, not go blind' }
    } catch { $ok = $false; $why += ('THREW: ' + $_.Exception.Message) }
    Add-SelfTestCase -Name 'W-10: incremental log reader advances and never re-reads' -Ok $ok -Detail ($why -join '; ')

    # =======================================================================
    # SC-126 cl. 7/9. The ONE thing six rows fixed by hand, now caught by construction.
    # The detail line publishes the DERIVED boundary on a pass as well as on any fail that
    # reached one, so a green ledger line is still evidence about where the header was
    # measured to end. It is omitted where nothing was derived, because a boundary of
    # "lines 0..0" is not a measurement. The omission keys off the BOUNDARY and never off
    # the exempt count, so the "0 exhibit line(s) excluded" tripwire -- the meter that
    # makes a defeated exemption visible -- still prints on the anchor-missing path.
    # =======================================================================
    $hdr = Get-HeaderRotProneAddress -Path $PSCommandPath
    $hdrLead = @()
    if ($hdr.Last -gt 0) {
        $hdrLead = @(('header derived as lines {0}..{1}; {2} exhibit line(s) excluded by substring, never by address' `
                      -f $hdr.First, $hdr.Last, $hdr.Exempt))
    }
    $hdrDetail = ($hdrLead + $hdr.Why) -join '; '
    Add-SelfTestCase -Name 'SC-126: header grows no rot-prone line address' -Ok $hdr.Ok -Detail $hdrDetail

    # =======================================================================
    Write-Head 'SELF TEST RESULT'
    if ($script:StFails.Count -eq 0) {
        Write-Verdict 'SELFTEST' ('{0} / {0} cases produced their EXPECTED result.' -f $script:StTotal) 'Green'
        Write-Host   '           Every guard has now been seen to return NO, not merely to return.'
        Write-Host   '           Covered: the judging half, the emitting half, the bound merge, the'
        Write-Host   '           -LogPath guard and blank-line input tolerance.'
        Write-Host   '           NOT covered here (needs a live process): the kill path itself.'
        return $EXIT_OK
    }
    Write-Verdict 'SELFTEST' ('{0} / {1} passed. FAILURES ({2} cases):' -f $script:StPass, $script:StTotal, $script:StFails.Count) 'Red'
    foreach ($f in $script:StFails) { Write-Host ('  - {0}' -f $f) -ForegroundColor Red }
    return $EXIT_TESTS_FAILED
}

# ===========================================================================
# REPORTING
# ===========================================================================
function Show-Verdict {
    param([Parameter(Mandatory)][hashtable] $Verdict, [switch] $AsPorcelain)

    Write-Head 'VERDICT'
    $code = $Verdict.Code

    if ($null -ne $Verdict.Echo) {
        $colour = 'Red'
        if ($Verdict.Echo.Ok) { $colour = 'Green' }
        Write-Verdict 'DISPATCH ECHO' ('{0} / {1}' -f $Verdict.Echo.Matched, $Verdict.Echo.Expected) $colour
        Write-Verdict 'terminator echo' $Verdict.Echo.TerminatorEcho 'Gray'
    }

    if ($null -ne $Verdict.Counts) {
        $c = $Verdict.Counts
        Write-Verdict 'N (started)'   ('{0}' -f $c.Started)   'Gray'
        Write-Verdict 'completed'     ('{0}' -f $c.Completed) 'Gray'
        Write-Verdict 'N / M'         ('{0} / {1}' -f $c.Success, $c.Fail) 'Gray'
        if ($c.Skipped -gt 0 -or $c.NotRun -gt 0) {
            Write-Verdict 'skipped / not-run' ('{0} / {1}' -f $c.Skipped, $c.NotRun) 'Yellow'
        }
        if ($c.Discovered -ge 0) {
            Write-Verdict 'discovered' ('{0}' -f $c.Discovered) 'Gray'
        }
    }

    # B-2: if a bound killed the run, the log's own opinion is a FRAGMENT and is
    # labelled as one. It is never the headline and never RUNNER_EXIT.
    if ($Verdict.BoundMessage -ne '') {
        Write-Verdict 'BOUND TRIPPED' $Verdict.BoundMessage 'Red'
        Write-Verdict 'log verdict (info)' ('exit {0} -- {1}' -f $Verdict.LogVerdictCode, $Verdict.LogVerdictReason) 'DarkYellow'
    }

    $colour = 'Red'
    $word = 'NO'
    if ($code -eq $EXIT_OK) { $colour = 'Green'; $word = 'OK' }
    Write-Verdict $word $Verdict.Reason $colour
    Write-Verdict 'log' $Verdict.LogPath 'Gray'
    Write-Verdict 'exit' ('{0}' -f $code) $colour

    if ($AsPorcelain) {
        Write-Host ''
        # RUNNER_EXIT is the value this script exits with. Same variable, one exit
        # statement, at the bottom of the file. They cannot disagree.
        Write-Host ('RUNNER_EXIT={0}' -f $code)
        Write-Host ('RUNNER_MODE={0}' -f $Verdict.Mode)
        Write-Host ('RUNNER_LOG={0}' -f $Verdict.LogPath)
        if ($Verdict.BoundMessage -ne '') {
            Write-Host ('RUNNER_BOUND={0}' -f $Verdict.BoundMessage)
            Write-Host ('RUNNER_LOG_VERDICT={0}' -f $Verdict.LogVerdictCode)
        }
        if ($null -ne $Verdict.Echo) {
            Write-Host ('RUNNER_ECHO_MATCHED={0}' -f $Verdict.Echo.Matched)
            Write-Host ('RUNNER_ECHO_EXPECTED={0}' -f $Verdict.Echo.Expected)
            Write-Host ('RUNNER_TERMINATOR_ECHO={0}' -f $Verdict.Echo.TerminatorEcho)
        }
        if ($null -ne $Verdict.Counts) {
            Write-Host ('RUNNER_STARTED={0}'   -f $Verdict.Counts.Started)
            Write-Host ('RUNNER_COMPLETED={0}' -f $Verdict.Counts.Completed)
            Write-Host ('RUNNER_SUCCESS={0}'   -f $Verdict.Counts.Success)
            Write-Host ('RUNNER_FAIL={0}'      -f $Verdict.Counts.Fail)
            Write-Host ('RUNNER_SKIPPED={0}'   -f $Verdict.Counts.Skipped)
        }
    }
}

# ===========================================================================
# MAIN  --  a FUNCTION that RETURNS a code. There is exactly one 'exit' statement
# in this file, at the very bottom, and it exits the value Show-Verdict printed.
# W-11: an unhandled throw is caught there and mapped to a DOCUMENTED code (70)
# instead of PowerShell's undocumented 1.
# ===========================================================================
function Invoke-Main {

    if ($SelfTest) {
        return (Invoke-SelfTest)
    }

    if ($VerifyLog -ne '') {
        if ($Mode -eq 'Command' -and $Commands.Count -eq 0) {
            Write-Host 'USAGE: -Mode Command requires -Commands.' -ForegroundColor Red
            return $EXIT_USAGE
        }
        Write-Head ('VERIFY ONLY -- no engine launched ({0} lane)' -f $Mode)
        $v = $null
        try {
            $v = Get-LogVerdict -Path $VerifyLog -ForMode $Mode -ForFilter $Filter -ForCommands $Commands
        } catch {
            # The judging half validates its input EXACTLY as the emitting half does, so
            # a caller who cannot be RUN cannot be VERIFIED either. Before this existed
            # the same input produced a false 'dispatch echo 0 / 1' -- exit 2 on a good log.
            Write-Host ('USAGE: {0}' -f $_.Exception.Message) -ForegroundColor Red
            return $EXIT_USAGE
        }
        Show-Verdict -Verdict $v -AsPorcelain:$Porcelain
        return $v.Code
    }

    # --- resolve paths -------------------------------------------------------
    if ($ProjectPath -eq '') {
        $ProjectPath = Join-Path $script:ProjectRoot 'GitClaudeUnrealTest.uproject'
    }
    if ($EditorCmd -eq '') {
        $EditorCmd = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
    }
    if ($LogPath -eq '') {
        $stamp   = Get-Date -Format 'yyyyMMdd-HHmmss'
        $LogPath = Join-Path $script:ProjectRoot ('Saved\Logs\run_suite_bounded_{0}_{1}.log' -f $Mode.ToLower(), $stamp)
    }

    # W-3: this script DELETES $LogPath before launching. Confine it before we do.
    $safe = Resolve-SafeLogPath -Candidate $LogPath -ProjectRootPath $script:ProjectRoot
    if (-not $safe.Ok) {
        Write-Host ('USAGE: {0}' -f $safe.Reason) -ForegroundColor Red
        return $EXIT_USAGE
    }
    $LogPath = $safe.Path

    try {
        $execValue = New-ExecCmdsValue -ForMode $Mode -ForFilter $Filter -ForCommands $Commands
    } catch {
        Write-Host ('USAGE: {0}' -f $_.Exception.Message) -ForegroundColor Red
        return $EXIT_USAGE
    }

    $commandLine = New-EditorCommandLine -Uproject $ProjectPath -ExecValue $execValue -AbsLog $LogPath

    Write-Head ('{0} LANE' -f $Mode.ToUpper())
    if ($Mode -eq 'Suite') {
        Write-Host "separator: ';'  -- consumed by the Automation handler's OWN parser, never by -ExecCmds. CORRECT AS WRITTEN."
        Write-Host "terminator: ';Quit' -- consumed too, so it NEVER echoes. Its echo is never demanded (SC-116 cl. 4c)."
    } else {
        Write-Host "separator: ','  -- the -ExecCmds parser splits ONLY on comma (ParseExecCommands.cpp:29)."
        Write-Host "terminator: QUIT_EDITOR -- appended by this script. 'Quit' does NOT quit an editor commandlet."
        Write-Host "            its echo is CORROBORATION ONLY and can never red this run (SC-116 cl. 4c)."
    }
    Write-Host ''
    Write-Host ('  {0} {1}' -f $EditorCmd, $commandLine)
    Write-Host ''
    Write-Host ('bounds: overall {0}s / boot {1}s / stall {2}s' -f $OverallSeconds, $BootSeconds, $StallSeconds)

    if ($DryRun) {
        Write-Host ''
        Write-Host 'DRY RUN -- nothing launched.' -ForegroundColor Yellow
        return $EXIT_OK
    }

    if (-not (Test-Path -LiteralPath $EditorCmd)) {
        Write-Host ('EDITOR NOT FOUND: {0}' -f $EditorCmd) -ForegroundColor Red
        return $EXIT_USAGE
    }
    if (-not (Test-Path -LiteralPath $ProjectPath)) {
        Write-Host ('UPROJECT NOT FOUND: {0}' -f $ProjectPath) -ForegroundColor Red
        return $EXIT_USAGE
    }

    # W-8: the editor is Jonathan's and may be in use. WARN LOUDLY; never auto-kill --
    # that is a human decision, and this script's Stop-Process is aimed only at the PID
    # it started itself.
    $existing = @(Get-Process -Name 'UnrealEditor*' -ErrorAction SilentlyContinue)
    if ($existing.Count -gt 0) {
        Write-Host ''
        Write-Host ('!! AN EDITOR IS ALREADY RUNNING ({0} process(es)): {1}' -f $existing.Count,
                    (($existing | ForEach-Object { '{0}(PID {1})' -f $_.ProcessName, $_.Id }) -join ', ')) -ForegroundColor Yellow
        Write-Host '!! A second instance may contend for Saved/, DDC and the asset registry.' -ForegroundColor Yellow
        Write-Host '!! NOT killing it -- that is a human decision. Ctrl-C now if this is a playtest.' -ForegroundColor Yellow
        Write-Host ''
    }

    $logDir = Split-Path -Parent $LogPath
    if (-not (Test-Path -LiteralPath $logDir)) {
        New-Item -ItemType Directory -Path $logDir -Force | Out-Null
    }
    if (Test-Path -LiteralPath $LogPath) { Remove-Item -LiteralPath $LogPath -Force }

    # Record exactly what was launched, beside the log. SC-116 cl. 4(a): a recipe is
    # published by someone who RAN it. N-1: ASCII, no BOM -- this file gets copy-pasted
    # and a BOM in front of a path is a bad first character.
    Set-Content -LiteralPath ($LogPath + '.cmdline') -Encoding ascii -Value ('{0} {1}' -f $EditorCmd, $commandLine)

    Write-Head 'LAUNCH'
    $proc = Start-Process -FilePath $EditorCmd -ArgumentList $commandLine -PassThru -WindowStyle Hidden
    Write-Host ('PID {0} started at {1:HH:mm:ss}' -f $proc.Id, (Get-Date))
    $startedAt    = Get-Date
    $lastSize     = -1L
    $lastGrowthAt = $startedAt
    $bootObserved = $false
    $boundMessage = ''
    $peekOffset   = 0L
    $killedPid    = -1
    $killSurvived = $false

    $expectedEchoes = @(Get-ExpectedEchoes -ForMode $Mode -ForFilter $Filter -ForCommands $Commands)

    while ($true) {
        if ($proc.HasExited) { break }

        Start-Sleep -Seconds 2

        $size = -1L
        if (Test-Path -LiteralPath $LogPath) {
            try { $size = (Get-Item -LiteralPath $LogPath).Length } catch { $size = $lastSize }
        }
        $now = Get-Date
        if ($size -gt $lastSize) {
            $lastSize = $size
            $lastGrowthAt = $now
        }

        if (-not $bootObserved) {
            # W-10: only the bytes added since the last poll. On the exact path this
            # exists to catch (the echo never appears) a full re-read every 2s for 420s
            # is ~210 re-reads of an ever-growing file.
            $peek = Read-LogSince -Path $LogPath -Offset $peekOffset
            $peekOffset = $peek.Offset
            if ($peek.Lines.Count -gt 0) {
                $peekEchoes = Get-CmdEchoes -Lines $peek.Lines
                foreach ($pe in $peekEchoes) {
                    foreach ($x in $expectedEchoes) {
                        if ((Test-ContainsLiteral -Haystack $pe -Needle $x) -or ($Mode -eq 'Suite' -and $pe -match '^\s*Automation\s+RunTests\s+')) {
                            $bootObserved = $true; break
                        }
                    }
                    if ($bootObserved) { break }
                }
                if ($bootObserved) {
                    Write-Host ('boot observed at {0:N0}s -- the command reached the engine.' -f ($now - $startedAt).TotalSeconds)
                }
            }
        }

        $elapsed     = ($now - $startedAt).TotalSeconds
        $sinceGrowth = ($now - $lastGrowthAt).TotalSeconds

        $boundMessage = Test-BoundExceeded -ElapsedSeconds $elapsed -SecondsSinceLogGrowth $sinceGrowth `
                                           -BootObserved $bootObserved -Overall $OverallSeconds `
                                           -Boot $BootSeconds -Stall $StallSeconds
        if ($boundMessage -ne '') {
            Write-Host ('BOUND TRIPPED: {0}' -f $boundMessage) -ForegroundColor Red
            $killedPid = $proc.Id
            try {
                Stop-Process -Id $proc.Id -Force -ErrorAction Stop
            } catch {
                Write-Host ('  (could not stop PID {0}: {1})' -f $proc.Id, $_.Exception.Message) -ForegroundColor Yellow
            }
            # W-7: SHIP-9 -- an unkilled process is the failure this bound exists to
            # detect. Do not assume the kill worked; confirm it, and name any survivor.
            #
            # THE PROBE IS $proc.HasExited, NOT Get-Process -Id.
            # MEASURED 2026-09-09, by running it: Start-Process -PassThru holds an OPEN
            # HANDLE to the process, and a killed process's PID stays RESOLVABLE by
            # Get-Process for as long as any handle is open. The first version of this
            # check used Get-Process and printed 'PID 30672 SURVIVED the kill' for a
            # process that Wait-Process had already confirmed dead (HasExited=True,
            # ExitCode=-1). A false red, in the one line a bound run most needs to trust.
            try {
                Wait-Process -Id $proc.Id -Timeout 15 -ErrorAction Stop
            } catch { }
            try { $proc.Refresh() } catch { }
            $exitedNow = $false
            try { $exitedNow = $proc.HasExited } catch { $exitedNow = $false }
            if (-not $exitedNow) {
                $killSurvived = $true
                Write-Host ('  !! PID {0} SURVIVED Stop-Process -Force and is STILL RUNNING. Anything read from the log below is being read from under a live process.' -f $proc.Id) -ForegroundColor Red
            } else {
                Write-Host ('  PID {0} confirmed gone (HasExited).' -f $proc.Id) -ForegroundColor Yellow
            }
            $orphans = @(Get-Process -Name 'UnrealEditor*', 'ShaderCompileWorker', 'CrashReportClient', 'EpicWebHelper' -ErrorAction SilentlyContinue)
            if ($orphans.Count -gt 0) {
                Write-Host ('  !! {0} engine process(es) still running after the kill: {1}' -f $orphans.Count,
                            (($orphans | ForEach-Object { '{0}(PID {1})' -f $_.ProcessName, $_.Id }) -join ', ')) -ForegroundColor Yellow
                Write-Host '  !! Some may be Jonathan''s. NOT killing them.' -ForegroundColor Yellow
            }
            break
        }
    }

    $wall = ((Get-Date) - $startedAt).TotalSeconds
    Write-Host ('wall clock: {0:N0}s' -f $wall)

    # $LASTEXITCODE / $proc.ExitCode is RECORDED, never consulted for the verdict.
    # Build.bat returns 0 on a failed build; the editor returns 0 on a run that executed
    # nothing. The log is the only witness.
    $recordedExit = 'n/a'
    try { if ($proc.HasExited) { $recordedExit = $proc.ExitCode } } catch { }
    Write-Host ('process exit code (RECORDED, NOT TRUSTED): {0}' -f $recordedExit)

    $verdict = Get-LogVerdict -Path $LogPath -ForMode $Mode -ForFilter $Filter -ForCommands $Commands

    # B-2: ONE reporting path. If a bound killed the run, the bound IS the verdict; the
    # log's opinion is demoted to LogVerdictCode and labelled as a fragment. There is no
    # branch here that prints a verdict the exit code will then contradict.
    $verdict = Merge-BoundIntoVerdict -Verdict $verdict -BoundMessage $boundMessage
    if ($killSurvived) {
        $verdict.Reason = ($verdict.Reason + (' !! AND THE KILL FAILED: PID {0} survived Stop-Process -Force.' -f $killedPid))
    }

    Show-Verdict -Verdict $verdict -AsPorcelain:$Porcelain
    return $verdict.Code
}

$exitCode = $EXIT_INTERNAL_ERROR
try {
    $exitCode = Invoke-Main
} catch {
    # W-11: StrictMode + $ErrorActionPreference='Stop' turn any slip into a TERMINATING
    # error. Without this the caller sees PowerShell's undocumented 1 and cannot tell it
    # from anything else. 70 is in the documented table.
    Write-Host ''
    Write-Host ('INTERNAL ERROR in run_suite_bounded.ps1: {0}' -f $_.Exception.Message) -ForegroundColor Red
    Write-Host $_.ScriptStackTrace -ForegroundColor DarkGray
    Write-Host ''
    Write-Host ('RUNNER_EXIT={0}' -f $EXIT_INTERNAL_ERROR)
    Write-Host 'RUNNER_INTERNAL_ERROR=1'
    $exitCode = $EXIT_INTERNAL_ERROR
}
exit $exitCode
