extern int __ffsdi2(long value);

void copy_large(void *destination, const void *source) {
  __builtin_memcpy(destination, source, 128);
}

int find_first_set(long value) { return __ffsdi2(value); }

double remainder_double(double lhs, double rhs) {
  return __builtin_fmod(lhs, rhs);
}

float fused_float(float lhs, float rhs, float addend) {
  return __builtin_fmaf(lhs, rhs, addend);
}
