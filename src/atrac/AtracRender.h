#pragma once

#include "AtracFrame.h"
#include "audio/QMF.h"

namespace Atrac3Render {

  // State to maintain consistency for sequentially-decoded sound units of the same channel
  struct ChannelRenderState {
    ChannelRenderState() {
      qmf.init(constants.qmfHalfCoefficients, Atrac3::kQmfDecodingScale);
    }
    Atrac3::Atrac3Constants constants;

    // accumulated state
    Qmf::QuadBandUpsampler qmf;
    FloatArray outputPcm;

    // scratch space
    struct Subband {
      FloatArray unscaled = FloatArray(512); // inverse DCT result
      FloatArray windowed = FloatArray(512); // inverse DCT scaled by decoding window
      FloatArray prevWindowed = FloatArray(512); // previous frame's windowed subband
      Atrac3Frame::GainDataPointArray prevGainData; // previous frame's gain compensation data, if any
      FloatArray gain = FloatArray(256); // rendered gain compensation data, applies to previous and current frames
      FloatArray mix = FloatArray(256); // gain compensated mix of windowed and prevWindowed overlap regions
    };
    FloatArray spectrum = FloatArray(1024);
    std::vector<Subband> subbands = { {}, {}, {}, {}}; // default-init 4 subbands
  };

  template<typename T>
  void accumulateSpectrum(FloatArray& targetSpectrum, const std::vector<T>& entries) {
    for (const T& entry : entries) {
      int n = entry.mantissas.size();
      for (int i=0; i<n; ++i) {
        targetSpectrum[entry.startFrequency+i] += entry.mantissas[i] * entry.scaleFactor;
      }

    }
  }

  void accumulateSpectrum(FloatArray& targetSpectrum, const std::vector<Atrac3Frame::TonalComponentGroup>& tonalGroups);

  int getInitialGainLevelCode(const std::vector<Atrac3Frame::GainDataPointArray>& bands, int bandIndex);


  // Calculate the gain compensation scaling curve for the overlapping portion (256 samples)
  // of 2 neighboring frames of a subband. The curve applies to the mix of the second half
  // of the previous-frame subband and the first half of the corresponding subband in the
  // current frame (times a constant scale).
  // @param constants The static ATRAC3 constants
  // @param prevFrameGainPoints The gain data points for a given subband from the previous
  //   frame sound unit, from 0 to 7 points.
  // @param currFrameLevelCode The gain level code for the first gain point of the current
  //   frame's same subband, or kGainCompensationNormalizedLevel (4) if none exists.
  // @param resultCurve The pre-allocated 256-value array to fill with the resulting per-sample
  //   gain scale. The values will range from 0.00048828125 (2^-11) to 16, and should be applied
  //   to both the previous frame's lead-out section and the current frame's lead-in.
  // @param resultLeadInScale A scale that will be set with an additional constant multiplier
  //   that should be applied to all of the current frame's lead-in samples during mixing.
  bool renderGainControlCurve(
    const Atrac3::Atrac3Constants& constants,
    const Atrac3Frame::GainDataPointArray& prevFrameGainPoints,
    int currFrameLevelCode,
    FloatArray& resultCurve,
    float& resultLeadInScale);

  // Render the current sound unit to output. This relies on rendering consecutive sound units for the same channel
  // number, since some of the rendering uses data from the previous frame.
  // NOTE: This is assuming stereo LP2, not Joint-stereo LP4.
  // @param state The persistent state for the given channel. The result PCM samples will be appended to `state.outputPcm`.
  // @param curr The current sound unit to process and render
  void renderSoundUnit(ChannelRenderState& state, const Atrac3Frame::SoundUnit& curr);

}
