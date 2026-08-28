#if defined(TEST_EXCEPTION)
int exception_boundary(void) {
  try {
    throw 1;
  } catch (int value) {
    return value;
  }
}
#else
struct Polymorphic {
  virtual long value() const;
};

long read(Polymorphic *object) { return object->value(); }
#endif
