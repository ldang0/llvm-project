long load_constant_index(const long *values) { return values[3]; }

long load_large_index(const long *values) { return values[32]; }

int load_variable_index(const int *values, unsigned long index) {
  return values[index];
}

void store_variable_index(unsigned short *values, unsigned long index,
                          unsigned short value) {
  values[index] = value;
}

unsigned char load_byte_offset(const unsigned char *bytes, long offset) {
  return *(bytes + offset);
}

int addresses_ordered(const long *lhs, const long *rhs) { return lhs < rhs; }

void copy_two_words(long *destination, const long *source) {
  destination[0] = source[0];
  destination[1] = source[1];
}

unsigned long local_objects(unsigned char seed, unsigned int pick) {
  volatile unsigned char bytes[3];
  volatile unsigned short half;

  bytes[0] = seed;
  bytes[1] = seed + 1;
  bytes[2] = seed + 2;
  half = seed + 3;
  return bytes[pick % 3] + half;
}
