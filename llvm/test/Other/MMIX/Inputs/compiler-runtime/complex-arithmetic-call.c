typedef _Complex float complex_float;
typedef _Complex double complex_double;

extern complex_float __mulsc3(float, float, float, float);
extern complex_float __divsc3(float, float, float, float);
extern complex_double __muldc3(double, double, double, double);
extern complex_double __divdc3(double, double, double, double);

volatile complex_float float_results[4];
volatile complex_double double_results[4];

int _start(void) {
  float infinity_f = __builtin_huge_valf();
  float nan_f = __builtin_nanf("");
  double infinity = __builtin_huge_val();
  double nan = __builtin_nan("");

  float_results[0] = __mulsc3(1.0f, -2.0f, 3.0f, 4.0f);
  float_results[1] = __divsc3(1.0f, -2.0f, 0.0f, -0.0f);
  float_results[2] = __mulsc3(infinity_f, 1.0f, 0.0f, 2.0f);
  float_results[3] = __divsc3(nan_f, 1.0f, 2.0f, infinity_f);
  double_results[0] = __muldc3(1.0, -2.0, 3.0, 4.0);
  double_results[1] = __divdc3(1.0, -2.0, 0.0, -0.0);
  double_results[2] = __muldc3(infinity, 1.0, 0.0, 2.0);
  double_results[3] = __divdc3(nan, 1.0, 2.0, infinity);
  return 0;
}
