# SkyMap ESP32-C3 firmware

This is the cleaned production-oriented replacement for the original monolithic sketch. It targets an ESP32-C3, a 240×240 GC9A01 round display, and a UART GPS receiver.

## Required Arduino libraries

- **Adafruit GFX Library**
- **Adafruit GC9A01A**
- **TinyGPSPlus**

Copy `config.h.example` to `config.h`, then set pins for your specific ESP32-C3 board and GPS module. `config.h` is deliberately ignored by Git because pin assignments are hardware-specific.

## Electrical assumptions

- GC9A01 logic is **3.3 V only**.
- GPS UART is 3.3 V logic. If a module exposes 5 V UART, level-shift GPS TX before the ESP32-C3 RX pin.
- Do not power the display from an ESP32 pin. Power it from the regulated 3.3 V rail.
- A complete fix requires valid GPS position **and** GPS UTC date/time. Until then, the status dot stays amber and the map uses the clearly marked fallback observation.

## Flashing

Open `skymap_c3.ino` as an Arduino sketch folder, select your ESP32-C3 board, install the listed libraries, and upload. Confirm the serial monitor at 115200 baud first, then verify north/east orientation and GPS time before making a PCB.
