# SkyMap

An interactive map of the visible sky for a chosen GPS location and UTC time. The repository keeps the browser experience, Python reference math, ESP32 firmware, and future product-design work in one clear place.

## Start here

- `web/dist/index.html` is the dependency-free interactive web sky map. Serve the `web/dist` folder with any static web server, then open it in a browser.
- `web/python/` is the cleaned Python reference implementation. It calculates visible stars from the supplied catalogue without a graphical dependency.
- `firmware/sky_rendererv3/` is the working-copy ESP32 sketch, retained separately from the preserved original.
- `originals/` is an unmodified source snapshot from the two supplied archives. It is the first Git commit and is not the active implementation.

## Repository map

| Folder | Role |
| --- | --- |
| `web/` | Browser sky map and Python reference logic |
| `firmware/` | ESP32 firmware under active development |
| `hardware/` | Schematic, PCB, enclosure CAD, manufacturing outputs |
| `shared/` | Catalogue conversion and shared data contracts |
| `docs/` | Validation records, build notes, decisions |
| `originals/` | Preserved supplied project sources |

## Scientific model

The project uses UTC plus longitude to calculate local sidereal time, converts star right ascension/declination to altitude/azimuth, and shows stars above altitude 0° in a circular horizon chart. The radial projection is azimuthal equidistant-style (`r = (90 - altitude) / 90`); it is not stereographic.

This is an educational sky viewer. Before making claims about accuracy, compare fixed locations and times against Astropy and document the result in `docs/validation.md`.
