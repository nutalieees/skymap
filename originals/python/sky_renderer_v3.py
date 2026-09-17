
import math
from datetime import datetime, timezone

import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.patheffects as pe
from matplotlib.patches import Circle
import numpy as np
import matplotlib.colors as mcolors


# ---------- 1. SETTINGS ----------

CATALOG_CSV = "stars_filtered.csv"
CONSTELLATION_FAB = "constellationship.fab.txt"

#to be changed later based on gps mod
LAT_DEG = 1.3521
LON_DEG = 103.8198   

RENDER_TIME = datetime(2026, 6, 30, 9,6, 0, tzinfo=timezone.utc)

SCREEN_SIZE = 240
CENTER = SCREEN_SIZE // 2
RADIUS = CENTER - 5
SHOW_CONSTELLATIONS = True
SHOW_LABELS = True
MAG_LABEL_CUTOFF = 1

SHOW_CONSTELLATION_LABELS = True
CONSTELLATION_LABEL_MIN_VISIBLE = 3

LABEL_OFFSET = 0.04

CANDIDATE_OFFSETS = [
    (1, 0),
    (1, 1),
    (0, 1),
    (-1, 1),
    (-1, 0),
    (-1, -1),
    (0, -1),
    (1, -1),
]

# --------- 2. dictionaries and colour palettes ----------

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
CONSTELLATION_LINE_COLOUR = "#2f6aa3"
CARDINAL_LABEL_COLOUR = "#a8aef3"
STAR_LABEL_COLOUR = "#aac8e8"
UI_METADATA_COLOUR = "#b2c6d5"


def star_colour(name):
    if pd.isna(name):
        return DEFAULT_STAR_COLOUR

    return STAR_COLOURS.get(str(name).strip(), DEFAULT_STAR_COLOUR)

STAR_LABELS = {
    "9Alp CMa":  "Sirius",
    "Alp Car":   "Canopus",
    "16Alp Boo": "Arcturus",
    "3Alp Lyr":  "Vega",
    "13Alp Aur": "Capella",
    "19Bet Ori": "Rigel",
    "10Alp CMi": "Procyon",
    "58Alp Ori": "Betelgeuse",
    "21Alp Sco": "Antares",
    "78Bet Gem": "Pollux",
    "24Alp PsA": "Fomalhaut",
    "87Alp Tau": "Aldebaran",
    "67Alp Vir": "Spica",
    "32Alp Leo": "Regulus",
}

CONSTELLATION_NAMES = {
    "And": "Andromeda",
    "Aql": "Aquila",
    "Ari": "Aries",
    "Aur": "Auriga",
    "Boo": "Bootes",
    "Cae": "Caelum",
    "CMa": "Canis Major",
    "CMi": "Canis Minor",
    "Cap": "Capricornus",
    "Car": "Carina",
    "Cas": "Cassiopeia",
    "Cen": "Centaurus",
    "Cet": "Cetus",
    "Cha": "Chamaeleon",
    "Cnc": "Cancer",
    "Col": "Columba",
    "Crv": "Corvus",
    "Cru": "Crux",
    "Cyg": "Cygnus",
    "Del": "Delphinus",
    "Eri": "Eridanus",
    "Gem": "Gemini",
    "Gru": "Grus",
    "Her": "Hercules",
    "Hya": "Hydra",
    "Leo": "Leo",
    "Lib": "Libra",
    "Lup": "Lupus",
    "Lyr": "Lyra",
    "Ori": "Orion",
    "Peg": "Pegasus",
    "Per": "Perseus",
    "PsA": "Piscis Austrinus",
    "Sco": "Scorpius",
    "Sgr": "Sagittarius",
    "Tau": "Taurus",
    "UMa": "Ursa Major",
    "UMi": "Ursa Minor",
    "Vel": "Vela",
    "Vir": "Virgo",
}

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

def sky_project(alt_deg, az_deg):
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


# ---------- 5. CONSTELLATION LINES
def load_stellarium_fab(fab_path, hip_to_xy):
    """
    Returns:
        segments:
            [(x1, y1, x2, y2), ...]

        constellation_points:
            {
                "Ori": [(x, y), (x, y), ...],
                "Tau": [(x, y), (x, y), ...],
                ...
            }
    """
    segments = []
    constellation_points = {}

    with open(fab_path, "r", encoding="utf-8") as file:
        for line in file:
            line = line.strip()

            if not line or line.startswith("#"):
                continue

            parts = line.split()

            abbrev = parts[0]
            pair_count = int(parts[1])
            hip_ids = [int(value) for value in parts[2:]]

            if abbrev not in constellation_points:
                constellation_points[abbrev] = []

            for index in range(0, pair_count * 2, 2):
                hip_1 = hip_ids[index]
                hip_2 = hip_ids[index + 1]

                if hip_1 in hip_to_xy:
                    constellation_points[abbrev].append(hip_to_xy[hip_1])

                if hip_2 in hip_to_xy:
                    constellation_points[abbrev].append(hip_to_xy[hip_2])

                if hip_1 in hip_to_xy and hip_2 in hip_to_xy:
                    x1, y1 = hip_to_xy[hip_1]
                    x2, y2 = hip_to_xy[hip_2]

                    segments.append((x1, y1, x2, y2))

    return segments, constellation_points

def draw_constellations(ax, segments):
    for x1, y1, x2, y2 in segments:
        ax.plot(
            [x1, x2],
            [y1, y2],
            color=CONSTELLATION_LINE_COLOUR,
            linewidth=1.4,
            alpha=0.8,
            zorder=2,
            solid_capstyle="round",
        )

#----------- 6. labels ----------
def star_label(name):
    if pd.isna(name):
        return None

    return STAR_LABELS.get(str(name).strip())


def draw_star_labels(ax, label_candidates):
    placed = []

    # brighter stars first
    label_candidates.sort(key=lambda item: item[0])

    for mag, x, y, name in label_candidates:
        half_width = len(name) * 0.012
        half_height = 0.025

        best_position = None
        best_score = float("inf")

        for dx, dy in CANDIDATE_OFFSETS:
            label_x = x + dx * LABEL_OFFSET
            label_y = y + dy * LABEL_OFFSET

            overlap_score = sum(
                1
                for placed_x, placed_y, placed_hw, placed_hh in placed
                if (
                    abs(label_x - placed_x) < half_width + placed_hw
                    and abs(label_y - placed_y) < half_height + placed_hh
                )
            )

            if overlap_score < best_score:
                best_score = overlap_score
                best_position = (label_x, label_y)

                if overlap_score == 0:
                    break

        if best_position is None:
            continue

        label_x, label_y = best_position

        ax.text(
            label_x,
            label_y,
            name,
            fontsize=6.5,
            color=STAR_LABEL_COLOUR,
            family="monospace",
            ha="center",
            va="center",
            zorder=8,
            path_effects=[
                pe.withStroke(
                    linewidth=1.5,
                    foreground=SKY_ZENITH_COLOUR
                )
            ],
        )

        placed.append((label_x, label_y, half_width, half_height))


def draw_constellation_labels(ax, constellation_points):
    placed = []

    for abbrev, points in constellation_points.items():
        # remove duplicate points
        points = list(dict.fromkeys(points))

        if len(points) < CONSTELLATION_LABEL_MIN_VISIBLE:
            continue

        cx = sum(x for x, _ in points) / len(points)
        cy = sum(y for _, y in points) / len(points)

        # keep label slightly inward if near edge
        r = math.hypot(cx, cy)
        if r > 0.9:
            scale = 0.9 / r
            cx *= scale
            cy *= scale

        label_text = CONSTELLATION_NAMES.get(abbrev, abbrev)

        half_width = len(label_text) * 0.015
        half_height = 0.03

        overlap = any(
            abs(cx - px) < half_width + pw and abs(cy - py) < half_height + ph
            for px, py, pw, ph in placed
        )

        if overlap:
            continue

        ax.text(
            cx,
            cy,
            label_text,
            fontsize=7.5,
            color=UI_METADATA_COLOUR,
            ha="center",
            va="center",
            zorder=6,
            alpha=0.9,
            path_effects=[
                pe.withStroke(
                    linewidth=2,
                    foreground=SKY_ZENITH_COLOUR
                )
            ],
        )

        placed.append((cx, cy, half_width, half_height))

def draw_sky_gradient(ax, resolution=500):

    y, x = np.mgrid[
        -1:1:complex(resolution),
        -1:1:complex(resolution)
    ]

    radius = np.sqrt(x**2 + y**2)

    gradient = np.clip(radius, 0, 1)

    sky_cmap = mcolors.LinearSegmentedColormap.from_list(
        "sky_gradient",
        [
            SKY_ZENITH_COLOUR,
            SKY_HORIZON_COLOUR,
        ]
    )

    rgba = sky_cmap(gradient)

    # Hide the gradient outside the circular sky map
    rgba[..., 3] = (radius <= 1).astype(float)

    ax.imshow(
        rgba,
        extent=(-1, 1, -1, 1),
        origin="lower",
        interpolation="bilinear",
        zorder=0,
    )

# ---------- 7. MAIN PROGRAM ----------

def main():
    stars = pd.read_csv(CATALOG_CSV)

    # Ensure HIP is numeric and allow missing HIP values
    stars["HIP"] = pd.to_numeric(
        stars["HIP"],
        errors="coerce"
    ).astype("Int64")

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

    draw_sky_gradient(ax)

    horizon = Circle(
    (0, 0),
    1,
    fill=False,
    edgecolor=HORIZON_RING_COLOUR,
    linewidth=1,
    zorder=3
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
        linestyle="--",
        zorder=1 )

        ax.add_patch(ring)

    # Precompute star positions for constellation lines
    projected_stars = []
    hip_to_xy = {}

    for _, star in visible.iterrows():
        x, y = sky_project(star["Alt"], star["Az"])

        if x is None:
            continue

        # Store the star and its projected map position
        projected_stars.append(
            (star, x, y)
        )

        # Store HIP position for constellation line lookup
        if pd.notna(star["HIP"]):
            hip_number = int(star["HIP"])
            hip_to_xy[hip_number] = (x, y)

    print(f"Visible stars with HIP IDs: {len(hip_to_xy)}")

    #draw constellation lines
    constellation_points = {}

    if SHOW_CONSTELLATIONS:
        constellation_segments, constellation_points = load_stellarium_fab(
            CONSTELLATION_FAB,
            hip_to_xy
        )

        draw_constellations(
            ax,
            constellation_segments
        )

        print(
            f"Visible constellation segments: "
            f"{len(constellation_segments)}"
        )

    #draw stars after constellation 
    label_candidates = []

    for star, x, y in projected_stars:
        mag = float(star["Mag"])
        name = star["Name"]

        size = star_radius(mag)
        colour = star_colour(name)

        ax.scatter(
            x,
            y,
            s=(size * 4) ** 1.6,
            color=colour,
            edgecolors="none",
            zorder=5
        )

        label = star_label(name)

        if SHOW_LABELS and mag <= MAG_LABEL_CUTOFF and label is not None:
            label_candidates.append((mag, x, y, label))

    if SHOW_CONSTELLATION_LABELS:
        draw_constellation_labels(ax, constellation_points)

    if SHOW_LABELS:
        draw_star_labels(ax, label_candidates)

    #draw cardinal direction labels on top of everything else
    ax.text(0, 0.92, "N", color=CARDINAL_LABEL_COLOUR, ha="center",zorder=20)
    ax.text(0, -0.92, "S", color=CARDINAL_LABEL_COLOUR, ha="center",zorder=20)
    ax.text(0.92, 0, "E", color=CARDINAL_LABEL_COLOUR, va="center",zorder=20)
    ax.text(-0.92, 0, "W", color=CARDINAL_LABEL_COLOUR, va="center",zorder=20)

    # Set limits and hide axes
    ax.set_xlim(-1.02, 1.02)
    ax.set_ylim(-1.02, 1.02)
    ax.axis("off")

    # Add title and save the figure
    plt.title("Circular Sky Map", color=STAR_LABEL_COLOUR)
    plt.savefig("sky_chart.png", dpi=200, facecolor=SKY_ZENITH_COLOUR)
    plt.show()

    print("Saved sky_chart.png")


if __name__ == "__main__":
    main()


