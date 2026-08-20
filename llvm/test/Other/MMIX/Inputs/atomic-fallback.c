struct ThreeBytes {
  unsigned char bytes[3];
};

struct SixteenBytes {
  unsigned char bytes[16];
};

struct __attribute__((packed)) PackedWord {
  unsigned char padding;
  unsigned value;
};

void fallback_load(struct ThreeBytes *ptr, struct ThreeBytes *result) {
  __atomic_load(ptr, result, __ATOMIC_ACQUIRE);
}

void fallback_overwide_load(struct SixteenBytes *ptr,
                            struct SixteenBytes *result) {
  __atomic_load(ptr, result, __ATOMIC_RELAXED);
}

void fallback_store(struct ThreeBytes *ptr, struct ThreeBytes *value) {
  __atomic_store(ptr, value, __ATOMIC_RELEASE);
}

void fallback_exchange(struct ThreeBytes *ptr, struct ThreeBytes *value,
                       struct ThreeBytes *result) {
  __atomic_exchange(ptr, value, result, __ATOMIC_ACQ_REL);
}

int fallback_compare_exchange(struct ThreeBytes *ptr,
                              struct ThreeBytes *expected,
                              struct ThreeBytes *desired) {
  return __atomic_compare_exchange(ptr, expected, desired, 0,
                                   __ATOMIC_SEQ_CST, __ATOMIC_ACQUIRE);
}

unsigned fallback_unaligned_add(struct PackedWord *ptr) {
  return __atomic_fetch_add(&ptr->value, 1, __ATOMIC_RELAXED);
}

int fallback_is_lock_free(unsigned long size, void *ptr) {
  return __atomic_is_lock_free(size, ptr);
}

unsigned long native_add(unsigned long *ptr) {
  return __atomic_fetch_add(ptr, 1, __ATOMIC_RELAXED);
}
