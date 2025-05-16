#include <functional>

#include "io/WavFile.h"
#include "io/Bitstream.h"
#include "atrac/AtracConstants.h"
#include "atrac/AtracRender.h"
#include "util/Logging.h"
#include "util/MathUtil.h"
#include "util/CommandLineOptionsParser.h"

namespace {
  constexpr const char* kLogCategory = "AtracDecoder";
}

struct DecoderOptions {
  std::string inputFilename;
  std::string outputFilename;
  LogLevel logLevel = LogLevel::Info;

  // TODO: source from stdin instead of a file?
  // TODO: optional other non-WAV output format?
};

bool isLP2WavFile(const WavFileInfo& wavInfo) {
  return (
    wavInfo.audioDataFormat == Atrac3::kWavFormatAtrac3 &&
    wavInfo.bitsPerSample == 0 &&
    wavInfo.blockAlign == Atrac3::kLP2BytesPerStereoBlock &&
    wavInfo.bytesPerSecond == Atrac3::kLP2BytesPerSecond &&
    wavInfo.numChannels == 2);
}

int runDecoder(const DecoderOptions& options) {
  LogInfo(kLogCategory, "Decoding LP2 WAV file: %s", options.inputFilename.c_str());

  WavFileInfo wavInfo;
  std::vector<uint8_t> atracData;
  if (!readWavFile(options.inputFilename, wavInfo, atracData)) {
    LogError(kLogCategory, "Unable to read WAV file %s", options.inputFilename.c_str());
    return -1;
  }

  if (!isLP2WavFile(wavInfo)) {
    LogError(kLogCategory, "WAV file is not ATRAC3 LP2 format: %s", options.inputFilename.c_str());
    return -1;
  }

  WavWriter wavWriter;
  if (!wavWriter.open(options.outputFilename, true, 44100)) {
    LogError(kLogCategory, "Could not open output WAV file: %s", options.outputFilename.c_str());
    return -1;
  }
  LogInfo(kLogCategory, "Start output WAV file: %s", options.outputFilename.c_str());
  LogInfo(kLogCategory, "Start decoding ATRAC3 data (%d bytes)", (int)atracData.size());

  Atrac3Frame::Parser parser;
  Atrac3Render::ChannelRenderState leftChannel, rightChannel;
  int numStereoBlocks = static_cast<int>(atracData.size()) / Atrac3::kLP2BytesPerStereoBlock;

  //numStereoBlocks = 44 * 30; // shorter clip for testing
  size_t numOutputSamplesPerChannel = 0;
  for (int blockIndex = 0; blockIndex < numStereoBlocks; ++blockIndex) {
    // Left channel
    int leftOffset = blockIndex * Atrac3::kLP2BytesPerStereoBlock;
    BitstreamReader leftBitstream(&atracData[leftOffset], Atrac3::kLP2BytesPerSoundUnitChannel);
    Atrac3Frame::SoundUnit leftSoundUnit;
    parser.parseSoundUnit(leftBitstream, leftSoundUnit);
    Atrac3Render::renderSoundUnit(leftChannel, leftSoundUnit);

    // Right channel
    int rightOffset = leftOffset + Atrac3::kLP2BytesPerSoundUnitChannel;
    BitstreamReader rightBitstream(&atracData[rightOffset], Atrac3::kLP2BytesPerSoundUnitChannel);
    Atrac3Frame::SoundUnit rightSoundUnit;
    parser.parseSoundUnit(rightBitstream, rightSoundUnit);
    Atrac3Render::renderSoundUnit(rightChannel, rightSoundUnit);

    // Append the interleaved stereo audio data to the output file
    wavWriter.appendFloat16StereoNonInterleaved(leftChannel.outputPcm, rightChannel.outputPcm);
    numOutputSamplesPerChannel += leftChannel.outputPcm.size();
    leftChannel.outputPcm.clear();
    rightChannel.outputPcm.clear();

    if (blockIndex == numStereoBlocks-1 || blockIndex % 20 == 0) {
      LogVerbose(kLogCategory, "Decoded frame %d / %d", blockIndex, numStereoBlocks);
    }
  }
  wavWriter.close();

  int durationSeconds = static_cast<int>(numOutputSamplesPerChannel / 44100);
  LogInfo(kLogCategory, "Done, audio file duration %d:%02d", durationSeconds/60, durationSeconds%60);
  return 0;
}

int main(int argn, char** argv) {
  // Make sure the logger exists so the command line options parser can output errors initially
  PrintfLogger logger;
  logger.SetLevel(LogLevel::Error);
  ILogger::Set(&logger);

  // Set up initial options
  DecoderOptions options;
  options.inputFilename = "data/play_dead_atrac3_lp2.wav";
  options.outputFilename = "output.wav";

  // Parse options from the command line
  CommandLineOptionsParser optionsParser;
  optionsParser.add({"-i","--input"}, options.inputFilename, "Select the filename for the input file (a .wav file in ATRAC3 LP2 format)");
  optionsParser.add({"-o","--output"}, options.outputFilename, "Select the output .wav file to write");
  optionsParser.add({"-q","--quiet"}, [&](){options.logLevel = LogLevel::None;}, "No logging");
  optionsParser.add({"--info"}, [&](){options.logLevel = LogLevel::None;}, "Info level logging (default)");
  optionsParser.add({"-v","--verbose"}, [&](){options.logLevel = LogLevel::Verbose;}, "Verbose logging");
  optionsParser.add({"-d","--debug"}, [&](){options.logLevel = LogLevel::Debug;}, "Debug level logging (all log messages)");
  if (argn == 1 || !optionsParser.parse(argn, argv)) {
    optionsParser.printHelp();
    return -1;
  }
  logger.SetLevel(options.logLevel);

  // Run the decoder
  return runDecoder(options);
}
