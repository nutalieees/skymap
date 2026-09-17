# Hardware product work

`star-pcb-spec.md` defines the proposed small five-point-star product and the required electrical architecture. `star-outline.svg` is a mechanical starting concept, not a manufacturing-ready PCB outline.

`kicad/skymap-star.sch` is the preliminary editable KiCad schematic. It captures connectivity and named signals, including the proven GPS UART path (GPS TX → GPIO16; GPS RX ← GPIO17). Its generic module blocks must be replaced with exact symbols and footprints before PCB layout.

Add the schematic, PCB, enclosure CAD, BOM, and manufacturing files after the exact display, GPS, LiPo, charger, and regulator parts are confirmed and the firmware passes on-device validation.
