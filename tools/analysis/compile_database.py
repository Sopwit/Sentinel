#!/usr/bin/env python3
"""Create an analysis-only compile database without compiler-specific PCH files."""
import argparse
import json
import shlex
import subprocess
import sys
from pathlib import Path


def without_pch(arguments):
    result = []
    index = 0
    while index < len(arguments):
        if arguments[index:index + 2] in (["-Xclang", "-include-pch"], ["-Xclang", "-include"]):
            if index + 3 < len(arguments) and arguments[index + 2] == "-Xclang" and "cmake_pch" in arguments[index + 3]:
                index += 4
                continue
        if arguments[index] == "-Winvalid-pch":
            index += 1
            continue
        result.append(arguments[index])
        index += 1
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    entries = json.loads(args.source.read_text())
    platform_arguments = []
    if sys.platform == "darwin":
        sdk = subprocess.check_output(["xcrun", "--show-sdk-path"], text=True).strip()
        compiler = Path(subprocess.check_output(["xcrun", "--find", "clang"], text=True).strip())
        standard_library = compiler.parent.parent / "include/c++/v1"
        platform_arguments = ["-isysroot", sdk, "-isystem", str(standard_library)]
    for entry in entries:
        arguments = entry.get("arguments", shlex.split(entry.get("command", "")))
        entry["arguments"] = without_pch(arguments) + platform_arguments
        entry.pop("command", None)
    args.destination.mkdir(parents=True, exist_ok=True)
    (args.destination / "compile_commands.json").write_text(json.dumps(entries, indent=2) + "\n")


if __name__ == "__main__":
    main()
