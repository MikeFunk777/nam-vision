#!/usr/bin/env python3

import re
import sys
from pathlib import Path


SCRIPT_PATH = Path(__file__).resolve().parent
PROJECT_PATH = SCRIPT_PATH.parent
CONFIG_PATH = PROJECT_PATH / "config.h"
RESOURCE_PATH = PROJECT_PATH / "resources"
RC_PATH = RESOURCE_PATH / "main.rc"

PD_RESOURCE_DEFINE = re.compile(
    r'^#define\s+(PD_[A-Z0-9_]+_FN)\s+"([^"]+)"', re.MULTILINE
)
RESOURCE_TYPES = {
    ".jpg": "JPG",
    ".png": "PNG",
    ".svg": "SVG",
    ".ttf": "TTF",
}


def main():
    config = CONFIG_PATH.read_text(encoding="utf-8")
    rc = RC_PATH.read_text(encoding="utf-8")
    resources = PD_RESOURCE_DEFINE.findall(config)
    errors = []

    if not resources:
        errors.append("No PD resource definitions were found in config.h")

    for macro, relative_path in resources:
        source = RESOURCE_PATH / relative_path
        resource_type = RESOURCE_TYPES.get(source.suffix.lower())

        if resource_type is None:
            errors.append(f"Unsupported resource type for {macro}: {relative_path}")
            continue

        if not source.is_file():
            errors.append(f"Missing resource file for {macro}: {source}")

        declaration = f"{macro} {resource_type} {macro}"
        if rc.count(declaration) < 2:
            errors.append(
                f"{macro} must appear in both main.rc resource sections as: {declaration}"
            )

    if errors:
        for error in errors:
            print(f"ERROR: {error}", file=sys.stderr)
        raise SystemExit(1)

    print(f"Validated {len(resources)} Pedal Division Windows resources")


if __name__ == "__main__":
    main()
