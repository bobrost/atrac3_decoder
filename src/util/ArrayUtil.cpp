#include "ArrayUtil.h"
#include "MathUtil.h"
#include <stdio.h>


FloatArray initArray(int n, const std::function<float(int i)>& fn) {
  FloatArray result(n);
  for (int i=0; i<n; ++i) {
    result[i] = fn(i);
  }
  return result;
}

void setArrayZeroes(FloatArray& values) {
  memset(values.data(), 0, values.size()*sizeof(float));
}

void reverseArrayInPlace(float* values, int numValues) {
  int n = numValues / 2;
  for (int i=0; i<n; ++i) {
    std::swap(values[i], values[numValues-1-i]);
  }
}

void reverseArrayInPlace(FloatArray& values) {
  reverseArrayInPlace(values.data(), (int)values.size());
}

void multiplyArrays(const FloatArray& source1, const FloatArray& source2, FloatArray& target) {
  size_t n = std::min(source1.size(), source2.size());
  target.resize(n);
  for (size_t i=0; i<n; ++i) {
    target[i] = source1[i] * source2[i];
  }
}

FloatArray scaledArray(const FloatArray& values, float scale) {
  size_t n = values.size();
  FloatArray result(n);
  for (size_t i=0; i<n; ++i) {
    result[i] = values[i] * scale;
  }
  return result;
}

void scaleArrayInPlace(float* values, int numValues, float scale) {
  for (int i=0; i<numValues; ++i) {
    values[i] *= scale;
  }
}

void scaleArrayInPlace(FloatArray& values, float scale) {
  scaleArrayInPlace(values.data(), (int)values.size(), scale);
}

void scaleArrayInPlace(FloatArray& values, const FloatArray& scaleValues) {
  size_t n = std::min(values.size(), scaleValues.size());
  for (size_t i=0; i<n; ++i) {
    values[i] *= scaleValues[i];
  }
}

void scaleArrayInPlace(float* values, const float* scaleValues, int numValues) {
  for (int i=0; i<numValues; ++i) {
    values[i] *= scaleValues[i];
  }
}

void copyArrayValues(const float* source, float* target, int numValues) {
  for (int i=0; i<numValues; ++i) {
    target[i] = source[i];
  }
}

void copyArrayValues(const FloatArray& source, int sourceOffset,
  FloatArray& target, int targetOffset, int numValues) {
    copyArrayValues(&source[sourceOffset], &target[targetOffset], numValues);
}

void addArrayValues(const float* source, float* target, int numValues) {
  for (int i=0; i<numValues; ++i) {
    target[i] += source[i];
  }
}

bool isClose(const FloatArray& a, const FloatArray& b, float tolerance) {
  if (a.size() != b.size()) {
    return false;
  }
  int n = (int)a.size();
  for (int i=0; i<n; ++i) {
    if (!IsClose(a[i], b[i], tolerance)) {
      return false;
    }
  }
  return true;
}

bool isClose(const SparseArray& a, const SparseArray& b, float tolerance) {
  if (a.size() != b.size()) {
    return false;
  }
  int n = (int)a.size();
  for (int i=0; i<n; ++i) {
    if (!IsClose(a.get(i), b.get(i), tolerance)) {
      return false;
    }
  }
  return true;
}

bool isZero(const FloatArray& a) {
  for (float value : a) {
    if (value != 0.0f) {
      return false;
    }
  }
  return true;
}

float getRMSE(const FloatArray& a, const FloatArray& b) {
  int n = (int)std::min(a.size(), b.size());
  if (n == 0) {
    return 0.0f;
  }
  float sum = 0.0f;
  float inverseN = 1.0f / static_cast<float>(n);
  for (int i=0; i<n; ++i) {
    float e = b[i] - a[i];
    sum += inverseN * (e*e);
  }
  return sqrtf(sum);
}


float getMaxDifference(const FloatArray& a, const FloatArray& b) {
  int n = (int)std::min(a.size(), b.size());
  float result = 0.0f;
  for (int i=0; i<n; ++i) {
    result = std::max(result, std::abs(a[i]-b[i]));
  }
  return result;
}

float getMaxDifference(const FloatArray& a, const FloatArray& b, int offsetA, int offsetB) {
  int n = std::min((int)a.size() - offsetA, (int)b.size() - offsetB);
  float result = 0.0f;
  for (int i=0; i<n; ++i) {
    result = std::max(result, std::abs(a[offsetA+i] - b[offsetB+i]));
  }
  return result;
}

float getAbsMax(const FloatArray& values) {
  float result = 0.0f;
  for (float v : values) {
    result = std::max(result, std::abs(v));
  }
  return result;
}

float getAbsMin(const FloatArray& values) {
  float result = (values.empty() ? 0.0f : std::abs(values[0]));
  for (float v : values) {
    result = std::min(result, std::abs(v));
  }
  return result;
}

void printArray(const char* label, const float* values, int numValues) {
  printf("%s: ", label);
  for (int i=0; i<numValues; ++i) {
    printf("%s%g", (i==0?"":", "), values[i]);
  }
  printf("\n");
}

void printArray(const char* label, const FloatArray& values) {
  printArray(label, values.data(), (int)values.size());
}

void printArray(const char* label, const SparseArray& values) {
  printf("%s: ", label);
  for (int i=0; i<(int)values.size(); ++i) {
    printf("%s%g", (i==0?"":", "), values.get(i));
  }
  printf("\n");

}
