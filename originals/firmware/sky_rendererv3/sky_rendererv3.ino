#include <TFT_eSPI.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include "sky_math.h"
#include "stars_data.h"
#include "constellation_data.h"

// ── GPS serial port ────────────────────────────────────────────────────────
#define GPS_BAUD     9600
#define GPS_RX_PIN   16   // ESP32 listens here  ← wired to GPS TX
#define GPS_TX_PIN   17   // ESP32 sends here    → wired to GPS RX

// ── User Settings  ────────────────────────────────────────────────────────
#define TFT_ROTATION 2
#define MAP_FLIP_X false
#define MAP_FLIP_Y false
#define TFT_INVERT_COLOURS true

// ── Display refresh ────────────────────────────────────────────────────────
#define REDRAW_INTERVAL_MS  30000UL   // redraw every 30 seconds

// ── Magnitude filter ───────────────────────────────────────────────────────
#define MAG_CUTOFF        4.5f        // skip stars dimmer than this
#define MAG_LABEL_CUTOFF  2.0f        // only draw name labels up to this mag


// ───── Defining colours ───────────────────────────────────────────────────────
#define COL_SKY_ZENITH       0x0001
#define COL_SKY_HORIZON      0x0148
#define COL_HORIZON_RING     0x12D1
#define COL_ALTITUDE_RING    0x0989
#define COL_CONST_LINE       0x1734
#define COL_CARDINAL         0xA45B
#define COL_STAR_LABEL       0xADD5
#define COL_DEFAULT_STAR     0xFFFF
#define COL_GPS_OK           0x07E0
#define COL_GPS_WAIT         0xF800

// ── Globals ────────────────────────────────────────────────────────────────
TFT_eSPI tft;
TinyGPSPlus gps;
HardwareSerial GPSSerial(1);

// Fallback values used until GPS gets a valid fix.
// These are only temporary; once GPS is valid, they are overwritten.
float g_lat = 1.3521f;       // Singapore fallback
float g_lon = 103.8198f;
int g_year = 2026;
int g_month = 6;
int g_day = 5;
int g_hour = 11;
int g_min = 39;
int g_sec = 0;
bool g_gps_fix = false;

unsigned long g_last_draw = 0;
unsigned long g_last_gps_debug = 0;

// HIP -> pixel lookup for constellation line drawing.
#define HIP_MAP_SIZE 512
struct HipEntry {
    uint32_t hip;
    int16_t px;
    int16_t py;
    float mag;
    bool valid;
};

HipEntry g_hip_map[HIP_MAP_SIZE]; // creates the table, the g_ makes it a global variable to be used by all functions

// ── HIP map helpers ────────────────────────────────────────────────────────
void hip_map_clear() {  // clears hip map when sky is redrawn
    for (int i = 0; i < HIP_MAP_SIZE; i++) {
        g_hip_map[i].valid = false;
    }
}

void hip_map_insert(uint32_t hip, int px, int py, float mag) {  //saves a visible star into the HIP map.
    if (hip == 0) return; //if no valid HIP no., skip

    uint16_t slot = hip % HIP_MAP_SIZE; 

    for (int i = 0; i < HIP_MAP_SIZE; i++) {
        uint16_t s = (slot + i) % HIP_MAP_SIZE;

        if (!g_hip_map[s].valid || g_hip_map[s].hip == hip) {
            g_hip_map[s].hip = hip;
            g_hip_map[s].px = (int16_t)px;
            g_hip_map[s].py = (int16_t)py;
            g_hip_map[s].mag = mag;
            g_hip_map[s].valid = true;
            return;
        }
    }
}

bool hip_map_get(uint32_t hip, int *px_out, int *py_out, float *mag_out) {
    if (hip == 0) return false;

    uint16_t slot = hip % HIP_MAP_SIZE;

    for (int i = 0; i < HIP_MAP_SIZE; i++) {
        uint16_t s = (slot + i) % HIP_MAP_SIZE;

        if (!g_hip_map[s].valid) return false;

        if (g_hip_map[s].hip == hip) {
            *px_out = g_hip_map[s].px;
            *py_out = g_hip_map[s].py;
            *mag_out = g_hip_map[s].mag;
            return true;
        }
    }

    return false;
}

//screen transform---------------
void apply_map_flip(int *px, int *py) {
    if (MAP_FLIP_X) *px = SCREEN_SIZE - 1 - *px;
    if (MAP_FLIP_Y) *py = SCREEN_SIZE - 1 - *py;
}

bool inside_screen(int px, int py) {
    return px >= 0 && px < SCREEN_SIZE && py >= 0 && py < SCREEN_SIZE;
}

bool inside_sky_circle(int px, int py) {
    int dx = px - CENTER;
    int dy = py - CENTER;
    return (dx * dx + dy * dy) <= (SKY_RADIUS * SKY_RADIUS);
}

// ============================================================================
// DRAWING HELPERS
// ============================================================================

void draw_sky_gradient() {
    int zR = (COL_SKY_ZENITH >> 11) & 0x1F;
    int zG = (COL_SKY_ZENITH >> 5) & 0x3F;
    int zB = COL_SKY_ZENITH & 0x1F;

    int hR = (COL_SKY_HORIZON >> 11) & 0x1F;
    int hG = (COL_SKY_HORIZON >> 5) & 0x3F;
    int hB = COL_SKY_HORIZON & 0x1F;

    for (int r = SKY_RADIUS; r >= 0; r--) {
        float t = (float)r / (float)SKY_RADIUS;

        int R = (int)(zR + t * (hR - zR));
        int G = (int)(zG + t * (hG - zG));
        int B = (int)(zB + t * (hB - zB));

        uint16_t col = ((R & 0x1F) << 11) | ((G & 0x3F) << 5) | (B & 0x1F);
        tft.fillCircle(CENTER, CENTER, r, col);
    }
}

void draw_altitude_rings() {
    int altitudes[] = {30, 60};

    for (int i = 0; i < 2; i++) {
        int a = altitudes[i];
        float ring_r = (90.0f - (float)a) / 90.0f * (float)SKY_RADIUS;

        for (int deg = 0; deg < 360; deg += 5) {
            float rad = deg * DEG_TO_RAD;
            int x = CENTER + (int)(ring_r * cosf(rad));
            int y = CENTER + (int)(ring_r * sinf(rad));
            if (inside_screen(x, y)) {
                tft.drawPixel(x, y, COL_ALTITUDE_RING);
            }
        }
    }
}

void draw_cardinals() {
    // Pull letters further inside the circle so the round TFT bezel does not cut them off.
    int off = SKY_RADIUS - 20;

    tft.setTextColor(COL_CARDINAL);
    tft.setTextSize(2);

    // text size 2 is roughly 12px wide and 16px tall
    int w = 12;
    int h = 16;

    // N
    tft.setCursor(CENTER - w / 2, CENTER - off - h / 2);
    tft.print("N");

    // S
    tft.setCursor(CENTER - w / 2, CENTER + off - h / 2);
    tft.print("S");

    // E
    tft.setCursor(CENTER + off - w / 2, CENTER - h / 2);
    tft.print("E");

    // W
    tft.setCursor(CENTER - off - w / 2, CENTER - h / 2);
    tft.print("W");
}
// Segments only draw if both endpoint stars are already being rendered
// (i.e. within MAG_CUTOFF and above the horizon) — no separate, stricter
// cutoff here anymore, so as many constellations show as your star cutoff allows.

void draw_constellation_line_thick(int x1, int y1, int x2, int y2) {
    // Anti-aliased thick line — much smoother than stacking two 1px lines.
    // bg colour is an approximation (true bg is a gradient) but blends far better than no AA at all.
    tft.drawWideLine(x1, y1, x2, y2, 1.4f, COL_CONST_LINE, COL_SKY_ZENITH);
}

void draw_constellation_lines() {
    for (uint16_t i = 0; i < SEGMENT_COUNT; i++) {
        ConstellationSegment seg;
        memcpy_P(&seg, &SEGMENTS[i], sizeof(ConstellationSegment));

        int x1, y1, x2, y2;
        float m1, m2;
        if (hip_map_get(seg.hip1, &x1, &y1, &m1) && hip_map_get(seg.hip2, &x2, &y2, &m2)) {
            if (inside_sky_circle(x1, y1) && inside_sky_circle(x2, y2)) {
                draw_constellation_line_thick(x1, y1, x2, y2);
            }
        }
    }
}

// Scales an RGB565 colour's brightness by `factor` (0.0–1.0), used to fade sparkle arms.
uint16_t dim_colour(uint16_t c, float factor) {
    int r = (c >> 11) & 0x1F;
    int g = (c >> 5) & 0x3F;
    int b = c & 0x1F;

    r = (int)(r * factor);
    g = (int)(g * factor);
    b = (int)(b * factor);

    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;

    return ((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F);
}

// Maps star magnitude to sparkle arm length. Brighter stars (lower/negative
// mag) get bigger sprites; this is a wider range than the old fillCircle
// radius so brightness variation actually reads on a 240px screen.
int sparkle_size_px(float mag) {
    const float BRIGHT_MAG = -1.5f;   // brightest stars we expect (e.g. Sirius)
    const float DIM_MAG    = MAG_CUTOFF; // dimmest stars we draw at all
    const int   MAX_SIZE   = 5;
    const int   MIN_SIZE   = 1;

    float t = (mag - BRIGHT_MAG) / (DIM_MAG - BRIGHT_MAG); // 0 = brightest, 1 = dimmest
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    int size = MAX_SIZE - (int)roundf(t * (MAX_SIZE - MIN_SIZE));
    if (size < MIN_SIZE) size = MIN_SIZE;
    if (size > MAX_SIZE) size = MAX_SIZE;
    return size;
}

// Draws a small 4-point sparkle/cross sprite instead of a plain dot.
// `size` (1-5) controls arm length, scaled by magnitude at the call site.
void draw_star(int px, int py, int size, uint16_t colour) {
    if (!inside_sky_circle(px, py)) return;

    // bright core pixel
    tft.drawPixel(px, py, colour);

    if (size <= 1) return;

    uint16_t arm_colour = dim_colour(colour, 0.55f);
    uint16_t tip_colour = dim_colour(colour, 0.25f);

    for (int d = 1; d <= size; d++) {
        uint16_t c = (d == size) ? tip_colour : arm_colour;
        tft.drawPixel(px + d, py, c);
        tft.drawPixel(px - d, py, c);
        tft.drawPixel(px, py + d, c);
        tft.drawPixel(px, py - d, c);
    }

    // Diagonal accents, closer to the core, for medium+ stars — reads as a
    // "sparkle" rather than a plain plus-sign.
    if (size >= 3) {
        tft.drawPixel(px + 1, py + 1, arm_colour);
        tft.drawPixel(px - 1, py - 1, arm_colour);
        tft.drawPixel(px + 1, py - 1, arm_colour);
        tft.drawPixel(px - 1, py + 1, arm_colour);
    }

    // A second, dimmer diagonal ring further out, only for the very brightest
    // stars — this is what makes the size difference actually visible.
    if (size >= 4) {
        tft.drawPixel(px + 2, py + 2, tip_colour);
        tft.drawPixel(px - 2, py - 2, tip_colour);
        tft.drawPixel(px + 2, py - 2, tip_colour);
        tft.drawPixel(px - 2, py + 2, tip_colour);
    }
}

void draw_star_label(int px, int py, const char *label) {
    if (label[0] == '\0') return;
    if (!inside_sky_circle(px, py)) return;

    tft.setTextColor(COL_STAR_LABEL);   // transparent background
    tft.setTextSize(1);

    int tx = px + 3;
    int ty = py - 8;

    // Basic boundary safety so labels do not start outside the display.
    if (tx < 0) tx = 0;
    if (tx > SCREEN_SIZE - 24) tx = SCREEN_SIZE - 24;
    if (ty < 0) ty = 0;
    if (ty > SCREEN_SIZE - 8) ty = SCREEN_SIZE - 8;

    tft.setCursor(tx, ty);
    tft.print(label);
}


// rendering
void project_star_to_pixel(const Star &s, float lst, bool *visible, int *px, int *py) {
    *visible = false;

    if (s.mag > MAG_CUTOFF) return;

    float alt, az;
    radec_to_altaz(s.ra_deg, s.dec_deg, g_lat, lst, &alt, &az);

    if (alt <= 0.0f) return;

    float nx, ny;
    sky_project(alt, az, &nx, &ny);
    to_pixel(nx, ny, px, py);
    apply_map_flip(px, py);

    if (!inside_sky_circle(*px, *py)) return;

    *visible = true;
}

void render_sky() {
    float lst = utc_to_lst(
        g_year, g_month, g_day,
        g_hour, g_min, g_sec,
        g_lon
    );

    draw_sky_gradient();
    draw_altitude_rings();
    tft.drawCircle(CENTER, CENTER, SKY_RADIUS, COL_HORIZON_RING);

    hip_map_clear();

    // First pass: build HIP map for visible stars.
    for (uint16_t i = 0; i < STAR_COUNT; i++) {
        Star s;
        memcpy_P(&s, &STARS[i], sizeof(Star));

        bool visible;
        int px, py;
        project_star_to_pixel(s, lst, &visible, &px, &py);

        if (visible && s.hip != 0) {
            hip_map_insert(s.hip, px, py, s.mag);
        }
    }

    // Lines under stars.
    draw_constellation_lines();

    // Second pass: stars and available star labels.
    for (uint16_t i = 0; i < STAR_COUNT; i++) {
        Star s;
        memcpy_P(&s, &STARS[i], sizeof(Star));

        bool visible;
        int px, py;
        project_star_to_pixel(s, lst, &visible, &px, &py);

        if (!visible) continue;

        int r = sparkle_size_px(s.mag);

        draw_star(px, py, r, s.colour);

        if (s.mag <= MAG_LABEL_CUTOFF) {
            draw_star_label(px, py, s.label);
        }
    }
    
    // Draw N/S/E/W last so they sit above stars/lines.
    draw_cardinals();

    // GPS status dot. Green = GPS fix, red = still fallback/no fix.
    tft.fillCircle(SCREEN_SIZE - 8, 8, 3, g_gps_fix ? COL_GPS_OK : COL_GPS_WAIT);

}

// GPS ---------------
void update_gps() {
    while (GPSSerial.available() > 0) {
        char c = GPSSerial.read();
        gps.encode(c);
    }

    bool has_location = gps.location.isValid();
    bool has_date = gps.date.isValid();
    bool has_time = gps.time.isValid();

    if (has_location) {
        g_lat = (float)gps.location.lat();
        g_lon = (float)gps.location.lng();
    }

    if (has_date) {
        g_year = gps.date.year();
        g_month = gps.date.month();
        g_day = gps.date.day();
    }

    if (has_time) {
        g_hour = gps.time.hour();
        g_min = gps.time.minute();
        g_sec = gps.time.second();
    }

    // Only location decides whether we are using live GPS coordinates.
    g_gps_fix = has_location;
}

void print_gps_debug() {
    Serial.print("GPS chars: ");
    Serial.print(gps.charsProcessed());

    Serial.print(" | sats: ");
    if (gps.satellites.isValid()) Serial.print(gps.satellites.value());
    else Serial.print(0);

    Serial.print(" | fix: ");
    Serial.print(g_gps_fix ? "YES" : "NO");

    Serial.print(" | MODE: ");
    Serial.print(g_gps_fix ? "LIVE GPS" : "FALLBACK");

    Serial.print(" | lat: ");
    Serial.print(g_lat, 6);

    Serial.print(" | lon: ");
    Serial.print(g_lon, 6);

    Serial.print(" | UTC: ");
    Serial.print(g_year);
    Serial.print("-");
    Serial.print(g_month);
    Serial.print("-");
    Serial.print(g_day);
    Serial.print(" ");
    Serial.print(g_hour);
    Serial.print(":");
    Serial.print(g_min);
    Serial.print(":");
    Serial.println(g_sec);
}


// arduino entry points-----------
void setup() {
    Serial.begin(115200);
    delay(300);

    Serial.println();
    Serial.println("Sky renderer fixed starting...");

    GPSSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    Serial.println("GPS serial started.");
    Serial.print("GPS TX should be connected to ESP32 GPIO ");
    Serial.println(GPS_RX_PIN);
    Serial.print("GPS RX should be connected to ESP32 GPIO ");
    Serial.println(GPS_TX_PIN);

    tft.init();
    tft.setRotation(TFT_ROTATION);
    tft.invertDisplay(TFT_INVERT_COLOURS);
    tft.fillScreen(COL_SKY_ZENITH);

    Serial.printf("Stars in flash: %u\n", STAR_COUNT);
    Serial.printf("Constellation segments: %u\n", SEGMENT_COUNT);

    render_sky();
    g_last_draw = millis();
    g_last_gps_debug = millis();
}

void loop() {
    update_gps();

    unsigned long now = millis();

    // Print GPS info every second so you can tell if NEO-6M is actually sending data.
    if (now - g_last_gps_debug >= 1000UL) {
        g_last_gps_debug = now;
        print_gps_debug();
    }

    // Redraw map every 30 seconds. The GPS dot updates on redraw.
    if (now - g_last_draw >= REDRAW_INTERVAL_MS) {
        render_sky();
        g_last_draw = now;
    }
}