
#!/usr/bin/env python3
"""Replay HVAC CSV or interactively set simulator temperatures over USB serial."""
import argparse
import csv
from datetime import datetime
import math
import select
import sys
import time

import serial


def timestamp(value):
    try:
        return datetime.fromisoformat(value.strip().replace("Z", "+00:00"))
    except (ValueError, AttributeError):
        return None


def send(port, temps):
    command = "F," + ",".join(f"{v:.3f}" for v in temps) + "\n"
    port.write(command.encode("ascii"))
    port.flush()
    response = port.readline().decode("ascii", errors="replace").strip()
    print(response or "(no response)", flush=True)


def manual(port):
    temps = [75.0, 75.0, 75.0]
    names = {"l": 0, "s": 1, "o": 2}
    print("Manual mode (F): l:75  s:95  o:70 | show | quit")
    print("Starting with all temperatures at 75 F.")
    send(port, temps)

    while True:
        print("temp> ", end="", flush=True)

        while True:
            ready, _, _ = select.select([sys.stdin], [], [], 10.0)
            if ready:
                command = sys.stdin.readline()
                if not command:
                    print()
                    return
                break

            print("\n(refresh)", end=" ", flush=True)
            send(port, temps)
            print("temp> ", end="", flush=True)

        command = command.strip().lower()

        if command in ("quit", "exit", "q"):
            return

        if command == "show":
            print(
                f"Large={temps[0]:.1f} F  "
                f"Small={temps[1]:.1f} F  "
                f"Outside={temps[2]:.1f} F"
            )
            continue

        if ":" not in command:
            print("Use l:75, s:95, o:70, show, or quit")
            continue

        key, value = (part.strip() for part in command.split(":", 1))

        if key not in names:
            print("Unknown sensor. Use l, s, or o.")
            continue

        try:
            number = float(value)
        except ValueError:
            print("Invalid temperature")
            continue

        if not math.isfinite(number) or not -20 <= number <= 200:
            print("Temperature must be between -20 and 200 F")
            continue

        temps[names[key]] = number
        send(port, temps)


def replay(port, args):
    with open(args.csv_file, newline="") as stream:
        rows = csv.DictReader(stream)

        if not {"large", "small", "outside"}.issubset(rows.fieldnames or []):
            raise ValueError("CSV requires large,small,outside columns")

        previous = None
        sent_count = 0

        for row in rows:
            try:
                temps = [
                    float(row[name])
                    for name in ("large", "small", "outside")
                ]
            except (ValueError, TypeError):
                print("Skipping row with missing/invalid temperature:", row)
                continue

            now = timestamp(row.get("timestamp", ""))

            if sent_count:
                gap = args.interval

                if now is not None and previous is not None:
                    try:
                        delta = (now - previous).total_seconds()
                        if delta > 0:
                            gap = delta
                    except TypeError:
                        pass  # Mixed timezone formats: use fallback.

                delay = gap / args.speed

                print(
                    f"Waiting {delay:.2f}s "
                    f"(recorded gap {gap:.2f}s, "
                    f"speed {args.speed:g}x)",
                    flush=True
                )

                time.sleep(delay)

            if args.units == "C":
                temps = [x * 1.8 + 32 for x in temps]

            print(row.get("timestamp", ""), end=" ", flush=True)
            send(port, temps)

            previous = now
            sent_count += 1


def main():
    p = argparse.ArgumentParser(description=__doc__)

    p.add_argument(
        "csv_file", nargs="?",
        help="HVAC Monitor history_v3.csv"
    )
    p.add_argument(
        "--manual", action="store_true",
        help="Interactive l:75 / s:95 / o:70 commands"
    )
    p.add_argument(
        "--port", required=True,
        help="e.g. /dev/cu.usbserial-XXXX"
    )
    p.add_argument(
        "--units", choices=["F", "C"], default="F",
        help="Units in history CSV (default: F)"
    )
    p.add_argument(
        "--interval", type=float, default=10.0,
        help="Fallback delay in seconds (default: 10)"
    )
    p.add_argument(
        "--speed", type=float, default=1.0,
        help="Replay speed multiplier (default: 1)"
    )

    args = p.parse_args()

    if args.interval < 0 or args.speed <= 0:
        p.error("--interval must be nonnegative and --speed positive")

    if not args.manual and not args.csv_file:
        p.error("Provide a CSV file or use --manual")

    if args.manual and args.csv_file:
        p.error("Do not provide a CSV file with --manual")

    with serial.Serial(args.port, 115200, timeout=1) as port:
        time.sleep(2)
        port.reset_input_buffer()

        if args.manual:
            manual(port)
        else:
            replay(port, args)


if __name__ == "__main__":
    main()
