#pragma once

#include "AtracConstants.h"
#include "io/Bitstream.h"

namespace Atrac3Frame {

  // Method used to encode coefficient mantissas in a SoundUnit
  enum CodingMode : uint8_t {
    VariableLengthCoded = 0,
    ConstantLengthCoded = 1,
    Invalid = 2,
    PerComponent = 3,
  };

  // Gain compensation points define a scaling curve for a 256-sample overlapping
  // section between two neighboring frames of a subband.
  //
  // A frame subband with no gain data points implicitly has a constant gain of 1.0.
  // Each gain point defines what the gain level (an exponential gain factor) should
  // already be before that location (multiplied by 8 to get a sample offset); the
  // gain then exponentially interpolates over the next 8 frames to the next level,
  // then maintains constant value until the next location.
  //
  // In the bitstream, a frame's gain data points define the scaling curve for the
  // second half of its subbands, as it is the outgoing frame in the mix. It also
  // implicitly provides a constant scale factor for its first half. During the overlap
  // between two neighboring frame subbands, the outgoing frame's gain curve applies to
  // both, and the incoming frame additionally scales by its constant factor.
  //
  // Note: This gain data is identical to the gain control data from the MPEG AAC SSR
  // profile, also developed by Sony.
  struct GainDataPoint {
    uint8_t levelCode; // 4 bits
    uint8_t locationCode; // 5 bits, offset within the frame, in multiples of 8 samples
  };
  using GainDataPointArray = std::vector<GainDataPoint>;


  // Tonal components are harmonically-important frequencies that are independently extracted
  // from the overall spectrum (from each of the 4 QMF subbands), allowing those frequencies
  // to be encoded with high precision or accuracy. After subtracting these components from the
  // spectrum, what's left is much more uniform amplitude and closer to the noise floor, so that
  // spectral encoding can work more efficiently, or enable its lossy encoding to have less effect
  // on the overall sound.
  //
  // In the ATRAC3 bitstream, tonal components are grouped by their encoding parameters. Each
  // group specifies its quantization step, entropy coding mode, and number of encoded
  // values (1-8) per component; these parameters are used for each of the child tonal components
  // in the group. Each child tonal component specifies a starting frequency offset and an array
  // of amplitudes.
  //
  // A single audio frame may contain up to 31 tonal groups, and each group may contain
  // up to 7 components.
  struct TonalComponent {
    int scaleFactorIndex = 0; // 0-63
    int positionOffset = 0; // start offset within the tonal bin, 0-63
    int tonalBin = 0; // major start offset within the frequency spectrum (64 frequencies per bin)
    int tableSelector = 0; // huffman table, same as quantization step index

    // Unscaled amplitude values. Signed integers within the range of the associated quantization table.
    std::vector<int> mantissas;

    int startFrequency = 0; // Position within the 1024 frequency spectrum
    float scaleFactor = 1;
// TODO: TonalComponent and SpectralSubband should both inherit from a
// subclass so it's easy to accumulate arrays of them into the spectrum.
// Or at least have the same value names so a template works.
  };

  struct TonalComponentGroup {
    int numValuesPerChildComponent = 0; // 1-8
    int quantizationStepIndex = 0; // table lookup index, 2-7
    CodingMode codingMode = CodingMode::Invalid;
    std::vector<TonalComponent> childComponents;
  };

  // Part of a SoundUnit
  struct SpectralSubband {
    int tableSelector = 0; // 3 bits
    int scaleFactorIndex = 0; // 6 bits

    int startFrequency = 0;
    int numValues = 0;
    std::vector<int> mantissas;

    float scaleFactor = 1;
  };

  // The primary encoding block for a channel of ATRAC3 data.
  // Each stereo frame has 2 sound units.
  struct SoundUnit {
    // Gain compensation interpolation points for each subband.
    // 4 arrays of gain points.
    std::vector<GainDataPointArray> gainCompensationBands; // 2 bits = (numBands-1)

    // Up to 31 tonal components define musically-relevant frequencies
    std::vector<TonalComponentGroup> tonalGroups; // 5 bits num groups

    std::vector<SpectralSubband> spectralBands; // 5 bits num subbands
  };


  class Parser {
  public:
    // TODO? Are there configuration options for parsing? Certainly LP2 vs LP4

    // Parse a single sound unit from a bitstream.
    // @param bitstream A bitstream for a single sound unit of data (192 bytes for LP2)
    // @return Number of bytes read, or -1 if error
    int parseSoundUnit(IBitstreamReader& bitstream, SoundUnit& result) const;

  private:

    // Control points for the gain compensation, up to 7 points per subband.
    bool parseGainCompensationSubbands(IBitstreamReader& bitstream,
      std::vector<GainDataPointArray>& subbands, int numEncodedSubbands) const;

    // Tonal components, specific frequencies with high amplitude or high precision
    // that generally are dissimilar from the surrounding spectrum.
    bool parseTonalComponentGroups(IBitstreamReader& bitstream, int numEncodedBands,
      std::vector<TonalComponentGroup>& tonalGroups) const;
    bool parseTonalComponentGroup( IBitstreamReader& bitstream, int numEncodedBands,
      CodingMode defaultCodingMode, TonalComponentGroup& resultGroup) const;
    bool parseTonalComponent(IBitstreamReader& bitstream,
      CodingMode codingMode, int quantizationStepIndex, int numValuesPerComponent,
      int tonalBin, TonalComponent& result) const;

    // Residual spectral amplitudes per freuency, 32 subbands of unequal size
    bool parseSpectralSubbands(IBitstreamReader& bitstream,
      std::vector<SpectralSubband>& resultSubbands) const;

    // Parse frequency mantissas, either constant length or variable length coded
    bool parseEncodedValues(IBitstreamReader& bitstream,
      CodingMode codingMode, int tableIndex, int numValues, std::vector<int>& result) const;
    bool parseConstantLengthEncodedValues(IBitstreamReader& bitstream,
      int tableIndex, int numValues, std::vector<int>& result) const;
    bool parseVariableLengthEncodedValues(IBitstreamReader& bitstream,
      int tableIndex, int numValues, std::vector<int>& result) const;

    // Constant data from the ATRAC spec
    Atrac3::Atrac3Constants _constants;
  };

}
