long signed_arithmetic(long lhs, long rhs, long divisor) {
  return (lhs + rhs) * rhs - lhs / divisor;
}

unsigned long unsigned_arithmetic(unsigned long lhs, unsigned long rhs,
                                  unsigned long divisor) {
  return (lhs - rhs) * rhs + lhs / divisor;
}

unsigned long shift_logic(unsigned long value, unsigned int amount) {
  amount &= 63;
  return (value << amount) ^ (value >> amount);
}

long promote_narrow(signed char signed_byte, unsigned short unsigned_half) {
  return signed_byte + unsigned_half;
}

long signed_branch(long lhs, long rhs) {
  if (lhs < rhs)
    return lhs - rhs;
  return lhs + rhs;
}

unsigned long unsigned_branch(unsigned long lhs, unsigned long rhs) {
  if (lhs >= rhs)
    return lhs - rhs;
  return rhs - lhs;
}

long loop_accumulate(const long *values, unsigned int count) {
  long total = 0;
  for (unsigned int index = 0; index != count; ++index)
    total += values[index];
  return total;
}

long switch_value(unsigned int selector, long value) {
  switch (selector) {
  case 1:
    return value + 1;
  case 4:
    return value - 4;
  case 9:
    return value * 9;
  default:
    return -1;
  }
}
