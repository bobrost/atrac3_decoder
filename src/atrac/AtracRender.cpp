#include "AtracRender.h"
#include "audio/DCT.h"
#include <cmath>

namespace {

  // Utility function for rendering gain compensation. If needed, ramps gain
  // geometrically from fromGain to toGain over 8 samples starting at fromOffset.
  // Then it maintains constant toGain through (toOffset-1).
  void rampThenConstant(
    const FloatArray& gainLevelTable,
    FloatArray& result,
      int fromOffset, int toOffset,
      int fromGainIndex, int toGainIndex) {
    int offset = fromOffset;
  
    if (fromGainIndex != toGainIndex && fromOffset < toOffset) {
      const int deltaIndex = fromGainIndex - toGainIndex;
      const float multiplier = std::powf(2.0f, (float)deltaIndex/8); // TODO: put another table in the constants for performance?
      const float fromGain = gainLevelTable[fromGainIndex];
      float gain = fromGain;
      for (int i=0; i<8; ++i) {
        result[offset++] = gain;
        gain *= multiplier;
      }
    }
    const float toGain = gainLevelTable[toGainIndex];
    while (offset < toOffset) {
      result[offset++] = toGain;
    }
  }
}

namespace Atrac3Render {

  void accumulateSpectrum(FloatArray& targetSpectrum, const std::vector<Atrac3Frame::TonalComponentGroup>& tonalGroups) {
    for (const auto& group : tonalGroups) {
      accumulateSpectrum(targetSpectrum, group.childComponents);
    }
  }

  bool renderGainControlCurve(
    const Atrac3::Atrac3Constants& constants,
    const Atrac3Frame::GainDataPointArray& gainPoints,
    int nextFrameLevelCode,
    FloatArray& resultCurve,
    float& resultLeadInScale) {

    // The lead-out curve will be a constant scale of the lead-in, based on the start of the next gain data block
    resultLeadInScale = constants.gainCompensationLevelTable[nextFrameLevelCode];

    // Verify inputs
    if (resultCurve.size() != Atrac3::kNumSamplesPerGainCompensation ||
      gainPoints.size() > Atrac3::kMaxGainCompensationPointsPerSubband) {
      return false;
    }

    // If this frame has no gain control, set to constant 1.0 gain
    if (gainPoints.empty()) {
      std::fill(resultCurve.begin(), resultCurve.end(), 1.0f);
      return true;
    }
  
    // Start calculating gain control. Maintain constant gain to each control point, then
    // ramp over 8 samples to the next gain value, repeat until the end of the buffer.
    int offset = 0;
    int gainIndex = gainPoints[0].levelCode;
    for (const auto& p : gainPoints) {
      const int toOffset = p.locationCode * 8;
      const int toGainIndex = p.levelCode;
      rampThenConstant(constants.gainCompensationLevelTable,
        resultCurve, offset, toOffset, gainIndex, toGainIndex);
      offset = toOffset;
      gainIndex = toGainIndex;
    }
    // Interpolate back to normalized scale to the final sample of this block
    rampThenConstant(constants.gainCompensationLevelTable,
      resultCurve, offset, Atrac3::kNumSamplesPerGainCompensation,
      gainIndex, Atrac3::kGainCompensationNormalizedLevel);

    return true;
  }

  int getInitialGainLevelCode(const std::vector<Atrac3Frame::GainDataPointArray>& bands, int bandIndex) {
    return ((int)bands.size() > bandIndex && !bands[bandIndex].empty() ?
      bands[bandIndex][0].levelCode :
      Atrac3::kGainCompensationNormalizedLevel);
  }

  void renderSoundUnit(ChannelRenderState& state, const Atrac3Frame::SoundUnit& curr) {
    
    // Populate the spectrum from the tonal components and spectral subbands
    FloatArray& spectrum = state.spectrum;
    spectrum.assign(Atrac3::kNumFrequenciesInSpectrum, 0.0f);
    accumulateSpectrum(spectrum, curr.tonalGroups);
    accumulateSpectrum(spectrum, curr.spectralBands);

    // Reverse the partial spectrum for subbands 1 and 3. (This likely is to account
    // for frequency reflection across the Nyquist frequency when downsampling
    // the upper QMF bands).
    constexpr int kInputDctSize = Atrac3::kNumFrequenciesPerSubband;
    float* spectrumSubbands[4] = {
      &spectrum[0],
      &spectrum[kInputDctSize],
      &spectrum[kInputDctSize*2],
      &spectrum[kInputDctSize*3]
    };
    reverseArrayInPlace(spectrumSubbands[1], kInputDctSize);
    reverseArrayInPlace(spectrumSubbands[3], kInputDctSize);

    // Render each QMF subband from its spectrum, and mix with the previous frame overlap
    constexpr float kDctScale = -1.0f;
    for (int bandIndex=0; bandIndex<Atrac3::kNumSubbands; ++bandIndex) {
      constexpr int kSubbandSize = kInputDctSize * 2;

      ChannelRenderState::Subband& subband = state.subbands[bandIndex];
      // TODO: use fast DCT
      DCT::MDCT_Inverse_Fast(spectrumSubbands[bandIndex], kInputDctSize, subband.unscaled.data(), kDctScale);
      multiplyArrays(subband.unscaled, state.constants.decodingScalingWindow, subband.windowed);

      // Calculate and apply gain compensation scaling per subband. The previous frame's gain data
      // defines the scaling curve for its lead-out and this frame's lead-in (256 sample overlap per
      // subband). This frame's lead-in is also constant-scaled based on its own initial gain data point.
      float leadInScale = 1.0f;
      renderGainControlCurve(
        state.constants,
        subband.prevGainData,
        getInitialGainLevelCode(curr.gainCompensationBands, bandIndex),
        subband.gain, leadInScale);
      for (int i=0; i<256; ++i) {
        subband.mix[i] = subband.gain[i] * (subband.windowed[i] * leadInScale + subband.prevWindowed[i+256]);
      }

      // Prepare for the next frame's calculation on this subband.
      // The rest of this frame's calculation will use the mix buffer.
      subband.prevWindowed = subband.windowed;
      subband.prevGainData = curr.gainCompensationBands[bandIndex];
    }

    // Upsample the QMF subbands, first 256 samples of each subband.
    // Generates 1024 samples for most frames, but less for the first frame.
    constexpr int kNumSamplesPerQmfBuffer = Atrac3::kNumSamplesPerGainCompensation;
    int numSamplesGenerated = state.qmf.combineSubbands(
      state.subbands[0].mix, state.subbands[1].mix,
      state.subbands[2].mix, state.subbands[3].mix,
      kNumSamplesPerQmfBuffer, state.outputPcm);
  }

}
