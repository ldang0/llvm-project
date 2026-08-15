extern int __ffsdi2(long long value);

int _start(long long value) { return __ffsdi2(value); }
