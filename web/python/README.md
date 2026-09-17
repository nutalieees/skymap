# Python reference model

Run from this directory:

```text
python -m skymap --latitude 1.3521 --longitude 103.8198
```

It prints the bright stars currently above the horizon using the project catalogue. Add `--time 2026-06-30T09:06:00Z` for a reproducible UTC case and `--json` for machine-readable output.

The module uses standard-library Python only. It deliberately does not render graphics; the web app owns the interactive visual presentation.
