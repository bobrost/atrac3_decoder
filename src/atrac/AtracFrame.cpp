#include "AtracFrame.h"
#include "util/Logging.h"

namespace Atrac3Frame {

  constexpr const char* kLogCategory = "Atrac3Frame";

  int Parser::parseSoundUnit(IBitstreamReader& bitstream, SoundUnit& result) const {

    int startByte = bitstream.getByteOffset();
    LogDebug(kLogCategory, "Atrac3 Sound Unit, offset byte %d", startByte);
    result = SoundUnit(); // clear

// TODO: support LP4

    // Verify the header
    LogDebug(kLogCategory, "verifying 0x%x (%d) magic header", Atrac3::kMagicHeaderLP2, Atrac3::kMagicHeaderLP2);
    if (bitstream.getBits(6) != Atrac3::kMagicHeaderLP2) {
      LogError(kLogCategory, "Failed to parse atrac3 sound unit magic header");
      return false;
    }

    // Parse how many of the 4 QMF subbands have encoded data
    int numEncodedQmfBands = bitstream.getBits(2) + 1; // 1 through 4, inclusive
    LogDebug(kLogCategory, "  %d QMF bands encoded", numEncodedQmfBands);

    // Parse the gain compensation curves for the encoded subbands
    if (!parseGainCompensationSubbands(bitstream, result.gainCompensationBands, numEncodedQmfBands)) {
      LogError(kLogCategory, "Failed to parse gain compensation subbands");
      return false;
    }

    // Parse the tonal component groups
    if (!parseTonalComponentGroups(bitstream, numEncodedQmfBands, result.tonalGroups)) {
      LogError(kLogCategory, "Failed to parse tonal components");
      return false;
    }

    // Parse spectral subbands
    if (!parseSpectralSubbands(bitstream, result.spectralBands)) {
      LogError(kLogCategory, "Failed to parse spectral subbands");
      return false;
    }
    LogDebug(kLogCategory, "  %d spectral subbands", static_cast<uint32_t>(result.spectralBands.size()));

    int endByte = bitstream.getByteOffset();
    LogDebug(kLogCategory, "Maybe read atrac3 sound unit (%d bytes)", endByte - startByte);
    return true;
  }

  bool Parser::parseGainCompensationSubbands(IBitstreamReader& bitstream,
      std::vector<GainDataPointArray>& subbands, int numEncodedBands) const {
    // Parse the MPEG AAC gain data level/location code pairs, to define
    // the gain curve for a subband. Up to 7 data points allowed per subband.
    subbands.resize(numEncodedBands);
    for (GainDataPointArray& gainArray : subbands) {
      int numGainPoints = bitstream.getBits(3);
      LogDebug(kLogCategory, "  ParseQmfBandGainDataPoints(%d data points)", numGainPoints);
      gainArray.resize(static_cast<size_t>(numGainPoints));
      for (GainDataPoint& g : gainArray) {
        if (!bitstream.tryGetBits(4, g.levelCode) ||
            !bitstream.tryGetBits(5, g.locationCode)) {
            return false;
        }
      }
    }
    LogDebug(kLogCategory, "  parsed %d gain compensation bands", numEncodedBands);
    return true;
  }

  bool Parser::parseTonalComponentGroups(IBitstreamReader& bitstream,
      int numEncodedBands, std::vector<TonalComponentGroup>& tonalGroups) const {

    int numGroups = bitstream.getBits(5);
    tonalGroups.resize(static_cast<size_t>(numGroups));
    LogDebug(kLogCategory, "  parseTonalComponentGroups(%d groups)", numGroups);
    if (numGroups == 0) {
      // Early exit if no tonal components
      return true;
    }

    CodingMode defaultCodingMode = static_cast<CodingMode>(bitstream.getBits(2));
    if (defaultCodingMode == CodingMode::Invalid) {
      return false;
    }

    for (TonalComponentGroup& group : tonalGroups) {
      if (!parseTonalComponentGroup(bitstream, numEncodedBands, defaultCodingMode, group)) {
        return false;
      }
    }
    LogDebug(kLogCategory, "  parsed %d tonal component groups", (int)(tonalGroups.size()));
    return true;
  }

  bool Parser::parseTonalComponentGroup(
        IBitstreamReader& bitstream,
        int numEncodedQmfBands,
        CodingMode defaultCodingMode,
        TonalComponentGroup& resultGroup) const {
    // Of the subbands encoded in this frame, get a bitmask telling which of those
    // subbands have data in this tonal group.
    std::vector<bool> encodedSubbands = bitstream.getBitArray(numEncodedQmfBands);
    resultGroup.numValuesPerChildComponent = bitstream.getBits(3)+1; // 1 to 8
    resultGroup.quantizationStepIndex = bitstream.getBits(3);
    if (resultGroup.quantizationStepIndex <= 1) {
      return false;
    }
    resultGroup.codingMode = (defaultCodingMode == CodingMode::PerComponent ?
      static_cast<CodingMode>(bitstream.getBit()) :
      defaultCodingMode);
    resultGroup.childComponents.clear();

    // See the Atrac constants header for information on the tonal bin concepts
    int totalNumComponents = 0;
    for (int qmfSubband = 0; qmfSubband < numEncodedQmfBands; ++qmfSubband) {
      if (encodedSubbands[qmfSubband]) {
        for (int subbandBin = 0; subbandBin < Atrac3::kNumTonalBinsPerSubband; ++subbandBin) {
          int numComponentsInBin = bitstream.getBits(3);
          int tonalBin = (qmfSubband * Atrac3::kNumTonalBinsPerSubband) + subbandBin;
          for (int i=0; i<numComponentsInBin; ++i) {
            TonalComponent component;
            if (!parseTonalComponent(bitstream, resultGroup.codingMode,
              resultGroup.quantizationStepIndex,
              resultGroup.numValuesPerChildComponent, tonalBin,
              component)) {
              return false;
            }
            resultGroup.childComponents.emplace_back(std::move(component));
            ++totalNumComponents;
          }
        }
      }
    }
    if (totalNumComponents > Atrac3::kMaxTonalComponentsPerGroup) {
      LogDebug(kLogCategory, "Parsed total %d tonal components, more than maximum %d",
        totalNumComponents, Atrac3::kMaxTonalComponentsPerGroup);
      return false;
    }
    return true;
  }


  bool Parser::parseTonalComponent(
      IBitstreamReader& bitstream,
      CodingMode codingMode,
      int quantizationStepIndex,
      int numValuesPerComponent,
      int tonalBin,
      TonalComponent& result) const {

    result.scaleFactorIndex = bitstream.getBits(6);
    result.positionOffset = bitstream.getBits(6); // 0-63, relative to bin start
    result.tonalBin = tonalBin;
    result.scaleFactor = _constants.getScaleFactor(result.scaleFactorIndex) *
        _constants.inverseQuantization[quantizationStepIndex];


    // Determine frequency range and num values, truncating if array will go past
    // the end of the spectrum
    result.startFrequency = (result.tonalBin * Atrac3::kNumFrequenciesPerTonalBin) + result.positionOffset;
    int endFrequency = std::min(
      result.startFrequency + numValuesPerComponent,
      Atrac3::kNumFrequenciesInSpectrum);
    int numValues = endFrequency - result.startFrequency;

    // Read the mantissas
    // Note: (quantizationStepIndex >= 2), from earlier verification
    result.tableSelector = quantizationStepIndex;
    return parseEncodedValues(bitstream, codingMode,
      result.tableSelector, numValues, result.mantissas);
  }

  /*

  const char* Parser::getCodingModeName(CodingMode mode) {
    switch(mode) {
      case CodingMode::VariableLengthCoded:
        return "VariableLength";
      case CodingMode::ConstantLengthCoded:
        return "ConstantLength";
      case CodingMode::PerComponent:
        return "PerComponent";
      default:
        return "Invalid";
    }
  }
  */


  bool Parser::parseSpectralSubbands(
        IBitstreamReader& bitstream,
        std::vector<SpectralSubband>& resultSubbands) const {
    int numSubbands = bitstream.getBits(5) + 1;
    CodingMode codingMode = static_cast<CodingMode>(bitstream.getBit());
    resultSubbands.resize(static_cast<size_t>(numSubbands));

    // Get 3 bits selector tables, get range, and init values to 0
    for (int i=0; i<numSubbands; ++i) {
      SpectralSubband& subband = resultSubbands[i];
      subband.tableSelector = bitstream.getBits(3);
      if (!_constants.getSpectralSubbandOffsets(
          i, subband.startFrequency, subband.numValues)) {
        return false;
      }
      subband.mantissas.assign(static_cast<size_t>(subband.numValues), 0);
    }

    // Get 6 bits scale factor index for each unskipped subband
    for (SpectralSubband& subband : resultSubbands) {
      subband.scaleFactorIndex = (subband.tableSelector == 0 ? 0 :
        bitstream.getBits(6));
      subband.scaleFactor =
        _constants.getScaleFactor(subband.scaleFactorIndex) *
        _constants.inverseQuantization[subband.tableSelector];
    }

    // Read values for each unskipped subband
    for (SpectralSubband& subband : resultSubbands) {
      if (subband.tableSelector != 0) {
        if (!parseEncodedValues(bitstream, codingMode,
          subband.tableSelector, subband.numValues, subband.mantissas)) {
          return false;
        }
      }
    }
    return true;
  }


  bool Parser::parseEncodedValues(
          IBitstreamReader& bitstream,
          CodingMode codingMode,
          int tableIndex,
          int numValues,
          std::vector<int>& result) const {
      // tableIndex != 0 for spectrum
      // tableIndex !=0 and !=1 for tonal
      switch (codingMode) {
        case CodingMode::ConstantLengthCoded:
          return parseConstantLengthEncodedValues(bitstream, tableIndex, numValues, result);
        case CodingMode::VariableLengthCoded:
          return parseVariableLengthEncodedValues(bitstream, tableIndex, numValues, result);
        default:
          break;
      }
    return false;
  }


  bool Parser::parseConstantLengthEncodedValues(
        IBitstreamReader& bitstream,
        int tableIndex,
        int numValues,
        std::vector<int>& result) const {
    result.resize(static_cast<size_t>(numValues));

    if (tableIndex <= 0) {
      for (int i=0; i<numValues; ++i) {
        result[i] = 0;
      }
    /*
    } else if (tableIndex == 1) {
      // TODO: ffmpeg interleaves from 4 bits, but isn't it the
      // same as just using 2 bits?
      constexpr int kTable1Mantissas[4] = {0,1,-2,-1};
      for (int i=0; i<numValues; ++i) {
        result[i] = kTable1Mantissas[bitstream.GetBits(2)];
      }
    */
    } else {
      //constexpr int kMantissaBitLengths[8] = { 0, 0/*4*/, 3, 3, 4, 4, 5, 6 };
      //int numBits = kMantissaBitLengths[tableIndex];
      int numBits = _constants.constantLengthNumBits[tableIndex];
      for (int i = 0; i < numValues; ++i) {
        result[i] = bitstream.getSignedBits(numBits);
      }
    }
    return true;
  }

  bool Parser::parseVariableLengthEncodedValues(
        IBitstreamReader& bitstream,
        int tableIndex,
        int numValues,
        std::vector<int>& result) const {
    result.resize(static_cast<size_t>(numValues));
    if (tableIndex < 0) {
      for (int i=0; i<numValues; ++i) {
        result[i] = 0;
      }
    } else if (tableIndex == 1) {
      // The even/odd pairs in this table cover all combinations
      // of -1,0, and 1, allowing a single Huffman-coded index to
      // efficiently specify two values.
      constexpr int kMantissaTable[18] = {
        0, 0, 0, 1, 0, -1, 1, 0, -1, 0,
        1, 1, 1, -1, -1, 1, -1, -1
      };
      // TODO: make sure numValues is always even for table 1?
      const HuffmanTable& table = _constants.huffmanTables[tableIndex];
      for (int i=0; i<numValues; i+=2) {
        int code = table.readCode(bitstream);
        result[i] = kMantissaTable[code*2];
        result[i+1] = kMantissaTable[code*2+1];
      }
    } else {
      //printf("table %d\n", tableIndex);
      const HuffmanTable& table = _constants.huffmanTables[tableIndex];
      if (!table.readCodes(bitstream, (int)result.size(), result)) {
        LogDebug(kLogCategory, "parseVariableLengthEncodedValues failed to read codes");
        return false;
      }
      //PrintIntVector(result);
    }
    return true;
  }

} // namespace

