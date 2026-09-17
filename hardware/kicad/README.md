# KiCad source

`skymap-star.sch` is an editable preliminary KiCad legacy-format schematic, selected because it can be created and versioned without inventing unconfirmed module footprints. Import it into KiCad's Schematic Editor, then save it as the current `.kicad_sch` format before PCB layout.

The circuit intentionally uses generic module connectors for the display, GPS, USB-C/power input, charger/power-path, regulator, and ESP32-C3. Replace each generic block with exact symbols and footprints after receiving the hardware part numbers; then run ERC and resolve every power and pin-type warning.
