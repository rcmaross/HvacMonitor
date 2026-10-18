Import("env")
import os
import subprocess
from pathlib import Path

root = Path(env["PROJECT_DIR"])
out = root / "include" / "generated" / "build_info.h"
out.parent.mkdir(parents=True, exist_ok=True)

def git(*args):
    try:
        return subprocess.check_output(["git", *args], cwd=root, text=True, stderr=subprocess.DEVNULL).strip()
    except (OSError, subprocess.CalledProcessError):
        return ""

sha = os.environ.get("FIRMWARE_SHA") or git("rev-parse", "HEAD") or "unknown"
short = sha[:8]
base = os.environ.get("FIRMWARE_BASE_VERSION")
if not base:
    tags = git("tag", "--merged", "HEAD", "--list", "v[0-9]*.[0-9]*.[0-9]*").splitlines()
    import re
    valid = [t for t in tags if re.fullmatch(r"v\d+\.\d+\.\d+", t)]
    base = max(valid, key=lambda t: tuple(map(int, t[1:].split(".")))) if valid else "v26.0.0"
version = os.environ.get("FIRMWARE_VERSION") or f"{base}-dev.{short}"
kind = "Release" if version == base else "Development"
date = os.environ.get("FIRMWARE_BUILD_DATE") or __import__("datetime").datetime.now(__import__("datetime").timezone.utc).strftime("%Y-%m-%d %H:%M UTC")
def cstr(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'
out.write_text(
    "#pragma once\n"
    f"#define HVAC_FW_VERSION {cstr(version)}\n"
    f"#define HVAC_FW_SHA {cstr(sha)}\n"
    f"#define HVAC_FW_TYPE {cstr(kind)}\n"
    f"#define HVAC_FW_DATE {cstr(date)}\n"
)
print(f"Firmware build info: {version} ({sha})")
