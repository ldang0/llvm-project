typedef long (*binary_function)(long, long);

extern long external_many(
    long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7,
    long a8, long a9, long a10, long a11, long a12, long a13, long a14,
    long a15, long a16, long a17);

__attribute__((noinline)) long local_add(long lhs, long rhs) {
  return lhs + rhs;
}

__attribute__((noinline)) long recursive_sum(long value) {
  if (value == 0)
    return 0;
  return value + recursive_sum(value - 1);
}

long call_local(long lhs, long rhs) { return local_add(lhs, rhs); }

long call_indirect(binary_function function, long lhs, long rhs) {
  return function(lhs, rhs);
}

long call_external_many(
    long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7,
    long a8, long a9, long a10, long a11, long a12, long a13, long a14,
    long a15, long a16, long a17) {
  return external_many(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11,
                       a12, a13, a14, a15, a16, a17);
}
