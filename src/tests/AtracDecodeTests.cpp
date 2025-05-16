#include "TestRunner.h"
#include "AtracTestSchema.h"
#include "atrac/AtracConstants.h"
#include "atrac/AtracFrame.h"
#include "atrac/AtracRender.h"

#include "io/IO.h"

#include "util/ArrayUtil.h"
#include "util/StringUtil.h"

#include "audio/DCT.h"
#include "audio/QMF.h"

#include <memory>

// It's unclear why we need the output scale of the inverse MDCT
// to be negative. I've implemented according to the formal definition
// on Wikipedia, but ffmpeg's output is negative of mine (also divided
// by 15 bits to normalize the scale).
constexpr float kAtracInverseMdctScale = -1.0f / 32768.0f;

constexpr float kTolerance = 0.00001f;

void TestSchema::AtracSchema::initIndices() {
  for (int frameIndex=0; frameIndex<(int)frames.size(); ++frameIndex) {
    auto& frame = frames[frameIndex];
    frame.index = frameIndex;

    for (int channelIndex=0; channelIndex<(int)frame.channels.size(); ++channelIndex) {
      auto& channel = frame.channels[channelIndex];
      channel.index = channelIndex;

      for (int bandIndex=0; bandIndex<(int)channel.bands.size(); ++bandIndex) {
        auto& band = channel.bands[bandIndex];
        band.index = bandIndex;
      }
    }
  }
}

namespace {

  static std::shared_ptr<TestSchema::AtracSchema> testDataInstance;

  // Make sure the constants class doesn't cause memory issues when
  // creating or deallocating
  TestResult createAtracConstants() {
    Atrac3::Atrac3Constants constants;
    return true;
  }

  // Given known audio decoding data, ensure that our inverse MDCT
  // output matches the reference decoder, including scale, sign,
  // and subband frequency reversal
  TestResult testDecodeInverseMdct() {
    const TestSchema::AtracSchema& testData = *testDataInstance;
    // Temp buffers for each subband's frequencies and samples
    constexpr int kNumFrequencies = 256;
    constexpr int kNumSamples = 512;
    FloatArray frequencies(kNumFrequencies);
    FloatArray samples(kNumSamples);

    for (const auto& frame : testData.frames) {
      for (const auto& channel : frame.channels) {
        for (const auto& band : channel.bands) {
          const FloatArray& spectrum = channel.spectrum;
          // Prepare the expected results
          const FloatArray& expectedFrequencies = band.frequencies;

          int numFrequencies = (int)expectedFrequencies.size();
          if (numFrequencies != kNumFrequencies) {
            return string_format(
              "Num Frequencies %d instead of %d (frame %d, channel %d, band %d)",
              numFrequencies, kNumFrequencies, frame.index, channel.index, band.index);
          }
          const FloatArray& expectedImdct = band.imdct;
          int imdctSize = (int)expectedImdct.size();
          if (imdctSize != kNumSamples) {
            return string_format("IMDCT buffer size %d instead of %d", imdctSize, kNumSamples);
          }

          // Copy the 256 frequencies subband from the spectrum, reversing odd bands
          copyArrayValues(spectrum, kNumFrequencies * band.index,
            frequencies, 0, kNumFrequencies);
          if (band.index % 2 == 1) {
            reverseArrayInPlace(frequencies);
          }
          if (!isClose(frequencies, expectedFrequencies, kTolerance)) {
            float maxDiff = getMaxDifference(frequencies, expectedFrequencies);
            return string_format(
              "Frequencies not equal (frame %d, channel %d, band %d, max difference %f)",
              frame.index, channel.index, band.index, maxDiff);
          }

          // Perform the inverse MDCT
          // Note: It's unclear why the scale should be -1 here. I've implemented iMDCT according
          // to the standard formula, but testing shows that it's the inverse of ffmpeg's Atrac3
          // setup.
          constexpr float kImdctOutputScale = -1.0f / 32768.0f;
          constexpr float kImdctTolerance = 0.01f / 32768.0f;
          // TODO: use fast instead of brute version
          if (!DCT::MDCT_Inverse_Brute(frequencies.data(), kNumFrequencies, samples.data(), kImdctOutputScale)) {
            return "Error performing IMDCT";
          }
          if (!isClose(samples, expectedImdct, kImdctTolerance)) {
            /*
            printf("Arrays not equal (frame %d, channel %d, band %d, max difference %f)\n",
              frame.index,
              channel.index,
              band.index,
              GetMaxDifference(samples, expectedImdct));
              */
            printArray("samples", samples);
            printArray("expected", expectedImdct);
            //return false;
            float maxDiff = getMaxDifference(samples, expectedImdct);
            return string_format("Arrays not equal (frame %d, channel %d, band %d, max difference %f)",
              frame.index, channel.index,  band.index, maxDiff);
          }

          // TODO: confirm the windowing
        }
      }
    }
    return true;
  }

  // Read and parse the json file of known data exported from an instrumented ffmpeg
  TestResult loadTestDataJson() {
    const std::string jsonFilename = "data/test_data.json";
    std::vector<uint8_t> jsonFileContents;
    if (!IO::readFileContents(jsonFilename, jsonFileContents)) {
      return string_format("Could not read JSON file: %s", jsonFilename.c_str());
    }
    TestSchema::AtracSchema testData = nlohmann::json::parse(std::string_view(
      reinterpret_cast<const char*>(jsonFileContents.data()),
      jsonFileContents.size()));
    testData.initIndices();
    testDataInstance = std::make_shared<TestSchema::AtracSchema>(std::move(testData));
    return true;
  }

  TestResult testDecodeBytesToSpectrum() {
    Atrac3Frame::Parser parser;

    const TestSchema::AtracSchema& testData = *testDataInstance;
    for (const auto& frame : testData.frames) {
      const auto& channel = frame.channels[0];
      BitstreamReader bitstream(frame.bytes);
      Atrac3Frame::SoundUnit su; // left channel
      if (!parser.parseSoundUnit(bitstream, su)) {
        return "error parsing sound unit";
      }
      // Build the spectrum from the sound unit
      FloatArray spectrum;
      spectrum.assign(1024, 0.0f);
      Atrac3Render::accumulateSpectrum(spectrum, su.tonalGroups);
      Atrac3Render::accumulateSpectrum(spectrum, su.spectralBands);
      // Check correctness
      constexpr float kTolerance = 0.0001f;
      if (!isClose(spectrum, channel.spectrum, kTolerance)) {
        return string_format("Non-matching spectrum, frame %d", frame.index);
      }
    }
    return true;
  }

  TestResult testDecodeScalingWindow() {
    const TestSchema::AtracSchema& testData = *testDataInstance;
    Atrac3::Atrac3Constants constants;
    for (const auto& frame : testData.frames) {
      for (const auto& channel : frame.channels) {
        for (const auto& band : channel.bands) {
          const FloatArray& source = band.imdct;
          const FloatArray& expected = band.imdctWindowed;
          FloatArray scaled = source;
          scaleArrayInPlace(scaled, constants.decodingScalingWindow);
          if (!isClose(expected, scaled, kTolerance)) {
            return false;
          }
        }
      }
    }
    return true;
  }

  TestResult testGainCompensationWindow() {
    constexpr float kGainTolerance = 0.001f;
    const TestSchema::AtracSchema& testData = *testDataInstance;
    Atrac3::Atrac3Constants constants;
    Atrac3Frame::Parser parser;


    std::vector<Atrac3Frame::GainDataPointArray> prevGainDataArrays;
    prevGainDataArrays.resize(8);

    FloatArray gainResult(256);

    for (const auto& frame : testData.frames) {
      for (const auto& channel : frame.channels) {
        int dataOffset = channel.index * 192;
        BitstreamReader bitstream(frame.bytes.data() + dataOffset, 192);
        Atrac3Frame::SoundUnit su;
        parser.parseSoundUnit(bitstream, su);
        for (const auto& band : channel.bands) {
          int gainArrayIndex = channel.index * 4 + band.index;
          const Atrac3Frame::GainDataPointArray& gainData = prevGainDataArrays[gainArrayIndex];

          int finalLevelCode = Atrac3Render::getInitialGainLevelCode(su.gainCompensationBands, band.index);
          float leadInScale = -1.0f;
          Atrac3Render::renderGainControlCurve(constants, gainData, finalLevelCode, gainResult, leadInScale);
          if (!isClose(gainResult, band.gainScale, kGainTolerance)) {
           return string_format("frame %d channel %d band %d gain scale mismatch", frame.index, channel.index, band.index);
          }
          prevGainDataArrays[gainArrayIndex] = su.gainCompensationBands[band.index];
        }
      }
    }
    return true;
  }


  TestResult testQmfUpsampling() {
    const TestSchema::AtracSchema& testData = *testDataInstance;

    // Ensure that the QMF upsampling matches known data. Since QMF relies
    // on 46 samples of previous data per stage, we can process all the
    // frames in order and make sure the output matches consistently.
    Atrac3::Atrac3Constants constants;
    FloatArray coefficients = Qmf::mirrorCoefficients(
      constants.qmfHalfCoefficients, Atrac3::kQmfDecodingScale);

    HistoryBuffer history01(Atrac3::kNumQmfCoefficients);
    HistoryBuffer history23(Atrac3::kNumQmfCoefficients);
    HistoryBuffer history0123(Atrac3::kNumQmfCoefficients);

    for (const auto& frame : testData.frames) {
      const auto& channel = frame.channels[0];
      const TestSchema::QmfStages& stages = channel.qmf;

      FloatArray out01, out23, out0123;
      Qmf::qmfCombineUpsample(coefficients, stages.stage01.low, stages.stage01.high,
        history01, out01);
      Qmf::qmfCombineUpsample(coefficients, stages.stage32.low, stages.stage32.high,
        history23, out23);
      Qmf::qmfCombineUpsample(coefficients, stages.stage0123.low, stages.stage0123.high,
        history0123, out0123);

      if (!isClose(out01, stages.stage01.out, kTolerance)) {
        return string_format("Frame %d stage01", frame.index);
      }
      if (!isClose(out23, stages.stage32.out, kTolerance)) {
        return string_format("Frame %d stage32", frame.index);
      }
      if (!isClose(out0123, stages.stage0123.out, kTolerance)) {
        return string_format("Frame %d stage0123", frame.index);
      }   
    }
    return true;
  }

  TestResult testQmfQuadBandUpsampling() {
    const TestSchema::AtracSchema& testData = *testDataInstance;

    // Ensure that the QMF upsampling matches known data. Since QMF relies
    // on 46 samples of previous data per stage, we can process all the
    // frames in order and make sure the output matches consistently.
    Atrac3::Atrac3Constants constants;
    Qmf::QuadBandUpsampler upsampler;
    upsampler.init(constants.qmfHalfCoefficients, Atrac3::kQmfDecodingScale);

    FloatArray outBuffer; // output samples per frame channel
    for (const auto& frame : testData.frames) {
      outBuffer.clear();
      const auto& channel = frame.channels[0];
      const TestSchema::QmfStages& stages = channel.qmf;

      int numOutputSamples = upsampler.combineSubbands(
        stages.stage01.low,
        stages.stage01.high,
        stages.stage32.high, // Note: bands 2 and 3 are swapped in the QMF upsampling
        stages.stage32.low,
        256, outBuffer);
      // TODO Frame 0 may have only 978 samples once this is fixed
      constexpr float kTolerance = 0.0001f; // Note: Relative to normalized sample values
      if (!isClose(outBuffer, stages.stage0123.out, kTolerance)) {
        float error = getMaxDifference(outBuffer, stages.stage0123.out);
        return string_format("Frame %d has QMF difference %f (tolerance %f)", frame.index, error, kTolerance);
      }
    }
    return true;
  }

  // Get the 4 subband initial level codes from the sound unit, or 
  std::vector<int> getSubbandInitialGainLevelCodes(const Atrac3Frame::SoundUnit& unit) {
    std::vector<int> result = {
      Atrac3::kGainCompensationNormalizedLevel,
      Atrac3::kGainCompensationNormalizedLevel,
      Atrac3::kGainCompensationNormalizedLevel,
      Atrac3::kGainCompensationNormalizedLevel
    };
    const auto& bands = unit.gainCompensationBands;
    for (size_t i=0; i<4; ++i) {
      if (bands.size() > i && !bands[i].empty()) {
        result[i] = bands[i][0].levelCode;
      }
    }
    return result;
  }

  void printDifference(const char* label, int frameIndex, const FloatArray& a, const FloatArray& b) {
    if (a.size() != b.size()) {
      printf("%s[%d] size difference (%d vs %d)\n", label, frameIndex, (int)a.size(), (int)b.size());
    }
    float maxA = getAbsMax(a);
    float maxB = getAbsMax(b);
    float diff = getMaxDifference(a,b);
    float maxVal = std::max(maxA, maxB);
    float percent = 100.0f * (maxVal == 0.0f ? 0.0f : diff / maxVal);
    float rmse = getRMSE(a, b);
    printf("%s[%d] max(%f, %f), error %f, rMSE %f\n",
      label, frameIndex,
      maxA, maxB, diff, rmse);
  }

  #if 0
  void printExtendedFrameInfo(
      const Atrac3Render::ChannelRenderState& channelRenderState,
      const TestSchema::AtracSchemaFrame& currFrame,
      const TestSchema::AtracSchemaFrame& nextFrame,
      int frameIndex,
      const Atrac3Frame::SoundUnit& currSoundUnit,
      const Atrac3Frame::SoundUnit& nextSoundUnit) {
    // TODO: Compare to expected output. Account for output delay in first frame?
    // TODO: double check the delay
    const FloatArray& actual = channelRenderState.outputPcm;
    const FloatArray& expected = currFrame.channels[0].qmf.stage0123.out;

    const FloatArray& expectedImdct = currFrame.channels[0].bands[0].imdct;
    const FloatArray& actualImdct = channelRenderState.subbands[0].unscaled;
    const FloatArray& expectedWindowed = currFrame.channels[0].bands[0].imdctWindowed;
    const FloatArray& actualWindowed = channelRenderState.subbands[0].windowed;
    float windowError = getMaxDifference(expectedWindowed, actualWindowed);

    const FloatArray& expectedGain = currFrame.channels[0].bands[0].gainScale;
    const FloatArray& actualGain = channelRenderState.subbands[0].gain;
    if (!isClose(expectedGain, actualGain, 0.001f)) {
      printf("frame %d gain window mismatch:\n", frameIndex);
      printArray("expected", expectedGain);
      printArray("actual", actualGain);
      printf("sound unit gain points: curr (%d), next (%d)\n",
        (int)currSoundUnit.gainCompensationBands[0].size(),
        (int)nextSoundUnit.gainCompensationBands[0].size());
      printf("test frame gain points: curr (%d), next (%d)\n",
        (int)currFrame.channels[0].bands[0].gain.size(),
        (int)nextFrame.channels[0].bands[0].gain.size());
      //return "gain scaling window mismatch";
      return;
    }

    // less than 1 sample value is good enough!
    const auto& qmf = currFrame.channels[0].qmf;
    printf("frame %d, max PCM value %f, error %f\n",
      frameIndex,
      getAbsMax(channelRenderState.outputPcm),
      getMaxDifference(channelRenderState.outputPcm, qmf.stage0123.out));
    printDifference("  qmf01.lo", frameIndex, channelRenderState.subbands[0].mix, qmf.stage01.low);
    printDifference("  qmf01.hi", frameIndex, channelRenderState.subbands[1].mix, qmf.stage01.high);
    printDifference("  qmf32.lo", frameIndex, channelRenderState.subbands[3].mix, qmf.stage32.low); // NOTE: Swapped!
    printDifference("  qmf32.hi", frameIndex, channelRenderState.subbands[2].mix, qmf.stage32.high);
    //printDifference("  qmf.0123", frameIndex, channelRenderState.subbands[2].mix, qmf.stage0123.low);
  }

  void printExtendedFrameFailureInfo(
      const Atrac3Render::ChannelRenderState& channelRenderState,
      const TestSchema::AtracSchemaFrame& currFrame,
      const TestSchema::AtracSchemaFrame& nextFrame,
      int frameIndex,
      const Atrac3Frame::SoundUnit& currSoundUnit,
      const Atrac3Frame::SoundUnit& nextSoundUnit) {

    const FloatArray& actual = channelRenderState.outputPcm;
    const FloatArray& expected = currFrame.channels[0].qmf.stage0123.out;
    float error = getMaxDifference(actual, expected);
    printArray("expected", expected); printf("\n");
    printf("frame %d: pcm length %d vs %d, error %f (max %f vs %f)\n",
      frameIndex, (int)actual.size(), (int)expected.size(), error, getAbsMax(actual), getAbsMax(expected));

    const auto& ch = currFrame.channels[0];
    printDifference("imdct0", frameIndex, channelRenderState.subbands[0].unscaled, ch.bands[0].imdct);
    printDifference("imdct1", frameIndex, channelRenderState.subbands[1].unscaled, ch.bands[1].imdct);
    printDifference("imdct2", frameIndex, channelRenderState.subbands[2].unscaled, ch.bands[2].imdct);
    printDifference("imdct3", frameIndex, channelRenderState.subbands[3].unscaled, ch.bands[3].imdct);

    printDifference("win0", frameIndex, channelRenderState.subbands[0].windowed, ch.bands[0].imdctWindowed);
    printDifference("win1", frameIndex, channelRenderState.subbands[1].windowed, ch.bands[1].imdctWindowed);
    printDifference("win2", frameIndex, channelRenderState.subbands[2].windowed, ch.bands[2].imdctWindowed);
    printDifference("win3", frameIndex, channelRenderState.subbands[3].windowed, ch.bands[3].imdctWindowed);

    printDifference("mix0", frameIndex, channelRenderState.subbands[0].mix, ch.bands[0].gainMixOverlap);
    printDifference("mix1", frameIndex, channelRenderState.subbands[1].mix, ch.bands[1].gainMixOverlap);
    printDifference("mix2", frameIndex, channelRenderState.subbands[2].mix, ch.bands[2].gainMixOverlap);
    printDifference("mix3", frameIndex, channelRenderState.subbands[3].mix, ch.bands[3].gainMixOverlap);

    const auto& qmf = currFrame.channels[0].qmf;
    printDifference("qmf01.lo", frameIndex, channelRenderState.subbands[0].mix, qmf.stage01.low);
    printDifference("qmf01.hi", frameIndex, channelRenderState.subbands[1].mix, qmf.stage01.high);
    printDifference("qmf32.lo", frameIndex, channelRenderState.subbands[3].mix, qmf.stage32.low); // NOTE: Swapped!
    printDifference("qmf32.hi", frameIndex, channelRenderState.subbands[2].mix, qmf.stage32.high);
  }

  #endif

  TestResult testFullDecoding() {
    // copy of the test data so it can be scaled in-place
    // Pre-scale some of the test data from normalized to signed 16-bit range
    TestSchema::AtracSchema testData = *testDataInstance;
    for (auto& frame : testData.frames) {
      constexpr float kScale = 32768.0f;
      for (auto& channel : frame.channels) {
        std::vector<FloatArray*> valueArrays = {
          &channel.qmf.stage01.low, &channel.qmf.stage01.high, &channel.qmf.stage01.out,
          &channel.qmf.stage32.low, &channel.qmf.stage32.high, &channel.qmf.stage32.out,
          &channel.qmf.stage0123.low, &channel.qmf.stage0123.high, &channel.qmf.stage0123.out,
          &channel.bands[0].imdct, &channel.bands[0].imdctWindowed, &channel.bands[0].gainMixOverlap,
          &channel.bands[1].imdct, &channel.bands[1].imdctWindowed, &channel.bands[1].gainMixOverlap,
          &channel.bands[2].imdct, &channel.bands[2].imdctWindowed, &channel.bands[2].gainMixOverlap,
          &channel.bands[3].imdct, &channel.bands[3].imdctWindowed, &channel.bands[3].gainMixOverlap,
        };
        for (auto& values : valueArrays) {
          scaleArrayInPlace(*values, kScale);
        }
      }
    }

    
    // Decode and render each left channel from the bitstream
    // Skip the last frame, because our test data has been truncated and
    // doesn't include the following gain control start value
    Atrac3Frame::Parser parser;
    Atrac3Render::ChannelRenderState channelRenderState;
    int numFrames = (int)testData.frames.size();
    Atrac3Frame::SoundUnit prevSoundUnit;
    for (int frameIndex = 0; frameIndex < numFrames-1; ++frameIndex) {
      const TestSchema::AtracSchemaFrame& currFrame = testData.frames[frameIndex];
      const TestSchema::AtracSchemaFrame& nextFrame = testData.frames[frameIndex+1];

      // Parse the bitstreams and sound units
      Atrac3Frame::SoundUnit currSoundUnit;
      BitstreamReader currBitstream(currFrame.bytes);
      if (!parser.parseSoundUnit(currBitstream, currSoundUnit)) {
        return string_format("error parsing frames %d - %d", frameIndex, frameIndex+1);
      }

      // Render and accumulate into the channelRenderState.outputPcm
      Atrac3Render::renderSoundUnit(channelRenderState, currSoundUnit);
      
      // Note: Tolerance here is in 16-bit sample values. Anything less than 1 is CD quality.
      constexpr float kTolerance = 0.01f;
      if (!isClose(channelRenderState.outputPcm, currFrame.channels[0].qmf.stage0123.out, kTolerance)) {
        return string_format("Error decoding frame, frame %d", currFrame.index);
      }
      channelRenderState.outputPcm.clear(); // Prepare for next frame
      prevSoundUnit = std::move(currSoundUnit);
    }
    return true;
  }

} // namespace


// Tests comparing internal logic to the decoding stages I exported
// to a JSON file from an instrumented reference decoder.
void addAtracDecodeTests(TestRunner& runner) {
  runner.add("atrac constants deallocation", createAtracConstants);
  runner.add("read atrac json data", loadTestDataJson);
  runner.add("spectrum imdct should match expected value", testDecodeInverseMdct);
  runner.add("decode scaling window should match expected curve", testDecodeScalingWindow);
  runner.add("gain compensation should decode the scaling window correctly", testGainCompensationWindow);
  runner.add("qmf upsampling should create expected output signal", testQmfUpsampling);
  runner.add("qmf quad band upsampling should work", testQmfQuadBandUpsampling);
  runner.add("decoding bytes should create expected spectrum", testDecodeBytesToSpectrum);
  runner.add("full channel decoding", testFullDecoding);
}
