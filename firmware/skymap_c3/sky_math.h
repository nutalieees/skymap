#pragma once

#include <Arduino.h>
#include <math.h>

constexpr int DISPLAY_SIZE = 240;
constexpr int DISPLAY_CENTER = DISPLAY_SIZE / 2;
constexpr int SKY_RADIUS = DISPLAY_CENTER - 6;

inline float wrap360(float value) {
  value = fmodf(value, 360.0f);
  return value < 0.0f ? value + 360.0f : value;
}

inline float local_sidereal_time(int year, int month, int day, int hour, int minute,
                                 int second, float longitude_deg) {
  if (month <= 2) { year--; month += 12; }
  const float ut_hours = hour + minute / 60.0f + second / 3600.0f;
  const int century = year / 100;
  const int correction = 2 - century + century / 4;
  const double jd = static_cast<long>(365.25 * (year + 4716))
      + static_cast<long>(30.6001 * (month + 1)) + day + correction - 1524.5 + ut_hours / 24.0;
  const double t = (jd - 2451545.0) / 36525.0;
  const double gmst = 280.46061837 + 360.98564736629 * (jd - 2451545.0)
      + 0.000387933 * t * t - t * t * t / 38710000.0;
  return wrap360(static_cast<float>(gmst) + longitude_deg);
}

inline bool equatorial_to_horizontal(float ra_deg, float dec_deg, float latitude_deg,
                                     float lst_deg, float* altitude_deg, float* azimuth_deg) {
  const float hour_angle = wrap360(lst_deg - ra_deg + 180.0f) - 180.0f;
  const float ha = radians(hour_angle);
  const float dec = radians(dec_deg);
  const float lat = radians(latitude_deg);
  const float sine_altitude = constrain(sinf(dec) * sinf(lat) + cosf(dec) * cosf(lat) * cosf(ha), -1.0f, 1.0f);
  *altitude_deg = degrees(asinf(sine_altitude));
  *azimuth_deg = wrap360(degrees(atan2f(-sinf(ha) * cosf(dec),
      cosf(lat) * sinf(dec) - sinf(lat) * cosf(dec) * cosf(ha))));
  return *altitude_deg > 0.0f;
}

inline void horizon_project(float altitude_deg, float azimuth_deg, int* x, int* y) {
  const float radius = (90.0f - altitude_deg) / 90.0f * SKY_RADIUS;
  const float azimuth = radians(azimuth_deg);
  *x = lroundf(DISPLAY_CENTER + radius * sinf(azimuth));
  *y = lroundf(DISPLAY_CENTER - radius * cosf(azimuth));
}
