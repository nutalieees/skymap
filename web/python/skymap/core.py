"""SkyMap coordinate transformations.

All calculation inputs use degrees. Times must be timezone-aware UTC datetimes.
"""

from __future__ import annotations

import csv
import math
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable


def local_sidereal_time(when: datetime, longitude_deg: float) -> float:
    """Return local mean sidereal time in degrees for a UTC instant."""
    if when.tzinfo is None:
        raise ValueError("when must include a timezone")
    utc = when.astimezone(timezone.utc)
    julian_date = utc.timestamp() / 86_400 + 2_440_587.5
    centuries = (julian_date - 2_451_545.0) / 36_525.0
    gmst = (
        280.46061837
        + 360.98564736629 * (julian_date - 2_451_545.0)
        + 0.000387933 * centuries**2
        - centuries**3 / 38_710_000.0
    )
    return (gmst + longitude_deg) % 360


def altitude_azimuth(
    ra_deg: float, dec_deg: float, latitude_deg: float, lst_deg: float
) -> tuple[float, float]:
    """Convert equatorial RA/Dec to altitude and azimuth (north=0°, east=90°)."""
    hour_angle = ((lst_deg - ra_deg + 180) % 360) - 180
    hour_angle, declination, latitude = map(
        math.radians, (hour_angle, dec_deg, latitude_deg)
    )
    altitude = math.asin(
        math.sin(declination) * math.sin(latitude)
        + math.cos(declination) * math.cos(latitude) * math.cos(hour_angle)
    )
    azimuth = math.atan2(
        -math.sin(hour_angle) * math.cos(declination),
        math.cos(latitude) * math.sin(declination)
        - math.sin(latitude) * math.cos(declination) * math.cos(hour_angle),
    )
    return math.degrees(altitude), math.degrees(azimuth) % 360


def catalogue(path: Path) -> Iterable[dict[str, str]]:
    """Yield rows from the supplied filtered CSV catalogue."""
    with path.open(newline="", encoding="utf-8") as source:
        yield from csv.DictReader(source)


def visible_stars(
    rows: Iterable[dict[str, str]], latitude_deg: float, longitude_deg: float, when: datetime
) -> list[dict[str, float | str]]:
    """Return catalogue stars above the horizon, ordered from brightest to faintest."""
    sidereal = local_sidereal_time(when, longitude_deg)
    visible = []
    for row in rows:
        altitude, azimuth = altitude_azimuth(
            float(row["RA_deg"]), float(row["Dec"]), latitude_deg, sidereal
        )
        if altitude > 0:
            visible.append({**row, "altitude_deg": altitude, "azimuth_deg": azimuth})
    return sorted(visible, key=lambda star: float(star["Mag"]))
