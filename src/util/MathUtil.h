#pragma once

#include <cmath>

constexpr float kPi = static_cast<float>(M_PI);
constexpr float kTwoPi = static_cast<float>(M_PI*2.0);

// @return Whether the given value is a positive power of 2
bool IsPowerOfTwo(int v);

bool IsClose(float a, float b, float tolerance);
