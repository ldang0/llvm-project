struct Direct {
  int word;
};
struct Large {
  long words[3];
};

long consume_variadic(
    long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7,
    long a8, long a9, long a10, long a11, long a12, long a13, long named,
    ...) {
  __builtin_va_list arguments;
  __builtin_va_list copy;
  __builtin_va_start(arguments, named);
  __builtin_va_copy(copy, arguments);
  int promoted_integer = __builtin_va_arg(arguments, int);
  double promoted_float = __builtin_va_arg(arguments, double);
  struct Direct direct = __builtin_va_arg(arguments, struct Direct);
  struct Large large = __builtin_va_arg(arguments, struct Large);
  int copied_integer = __builtin_va_arg(copy, int);
  __builtin_va_end(copy);
  __builtin_va_end(arguments);
  return promoted_integer + (long)promoted_float + direct.word +
         large.words[1] + copied_integer;
}

long call_variadic(signed char narrow, float single, struct Direct direct,
                   struct Large large) {
  return consume_variadic(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
                          14, narrow, single, direct, large);
}
