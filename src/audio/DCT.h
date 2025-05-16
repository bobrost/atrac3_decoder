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
  // @param outputSignal The output samples buffer, must be size (numInputs*2)
  // @param outputScale Optional constant scale to apply to outputs
  // @return Whether successful (numInputs was a power of 2)
  bool MDCT_Inverse_Brute(const float* inputFrequencies, int numInputs, float* outputSignal, float outputScale=1.0f);

  // resizes the output signal appropriately
  bool MDCT_Inverse_Brute(
    const std::vector<float>& inputFrequencies,
    std::vector<float>& outputSignal, float outputScale);

  // Perform an Inverse MDCT using a fast method.
  // @param inputFrequencies The input frequencies buffer
  // @param numInputs Size of the input frequencies, must be a power of 2
  // @param outputSignal The output samples buffer, must be size (numInputs*2)
  // @param outputScale Optional constant scale to apply to outputs
  // @return Whether successful (numInputs was a power of 2)
  bool MDCT_Inverse_Fast(const float* inputFrequencies, int numInputs, float* outputSignal, float outputScale=1.0f);
    
} // namespace DCT
