#include "conversion_utils.h"

float convert_ms_to_kph(float ms) {
  return ms * 3.6;
};

float convert_kph_to_mph(float kph) {
  return kph * 0.621371;
};

float convert_m_to_ft(float m) {
  return m * 3.28084;
};

float convert_c_to_f(float c) {
  return c * 1.8 + 32;
};
