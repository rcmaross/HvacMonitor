#!/usr/bin/env python3

import argparse
import re
import subprocess
import sys
import time
from pathlib import Path


PROJECT_DIR = Path(__file__).resolve().parent.parent
ENVIRONMENT = "m5stack-core2"

HOST_PREFIX = "hvac-monitor-"
DISCOVERY_SECONDS = 5

PLATFORMIO = Path.home() / ".platformio" / "penv" / "bin" / "pio"


def run(command, check=True, capture=False):
    print()
    print("$", " ".join(str(arg) for arg in command))
    print()

    return subprocess.run(
        command,
        cwd=PROJECT_DIR,
        check=check,
        text=True,
        capture_output=capture
    )


def build():
    print("Building firmware...")

    result = run(
        [
            str(PLATFORMIO),
            "run",
            "-e",
            ENVIRONMENT
        ],
        check=False
    )

    if result.returncode != 0:
        print("Build failed.")
        sys.exit(result.returncode)


def discover_hosts():
    print()
    print(f"Searching for {HOST_PREFIX}* devices for {DISCOVERY_SECONDS} seconds...")

    process = subprocess.Popen(
        [
            "dns-sd",
            "-B",
            "_arduino._tcp",
            "local"
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True
    )

    try:
        output, _ = process.communicate(timeout=DISCOVERY_SECONDS)

    except subprocess.TimeoutExpired:
        process.terminate()

        try:
            output, _ = process.communicate(timeout=1)
        except subprocess.TimeoutExpired:
            process.kill()
            output, _ = process.communicate()

    hosts = set()

    for line in output.splitlines():
        match = re.search(
            r"(hvac-monitor-[A-Za-z0-9_-]+)\s*$",
            line
        )

        if match:
            hosts.add(match.group(1) + ".local")

    return sorted(hosts)

def upload(host):
    print()
    print("=" * 60)
    print(f"Uploading to {host}")
    print("=" * 60)

    result = run(
        [
            str(PLATFORMIO),
            "run",
            "-e",
            ENVIRONMENT,
            "-t",
            "upload",
            "--upload-port",
            host
        ],
        check=False
    )

    if result.returncode != 0:
        print()
        print(f"FAILED: {host}")
        return False

    print()
    print(f"SUCCESS: {host}")

    return True


def main():
    if not PLATFORMIO.exists():
        print(f"PlatformIO not found at:")
        print(f"  {PLATFORMIO}")
        sys.exit(1)

    parser = argparse.ArgumentParser(
        description="Upload HVAC Monitor firmware via OTA."
    )

    parser.add_argument(
        "hostname",
        nargs="?",
        help="Exact OTA hostname, for example hvac-monitor-A3F921.local"
    )

    args = parser.parse_args()

    build()

    if args.hostname:
        hosts = [args.hostname]

    else:
        hosts = discover_hosts()

        if not hosts:
            print()
            print("No HVAC Monitor OTA devices found.")
            sys.exit(1)

        print()
        print("Found HVAC Monitor devices:")

        for host in hosts:
            print(f"  {host}")

    failures = []

    for host in hosts:
        if not upload(host):
            failures.append(host)

    print()
    print("=" * 60)

    if failures:
        print("OTA update completed with failures:")

        for host in failures:
            print(f"  {host}")

        sys.exit(1)

    print(f"OTA update successful on {len(hosts)} device(s).")


if __name__ == "__main__":
    main()
