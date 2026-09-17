#pragma once

#include <Arduino.h>
#include <math.h>

// =============================================================================
// Sky math helpers for the ESP32 circular sky renderer
// =============================================================================
// Coordinate flow:
//   UTC time + longitude  -> Local Sidereal Time (LST)
//   RA/Dec + latitude/LST -> Altitude/Azimuth
//   Altitude/Azimuth      -> normalised sky x/y
//   normalised x/y        -> TFT pixel coordinates
//
// Screen orientation:
//   North = top, East = right, South = bottom, West = left
//   Zenith = centre of screen
//   Horizon = outer circle
// =============================================================================


// =============================================================================
// Screen constants
// =============================================================================

static const int SCREEN_SIZE = 240;
static const int CENTER = SCREEN_SIZE / 2;
static const int SKY_RADIUS = CENTER - 5;


// =============================================================================
// Angle helpers
// =============================================================================

static inline float wrap_360(float angle_deg)
{
    float wrapped = fmodf(angle_deg, 360.0f);

    if (wrapped < 0.0f) {
        wrapped += 360.0f;
    }

    return wrapped;
}


static inline float wrap_180(float angle_deg)
{
    float wrapped = wrap_360(angle_deg + 180.0f);
    return wrapped - 180.0f;
}


// =============================================================================
// UTC date/time + longitude -> Local Sidereal Time
// =============================================================================
// Inputs:
//   year, month, day, hour, minute, second : UTC time
//   lon_deg                                : longitude in degrees, East positive
//
// Output:
//   Local Sidereal Time in degrees, range [0, 360)
// =============================================================================

static inline float utc_to_lst(int year, int month, int day,
                               int hour, int minute, int second,
                               float lon_deg)
{
    float ut_hours = hour + minute / 60.0f + second / 3600.0f;

    // Julian date formula treats Jan and Feb as months 13 and 14
    // of the previous year.
    if (month <= 2) {
        year -= 1;
        month += 12;
    }

    int A = year / 100;
    int B = 2 - A + A / 4;

    double JD = (long)(365.25 * (year + 4716))
              + (long)(30.6001 * (month + 1))
              + day
              + B
              - 1524.5
              + ut_hours / 24.0;

    double T = (JD - 2451545.0) / 36525.0;

    double GMST = 280.46061837
                + 360.98564736629 * (JD - 2451545.0)
                + 0.000387933 * T * T
                - (T * T * T) / 38710000.0;

    float gmst_deg = wrap_360((float)GMST);
    float lst_deg = wrap_360(gmst_deg + lon_deg);

    return lst_deg;
}


// =============================================================================
// RA/Dec -> Altitude/Azimuth
// =============================================================================
// Inputs:
//   ra_deg   : Right Ascension in degrees
//   dec_deg  : Declination in degrees
//   lat_deg  : observer latitude in degrees, North positive
//   lst_deg  : Local Sidereal Time in degrees
//
// Outputs:
//   alt_out  : altitude in degrees, positive means above horizon
//   az_out   : azimuth in degrees, 0 = North, 90 = East, clockwise positive
// =============================================================================

static inline void radec_to_altaz(float ra_deg, float dec_deg,
                                  float lat_deg, float lst_deg,
                                  float *alt_out, float *az_out)
{
    // Same as Python:
    // HA = ((lst_deg - ra_deg) + 180) % 360 - 180
    float HA = wrap_180(lst_deg - ra_deg);

    float ha = HA * DEG_TO_RAD;
    float dec = dec_deg * DEG_TO_RAD;
    float lat = lat_deg * DEG_TO_RAD;

    float sin_alt = sinf(dec) * sinf(lat)
                  + cosf(dec) * cosf(lat) * cosf(ha);

    // Prevent asin() errors caused by tiny floating-point overshoots.
    sin_alt = constrain(sin_alt, -1.0f, 1.0f);

    float alt = asinf(sin_alt);

    float az = atan2f(
        -sinf(ha) * cosf(dec),
         cosf(lat) * sinf(dec) - sinf(lat) * cosf(dec) * cosf(ha)
    );

    *alt_out = alt * RAD_TO_DEG;
    *az_out = wrap_360(az * RAD_TO_DEG);
}


// =============================================================================
// Altitude/Azimuth -> normalised circular sky coordinates
// =============================================================================
// Projection used:
//   r = (90 - altitude) / 90
//
// This is a linear altitude projection:
//   altitude 90 deg -> r = 0      -> centre
//   altitude 0 deg  -> r = 1      -> horizon
//
// Returns:
//   true  if object is above the horizon
//   false if object is at/below the horizon
// =============================================================================

static inline bool sky_project(float alt_deg, float az_deg,
                               float *x_out, float *y_out)
{
    if (alt_deg <= 0.0f) {
        return false;
    }

    float r = (90.0f - alt_deg) / 90.0f;
    float az = az_deg * DEG_TO_RAD;

    *x_out = r * sinf(az);
    *y_out = r * cosf(az);

    return true;
}


// =============================================================================
// Normalised sky coordinates -> TFT pixel coordinates
// =============================================================================
// Input x/y range is approximately [-1, 1].
// Screen y is inverted, so positive sky y maps upward on the TFT.
// =============================================================================

static inline void to_pixel(float x, float y, int *px_out, int *py_out)
{
    *px_out = (int)(CENTER + x * SKY_RADIUS);
    *py_out = (int)(CENTER - y * SKY_RADIUS);
}


// =============================================================================
// Star magnitude -> dot radius in pixels
// =============================================================================
// Smaller magnitude means brighter star.
// This keeps the same formula as the Python renderer, but rounds instead of
// truncating so the ESP32 result is closer to the Python visual.
// =============================================================================

static inline int star_radius_px(float mag)
{
    float r = 3.0f - mag * 0.6f;

    if (r < 1.0f) {
        r = 1.0f;
    }

    return (int)(r + 0.5f);
}
