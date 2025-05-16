#include "TestRunner.h"
#include "util/ArrayUtil.h"
#include "audio/FFT.h"
#include <cmath>

namespace {
  constexpr float kTolerance = 0.00001f;

  // These tests are using an 8-point FFT against known values

  TestResult testForwardFFT() {
    FloatArray signalReal = {1,2,3,4,5,6,7,8};
    FloatArray signalImag = {0,0,0,0,0,0,0,0};
    FloatArray expectedReal = {36,-4,-4,-4,-4,-4,-4,-4};
    FloatArray expectedImag = {0, 9.65685424949238f, 4, 1.6568542494923797f, 0, -1.6568542494923797f, -4, -9.65685424949238f };
    FFT::forwardFFT(signalReal.data(), signalImag.data(), 8);
    return (isClose(signalReal, expectedReal, kTolerance) &&
      isClose(signalImag, expectedImag, kTolerance));
  }

  TestResult testForwardFFTInterleaved() {
    FloatArray signal = {1,0,2,0,3,0,4,0,5,0,6,0,7,0,8,0};
    FloatArray expected = {36,0,-4,9.65685424949238f,-4,4,-4,1.6568542494923797f,-4,0,-4,-1.6568542494923797f,-4,-4,-4,-9.65685424949238f };
    FFT::forwardFFT(&signal[0], &signal[1], 8, 2);
    return isClose(signal, expected, kTolerance);
  }

  TestResult testInverseFFT() {
    FloatArray input = {1,2,3,4,5,6,7,8,0,0,0,0,0,0,0,0};
    FloatArray expectedFft = {
      36,-4,-4,-4,-4,-4,-4,-4, 
      0, 9.65685424949238f, 4, 1.6568542494923797f, 0, -1.6568542494923797f, -4, -9.65685424949238f
    };
    FloatArray signal = input;
    FFT::forwardFFT(&signal[0], &signal[8], 8);
    if (!isClose(signal, expectedFft, kTolerance)) {
      return "Frequency transform doesn't match";
    }
    FFT::inverseFFT(&signal[0], &signal[8], 8);
    return isClose(input, signal, kTolerance);
  }
} // namespace

void addFftTests(TestRunner& runner) {
  runner.add("forward FFT", testForwardFFT);
  runner.add("forward FFT (interleaved)", testForwardFFTInterleaved);
  runner.add("inverse FFT", testInverseFFT);
}
