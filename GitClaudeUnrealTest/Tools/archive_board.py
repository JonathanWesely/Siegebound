r"""archive_board.py - move finished TASKBOARD.md work into .claude/pipeline/archive/.

WHY (2026-10-04, Jonathan's rule change): the live board had grown to 43,560 lines / 12.7 MB,
which no agent can read end to end and every dispatch pays to grep. The board's write
discipline exists largely because of that size. A terminal row is history, not work.

TWO PASSES, BOTH BYTE-PRESERVING (rows move; they are never edited):
  1. SECTIONS  - a `## ` section whose EVERY task row is terminal moves whole into
                 archive/TASKBOARD-NNN-<slug>.md.
  2. ROWS      - inside the sections that stay (because at least one row is live), every
                 TERMINAL row (its `#### TASK-###` heading through the line before the next
                 `##`/`###`/`####` heading) moves into archive/TASKBOARD-rows-NNN-<slug>.md,
                 and ONE note line is left under the section heading naming the moved ids
                 and the file. Live rows, manager decisions and checkpoints stay in place.

TERMINAL = the first word of the row's `- status:` line (after stripping emoji, backticks and
asterisks) is one of TERMINAL below. One live row keeps a SECTION live (pass 1) but does not
protect its terminal neighbours (pass 2). A row without a status line is never moved.

A TASK ROW is a `###` / `####` heading whose text begins with `TASK-###` (an optional `~~` strike
in front is allowed). A heading with anything else before the id (an emoji, a word) is NOT a row:
it bounds the row above it and stays on the board. None exists on the board or in the archive
today; this is the rule, not an accident.

Head sections are never touched (Status flow, WRITE DISCIPLINE, Task template, Milestones,
Active tasks, Archive). The board has ONE `## Archive` section with the lookup rule: the tool
rewrites that section's `Last run ...` line IN PLACE and creates the section only when absent.

RE-RUN SAFE (TASK-1605; law SC-§143 cl. 3):
  * archive numbers CONTINUE from the highest NNN per prefix (TASKBOARD-NNN-..., TASKBOARD-rows-NNN-...)
    that archive/INDEX.md lists - the register of what the tool itself has written, regenerated from
    the disk on every successful run (INDEX.md absent: the highest on disk) - so a later run never
    writes a number that is on disk;
  * a target path that nonetheless EXISTS on disk is a file the register does not know (a crashed
    run, a hand-copied file): it is refused BEFORE any write - `SANITY FAIL: would overwrite <path>`,
    exit 2, zero bytes written anywhere - and a human decides;
  * archive/INDEX.md is regenerated from a SCAN of the archive dir on every run: every
    TASKBOARD-*.md present is listed, sorted by filename;
  * the moved bytes (sha256 and bytes) are compared with what is READ BACK from each archive
    file after it is written, and the ids in the read-back are matched at heading level
    (TASK_RE, never a substring); TASKBOARD.md is written LAST, only after that read-back
    matched, so an exit 2 leaves the board untouched (the files this run wrote are removed);
  * the new board is checked as a LINE MULTISET against the old one
    (original + added == new + moved + removed), so a same-length corruption cannot pass.

USAGE (Python 3.7+; allow-listed interpreter: C:\Python314\python.exe)
  python Tools/archive_board.py                 # plan both passes, write nothing
  python Tools/archive_board.py --apply         # run pass 1 only
  python Tools/archive_board.py --apply --rows  # run pass 1 then pass 2

EXIT CODES: 0 ok - 1 nothing to archive (also `--apply` without `--rows` when pass 1 finds nothing)
            2 sanity check failed (the board is never changed on a 2) - 64 usage
"""
import re, sys
if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')  # Windows console is cp1252; headings carry em dashes and emoji
import hashlib, datetime
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BOARD = ROOT / ".claude" / "pipeline" / "TASKBOARD.md"
ARCHIVE_DIR = ROOT / ".claude" / "pipeline" / "archive"
INDEX = ARCHIVE_DIR / "INDEX.md"

KEEP_HEADINGS = ("Status flow", "Task template", "Milestones", "Active tasks", "Archive",
                 "⛔⛔ WRITE DISCIPLINE", "WRITE DISCIPLINE")
TERMINAL = {"done", "done/integrated", "integrated", "closed", "closed-by-reconcile", "superseded",
            "rejected", "cancelled", "canceled", "withdrawn", "retired", "obsolete", "struck",
            "shipped", "committed"}

TASK_RE = re.compile(r"^#{3,4}\s+(?:~~)?TASK-(\d{1,4})\b")
HEADING_RE = re.compile(r"^#{2,4}\s")
STATUS_RE = re.compile(r"^\s*-\s*\*{0,2}status\*{0,2}\s*:\s*(.*)$", re.IGNORECASE)
FIRST_WORD_RE = re.compile(r"[A-Za-z][A-Za-z/_-]*")
WHOLE_FILE_RE = re.compile(r"^TASKBOARD-(\d{3})-.*\.md$")
ROWS_FILE_RE = re.compile(r"^TASKBOARD-rows-(\d{3})-.*\.md$")
ARCHIVED_MARK = "<!-- ARCHIVED"
HDR_SECTION_RE = re.compile(r'section "(.*)" on \d{4}-\d{2}-\d{2} by Tools/archive_board\.py')
LAST_RUN_RE = re.compile(r"^Last run \d{4}-\d{2}-\d{2}:")
INDEX_ROW_RE = re.compile(r"^\| `([^`]+)` \| (.*) \| (whole section|terminal rows from a live section) \| (\d+) \| (.*) \|$")
KIND_WHOLE, KIND_ROWS = "whole section", "terminal rows from a live section"


def normalise_status(raw):
    s = re.sub(r"[`*~]", "", raw)
    m = FIRST_WORD_RE.search(s)
    return m.group(0).lower() if m else ""


def split_sections(lines):
    sections, cur, head, fence = [], [], None, False
    for ln in lines:
        s = ln.rstrip("\r\n")
        if s.startswith("```"):
            fence = not fence
        if not fence and s.startswith("## "):
            sections.append((head, cur))
            head, cur = s[3:].strip(), [ln]
            continue
        cur.append(ln)
    sections.append((head, cur))
    return sections


def rows_in(body):
    """Return list of (start, end, task_id, status_word_or_None) for each task row in a section body."""
    fence = False
    heads = []  # (index, is_task, task_id)
    for i, ln in enumerate(body):
        s = ln.rstrip("\r\n")
        if s.startswith("```"):
            fence = not fence
        if fence or i == 0:
            continue
        if HEADING_RE.match(s):
            m = TASK_RE.match(s)
            heads.append((i, bool(m), int(m.group(1)) if m else None))
    rows = []
    for k, (i, is_task, tid) in enumerate(heads):
        if not is_task:
            continue
        end = heads[k + 1][0] if k + 1 < len(heads) else len(body)
        status = None
        for ln in body[i:end]:
            sm = STATUS_RE.match(ln.rstrip("\r\n"))
            if sm:
                status = normalise_status(sm.group(1)); break
        rows.append((i, end, tid, status))
    return rows


def ids_in(lines):
    """Heading-level TASK ids: TASK_RE on every ###/#### line outside code fences, in order, duplicates kept."""
    fence, ids = False, []
    for ln in lines:
        s = ln.rstrip("\r\n")
        if s.startswith("```"):
            fence = not fence; continue
        if fence:
            continue
        m = TASK_RE.match(s)
        if m:
            ids.append(int(m.group(1)))
    return ids


def slug(heading, n, prefix):
    base = re.sub(r"[^A-Za-z0-9]+", "-", heading).strip("-")
    base = re.sub(r"-{2,}", "-", base)[:60].strip("-") or "section"
    return f"{prefix}-{n:03d}-{base}.md"


def sha(b):
    return hashlib.sha256(b).hexdigest()


def first_diff(a, b):
    """Offset of the first differing byte of two byte strings (len of the shorter when one is a prefix)."""
    n = min(len(a), len(b))
    for i in range(n):
        if a[i] != b[i]:
            return i
    return n


# ---- archive dir: numbering, headers, scan ---------------------------------------------------------

def next_numbers():
    """(next whole-section NNN, next rows NNN, source): one above the highest NNN per prefix that INDEX.md
    lists - the register of what the tool has written. INDEX.md absent => the highest on disk. A file that
    exists on disk at a computed name is therefore one the register does not know; the overwrite refusal
    in main() is what catches it (and is reachable only because the register is not the raw listing)."""
    if INDEX.is_file():
        names, source = re.findall(r"`(TASKBOARD-[^`]+\.md)`", INDEX.read_bytes().decode("utf-8")), "INDEX.md"
    else:
        names, source = ([f.name for f in ARCHIVE_DIR.iterdir()] if ARCHIVE_DIR.is_dir() else []), "disk (no INDEX.md)"
    hi_w = hi_r = 0
    for n in names:
        m = ROWS_FILE_RE.match(n)
        if m:
            hi_r = max(hi_r, int(m.group(1))); continue
        m = WHOLE_FILE_RE.match(n)
        if m:
            hi_w = max(hi_w, int(m.group(1)))
    return hi_w + 1, hi_r + 1, source


def archive_header(heading, today, nl):
    """The one-line comment that heads every archive file; it names the section the bytes came from."""
    safe = heading.replace("-->", "-- >")
    return (f'{ARCHIVED_MARK} from .claude/pipeline/TASKBOARD.md section "{safe}" on {today} by Tools/archive_board.py. '
            f"Every row below was in a terminal state when moved; bytes are unchanged and this comment is the only "
            f"addition. Law: CONVENTIONS.md SC-§143 cl. 3. -->{nl}")


def split_header(data):
    """(header line or '', body text) of an archive file's text."""
    if data.startswith(ARCHIVED_MARK):
        i = data.find("\n")
        return (data[:i + 1], data[i + 1:]) if i >= 0 else (data, "")
    return "", data


def legacy_index_sections():
    """{filename: from-section} parsed from the INDEX.md on disk - the only record of the section a rows
    file written before TASK-1605 came from (files written since carry it in their header comment)."""
    out = {}
    if INDEX.is_file():
        for ln in INDEX.read_bytes().decode("utf-8").splitlines():
            m = INDEX_ROW_RE.match(ln)
            if m:
                out[m.group(1)] = m.group(2).replace("\\|", "|")
    return out


def index_entry(fname, data, legacy_from):
    """(fname, from-section, kind, ids) for one archive file, from its text alone (plus the legacy index)."""
    hdr, body = split_header(data)
    lines = body.splitlines(keepends=True)
    kind = KIND_ROWS if ROWS_FILE_RE.match(fname) else KIND_WHOLE
    m = HDR_SECTION_RE.search(hdr)
    if kind == KIND_WHOLE and lines and lines[0].startswith("## "):
        frm = lines[0][3:].strip()
    elif m:
        frm = m.group(1)
    else:
        frm = legacy_from.get(fname, "")
    return fname, frm, kind, ids_in(lines)


def scan_archive(legacy_from, extra=None):
    """Index entries for every TASKBOARD-*.md in ARCHIVE_DIR plus `extra` {fname: text} (files not yet
    on disk), sorted by filename - the order INDEX.md lists them in."""
    texts = {}
    if ARCHIVE_DIR.is_dir():
        for f in ARCHIVE_DIR.glob("TASKBOARD-*.md"):
            texts[f.name] = f.read_bytes().decode("utf-8")
    for k, v in (extra or {}).items():
        texts[k] = v
    return [index_entry(fn, texts[fn], legacy_from) for fn in sorted(texts)]


def index_text(entries, today, nl):
    idx = [f"# TASKBOARD archive index{nl}{nl}",
           f"Regenerated {today} by `Tools/archive_board.py` from a scan of this directory: every `TASKBOARD-*.md` present "
           f"is listed, sorted by filename, on every run. Each file is a byte-for-byte move out of `.claude/pipeline/TASKBOARD.md` "
           f"(its one-line header comment is the only addition); file numbers continue from the highest listed here and a file "
           f"on disk is never overwritten. Lookup: `grep -rn \"TASK-###\" .claude/pipeline/archive/`.{nl}{nl}",
           f"| File | From section | Kind | Rows | TASK ids |{nl}|---|---|---|---|---|{nl}"]
    for fname, head, kind, ids in entries:
        safe = head.replace("|", "\\|")[:120]
        shown = ", ".join(f"TASK-{t}" for t in ids[:12]) + (f" … (+{len(ids)-12})" if len(ids) > 12 else "")
        idx.append(f"| `{fname}` | {safe} | {kind} | {len(ids)} | {shown} |{nl}")
    return "".join(idx)


# ---- the board's ## Archive section ----------------------------------------------------------------

def archive_section_lines(last_run, nl):
    """The canonical `## Archive` section, used only when the board has none."""
    return [
        f"## Archive{nl}", nl,
        f"Finished work lives in `.claude/pipeline/archive/` (index: `.claude/pipeline/archive/INDEX.md`, regenerated from a scan "
        f"of that directory on every run). `python Tools/archive_board.py --apply --rows` moves (1) every `## ` section whose rows "
        f"are all terminal, whole, and (2) every terminal row out of a section that still has live rows, leaving one italic note "
        f"under that section's heading naming the moved ids and the file. Rows move byte-for-byte and are never edited afterward; "
        f"a terminal row is read, never re-opened. Archive file numbers continue from the highest the index lists and a file on "
        f"disk is never overwritten. Run it at every milestone checkpoint (`SC-§143` cl. 3).{nl}", nl,
        f"**Lookup rule:** a `TASK-###` that is not in this file is looked up with "
        f"`grep -rn \"TASK-###\" .claude/pipeline/archive/` before anyone concludes it does not exist; a `gate:` or `blocked-by:` "
        f"pointer to an archived row resolves there. `git log --oneline --grep=TASK-###` (anchored at the git root, one level up) "
        f"remains the authority on whether it was committed.{nl}", nl,
        last_run, nl,
    ]


def refresh_archive_section(body, last_run, nl):
    """Rewrite the `Last run ...` line of an existing `## Archive` body in place (add one when absent).
    Returns (new_body, removed_lines, added_lines)."""
    for i, ln in enumerate(body):
        if LAST_RUN_RE.match(ln.rstrip("\r\n")):
            return body[:i] + [last_run] + body[i + 1:], [ln], [last_run]
    j = len(body)
    while j > 1 and body[j - 1].strip() == "":
        j -= 1
    return body[:j] + [nl, last_run] + body[j:], [], [nl, last_run]


# ---- sanity checks (each callable on its own, so a driver can prove it goes red) --------------------

def check_conservation(orig_lines, new_lines, moved, added, removed):
    """Line-multiset identity of the board edit: original + added == new + moved + removed.
    Returns None when it holds, else a message naming the count and the first line that is off."""
    lhs, rhs = Counter(orig_lines), Counter(new_lines)
    lhs.update(added); rhs.update(moved); rhs.update(removed)
    if lhs == rhs:
        return None
    off = (lhs - rhs) + (rhs - lhs)
    first = next(iter(off))
    return (f"line multiset differs by {sum(off.values())} line(s); first: {first.rstrip()[:100]!r} "
            f"(original {len(orig_lines)} + added {len(added)} vs new {len(new_lines)} + moved {len(moved)} + removed {len(removed)})")


def check_readback(path, moved_text, ids):
    """READ BACK an archive file and compare it with the bytes that were moved: the header comment is
    stripped, then sha256 and bytes must match and the heading-level ids (TASK_RE) must equal `ids`.
    Returns None when everything matches, else a message naming the file and the first differing byte."""
    data = path.read_bytes().decode("utf-8")
    _, body = split_header(data)
    exp, got = moved_text.encode("utf-8"), body.encode("utf-8")
    if sha(exp) != sha(got) or exp != got:
        return (f"{path.name}: read-back differs from the moved bytes at byte {first_diff(exp, got)} "
                f"(moved {len(exp)} B sha256 {sha(exp)[:16]}, read {len(got)} B sha256 {sha(got)[:16]})")
    got_ids = ids_in(body.splitlines(keepends=True))
    if got_ids != list(ids):
        return f"{path.name}: heading-level ids read back {got_ids} != moved {list(ids)}"
    return None


def count_archive_sections(lines):
    return sum(1 for h, _ in split_sections(lines) if h is not None and h.startswith("Archive"))


def remove_written(paths):
    for p in paths:
        try:
            p.unlink(); print(f"  removed {p}")
        except OSError as e:
            print(f"  could not remove {p}: {e}")


def main(argv):
    flags = set(argv[1:])
    if flags - {"--apply", "--rows", "--dry-run"}:
        print(__doc__); return 64
    apply, do_rows = "--apply" in flags, "--rows" in flags
    text = BOARD.read_bytes().decode("utf-8")
    nl = "\r\n" if "\r\n" in text else "\n"
    lines = text.splitlines(keepends=True)
    sections = split_sections(lines)
    today = datetime.date.today().isoformat()

    # ---- pass 1: whole sections -------------------------------------------------------------
    whole, rest = [], []
    seen_task_section = False
    for head, body in sections:
        if head is None:
            rest.append((head, body)); continue
        rows = rows_in(body)
        is_head = any(head.startswith(k) for k in KEEP_HEADINGS)
        if rows:
            seen_task_section = True
        ok = (bool(rows) and not is_head and seen_task_section
              and all(st in TERMINAL for *_, st in rows) and all(st is not None for *_, st in rows))
        (whole if ok else rest).append((head, body))

    # ---- pass 2: terminal rows inside the remaining sections ----------------------------------
    row_moves = []  # (head, [(start,end,tid)], body)
    for head, body in rest:
        if head is None or any(head.startswith(k) for k in KEEP_HEADINGS):
            continue
        rows = rows_in(body)
        term = [(a, b, tid) for a, b, tid, st in rows if st in TERMINAL]
        if term:
            row_moves.append((head, term, body))

    # ---- plan: filenames continue from the highest NNN on disk, per prefix ----------------------
    n_whole0, n_rows0, n_source = next_numbers()
    whole_plan = [(slug(head, n_whole0 + k, "TASKBOARD"), head, body) for k, (head, body) in enumerate(whole)]
    rows_plan = [(slug(head, n_rows0 + k, "TASKBOARD-rows"), head, term, body) for k, (head, term, body) in enumerate(row_moves)]

    total_whole = sum(len(b) for _, b in whole)
    total_rows = sum(b - a for _, term, _ in row_moves for a, b, _ in term)
    n_rows_whole = sum(len(rows_in(b)) for _, b in whole)
    n_rows_rows = sum(len(t) for _, t, _ in row_moves)
    print(f"board: {len(lines)} lines, {len(sections)} sections")
    print(f"archive dir: next numbers TASKBOARD-{n_whole0:03d} / TASKBOARD-rows-{n_rows0:03d} (register: {n_source})")
    print(f"pass 1 (sections): {len(whole)} sections, {total_whole} lines, {n_rows_whole} rows")
    print(f"pass 2 (rows):     {len(row_moves)} sections touched, {n_rows_rows} terminal rows, {total_rows} lines")
    print(f"remaining after both: ~{len(lines) - total_whole - total_rows} lines")
    for fname, head, body in whole_plan:
        ids = [t for *_, t, _ in rows_in(body)]
        print(f"  section: {fname}  rows={len(ids)} TASK-{min(ids):04d}..TASK-{max(ids):04d} {len(body)} lines")
    for fname, head, term, body in rows_plan:
        ids = [t for *_, t in term]
        live = len(rows_in(body)) - len(term)
        print(f"  rows   : {fname}  move={len(ids)} keep-live={live} TASK-{min(ids):04d}..TASK-{max(ids):04d}")
    if not whole and not row_moves:
        print("nothing archivable"); return 1
    if not apply:
        print("\n(dry run; --apply runs pass 1, --apply --rows runs both)"); return 0
    if not whole and not do_rows:
        print("pass 1 found nothing; pass --rows for pass 2"); return 1

    # ---- build everything in memory first ---------------------------------------------------------
    new_files = {}       # fname -> (header, moved_text, ids)
    moved_lines, added, removed = [], [], []
    for fname, head, body in whole_plan:
        new_files[fname] = (archive_header(head, today, nl), "".join(body), ids_in(body))
        moved_lines.extend(body)

    out_sections = []    # (head, body) of the new board, in order
    for head, body in rest:
        if head is None or any(head.startswith(k) for k in KEEP_HEADINGS):
            out_sections.append((head, body)); continue
        plan = next((p for p in rows_plan if p[3] is body), None) if do_rows else None
        if plan is None:
            out_sections.append((head, body)); continue
        fname, _, term, _ = plan
        moved_text = "".join("".join(body[a:b]) for a, b, _ in term)
        ids = [t for *_, t in term]
        new_files[fname] = (archive_header(head, today, nl), moved_text, ids)
        keep_mask = [True] * len(body)
        for a, b, _ in term:
            for i in range(a, b):
                keep_mask[i] = False
            moved_lines.extend(body[a:b])
        kept = [ln for ln, k in zip(body, keep_mask) if k]
        id_list = ", ".join(f"TASK-{t}" for t in ids)
        note = (f"_Archive note ({today}): {len(ids)} terminal row(s) moved by `Tools/archive_board.py --rows` to "
                f"`.claude/pipeline/archive/{fname}` — {id_list}. Bytes unchanged; look them up there._{nl}")
        ins = 2 if (len(kept) > 1 and kept[1].strip() == "") else 1   # after the heading (and its blank line)
        kept = kept[:ins] + [note, nl] + kept[ins:]
        added.extend([note, nl])
        out_sections.append((head, kept))

    # refuse to overwrite - BEFORE anything is written anywhere
    clash = [ARCHIVE_DIR / f for f in new_files if (ARCHIVE_DIR / f).exists()]
    if clash:
        for p in clash:
            print(f"SANITY FAIL: would overwrite {p}")
        return 2

    # INDEX from a scan of the dir (plus this run's files, still in memory) and the Archive section's Last-run line
    legacy = legacy_index_sections()
    entries = scan_archive(legacy, {f: h + t for f, (h, t, _) in new_files.items()})
    idx_text = index_text(entries, today, nl)
    all_ids = [i for *_, ids in entries for i in ids]
    span = f"TASK-{min(all_ids):04d}..TASK-{max(all_ids):04d}" if all_ids else "no rows"
    last_run = (f"Last run {today}: the archive holds {len(entries)} files, {len(all_ids)} rows ({span}); "
                f"this run added {len(new_files)} file(s).{nl}")

    has_archive = any(h is not None and h.startswith("Archive") for h, _ in out_sections)
    out, refreshed, inserted = [], False, False
    for head, body in out_sections:
        if head is not None and head.startswith("Archive") and not refreshed:
            body, rem, add = refresh_archive_section(body, last_run, nl)
            removed.extend(rem); added.extend(add); refreshed = True
        out.extend(body)
        if not has_archive and not inserted and head is not None and head.startswith("Task template"):
            if out and not out[-1].endswith(nl):
                out.append(nl); added.append(nl)
            sec = archive_section_lines(last_run, nl)
            out.extend(sec); added.extend(sec); inserted = True
    if not has_archive and not inserted:
        if out and not out[-1].endswith(nl):
            out.append(nl); added.append(nl)
        sec = archive_section_lines(last_run, nl)
        out.extend(sec); added.extend(sec)
    new_text = "".join(out)

    # ---- in-memory checks (nothing written yet) ---------------------------------------------------
    moved_ids = set(i for *_, ids in new_files.values() for i in ids)
    board_ids = set(ids_in(out))
    dup = sorted(moved_ids & board_ids)
    if dup:
        # The board carries a few ids twice (re-boarded rows). One copy moved, the other is live: allowed, reported.
        print(f"NOTE: {len(dup)} id(s) still have another (live) heading on the board: {dup}")
    msg = check_conservation(lines, out, moved_lines, added, removed)
    if msg:
        print(f"SANITY FAIL: {msg}"); return 2
    n_arch = count_archive_sections(out)
    if n_arch != 1:
        print(f"SANITY FAIL: the new board would hold {n_arch} '## Archive' sections, not 1"); return 2

    # ---- write the archive files, READ THEM BACK, then INDEX, then the board LAST -------------------
    ARCHIVE_DIR.mkdir(exist_ok=True)
    written = []
    for fname, (h, t, _) in new_files.items():
        p = ARCHIVE_DIR / fname
        p.write_bytes((h + t).encode("utf-8")); written.append(p)
    for fname, (_, t, ids) in new_files.items():
        msg = check_readback(ARCHIVE_DIR / fname, t, ids)
        if msg:
            print(f"SANITY FAIL: {msg}"); remove_written(written); return 2
        print(f"  ok: {fname}  sha256(moved) == sha256(read back) == {sha(t.encode('utf-8'))[:16]}  rows={len(ids)}")

    disk_idx = index_text(scan_archive(legacy), today, nl)
    if disk_idx != idx_text:
        a, b = idx_text.encode("utf-8"), disk_idx.encode("utf-8")
        print(f"SANITY FAIL: INDEX from the disk scan differs from the in-memory build at byte {first_diff(a, b)}")
        remove_written(written); return 2
    old_index = INDEX.read_bytes() if INDEX.is_file() else None
    INDEX.write_bytes(idx_text.encode("utf-8"))
    if INDEX.read_bytes() != idx_text.encode("utf-8"):
        print("SANITY FAIL: INDEX.md read-back differs from what was written")
        if old_index is not None:
            INDEX.write_bytes(old_index); print("  restored the previous INDEX.md")
        remove_written(written); return 2

    BOARD.write_bytes(new_text.encode("utf-8"))
    rb = BOARD.read_bytes()
    if sha(rb) != sha(new_text.encode("utf-8")):
        print(f"SANITY FAIL: board read-back sha256 {sha(rb)[:16]} != written {sha(new_text.encode('utf-8'))[:16]}"); return 2
    print(f"\napplied: {len(new_files)} new archive file(s); INDEX lists {len(entries)} files; board now {len(out)} lines "
          f"({len(rb)} bytes); sha256 {sha(rb)[:16]}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
