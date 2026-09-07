int exception_boundary(void) {
  try {
    throw 1;
  } catch (int value) {
    return value;
  }
}
