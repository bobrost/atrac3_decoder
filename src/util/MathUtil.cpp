#include "MathUtil.h"

bool isPowerOfTwo(int v) {
  return (v >= 2 && ((v&(v-1)) == 0));
}

bool isClose(float a, float b, float tolerance) {
  return (std::abs(a-b) <= tolerance);
}

int twosComplementToSigned(int twosComplement, int numBits) {
  int mask = ((1<<numBits)-1); // All ones, for numBits
  int highestBit = 1 << (numBits-1); // 1000 (for 4 bits)
  if ((twosComplement & highestBit) == 0) {
    return (twosComplement & mask); // value is positive, no change
  }
  // Convert to negative with 2's complement
  int lowerMask = highestBit - 1;
  return (twosComplement & lowerMask) - highestBit;
}

int signedToTwosComplement(int signedValue, int numBits) {
  int mask = ((1<<numBits)-1); // All ones, for numBits
  if (signedValue >= 0) {
    return (signedValue & mask); // positive
  }
  // Convert from negative with 2's complement
  return ((1<<numBits) + signedValue) & mask;
}
