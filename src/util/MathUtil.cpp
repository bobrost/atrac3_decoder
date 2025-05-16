#include "MathUtil.h"

bool IsPowerOfTwo(int v) {
  return (v >= 2 && ((v&(v-1)) == 0));
}

bool IsClose(float a, float b, float tolerance) {
  return (std::abs(a-b) <= tolerance);
}
