#!/usr/bin/env python3
"""List the public API of the Python wrapper, one declaration per line.

Copyright (c) 2026 Pumatronix

    api_python.py <path to src/wrappers/python/itscam>

Reads the sources (nothing is imported, so no native library is needed):
the names __init__ exports, and in every public module the functions,
classes, methods, class attributes (dataclass fields, enum members) and
module constants not named with a leading underscore.  Python 3.9+.
"""

import ast
import sys
from pathlib import Path


def signature(node):
    returns = " -> " + ast.unparse(node.returns) if node.returns else ""
    prefix = "async def" if isinstance(node, ast.AsyncFunctionDef) else "def"
    return "%s %s(%s)%s" % (prefix, node.name, ast.unparse(node.args), returns)


def public(name):
    return not name.startswith("_") or (name.startswith("__") and name.endswith("__"))


def class_lines(prefix, node):
    bases = ", ".join(ast.unparse(b) for b in node.bases)
    yield "%s: class %s(%s)" % (prefix, node.name, bases)
    for item in node.body:
        if isinstance(item, (ast.FunctionDef, ast.AsyncFunctionDef)) and public(item.name):
            decorators = "".join("@%s " % ast.unparse(d) for d in item.decorator_list)
            yield "%s.%s: %s%s" % (prefix, item.name, decorators, signature(item))
        elif isinstance(item, ast.AnnAssign) and isinstance(item.target, ast.Name) \
                and public(item.target.id):
            value = " = " + ast.unparse(item.value) if item.value is not None else ""
            yield "%s.%s: %s%s" % (prefix, item.target.id, ast.unparse(item.annotation), value)
        elif isinstance(item, ast.Assign):
            for target in item.targets:
                if isinstance(target, ast.Name) and public(target.id):
                    yield "%s.%s = %s" % (prefix, target.id, ast.unparse(item.value))
        elif isinstance(item, ast.ClassDef) and public(item.name):
            yield from class_lines("%s.%s" % (prefix, item.name), item)


def module_lines(module, tree):
    for node in tree.body:
        if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)) and public(node.name):
            yield "%s.%s: %s" % (module, node.name, signature(node))
        elif isinstance(node, ast.ClassDef) and public(node.name):
            yield from class_lines("%s.%s" % (module, node.name), node)
        elif isinstance(node, ast.Assign):
            for target in node.targets:
                if isinstance(target, ast.Name) and target.id == "__all__":
                    names = sorted(ast.literal_eval(node.value))
                    for name in names:
                        yield "%s.__all__: %s" % (module, name)
                elif isinstance(target, ast.Name) and target.id.isupper():
                    yield "%s.%s = %s" % (module, target.id, ast.unparse(node.value))


def main():
    package = Path(sys.argv[1])
    lines = []
    for path in sorted(package.glob("*.py")):
        if path.name.startswith("_") and path.name != "__init__.py":
            continue
        module = package.name if path.name == "__init__.py" else "%s.%s" % (package.name, path.stem)
        tree = ast.parse(path.read_text(encoding="utf-8"), str(path))
        lines.extend(module_lines(module, tree))
    for line in sorted(set(lines)):
        print(line)


if __name__ == "__main__":
    main()
