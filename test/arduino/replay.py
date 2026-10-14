#!/usr/bin/env python3
"""Replay HvacMonitor history over USB serial at the recorded timestamps."""
import argparse
import csv
import datetime as dt
import time

import serial


def parse_time(value):
    value = value.strip()
    try:
        return dt.datetime.fromisoformat(value.replace("Z", "+00:00"))
    except ValueError:
        return None


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("csv_file")
    parser.add_argument("--port", required=True, help="e.g. /dev/cu.usbmodem1101")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--interval", type=float, default=10.0,
                        help="Fallback seconds if timestamps are unavailable")
    args = parser.parse_args()

    with open(args.csv_file, newline="") as stream, serial.Serial(args.port, args.baud, timeout=1) as link:
        time.sleep(2)  # Mega resets when serial opens.
        reader = csv.DictReader(stream)
        previous_time = None
        for row in reader:
            try:
                large = float(row["large"])
                small = float(row["small"])
                outside = float(row["outside"])
            except (KeyError, ValueError) as error:
                print("Skipping invalid row:", error)
                continue

            current_time = parse_time(row.get("timestamp", ""))
            if previous_time is not None:
                delay = args.interval
                if current_time is not None:
                    delta = (current_time - previous_time).total_seconds()
                    if delta >= 0:
                        delay = delta
                time.sleep(delay)

            command = f"F,{large:.3f},{small:.3f},{outside:.3f}\n"
            link.write(command.encode("ascii"))
            link.flush()
            print(row.get("timestamp", ""), command.strip())
            previous_time = current_time


if __name__ == "__main__":
    main()
