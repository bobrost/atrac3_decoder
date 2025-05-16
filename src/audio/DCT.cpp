#include "DCT.h"
#include "../util/ArrayUtil.h"
#include "../util/MathUtil.h"
#include <cmath>


namespace DCT {

  bool DCT2_Brute(const float* inputSignal, float* outputFrequencies, int N) {
    if (!IsPowerOfTwo(N)) {
      return false;
    }
    const float piOverN = kPi / static_cast<float>(N);
    for (int outputIndex=0; outputIndex<N; ++outputIndex) {
      float sum = 0.0f;
      for (int inputIndex=0; inputIndex<N; ++inputIndex) {
        float scale = std::cosf(piOverN * (inputIndex+0.5f) * outputIndex);
        sum += inputSignal[inputIndex] * scale;
      }
      outputFrequencies[outputIndex] = sum;
    }
    return true;
  }

  bool DCT2_Inverse_Brute(const float* inputFrequencies, float* outputSignal, int N, float outputScale) {
    // Note: The DCT-III is the inverse of DCT-II, times a scale factor
    if (!IsPowerOfTwo(N)) {
      return false;
    }

    // The inverse DCT2 is equal to the DCT3 times 2/N
    outputScale *= 2.0f / static_cast<float>(N);

    // The rest of the DCT2 inverse is the standard DCT3
    const float piOverN = kPi / static_cast<float>(N);
    float halfInput0 = 0.5f * inputFrequencies[0];
    for (int outputIndex=0; outputIndex<N; ++outputIndex) {
      float sum = halfInput0;
      for (int inputIndex=1; inputIndex<N; ++inputIndex) {
        float scale = std::cosf(piOverN * inputIndex * (outputIndex + 0.5f));
        sum += inputFrequencies[inputIndex] * scale;
      }
      outputSignal[outputIndex] = sum * outputScale;
    }
    return true;
  }

  bool DCT4_Brute(const float* inputSignal, float* outputFrequencies, int N, float outputScale) {
    const float piOverN = kPi / static_cast<float>(N);
    for (int outputIndex=0; outputIndex<N; ++outputIndex) {
      float sum = 0.0f;
      for (int inputIndex=0; inputIndex<N; ++inputIndex) {
        float scale = std::cosf(
          piOverN *
          (inputIndex+0.5f) *
          (outputIndex + 0.5f));
        sum += inputSignal[inputIndex] * scale;
      }
      outputFrequencies[outputIndex] = sum * outputScale;
    }
    return true;
  }

  bool MDCT_Brute(const float* inputSignal, int numInputs, float* outputFrequencies) {
    if (!IsPowerOfTwo(numInputs)) {
      return false;
    }
    // for 2N input samples and N output frequencies
    const int numOutputs = numInputs / 2;
    const int N = numOutputs;
    const float piOverN =  kPi / N;

    for (int outIndex = 0; outIndex < numOutputs; ++outIndex) {
      float sum = 0.0f;
      const float tScale = piOverN * (outIndex + 0.5f);
      for (int inputIndex = 0; inputIndex < numInputs; ++inputIndex) {
        float scale = std::cosf(
          tScale *
          (inputIndex + 0.5f + numOutputs*0.5f)
        );
        sum += inputSignal[inputIndex] * scale;
      }
      outputFrequencies[outIndex] = sum;
    }
    return true;
  }

  bool MDCT_Inverse_Brute(
      const float* inputFrequencies, int numInputs, float* outputSignal, float outputScale) {
    const int N = numInputs;
    const float Nf = static_cast<float>(N);
    const int TwoN = N * 2;
    const float piOverN =  kPi / Nf;
    const float NPlus1Over2 = (Nf+1.0f) / 2.0f;

    for (int outputIndex = 0; outputIndex < TwoN; ++outputIndex) {
      float sum = 0.0f;
      for (int inputIndex = 0; inputIndex < N; ++inputIndex) {
        sum += inputFrequencies[inputIndex] * std::cos(
          piOverN *
          (outputIndex + NPlus1Over2) *
          (inputIndex + 0.5f));
      }
      outputSignal[outputIndex] = sum * outputScale;
    }
    return true;
  }

  bool MDCT_Inverse_Brute(const FloatArray& inputFrequencies, FloatArray& outputSignal, float outputScale) {
    outputSignal.resize(inputFrequencies.size() * 2);
    return MDCT_Inverse_Brute(inputFrequencies.data(), (int)inputFrequencies.size(),
      outputSignal.data(), outputScale);
  }

} // namespace DCT
