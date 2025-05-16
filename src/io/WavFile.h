#pragma once

#include "IO.h"

constexpr uint16_t kWavAudioFormatPcmUncompressed = 1;

struct WavFileInfo {
  IO::Endian endian = IO::Endian::Little;

  uint16_t audioDataFormat = 0; // 1=PCM uncompressed
  size_t audioDataOffset = 0;
  size_t audioDataSize = 0;

  uint16_t numChannels = 0;
  uint32_t samplesPerSecond = 0;
  uint16_t bitsPerSample = 0;

  uint32_t bytesPerSecond = 0;
  uint16_t blockAlign = 0;

};

bool readWavFile(const std::string& filename, WavFileInfo& resultInfo, std::vector<uint8_t>& resultAudioData);

// Class to write a simple PCM WAV file
class WavWriter {
  public:
    ~WavWriter();
    bool open(const std::string& filename, bool isStereo, uint32_t samplesPerSecond);
    void close();

    // Append samples in floating point 0-1 scale
    // @param samples The values to convert to signed 16-bit and write to the
    //   wav file. Should be (numSamples*numChannels) in length.
    // @param numSamples Number of time steps, regardless of mono or stereo
    void appendNormalized(const float* samples, size_t numSamplesPerChannel);

    void appendSigned16(const int16_t* samples, size_t numSamplesPerChannel);

    // Append data that is floating point but in the -32768 to +32767 range, and non-interleaved stereo
    // Wav file must be stereo
    bool appendFloat16StereoNonInterleaved(const float* left, const float* right, size_t numSamplesPerChannel);
    // Will use the shorter of the two buffers for the size
    bool appendFloat16StereoNonInterleaved(const std::vector<float>& left, const std::vector<float>& right);

  private:
    int16_t normalizedToSigned(float v) const;

    IO::Endian _endian = IO::Endian::Little;
    IO::FileWriter _file;
    uint32_t _sampleRate = 1;
    size_t _numChannels = 1;
    size_t _totalPcmByteSize = 0;
    std::vector<int16_t> _temp;
};
