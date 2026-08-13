struct Empty {};
struct OneOcta {
  long value;
};
struct PackedFive {
  unsigned char bytes[5];
} __attribute__((packed));
struct Large {
  long words[3];
};

extern struct OneOcta external_direct(struct Empty empty,
                                      struct OneOcta direct);
extern struct Large external_boundary(
    long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7,
    long a8, long a9, long a10, long a11, long a12, long a13, long a14,
    struct Empty empty, struct OneOcta direct, struct PackedFive odd,
    struct Large copy, long tail);

struct OneOcta forward_direct(struct Empty empty, struct OneOcta direct) {
  return external_direct(empty, direct);
}

struct Large forward_boundary(
    long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7,
    long a8, long a9, long a10, long a11, long a12, long a13, long a14,
    struct Empty empty, struct OneOcta direct, struct PackedFive odd,
    struct Large copy, long tail) {
  return external_boundary(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10,
                           a11, a12, a13, a14, empty, direct, odd, copy, tail);
}
