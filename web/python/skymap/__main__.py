"""Command-line entrypoint for the Python reference model."""

import argparse
import json
from datetime import datetime, timezone
from pathlib import Path

from .core import catalogue, visible_stars


def parse_time(value: str | None) -> datetime:
    if not value:
        return datetime.now(timezone.utc)
    parsed = datetime.fromisoformat(value.replace("Z", "+00:00"))
    if parsed.tzinfo is None:
        raise argparse.ArgumentTypeError("time must include UTC offset or Z")
    return parsed.astimezone(timezone.utc)


def main() -> None:
    parser = argparse.ArgumentParser(description="List visible SkyMap catalogue stars.")
    parser.add_argument("--latitude", type=float, required=True)
    parser.add_argument("--longitude", type=float, required=True)
    parser.add_argument("--time", help="ISO-8601 UTC time, e.g. 2026-06-30T09:06:00Z")
    parser.add_argument("--json", action="store_true", help="emit full data as JSON")
    args = parser.parse_args()
    if not -90 <= args.latitude <= 90 or not -180 <= args.longitude <= 180:
        parser.error("latitude must be -90..90 and longitude must be -180..180")
    when = parse_time(args.time)
    data = visible_stars(catalogue(Path(__file__).parents[2] / "dist" / "stars_filtered.csv"), args.latitude, args.longitude, when)
    if args.json:
        print(json.dumps({"time_utc": when.isoformat(), "visible_stars": data}, indent=2))
        return
    print(f"{len(data)} stars above horizon at {when.isoformat()}")
    for star in data[:15]:
        print(f"{star['Name'] or 'unnamed':18} alt {star['altitude_deg']:6.1f}°  az {star['azimuth_deg']:6.1f}°")


if __name__ == "__main__":
    main()
