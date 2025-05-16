#pragma once

#include <vector>

// Functions for handling forms of the DCT (Discrete Cosine Transform)
namespace DCT {

  // Perform a forward MDCT (Modified Discrete Cosine Transform)
  // Note that outputFrequencies is half as large as inputSignal.
  bool MDCT_Brute(const float* inputSignal, int numInputs, float* outputFrequencies);

  // Perform an Inverse MDCT (Modified Discrete Cosine Transform)
  // Note that outputSignals must be twice as large as inputFrequencies.
  // @param inputFrequencies The input frequencies buffer
  // @param numInputs Size of the input frequencies, must be a power of 2
  // @param outputSamples The output samples buffer, must be size (numFrequencies*2)
  // @return Whether successful
  bool MDCT_Inverse_Brute(const float* inputFrequencies, int numInputs, float* outputSignal, float outputScale=1.0f);

  // resizes the output signal appropriately
  bool MDCT_Inverse_Brute(
    const std::vector<float>& inputFrequencies,
    std::vector<float>& outputSignal, float outputScale);

} // namespace DCT
