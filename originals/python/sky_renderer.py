import math
from datetime import datetime, timezone

import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import matplotlib.patheffects as pe
from matplotlib.patches import Circle


# ---------- 1. SETTINGS ----------

CATALOG_CSV = "stars_filtered.csv"

#to be changed later based on gps mod
LAT_DEG = 1.352       # Singapore latitude
LON_DEG = 103.820     # Singapore longitude

RENDER_TIME = datetime(2025, 1, 15, 13, 0, 0, tzinfo=timezone.utc)

SCREEN_SIZE = 240
CENTER = SCREEN_SIZE // 2
RADIUS = CENTER - 5
SHOW_CONSTELLATIONS = True
SHOW_LABELS = True
MAG_LABEL_CUTOFF = 1.5

# ---------- COLOUR PALETTE ----------

STAR_COLOURS = {
    "9Alp CMa":  "#e8eeff",  # Sirius
    "Alp Car":   "#fff8e8",  # Canopus
    "16Alp Boo": "#ffaa55",  # Arcturus
    "3Alp Lyr":  "#e0eaff",  # Vega
    "13Alp Aur": "#ffeeaa",  # Capella
    "19Bet Ori": "#c8d8ff",  # Rigel
    "10Alp CMi": "#fff5d8",  # Procyon
    "58Alp Ori": "#ff7733",  # Betelgeuse
    "21Alp Sco": "#ff5522",  # Antares
    "78Bet Gem": "#ffcc88",  # Pollux
    "24Alp PsA": "#eef2ff",  # Fomalhaut
    "87Alp Tau": "#ff9944",  # Aldebaran
    "67Alp Vir": "#c0ccff",  # Spica
    "32Alp Leo": "#ccd6ff",  # Regulus
}

DEFAULT_STAR_COLOUR = "#ffffff"

SKY_ZENITH_COLOUR = "#030912"
SKY_HORIZON_COLOUR = "#0c1e35"
HORIZON_RING_COLOUR = "#2a5a8a"
ALTITUDE_RING_COLOUR = "#1a3050"
CONSTELLATION_LINE_COLOUR = "#1e3a5f"
CARDINAL_LABEL_COLOUR = "#5a9abf"
STAR_LABEL_COLOUR = "#aac8e8"
UI_METADATA_COLOUR = "#2a5070"


def star_colour(name):
    if pd.isna(name):
        return DEFAULT_STAR_COLOUR

    return STAR_COLOURS.get(str(name).strip(), DEFAULT_STAR_COLOUR)

# ---------- 2. UTC + LONGITUDE -> LST ----------

def utc_to_lst(dt_utc, lon_deg):
    y = dt_utc.year
    m = dt_utc.month
    d = dt_utc.day

    ut_hours = dt_utc.hour + dt_utc.minute / 60 + dt_utc.second / 3600

    # Adjust month and year for Jan/Feb --> months 13/14 of previous year
    if m <= 2:
        y -= 1
        m += 12

    A = int(y / 100)
    B = 2 - A + int(A / 4)

    # Julian Date at 0h UT
    JD = (
        int(365.25 * (y + 4716))
        + int(30.6001 * (m + 1))
        + d
        + B
        - 1524.5
        + ut_hours / 24
    )

    # Centuries since J2000.0
    T = (JD - 2451545.0) / 36525.0

    # Greenwich Mean Sidereal Time in degrees
    GMST = (
        280.46061837
        + 360.98564736629 * (JD - 2451545.0)
        + 0.000387933 * T**2
        - T**3 / 38710000.0
    )

    GMST = GMST % 360
    LST = (GMST + lon_deg) % 360

    return LST


# ---------- 3. RA/DEC -> ALT/AZ ----------

def radec_to_altaz(ra_deg, dec_deg, lat_deg, lst_deg):
    HA = ((lst_deg - ra_deg) + 180) % 360 - 180

    ha = math.radians(HA)
    dec = math.radians(dec_deg)
    lat = math.radians(lat_deg)

    sin_alt = (
        math.sin(dec) * math.sin(lat)
        + math.cos(dec) * math.cos(lat) * math.cos(ha)
    )

    sin_alt = max(-1, min(1, sin_alt))
    alt = math.asin(sin_alt)

    az = math.atan2(
        -math.sin(ha) * math.cos(dec),
        math.cos(lat) * math.sin(dec)
        - math.sin(lat) * math.cos(dec) * math.cos(ha)
    )

    alt_deg = math.degrees(alt)
    az_deg = math.degrees(az) % 360

    return alt_deg, az_deg


# ---------- 4. ALT/AZ -> CIRCULAR SCREEN ----------

def stereo_project(alt_deg, az_deg):
    if alt_deg <= 0:
        return None, None

    r = (90 - alt_deg) / 90

    az = math.radians(az_deg)

    x = r * math.sin(az)
    y = r * math.cos(az)

    return x, y


def to_pixel(x, y):
    px = int(CENTER + x * RADIUS)
    py = int(CENTER - y * RADIUS)
    return px, py


def star_radius(mag):
    return max(0.7, 3.0 - mag * 0.6)


# ---------- 5. MAIN PROGRAM ----------

def main():
    stars = pd.read_csv(CATALOG_CSV)

    lst = utc_to_lst(RENDER_TIME, LON_DEG)

    print(f"Stars loaded: {len(stars)}")
    print(f"Render time: {RENDER_TIME}")
    print(f"LST: {lst:.3f} degrees")

    altitudes = []
    azimuths = []

    for _, star in stars.iterrows():
        alt, az = radec_to_altaz(
            star["RA_deg"],
            star["Dec"],
            LAT_DEG,
            lst
        )

        altitudes.append(alt)
        azimuths.append(az)

    stars["Alt"] = altitudes
    stars["Az"] = azimuths

    visible = stars[stars["Alt"] > 0].copy()

    print(f"Visible stars: {len(visible)}")

    fig, ax = plt.subplots(figsize=(6, 6))
    ax.set_facecolor(SKY_ZENITH_COLOUR)
    fig.patch.set_facecolor(SKY_ZENITH_COLOUR)
    ax.set_aspect("equal")

    horizon = Circle(
    (0, 0),
    1,
    fill=False,
    edgecolor=HORIZON_RING_COLOUR,
    linewidth=1
)
    ax.add_patch(horizon)
    # Altitude guide rings: 30 degrees and 60 degrees
    for altitude in [30, 60]:
        ring_radius = (90 - altitude) / 90

        ring = Circle(
        (0, 0),
        ring_radius,
        fill=False,
        edgecolor=ALTITUDE_RING_COLOUR,
        linewidth=0.6,
        linestyle="--" )

        ax.add_patch(ring)

    for _, star in visible.iterrows():
        x, y = stereo_project(star["Alt"], star["Az"])

        if x is None:
            continue

        size = star_radius(star["Mag"])

        ax.scatter(
            x,
            y,
            s=(size * 4.5) ** 1.7,
            color=star_colour(star["Name"]),
            edgecolors= "none"
        )

    ax.text(0, 1.08, "N", color=CARDINAL_LABEL_COLOUR, ha="center")
    ax.text(0, -1.08, "S", color=CARDINAL_LABEL_COLOUR, ha="center")
    ax.text(1.08, 0, "E", color=CARDINAL_LABEL_COLOUR, va="center")
    ax.text(-1.08, 0, "W", color=CARDINAL_LABEL_COLOUR, va="center")

    ax.set_xlim(-1.15, 1.15)
    ax.set_ylim(-1.15, 1.15)
    ax.axis("off")

    plt.title("Circular Sky Map", color=STAR_LABEL_COLOUR)
    plt.savefig("sky_chart.png", dpi=200, facecolor=SKY_ZENITH_COLOUR)
    plt.show()

    print("Saved sky_chart.png")


if __name__ == "__main__":
    main()