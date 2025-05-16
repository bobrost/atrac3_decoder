#include "WavFile.h"
#include "IO.h"
#include "../util/Logging.h"

namespace WavFileInternal {

  constexpr const char* kLogCategory = "WavFile";

  constexpr size_t kRiffWavHeaderSize = 12;
  constexpr size_t kChunkHeaderSize = 8;
  constexpr size_t kFmtChunkMinPayloadSize = 16;
  constexpr uint32_t kTotalHeadersSize = 44; // RIFF (12), FMT header (8), FMT chunk (16), Data header (8)
  constexpr uint16_t kInvalidBlockAlign = 0xffff;

  constexpr const char* kChunkTypeFmt = "fmt ";
  constexpr const char* kChunkTypeData = "data";

  struct RiffWavHeader {
    // Byte ordering for internal structures
    IO::Endian endian = IO::Endian::Little;
  };

  struct ChunkHeader {
    // The type of chunk, a 4-character ASCII string
    std::string type;
    // Size of the chunk in bytes, including both header and payload
    size_t totalChunkSize = 0;
    // Start of the chunk payload data, relative to the file start
    //size_t payloadStart = 0;
    // Size of the chunk payload in bytes
    size_t payloadSize = 0;
  };

  // Try to parse the 12 byte RIFF WAVE header portion of a file (the beginning of the file).
  // @param data The data buffer to parse from (start of the file)
  // @param dataSize Size of the data buffer, must be at least 12
  // @Param totalFileSize Total size in bytes of the file being read, to verify completeness
  // @param result Will be set with the results on success
  // @return bytes successfully parsed, or 0 on failure
  size_t parseRiffWavHeader(const uint8_t* data, size_t dataSize, size_t totalFileSize, RiffWavHeader& result) {
    if (dataSize < kRiffWavHeaderSize) {
      return 0; // Error: not big enough
    }

    // Read the "RIFF" or "RIFX" type to get file endianness
    if (memcmp(data, "RIFF", 4) == 0) {
      result.endian = IO::Endian::Little;
    } else if (memcmp(data, "RIFX", 4) == 0) {
      result.endian = IO::Endian::Big;
    } else {
      return 0; // Error: invalid RIFF header
    }

    // Verify the total reported file size (note that 2 interpretations are common)
    // Note that although the spec says the header's reported file size should be just the
    // RIFF portion (actual file size - 8), it's also common to report the total file size.
    size_t riffFileSize = static_cast<size_t>(IO::readUInt32(result.endian, data+4));
    if (riffFileSize != totalFileSize && riffFileSize+8 != totalFileSize) {
      return 0; // Error: Invalid reported file size. Partial file?
    }

    // Verify it's a WAV file
    if (memcmp(data+8, "WAVE", 4) != 0) {
      return 0; // Error: not a WAVE file
    }
    return kRiffWavHeaderSize; // 12
  }

  // Attempt to parse a chunk header within the RIFF file, and verify the entire payload exists in the file
  // @param data The data buffer to parse from, aligned to the chunk header start
  // @param dataSize Available size of the data buffer, must be at least 8
  // @param totalFileSize Remaining size of the file in bytes, including this header, to verify completeness
  // @param endian The endianness of the file, from the RIFF header
  // @param result Will be set with the results on success
  // @return Num bytes successfully parsed, or 0 on failure
  size_t parseChunkHeader(const uint8_t* data, size_t dataSize, size_t remainingFileSize, IO::Endian endian, ChunkHeader& result) {
    // Chunk header format (8 bytes):
    //   4 bytes: fourcc chunk type, ascii
    //   4 bytes: uint32 chunk data payload size
    //   [chunk data payload immediately follows header]
    if (dataSize < kChunkHeaderSize) {
      LogError(kLogCategory, "Programmer error: Not enough buffer allocated");
      return 0; // Error: Not enough data to read a chunk header (need 8 bytes)
    }

    // Parse the 4-byte fourcc chunk type and make sure it is ascii
    result.type.clear();
    for (size_t i=0; i<4; ++i) {
      char c = data[i];
      if (c >= 32 && c <= 126) {
        result.type += c;
      } else {
        LogError(kLogCategory, "Chunk header type is not valid ASCII");
        return 0; // Error: Chunk type is not valid ASCII
      }
    }

    // Read the payload size
    result.payloadSize = static_cast<size_t>(IO::readUInt32(endian, data, 4));
    result.totalChunkSize = result.payloadSize + kChunkHeaderSize;
    if (result.totalChunkSize > remainingFileSize) {
      LogError(kLogCategory, "Not enough remaining file size for chunk payload (has %d, need %d)", (int)remainingFileSize, (int)result.totalChunkSize);
      return 0; // Error: Not enough data in the file for the chunk payload
    }
    return kChunkHeaderSize;
  }

  // Attempt to parse the Format chunk portion of a WAVE file (immediately after the 12 byte RIFF WAVE header)
  // @return Num bytes successfully parsed, or 0 on failure
  size_t parseFmtChunkPayload(const uint8_t* payload, size_t payloadSize, IO::Endian endian, WavFileInfo& result) {
    // Check minimum size
    if (payloadSize < kFmtChunkMinPayloadSize) {
      LogError(kLogCategory, "Fmt chunk payload size (%d) is not enough (expected %d or greater)", (int)payloadSize, (int)kFmtChunkMinPayloadSize);
      return 0; // Error: payload not enough
    }

    // Read FMT chunk data
    // 2 bytes uint16 audio data format (1=PCM)
    // 2 bytes uint16 num channels
    // 4 bytes uint32 sample rate
    // 4 bytes uint32 byte rate, total sample bytes per second (all channels)
    // 2 bytes uint16 block align, total bytes per sample (all channels)
    // 2 bytes uint16 bits per sample (per channel)
    result.audioDataFormat = IO::readUInt16(endian, payload, 0);
    result.numChannels = IO::readUInt16(endian, payload, 2);
    result.samplesPerSecond = IO::readUInt32(endian, payload, 4);
    result.bytesPerSecond = IO::readUInt32(endian, payload, 8);
    result.blockAlign = IO::readUInt16(endian, payload, 12);
    result.bitsPerSample = IO::readUInt16(endian, payload, 14);
    size_t resultNumBytesRead = 16;

    LogVerbose(kLogCategory, "WAV Format chunk");
    LogVerbose(kLogCategory, "  audio format = %d (hex %x)", result.audioDataFormat, result.audioDataFormat);
    LogVerbose(kLogCategory, "  num channels = %d", result.numChannels);
    LogVerbose(kLogCategory, "  sample rate = %d", result.samplesPerSecond);
    LogVerbose(kLogCategory, "  byte rate = %d", result.bytesPerSecond);
    LogVerbose(kLogCategory, "  block align = %d", result.blockAlign);
    LogVerbose(kLogCategory, "  bits per sample = %d", result.bitsPerSample);

    // https://www.mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
    if (payloadSize >= 18) {
      uint16_t maxExtraFormatSize = static_cast<uint16_t>(payloadSize - 18);
      uint16_t extraFormatSize = IO::readUInt16(endian, payload, 16);
      LogVerbose(kLogCategory, "  extra format size = %d (expected %d)",
        extraFormatSize, maxExtraFormatSize);
      if (extraFormatSize <= maxExtraFormatSize) {
        resultNumBytesRead += (2 + extraFormatSize);
        // TODO: additional payload starts at byte 18
      }
    }
    return resultNumBytesRead;
  }


  // Attempt to read and parse the RIFF WAVE header from the start of the file.
  // On sucess, the file will point to the start of the Fmt chunk.
  bool readRiffWavHeader(IO::FileReader& file, RiffWavHeader& result) {
    size_t startOffset = file.getReadOffset();
    std::vector<uint8_t> buffer;
    if ((file.readNext(kRiffWavHeaderSize, buffer) != kRiffWavHeaderSize) ||
        (parseRiffWavHeader(buffer.data(), buffer.size(), file.getSize(), result) != kRiffWavHeaderSize)) {
      file.seekTo(startOffset);
      return false;
    }
    return true;
  }

  // Attempt to read and parse an 8-byte chunk header from the current file position.
  // On success, the file will point to the start of the chunk payload.
  bool readChunkHeader(IO::FileReader& file, IO::Endian endian, ChunkHeader& result) {
    size_t startOffset = file.getReadOffset();
    size_t remainingFileSize = file.getSize() - startOffset;
    std::vector<uint8_t> buffer;
    
    // Make sure we read a valid chunk and the file has enough space for the payload
    if ((file.readNext(kChunkHeaderSize, buffer) != kChunkHeaderSize)) {
      LogError(kLogCategory, "File did not read %d bytes for chunk header", (int)kChunkHeaderSize);
      file.seekTo(startOffset);
      return false;
    }
    if (parseChunkHeader(buffer.data(), buffer.size(), remainingFileSize, endian, result) != kChunkHeaderSize) {
      LogError(kLogCategory, "Unable to parse chunk header successfully");
      file.seekTo(startOffset);
      return false;
    }
    return true;
  }

  // Attempt to read and parse the Fmt chunk (both header and payload) from the current file position.
  // On success, the file will point to the start of the next chunk.
  bool readFmtChunk(IO::FileReader& file, IO::Endian endian, WavFileInfo& result) {
    size_t startOffset = file.getReadOffset();
    size_t remainingFileSize = file.getSize() - startOffset;
    std::vector<uint8_t> buffer;

    // Make sure we read a valid chunk, it is the Fmt chunk type, and the payload is large enough
    ChunkHeader chunk;
    if (!readChunkHeader(file, endian, chunk)) {
      LogError(kLogCategory, "Unable to read fmt chunk header from file");
      file.seekTo(startOffset);
      return false;
    }
    if (chunk.type != kChunkTypeFmt || chunk.payloadSize < kFmtChunkMinPayloadSize) {
      LogError(kLogCategory, "Chunk header does not match expected (type '%s' expected '%s', size '%d' expected  '%d')",
        chunk.type.c_str(), kChunkTypeFmt, (int)chunk.payloadSize, (int)kFmtChunkMinPayloadSize);
      file.seekTo(startOffset);
      return false;
    }

    // Read and verify the Fmt payload
    if ((file.readNext(chunk.payloadSize, buffer) != chunk.payloadSize) ||
      (parseFmtChunkPayload(buffer.data(), buffer.size(), endian, result) != chunk.payloadSize)) {
      LogError(kLogCategory, "Unable to parse fmt chunk payload");
      file.seekTo(startOffset);
      return false; // Error: The fmt payload didn't parse correctly or was the wrong size
    }
    return true;
  }

  // Attempt to read past any unknown chunks, and find the audio data chunk's payload.
  // On success, the file will point to the start of the audio data.
  bool findAudioDataChunkPayload(IO::FileReader& file, IO::Endian endian, size_t& resultAudioDataPayloadSize) {
    size_t startOffset = file.getReadOffset();
    size_t totalFileSize = file.getSize();
    ChunkHeader chunk;
    while (readChunkHeader(file, endian, chunk)) {
      if (chunk.type == kChunkTypeData) {
        resultAudioDataPayloadSize = chunk.payloadSize;
        return true;
      } else {
        size_t nextOffset = file.getReadOffset() + chunk.payloadSize;
        file.seekTo(nextOffset);
      }
    }
    return false;
  }


  // Write an 8-byte chunk header
  // @param result Must be at least 8 bytes big
  // @param chunkType The chunk's ASCII fourcc
  // @return Total num bytes written
  uint32_t writeChunkHeader(uint8_t* result, IO::Endian endian, const char* chunkType, uint32_t chunkDataPayloadSize) {
    if (strlen(chunkType) != 4) {
      return 0;
    }
    strncpy((char*)result, chunkType, 4);
    IO::writeUInt32(result+4, endian, chunkDataPayloadSize);
    return kChunkHeaderSize;
  }

  // Write the standard 12-byte WAV RIFF file header
  // @param result Must be at least 12 bytes big
  // @return Total num bytes written
  uint32_t writeRiffWavHeader(uint8_t* result, IO::Endian endian, uint32_t expectedAudioDataPayloadSize) {
    // 4 bytes "RIFF" (little endian) or "RIFX" (big endian)
    strncpy((char*)(result), (endian == IO::Endian::Little ? "RIFF" : "RIFX"), 4);

    // 4 bytes uint32, (file size - 8)
    uint32_t expectedFileSize = kTotalHeadersSize + expectedAudioDataPayloadSize;
    IO::writeUInt32(result+4, endian, expectedFileSize - 8);

    // 4 bytes "WAVE"
    strncpy((char*)(result+8), "WAVE", 4);
    return kRiffWavHeaderSize;
  }

  // Write the 16-byte payload for the Wav format chunk
  uint32_t writeFmtChunkPayload(
      uint8_t* result,
      IO::Endian endian,
      uint16_t numChannels, // 1=mono, 2=stereo
      uint32_t samplesPerSecond,
      uint16_t audioDataFormat = 1, // 1 for PCM
      uint16_t bitsPerSample = 16, // assuming type 1 PCM 16-bit
      uint16_t blockAlign = kInvalidBlockAlign
      ) {
    
    uint32_t bytesPerSecond =
      static_cast<uint32_t>(bitsPerSample) *
      static_cast<uint32_t>(numChannels) *
      samplesPerSecond;
    if (blockAlign == kInvalidBlockAlign) {
      blockAlign = numChannels * (bitsPerSample/8);
    }
    uint8_t* writeCursor = result;

    // 2 bytes uint16 audio data format (1=PCM)
    writeCursor = IO::writeUInt16(writeCursor, endian, audioDataFormat);
    // 2 bytes uint16 num channels
    writeCursor = IO::writeUInt16(writeCursor, endian, numChannels);
    // 4 bytes uint32 sample rate
    writeCursor = IO::writeUInt32(writeCursor, endian, samplesPerSecond);
    // 4 bytes uint32 byte rate, total sample bytes per second (all channels)
    writeCursor = IO::writeUInt32(writeCursor, endian, bytesPerSecond);
    // 2 bytes uint16 block align, total bytes per sample (all channels)
    writeCursor = IO::writeUInt16(writeCursor, endian, blockAlign);
    // 2 bytes uint16 bits per sample (per channel)
    writeCursor = IO::writeUInt16(writeCursor, endian, bitsPerSample);
    return (writeCursor - result); //16;
  }


  bool writeWavFileHeaders(
      IO::FileWriter& file, IO::Endian endian,
      uint16_t numChannels, uint32_t samplesPerSecond,
      uint32_t expectedAudioDataSizeBytes) {
    if ((numChannels != 1 && numChannels != 2) || (samplesPerSecond < 1)) {
      return false;
    }

    uint8_t tempBuffer[44];
    uint32_t numBytes = 0;

    if (writeRiffWavHeader(tempBuffer, endian, expectedAudioDataSizeBytes) != kRiffWavHeaderSize ||
      !file.append(tempBuffer, kRiffWavHeaderSize)) {
      return false;
    }
    constexpr size_t kTotalFmtChunkSize = (kChunkHeaderSize + kFmtChunkMinPayloadSize);
    if (writeChunkHeader(tempBuffer, endian, kChunkTypeFmt, kFmtChunkMinPayloadSize) != kChunkHeaderSize ||
      writeFmtChunkPayload(tempBuffer+kChunkHeaderSize, endian, numChannels, samplesPerSecond) != kFmtChunkMinPayloadSize ||
      !file.append(tempBuffer, kTotalFmtChunkSize)) {
      return false;
    }
    if (writeChunkHeader(tempBuffer, endian, kChunkTypeData, expectedAudioDataSizeBytes) != kChunkHeaderSize ||
      !file.append(tempBuffer, kChunkHeaderSize)) {
      return false;
    }
    return true; // ready to write the audio data payload
  }
}

bool readWavFile(const std::string& filename, WavFileInfo& resultInfo, std::vector<uint8_t>& resultAudioData) {
  using WavFileInternal::kLogCategory;
  IO::FileReader file(filename);
  if (!file.isOpen()) {
    LogError(kLogCategory, "Could not open wav file for reading");
    return false;
  }

  // Read RIFF wav header
  WavFileInternal::RiffWavHeader riff;
  if (!WavFileInternal::readRiffWavHeader(file, riff)) {
    LogError(kLogCategory, "Unable to parse RIFF WAV header");
    return false;
  }

  // Read FMT chunk header and data
  if (!WavFileInternal::readFmtChunk(file, riff.endian, resultInfo)) {
    LogError(kLogCategory, "Unable to parse WAV format chunk");
    return false;
  }

  // Find the start of the audio data
  size_t audioDataPayloadSize = 0;
  if (!WavFileInternal::findAudioDataChunkPayload(file, riff.endian, audioDataPayloadSize)) {
    LogError(kLogCategory, "Unable to locate WAV audio data");
    return false;
  }
  if (file.readNext(audioDataPayloadSize, resultAudioData) != audioDataPayloadSize) {
    LogError(kLogCategory, "Unable to read WAV audio data buffer");
    return false;
  }
  return true;
}



WavWriter::~WavWriter() {
  close();
}

bool WavWriter::open(const std::string& filename, bool isStereo, uint32_t samplesPerSecond) {
  if (_file.isOpen()) {
    _file.close();
  }
  if (!_file.open(filename)) {
    return false;
  }

  _totalPcmByteSize = 0;
  _sampleRate = samplesPerSecond;
  uint16_t numChannels = (isStereo ? 2 : 1);
  _numChannels = static_cast<size_t>(numChannels);
  constexpr uint32_t expectedPcmDataSize = 0;
  if (!WavFileInternal::writeWavFileHeaders(_file, _endian, numChannels, samplesPerSecond, expectedPcmDataSize)) {
    return false;
  }
  return true;
}

void WavWriter::appendSigned16(const int16_t* samples, size_t numSamplesPerChannel) {
  //printf("append signed %d\n", (int)numSamplesPerChannel);
  // TODO: If only adding a small number of samples, buffer until they
  // reach a minimum threshold
  size_t n = numSamplesPerChannel * _numChannels;
  size_t numBytes = n * 2;
  _file.append(reinterpret_cast<const uint8_t*>(samples), numBytes); // TODO: Probably Endian issues?!
  _totalPcmByteSize += numBytes;
}

void WavWriter::appendNormalized(const float* samples, size_t numSamplesPerChannel) {
  size_t n = numSamplesPerChannel * _numChannels;
  if (_temp.size() < n) {
    _temp.resize(n);
  }
  for (size_t i=0; i<n; ++i) {
    _temp[i] = normalizedToSigned(samples[i]);
  }
  appendSigned16(_temp.data(), numSamplesPerChannel);
}

bool WavWriter::appendFloat16StereoNonInterleaved(const float* left, const float* right, size_t numSamplesPerChannel) {
  if (_numChannels != 2) {
    return false;
  }
  size_t n = numSamplesPerChannel * _numChannels;
  if (_temp.size() < n) {
    _temp.resize(n);
  }
  for (size_t i=0; i<numSamplesPerChannel; ++i) {
    _temp[i*2] = static_cast<int16_t>(left[i]);
    _temp[i*2+1] = static_cast<int16_t>(right[i]);
  }
  appendSigned16(_temp.data(), numSamplesPerChannel);
  return true;
}

bool WavWriter::appendFloat16StereoNonInterleaved(const std::vector<float>& left, const std::vector<float>& right) {
  size_t numSamplesPerChannel = std::min(left.size(), right.size());
  return appendFloat16StereoNonInterleaved(left.data(), right.data(), numSamplesPerChannel);
}

void WavWriter::close() {
  if (!_file.isOpen()) {
    return;
  }
  // TODO: flush buffer if any
  
  // Update the total file data size in the RIFF WAV header
  uint8_t buf[4];
  size_t totalFileSize = _file.getSize();
  IO::writeUInt32(buf, _endian, static_cast<uint32_t>(totalFileSize - 8));
  _file.rewrite(4, buf, 4);

  // Update the PCM data chunk size in the WAV file headers
  // skip RIFF WAV header (12 bytes)
  // skip FMT chunk header (8 bytes)
  // skip FMT chunk data (16 bytes)
  // partially Data chunk header (4 out of 8 bytes)
  IO::writeUInt32(buf, _endian, _totalPcmByteSize);
  _file.rewrite(12+8+16+4, buf, 4);
  _file.close();
 }

int16_t WavWriter::normalizedToSigned(float v) const {
    int i = static_cast<int>(v * 32768.0f);
    return static_cast<int16_t>(IO::clamp(i, -32768, 32767));
}


