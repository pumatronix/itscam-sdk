#!/usr/bin/env python3
"""List the public API of the Java wrapper, one member per line.

Copyright (c) 2026 Pumatronix

    api_java.py <itscam-sdk.jar>

Runs `javap -public` on every public class of the jar (internal packages
included: JNA needs their declarations public, and applications can see
them), and prefixes each member with its class.  Python 3.6+, a JDK on PATH.
"""

import re
import subprocess
import sys
import zipfile


def main():
    jar = sys.argv[1]
    with zipfile.ZipFile(jar) as z:
        classes = sorted(n[:-6].replace("/", ".") for n in z.namelist()
                         if n.endswith(".class") and not n.startswith("META-INF/"))
    lines = []
    for start in range(0, len(classes), 200):
        out = subprocess.check_output(
            ["javap", "-public", "-cp", jar] + classes[start:start + 200],
            universal_newlines=True)
        current = None
        for raw in out.splitlines():
            line = re.sub(r"\s+", " ", raw).strip()
            if not line or line.startswith("Compiled from"):
                continue
            if line.endswith("{"):
                # javap prints the header of every class it's given:
                # anonymous and package-private ones aren't API
                current = line[:-1].strip() if line.startswith("public ") else None
                if current:
                    lines.append(current)
            elif line == "}":
                current = None
            elif current:
                lines.append("%s > %s" % (current.split(" class ")[-1].split(" interface ")[-1]
                                         .split(" enum ")[-1].split()[0], line.rstrip(";")))
    for line in sorted(set(lines)):
        print(line)


if __name__ == "__main__":
    main()
