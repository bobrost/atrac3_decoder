#pragma once

#include <vector>
#include <functional>

using FloatArray = std::vector<float>;

FloatArray initArray(int n, const std::function<float(int i)>& fn);

struct SparseArray;

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
bool isClose(const SparseArray& a, const SparseArray& b, float tolerance);

bool isZero(const FloatArray& a);

float getRMSE(const FloatArray& a, const FloatArray& b);

float getMaxDifference(const FloatArray& a, const FloatArray& b);
float getAbsMax(const FloatArray& values);
float getAbsMin(const FloatArray& values);

float getMaxDifference(const FloatArray& a, const FloatArray& b, int offsetA, int offsetB);

void printArray(const char* label, const float* values, int numValues);
void printArray(const char* label, const FloatArray& values);
void printArray(const char* label, const SparseArray& values);

/*
void SwapArrayValues(float* values, int indexA, int indexB) {
  std::swap(values[indexA], values[indexB]);
}
*/

struct SparseArray {
private:
  float* _values;
  int _numValues;
  int _startOffset;
  int _stride;
public:

  SparseArray(std::vector<float>& values)
    : SparseArray(values.data(), (int)values.size(), 0, 1) {}

  SparseArray(float* values, int numValues, int startOffset=0, int stride=1)
    : _values(values),
    _numValues(numValues),
    _startOffset(startOffset),
    _stride(stride) {}


  float get(int index) const {
    return _values[_startOffset + index * _stride];
  }

  void set(int index, float value) {
    _values[_startOffset + index * _stride] = value;
  }


  float operator[](int index) const {
    return _values[_startOffset + index * _stride];
  }
/*

  float& operator[](int index) {
    return _values[_startOffset + index * _stride];
  }
  */

  int size() const { return _numValues; }

  SparseArray offset(int amount) const {
    return SparseArray(
      _values,
      _numValues - amount,
      _startOffset + (amount * _stride),
      _stride);
  }

  void reverseRangeInPlace(int startIndex, int numValues) {
    const int halfN = numValues / 2;
    for (int i=0; i<halfN; ++i) {
      swapValues(startIndex+i, startIndex+numValues-1-i);
    }
  }

  void swapValues(int indexA, int indexB) {
    // TODO: use std::swap and operator[], deprecate this function
    const float temp = get(indexA);
    set(indexA, get(indexB));
    set(indexB, temp);
  }

  void swapRanges(int startA, int startB, int numValues) {
    for (int i=0; i<numValues; ++i) {
      swapValues(startA+i, startB+i);
    }
  }

};
/*
  // Copy values into the start of this Span from a source, with optional scaling
  public copyValues(values:ReadOnlySpan, n:number, scale:number=1.0):void {
    //console.log('copyValues', values, n, scale);
    for (let i:number=0; i<n; ++i) {
      //set(i, values[i]*scale);
      set(i, values.get(i)*scale);
    }
  }
  public offset(amount:number):Span {
    return new Span(source, start+(amount*stride), stride, numValues-amount);
  }

  public offsetReversed(offset:number):Span {
    return new Span(source, start+(offset*stride), -stride);
  }
  public even():Span {return new Span(source, start, stride*2);}
  public odd():Span {return new Span(source, start+1, stride*2);}
  
  public scaleValue(index:number, scale:number):void {
    set(index, get(index)*scale);
  }
  public addValue(index:number, value:number):void {
    set(index, get(index)+value);
  }

  public scaleValues(startIndex:number, numValues:number, scale:number):void {
    for (let i:number=0; i<numValues; ++i) {
      const index:number = startIndex+i;
      set(index, get(index)*scale);
    }
  }
  

*/


// Circular buffer for sample history
class HistoryBuffer /*: public IFloatBuffer*/ {
public:
  HistoryBuffer(int historySize = 100) {
    _n = std::max(1, historySize);
    _values.assign(_n, 0.0f);
  }
  HistoryBuffer(const HistoryBuffer&) = default;
  HistoryBuffer& operator=(const HistoryBuffer&) = default;

  /*
  void Append(const float* values, int n) {
    for (int i=0; i<n; ++i) {
      Append(values[i]);
    }
  }
  */

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