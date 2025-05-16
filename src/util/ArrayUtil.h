#pragma once

#include <vector>
#include <functional>

using FloatArray = std::vector<float>;

FloatArray initArray(int n, const std::function<float(int i)>& fn);

// Resize and zero out an array
void setArrayZeroes(FloatArray& values);

void reverseArrayInPlace(float* values, int numValues);
void reverseArrayInPlace(FloatArray& values);

void multiplyArrays(const FloatArray& source1, const FloatArray& source2, FloatArray& target);

FloatArray scaledArray(const FloatArray& values, float scale);

void scaleArrayInPlace(float* values, int numValues, float scale);
void scaleArrayInPlace(FloatArray& values, float scale);
void scaleArrayInPlace(FloatArray& values, const FloatArray& scaleValues);
void scaleArrayInPlace(float* values, const float* scaleValues, int numValues);

void copyArrayValues(const float* source, float* target, int numValues);
void copyArrayValues(const FloatArray& source, int sourceOffset,
  FloatArray& target, int targetOffset, int numValues);

void addArrayValues(const float* source, float* target, int numValues);

bool isClose(const FloatArray& a, const FloatArray& b, float tolerance);

bool isZero(const FloatArray& a);

float getRMSE(const FloatArray& a, const FloatArray& b);

float getMaxDifference(const FloatArray& a, const FloatArray& b);
float getAbsMax(const FloatArray& values);
float getAbsMin(const FloatArray& values);

float getMaxDifference(const FloatArray& a, const FloatArray& b, int offsetA, int offsetB);

void printArray(const char* label, const float* values, int numValues);
void printArray(const char* label, const FloatArray& values);

// Circular buffer for sample history
class HistoryBuffer {
public:
  HistoryBuffer(int historySize = 100) {
    _n = std::max(1, historySize);
    _values.assign(_n, 0.0f);
  }
  HistoryBuffer(const HistoryBuffer&) = default;
  HistoryBuffer& operator=(const HistoryBuffer&) = default;

  void clear() {
    _values.assign(_n, 0.0f);
    _offset = 0;
  }

  void append(float value) {
    _values[_offset] = value;
    _offset = (_offset+1) % _n;
  }

  // @param index Index where 0 is the "current" invalid value, -1 is the most
  //   recently appended sample, and negative numbers are valid
  float operator[](int index) const {
    return ((index >= 0 || index < -_n) ? 0.0f :
      _values[ (_offset+_n+index)%_n]);
  }

private:
  int _n=0;
  int _offset = 0;
  std::vector<float> _values;
};