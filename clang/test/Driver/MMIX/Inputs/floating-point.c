extern double fmod(double, double);
extern long double nearbyintl(long double);

float float_arithmetic(float lhs, float rhs, float factor) {
  return (lhs + rhs) * factor;
}

double double_arithmetic(double lhs, double rhs, double divisor) {
  return (lhs - rhs) / divisor;
}

long double long_double_arithmetic(long double lhs, long double rhs) {
  return lhs + rhs;
}

int compare_float(float lhs, float rhs) { return lhs <= rhs; }

int compare_double(double lhs, double rhs) { return lhs != rhs; }

double widen_float(float value) { return value; }

float narrow_double(double value) { return value; }

double signed_to_double(long value) { return value; }

long double_to_signed(double value) { return value; }

float fused_float(float lhs, float rhs, float addend) {
  return __builtin_fmaf(lhs, rhs, addend);
}

double remainder_double(double lhs, double rhs) { return fmod(lhs, rhs); }

long double nearby_long_double(long double value) {
  return nearbyintl(value);
}
