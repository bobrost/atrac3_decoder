#include "QMF.h"

namespace Qmf {

  FloatArray mirrorCoefficients(const FloatArray& halfCoefficients, float scale) {
    size_t halfN = halfCoefficients.size();
    size_t n = halfN * 2;
    FloatArray result(n);
    for (size_t i=0; i<halfN; ++i) {
      result[i] = result[n-1-i] = (halfCoefficients[i] * scale);
    }
    return result;
  }

  void qmfCombineUpsample(
      const FloatArray& coefficients,
      float lowpass, float highpass,
      HistoryBuffer& demodulationBuffer,
      float& outSample1, float& outSample2) {
    // Demodulation
    demodulationBuffer.append(lowpass + highpass);
    demodulationBuffer.append(lowpass - highpass);

    // Calculate the odd and even sparse dot products between the coefficients
    // and the demodulation history
    int n = (int)coefficients.size();
    //int historyOffset = -n;
    outSample1 = 0.0f;
    outSample2 = 0.0f;
    for (int i=0; i<n; i+=2) {
      outSample1 += coefficients[i+1] * demodulationBuffer[i+1-n];
      outSample2 += coefficients[i] * demodulationBuffer[i-n];

    }
  }

  void qmfCombineUpsample(
    const FloatArray& coefficients,
    const FloatArray& lowpass,
    const FloatArray& highpass,
    HistoryBuffer& demodulationBuffer,
    FloatArray& appendToOutput) {
    size_t n = lowpass.size();
    appendToOutput.reserve(appendToOutput.size() + (n*2));

    float s1, s2;
    for (size_t i=0; i<n; ++i) {
      qmfCombineUpsample(coefficients, lowpass[i], highpass[i],
        demodulationBuffer, s1, s2);
      appendToOutput.push_back(s1);
      appendToOutput.push_back(s2);
    }
  }

  void QuadBandUpsampler::init(const FloatArray& halfCoefficients, float decodingScale) {
    _coefficients = Qmf::mirrorCoefficients(halfCoefficients, decodingScale);
    _numCoefficients = static_cast<int>(_coefficients.size());
    _history01 = HistoryBuffer(_numCoefficients);
    _history32 = HistoryBuffer(_numCoefficients);
    _history0132 = HistoryBuffer(_numCoefficients);
  }

  void QuadBandUpsampler::clear() {
    _history01.clear();
    _history32.clear();
    _history0132.clear();
  }

  void QuadBandUpsampler::combineSubbands(
      float b0, float b1, float b2, float b3,
      float& out0, float& out1, float& out2, float& out3) {
    float out01[2];
    float out32[2];
    qmfCombineUpsample(_coefficients, b0, b1, _history01, out01[0], out01[1]);
    qmfCombineUpsample(_coefficients, b3, b2, _history32, out32[0], out32[1]);
    qmfCombineUpsample(_coefficients, out01[0], out32[0], _history0132, out0, out1);
    qmfCombineUpsample(_coefficients, out01[1], out32[1], _history0132, out2, out3);
  }

  int QuadBandUpsampler::combineSubbands(
      const FloatArray& b0, const FloatArray& b1,
      const FloatArray& b2, const FloatArray& b3,
      int numInputSamples,
      FloatArray& outputAppendTarget) {
    // TODO: account for initial 46 sample output delay
    int outputOffset = (int)outputAppendTarget.size();
    outputAppendTarget.resize(outputOffset + numInputSamples * 4);
    for (int i=0; i<numInputSamples; ++i, outputOffset+=4) {
      combineSubbands(
        b0[i], b1[i], b2[i], b3[i],
        outputAppendTarget[outputOffset],
        outputAppendTarget[outputOffset+1],
        outputAppendTarget[outputOffset+2],
        outputAppendTarget[outputOffset+3]);
    }
    return (numInputSamples * 4);
  }


} // namespace
