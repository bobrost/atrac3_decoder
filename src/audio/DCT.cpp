#include "DCT.h"
#include "../util/ArrayUtil.h"
#include "../util/MathUtil.h"
#include "FFT.h"
#include <cmath>


namespace DCT {

  bool DCT2_Brute(const float* inputSignal, float* outputFrequencies, int N) {
    if (!isPowerOfTwo(N)) {
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
    if (!isPowerOfTwo(N)) {
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
    if (!isPowerOfTwo(numInputs)) {
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


  // Note: For further research, MDCT can be calculated as an N/4 FFT
  // https://ccrma.stanford.edu/~bosse/proj/node28.html

  bool MDCT_Inverse_Fast(const float* inputFrequencies, int numInputs, float* outputSignal, float outputScale) {
    if (!isPowerOfTwo(numInputs)) {
      return false;
    }
    // Note: by convention, inputs index by [k] and outputs by [n]
    const int N = numInputs;
    const int nFFT = 2 * N; // The FFT backing the computation is larger than the MDCT

    // Prepare temporary real and imag arrays for a size 2N FFT.
    // Zero pad the second half (from N through 2N-1), and ensure they
    // are at least 2N size. In this case, we're actually sizing them to
    // exactly 2N, but that should not be a problem since all of our MDCT
    // calculations are the same size.
    // (Note: Not threadsafe due to static vectors)
    static std::vector<float> real(nFFT, 0.0f);
    static std::vector<float> imag(nFFT, 0.0f);
    real.assign(nFFT, 0.0f);
    imag.assign(nFFT,0.0f);

    // Preprocess: complex rotation of inputFrequencies
    const float minusPiOver2N = -kPi / (2.0f * N);
    for (int k = 0; k < N; ++k) {
      float theta = minusPiOver2N * (N+1) * k;
      real[k] = inputFrequencies[k] * std::cos(theta);
      imag[k] = inputFrequencies[k] * std::sin(theta);
    }

    // Perform FFT
    FFT::forwardFFT(real.data(), imag.data(), nFFT);

    // Postprocess: another complex rotation
    float twoOverN = 2.0f / static_cast<float>(N);
    outputScale *= (N/2); // Account for the FFT internal scaling
    for (int n = 0; n < nFFT; ++n) {
      float theta = minusPiOver2N * (0.5f + N / 2.0f + n);
      float x = (real[n] * std::cos(theta)) - (imag[n] * std::sin(theta));
      outputSignal[n] = outputScale * x * twoOverN;
    }
    return true;
  }

} // namespace DCT
