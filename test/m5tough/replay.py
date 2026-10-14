#!/usr/bin/env python3
"""Send HVAC history CSV temperatures to the M5Tough over USB serial."""
import argparse
import csv
from datetime import datetime
import time

import serial


def timestamp(value):
    try:
        return datetime.fromisoformat(value.strip().replace("Z", "+00:00"))
    except (ValueError, AttributeError):
        return None


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("csv_file", help="HVAC Monitor history_v3.csv")
    p.add_argument("--port", required=True, help="e.g. /dev/cu.usbserial-XXXX")
    p.add_argument("--units", choices=["F", "C"], default="F",
                   help="Units in history CSV (default: F)")
    p.add_argument("--interval", type=float, default=10.0,
                   help="Fallback delay in seconds (default: 10)")
    p.add_argument("--speed", type=float, default=1.0,
                   help="Replay speed multiplier (default: 1)")
    args = p.parse_args()
    if args.interval < 0 or args.speed <= 0:
        p.error("--interval must be nonnegative and --speed positive")

    with open(args.csv_file, newline="") as stream:
        rows = csv.DictReader(stream)
        if not {"large", "small", "outside"}.issubset(rows.fieldnames or []):
            p.error("CSV requires large,small,outside columns")

        with serial.Serial(args.port, 115200, timeout=1) as port:
            time.sleep(2)  # USB serial opening may reset ESP32.
            port.reset_input_buffer()
            previous = None
            for row in rows:
                try:
                    temps = [float(row[name]) for name in ("large", "small", "outside")]
                except (ValueError, TypeError):
                    print("Skipping row with missing/invalid temperature:", row)
                    continue
                now = timestamp(row.get("timestamp", ""))
                if previous is not None:
                    gap = args.interval
                    if now is not None and previous is not None:
                        delta = (now - previous).total_seconds()
                        if delta >= 0:
                            gap = delta
                    time.sleep(gap / args.speed)

                if args.units == "C":
                    temps = [x * 1.8 + 32 for x in temps]
                command = "F," + ",".join(f"{v:.3f}" for v in temps) + "\n"
                port.write(command.encode("ascii"))
                port.flush()
                response = port.readline().decode("ascii", errors="replace").strip()
                print(row.get("timestamp", ""), command.strip(), response, flush=True)
                previous = now


if __name__ == "__main__":
    main()
