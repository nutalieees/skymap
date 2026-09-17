from datetime import datetime, timezone

import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.patches import Circle

from astropy.coordinates import SkyCoord, EarthLocation, AltAz
from astropy.time import Time
import astropy.units as u


CATALOG_CSV = "stars_filtered.csv"

LAT_DEG = 1.352
LON_DEG = 103.820

RENDER_TIME = datetime(2025, 1, 15, 13, 0, 0, tzinfo=timezone.utc)


stars = pd.read_csv(CATALOG_CSV)

location = EarthLocation(
    lat=LAT_DEG * u.deg,
    lon=LON_DEG * u.deg,
    height=0 * u.m
)

time = Time(RENDER_TIME)

star_coords = SkyCoord(
    ra=stars["RA_deg"].values * u.deg,
    dec=stars["Dec"].values * u.deg,
    frame="icrs"
)

altaz_frame = AltAz(
    obstime=time,
    location=location
)

altaz = star_coords.transform_to(altaz_frame)

stars["Alt"] = altaz.alt.deg
stars["Az"] = altaz.az.deg

visible = stars[stars["Alt"] > 0].copy()

print(f"Visible stars: {len(visible)} / {len(stars)}")
print(visible[["Name", "Mag", "Alt", "Az"]].head(10))


fig, ax = plt.subplots(figsize=(6, 6))
fig.patch.set_facecolor("black")
ax.set_facecolor("black")
ax.set_aspect("equal")

horizon = Circle((0, 0), 1, fill=False, edgecolor="white")
ax.add_patch(horizon)

for _, star in visible.iterrows():
    alt_rad = star["Alt"] * 3.141592653589793 / 180
    az_rad = star["Az"] * 3.141592653589793 / 180

    r = (90 - star["Alt"]) / 90

    x = r * __import__("math").sin(az_rad)
    y = r * __import__("math").cos(az_rad)

    size = max(5, (5 - star["Mag"]) ** 2 * 5)

    ax.scatter(x, y, s=size, color="white")

ax.text(0, 1.08, "N", color="white", ha="center")
ax.text(1.08, 0, "E", color="white", va="center")
ax.text(0, -1.08, "S", color="white", ha="center")
ax.text(-1.08, 0, "W", color="white", va="center")

ax.set_xlim(-1.15, 1.15)
ax.set_ylim(-1.15, 1.15)
ax.axis("off")

plt.title("Sky map using Astropy Alt/Az", color="white")
plt.savefig("astropy_sky_map.png", dpi=200, facecolor="black")
plt.show()