#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <fstream>

namespace IO {

  enum class Endian { Little, Big };

  // Read a signed 16-bit int from a little-endian byte array
  // @param bytes The source data
  // @param byteOffset Optional starting offset within the source data
  // @return The value
  int16_t readInt16LE(const uint8_t* bytes, size_t byteOffset=0);

  // Read a signed 16-bit integer from a big-endian byte array
  // @param bytes The source data
  // @param byteOffset Optional starting offset within the source data
  // @return The value
  int16_t readInt16BE(const uint8_t* bytes, size_t byteOffset=0);

  // Read a signed 16-bit integer from a byte array
  // @param endian Which byte ordering to read
  // @param bytes The source data
  // @param byteOffset Optional starting offset within the source data
  // @return The value
  int16_t readInt16(Endian endian, const uint8_t* bytes, size_t byteOffset=0);


  // Read a uint16_t from a little-endian byte array
  uint16_t readUInt16LE(const uint8_t* bytes, size_t byteOffset=0);

  // Read a uint16_t from a big-endian byte array
  uint16_t readUInt16BE(const uint8_t* bytes, size_t byteOffset=0);

  // Read a uint16_t from a big-endian byte array
  uint16_t readUInt16(Endian endian, const uint8_t* bytes, size_t byteOffset=0);


  // Read a uint32_t from a little-endian byte array
  uint32_t readUInt32LE(const uint8_t* bytes, size_t byteOffset=0);

  // Read a uint32_t from a big-endian byte array
  uint32_t readUInt32BE(const uint8_t* bytes, size_t byteOffset=0);

  // Read a uint32_t from a big-endian byte array
  uint32_t readUInt32(Endian endian, const uint8_t* bytes, size_t byteOffset=0);

  // @return The next destination after writing the value
  uint8_t* writeInt16LE(uint8_t* dest, int16_t value);
  uint8_t* writeInt16BE(uint8_t* dest, int16_t value);
  uint8_t* writeInt16(uint8_t* dest, Endian endian, int16_t value);

  uint8_t* writeUInt16LE(uint8_t* dest, uint16_t value);
  uint8_t* writeUInt16BE(uint8_t* dest, uint16_t value);
  uint8_t* writeUInt16(uint8_t* dest, Endian endian, uint16_t value);

  uint8_t* writeUInt32LE(uint8_t* dest, uint32_t value);
  uint8_t* writeUInt32BE(uint8_t* dest, uint32_t value);
  uint8_t* writeUInt32(uint8_t* dest, Endian endian, uint32_t value);

  // Attempt to read a file from disk
  // @return Whether successful
  bool readFileContents(const std::string& filename, std::vector<uint8_t>& result);

  // Attempt to write a file to disk
  // @return Whether successful
  bool writeFileContents(const std::string& filename, const std::vector<uint8_t>& fileContents);

  class FileReader {
    public:
      FileReader() = default;
      FileReader(const std::string& filename);
      ~FileReader();

      bool open(const std::string& filename);
      bool isOpen() const;
      bool close();
      // @return total size of the file in bytes
      size_t getSize() const;

      // Set the position for the next read
      bool seekTo(size_t offset);

      size_t getReadOffset() const;

      // Continue reading from the last location
      // @param numBytes Maximum number of bytes to read
      // @param result Target buffer to read into
      // @param append Whether to append to the target buffer instead of rewriting
      // @return Number of bytes read
      size_t readNext(size_t numBytes, std::vector<uint8_t>& result, bool append=false);

      // Read a specified portion of the file
      // @param startOffset Starting byte of the range
      // @param numBytes Maximum number of bytes to read
      // @param result Target buffer to read into
      // @param append Whether to append to the target buffer instead of rewriting
      // @return Number of bytes read
      size_t readRange(size_t startOffset, size_t numBytes, std::vector<uint8_t>& result, bool append=false);

    private:
      std::ifstream _file;
      bool _isOpen = false;
      size_t _fileSize = 0;
      size_t _readOffset = 0;
  };

  class FileWriter {
    public:
      FileWriter() = default;
      FileWriter(const std::string& filename);
      ~FileWriter();

      bool open(const std::string& filename);
      bool isOpen() const;
      bool close();
      size_t getSize() const;

      bool rewrite(size_t offset, const uint8_t* data, size_t numBytes);

      bool append(const std::vector<uint8_t>& data);
      bool append(const uint8_t* data, size_t numBytes);

      template<size_t size>
      bool append(const uint8_t (&data)[size]) {return append(data, size);}

    private:
      std::ofstream* _file = nullptr;
      size_t _size = 0;
  };


  // Convert an N-bit unsigned 2's complement encoded number to its signed value.
  // @param twosComplement The positive representation to decode
  // @param numBits Size of the number in bits
  int twosComplementToSigned(int twosComplement, int numBits);

  // Convert an N-bit signed integer to its unsigned 2's complement encoding.
  // @param signedValue The signed value to convert
  // @param numBits Size of the number in bits
  int signedToTwosComplement(int signedValue, int numBits);

  // A clamp implementation for C++11 that doesn't support std::clamp
  template <class T>
  constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
    return (v < lo) ? lo : (hi < v) ? hi : v;
  }
}
