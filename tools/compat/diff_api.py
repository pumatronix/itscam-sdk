#!/usr/bin/env python3
"""Compare two API listings: what the baseline had and the change lacks.

Copyright (c) 2026 Pumatronix

    diff_api.py <surface> <baseline listing> <current listing> <accepted file>

A line of the baseline missing from the current listing is a removed or
changed declaration: a break, unless the accepted file lists it as
"<surface> <line>" (compat/accepted-api-breaks.txt).  Added lines are
reported, never fail.  Exits 1 on an unaccepted break.  Python 3.6+.
"""

import sys


def read(path):
    with open(path, encoding="utf-8") as f:
        return set(l.rstrip("\n") for l in f if l.strip())


def main():
    surface, baseline_path, current_path, accepted_path = sys.argv[1:5]
    baseline, current = read(baseline_path), read(current_path)
    accepted = set()
    for line in read(accepted_path):
        if not line.startswith("#") and line.startswith(surface + " "):
            accepted.add(line[len(surface) + 1:])

    if not baseline:
        print("%s: FAIL the baseline listing is empty" % surface)
        return 1
    removed = sorted(baseline - current)
    added = sorted(current - baseline)
    breaks = [l for l in removed if l not in accepted]
    stale = sorted(a for a in accepted if a not in removed)

    for l in breaks:
        print("  - %s" % l)
    for l in removed:
        if l in accepted:
            print("  - %s   (accepted)" % l)
    for l in added:
        print("  + %s" % l)
    for l in stale:
        print("  ? accepted but not removed: %s" % l)

    status = "FAIL" if breaks else "OK  "
    print("%s: %s %d removed or changed (%d accepted), %d added, %d declarations"
          % (surface, status, len(removed), len(removed) - len(breaks), len(added), len(current)))
    return 1 if breaks else 0


if __name__ == "__main__":
    sys.exit(main())
