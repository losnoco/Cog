#!/usr/bin/env python3
"""Regenerate Acknowledgements/Acknowledgements.json from components.json.

The generated file is the bundled resource read by the Acknowledgements
window. License texts are embedded once each and referenced by key, so a
license shared by many components (GPL, Apache, ...) ships only once.

Usage: Scripts/build-acknowledgements.py   (run from anywhere)
"""

import json
import re
import sys
import textwrap
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "Acknowledgements" / "components.json"
OUTPUT = ROOT / "Acknowledgements" / "Acknowledgements.json"


def read_text(path: Path) -> str:
    data = path.read_bytes()
    try:
        text = data.decode("utf-8")
    except UnicodeDecodeError:
        text = data.decode("latin-1")
    return text.replace("\r\n", "\n").replace("\r", "\n")


NOTICE = re.compile(r"copyright|licen[cs]e|public domain", re.IGNORECASE)
COMMENT = re.compile(r"/\*(.*?)\*/|((?:^[ \t]*//[^\n]*\n?)+)", re.DOTALL | re.MULTILINE)
NOISE = re.compile(r"^\s*(vim?:|\$Id)")


def license_comment(text: str) -> str:
    """Return the first comment block near the top of a source file that
    carries a copyright or license notice, without comment markers."""
    for match in COMMENT.finditer(text[:8000]):
        if match.group(1) is not None:
            lines = [re.sub(r"^\s*\*\s?", "", line) for line in match.group(1).split("\n")]
        else:
            lines = [re.sub(r"^\s*//\s?", "", line) for line in match.group(2).split("\n")]
        lines = [line.rstrip() for line in lines if not NOISE.match(line)]
        block = textwrap.dedent("\n".join(lines)).strip("\n")
        if NOTICE.search(block):
            return block
    raise ValueError("no comment with a copyright or license notice near the top")


def resolve(source: str, shared: dict) -> str:
    kind, _, value = source.partition(":")
    if kind == "shared":
        if value not in shared:
            raise ValueError(f"unknown shared license '{value}'")
        return read_text(ROOT / shared[value]).strip("\n")
    if kind == "file":
        return read_text(ROOT / value).strip("\n")
    if kind == "header":
        return license_comment(read_text(ROOT / value))
    raise ValueError(f"unknown text source '{source}'")


def main() -> int:
    manifest = json.loads(SOURCE.read_text(encoding="utf-8"))
    shared = manifest["shared"]
    group_ids = set(manifest["groups"])

    licenses = {}
    components = []
    errors = []
    for component in manifest["components"]:
        name = component["name"]
        if component["group"] not in group_ids:
            errors.append(f"{name}: unknown group '{component['group']}'")
        keys = []
        for source in component.get("texts", []):
            if source not in licenses:
                try:
                    licenses[source] = resolve(source, shared)
                except (OSError, ValueError) as error:
                    errors.append(f"{name}: {source}: {error}")
                    continue
            keys.append(source)
        entry = {key: component[key] for key in ("group", "name", "version", "url", "license", "holders", "note") if component.get(key)}
        entry["texts"] = keys
        components.append(entry)

    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1

    output = {"groups": manifest["groups"], "licenses": licenses, "components": components}
    OUTPUT.write_text(json.dumps(output, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"Wrote {OUTPUT.relative_to(ROOT)}: {len(components)} components, {len(licenses)} license texts")
    return 0


if __name__ == "__main__":
    sys.exit(main())
