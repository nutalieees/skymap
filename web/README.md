# Web and Python sky viewer

`dist/` is the finished first browser interface. It loads the supplied star catalogue and constellation line file locally, offers typed coordinates or optional device location, and renders the current chosen UTC time on a canvas.

`python/` is a small, dependency-free reference implementation for validating the core positional calculations. It keeps astronomy math separate from any UI or renderer.

For local development, serve `dist/` over HTTP so the browser can load the catalogue files. Browser location access requires HTTPS or localhost and explicit permission.
