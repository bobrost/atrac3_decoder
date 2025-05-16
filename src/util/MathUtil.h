#pragma once

#include <cmath>

constexpr float kPi = static_cast<float>(M_PI);
constexpr float kTwoPi = static_cast<float>(M_PI*2.0);

// @return Whether the given value is a positive power of 2
bool isPowerOfTwo(int v);

bool isClose(float a, float b, float tolerance);

// Convert an N-bit unsigned 2's complement encoded number to its signed value.
// @param twosComplement The positive representation to decode
// @param numBits Size of the number in bits
int twosComplementToSigned(int twosComplement, int numBits);

// Convert an N-bit signed integer to its unsigned 2's complement encoding.
// @param signedValue The signed value to convert
// @param numBits Size of the number in bits
int signedToTwosComplement(int signedValue, int numBits);

// A clamp implementation for C++11 that doesn't support std::clamp
template <class T>
constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
  return (v < lo) ? lo : (hi < v) ? hi : v;
}
