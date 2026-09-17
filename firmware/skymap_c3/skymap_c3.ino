#include <Adafruit_GC9A01A.h>
#include <Adafruit_GFX.h>
#include <HardwareSerial.h>
#include <TinyGPSPlus.h>
#include "config.h"
#include "constellation_data.h"
#include "sky_math.h"
#include "stars_data.h"

namespace {
constexpr uint16_t SKY_CENTER = 0x0001, SKY_EDGE = 0x0148, HORIZON = 0x12D1;
constexpr uint16_t ALTITUDE_RING = 0x0989, CONSTELLATION = 0x1734, CARDINAL = 0xA45B;
constexpr uint16_t LABEL = 0xADD5, FIX_OK = 0x07E0, FIX_WAITING = 0xFDE0;
constexpr uint16_t HIP_MAP_SIZE = 512;

Adafruit_GC9A tft(TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN);
TinyGPSPlus gps;
HardwareSerial gps_serial(1);

struct Observation { float latitude = FALLBACK_LATITUDE; float longitude = FALLBACK_LONGITUDE; int year = 2026, month = 6, day = 30, hour = 9, minute = 6, second = 0; bool valid = false; } observation;
struct HipPixel { uint32_t hip = 0; int16_t x = 0, y = 0; bool used = false; } hip_pixels[HIP_MAP_SIZE];
uint32_t last_render = 0, last_log = 0;

bool inside_sky(int x, int y) { const int dx = x - DISPLAY_CENTER, dy = y - DISPLAY_CENTER; return dx * dx + dy * dy <= SKY_RADIUS * SKY_RADIUS; }
void flip(int* x, int* y) { if (MAP_FLIP_X) *x = DISPLAY_SIZE - 1 - *x; if (MAP_FLIP_Y) *y = DISPLAY_SIZE - 1 - *y; }

void clear_hip_map() { for (auto& point : hip_pixels) point.used = false; }
void put_hip(uint32_t hip, int x, int y) { if (!hip) return; for (uint16_t step = 0, slot = hip % HIP_MAP_SIZE; step < HIP_MAP_SIZE; ++step, slot = (slot + 1) % HIP_MAP_SIZE) { if (!hip_pixels[slot].used || hip_pixels[slot].hip == hip) { hip_pixels[slot] = {hip, static_cast<int16_t>(x), static_cast<int16_t>(y), true}; return; } } }
bool get_hip(uint32_t hip, int* x, int* y) { if (!hip) return false; for (uint16_t step = 0, slot = hip % HIP_MAP_SIZE; step < HIP_MAP_SIZE; ++step, slot = (slot + 1) % HIP_MAP_SIZE) { if (!hip_pixels[slot].used) return false; if (hip_pixels[slot].hip == hip) { *x = hip_pixels[slot].x; *y = hip_pixels[slot].y; return true; } } return false; }

bool star_position(const Star& star, float lst, int* x, int* y) {
  if (star.mag > MAGNITUDE_CUTOFF) return false;
  float altitude, azimuth;
  if (!equatorial_to_horizontal(star.ra_deg, star.dec_deg, observation.latitude, lst, &altitude, &azimuth)) return false;
  horizon_project(altitude, azimuth, x, y); flip(x, y); return inside_sky(*x, *y);
}

void draw_background() {
  for (int r = SKY_RADIUS; r >= 0; --r) { const float f = static_cast<float>(r) / SKY_RADIUS; const uint8_t blue = 1 + lroundf(f * 7); tft.fillCircle(DISPLAY_CENTER, DISPLAY_CENTER, r, (blue & 0x1F)); }
  for (int altitude : {30, 60}) tft.drawCircle(DISPLAY_CENTER, DISPLAY_CENTER, (90 - altitude) * SKY_RADIUS / 90, ALTITUDE_RING);
  tft.drawCircle(DISPLAY_CENTER, DISPLAY_CENTER, SKY_RADIUS, HORIZON);
}
void draw_cardinals() { tft.setTextSize(2); tft.setTextColor(CARDINAL); tft.setCursor(DISPLAY_CENTER - 6, 10); tft.print('N'); tft.setCursor(DISPLAY_CENTER - 6, 214); tft.print('S'); tft.setCursor(214, DISPLAY_CENTER - 8); tft.print('E'); tft.setCursor(14, DISPLAY_CENTER - 8); tft.print('W'); }
void draw_lines() { for (uint16_t i = 0; i < SEGMENT_COUNT; ++i) { ConstellationSegment segment; memcpy_P(&segment, &SEGMENTS[i], sizeof segment); int x1, y1, x2, y2; if (get_hip(segment.hip1, &x1, &y1) && get_hip(segment.hip2, &x2, &y2)) tft.drawLine(x1, y1, x2, y2, CONSTELLATION); } }
void draw_star(int x, int y, const Star& star) { const int radius = constrain(lroundf(4.0f - star.mag * 0.55f), 1, 5); tft.fillCircle(x, y, radius, star.colour); if (radius >= 4) { tft.drawFastHLine(x - radius - 1, y, 2 * radius + 3, star.colour); tft.drawFastVLine(x, y - radius - 1, 2 * radius + 3, star.colour); } if (star.mag <= LABEL_MAGNITUDE_CUTOFF && star.label[0]) { tft.setTextSize(1); tft.setTextColor(LABEL); tft.setCursor(constrain(x + 5, 0, 200), constrain(y - 8, 0, 230)); tft.print(star.label); } }

void render() {
  const float lst = local_sidereal_time(observation.year, observation.month, observation.day, observation.hour, observation.minute, observation.second, observation.longitude);
  tft.fillScreen(SKY_CENTER); draw_background(); clear_hip_map();
  for (uint16_t i = 0; i < STAR_COUNT; ++i) { Star star; memcpy_P(&star, &STARS[i], sizeof star); int x, y; if (star_position(star, lst, &x, &y)) put_hip(star.hip, x, y); }
  draw_lines();
  for (uint16_t i = 0; i < STAR_COUNT; ++i) { Star star; memcpy_P(&star, &STARS[i], sizeof star); int x, y; if (star_position(star, lst, &x, &y)) draw_star(x, y, star); }
  draw_cardinals(); tft.fillCircle(231, 8, 3, observation.valid ? FIX_OK : FIX_WAITING);
}

void update_gps() {
  while (gps_serial.available()) gps.encode(static_cast<char>(gps_serial.read()));
  if (!gps.location.isValid() || !gps.date.isValid() || !gps.time.isValid()) { observation.valid = false; return; }
  observation.latitude = gps.location.lat(); observation.longitude = gps.location.lng(); observation.year = gps.date.year(); observation.month = gps.date.month(); observation.day = gps.date.day(); observation.hour = gps.time.hour(); observation.minute = gps.time.minute(); observation.second = gps.time.second(); observation.valid = true;
}
void log_status() { Serial.printf("GPS %s | chars %lu | sats %lu | %.5f, %.5f | %04d-%02d-%02dT%02d:%02d:%02dZ\n", observation.valid ? "READY" : "WAITING", gps.charsProcessed(), gps.satellites.isValid() ? gps.satellites.value() : 0, observation.latitude, observation.longitude, observation.year, observation.month, observation.day, observation.hour, observation.minute, observation.second); }
}

void setup() {
  Serial.begin(115200); gps_serial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  SPI.begin(TFT_SCLK_PIN, -1, TFT_MOSI_PIN, TFT_CS_PIN); tft.begin(); tft.setRotation(TFT_ROTATION); render(); last_render = millis();
}
void loop() {
  update_gps(); const uint32_t now = millis();
  if (now - last_log >= 1000) { last_log = now; log_status(); }
  if (now - last_render >= RENDER_INTERVAL_MS) { last_render = now; render(); }
}
