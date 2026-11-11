#!/usr/bin/env python3
"""List the public API of the Node.js wrapper, one declaration per line.

Copyright (c) 2026 Pumatronix

    api_dts.py <path to src/wrappers/nodejs/types/index.d.ts>

The typings are the wrapper's contract: every statement, with comments
dropped and whitespace collapsed, prefixed by the declarations enclosing it
(module, class, interface, namespace, enum).  Python 3.6+.
"""

import re
import sys


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    return re.sub(r"//[^\n]*", " ", text)


def normalize(statement):
    return re.sub(r"\s+", " ", statement).strip().rstrip(";,").strip()


def main():
    text = strip_comments(open(sys.argv[1], encoding="utf-8").read())
    lines = []
    scopes = []          # names of the enclosing declarations
    statement = ""
    # Braces of object types inside a signature are part of the statement,
    # not a scope: only a brace after a declaration header opens one
    header = re.compile(r"^(export\s+|declare\s+|default\s+|abstract\s+)*"
                        r"(class|interface|namespace|module|enum)\b")
    depth_in_statement = 0
    for ch in text:
        if ch == "{" and depth_in_statement == 0 and header.match(normalize(statement)):
            name = normalize(statement)
            lines.append(" > ".join(scopes + [name]))
            scopes.append(name)
            statement = ""
        elif ch == "{":
            depth_in_statement += 1
            statement += ch
        elif ch == "}" and depth_in_statement > 0:
            depth_in_statement -= 1
            statement += ch
        elif ch == "}":
            if normalize(statement):
                lines.append(" > ".join(scopes + [normalize(statement)]))
            statement = ""
            if scopes:
                scopes.pop()
        elif ch in ";," and depth_in_statement == 0 and scopes and scopes[-1].split()[-2:-1] == ["enum"]:
            if normalize(statement):
                lines.append(" > ".join(scopes + [normalize(statement)]))
            statement = ""
        elif ch == ";" and depth_in_statement == 0:
            if normalize(statement):
                lines.append(" > ".join(scopes + [normalize(statement)]))
            statement = ""
        else:
            statement += ch
    for line in sorted(set(lines)):
        print(line)


if __name__ == "__main__":
    main()
