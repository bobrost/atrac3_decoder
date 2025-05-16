#pragma once

#include <stddef.h>
#include <cstdint>
#include <vector>

// TODO: read a bitstream
class IBitstreamReader {
public:
  // @return The current offset of the stream, in full bytes read
  virtual size_t getByteOffset() const = 0;

  // @return Number of bits not yet read from the stream
  virtual size_t getRemainingBits() const = 0;

  // @return Whether the stream has at least the given number of bits remaining
  virtual bool hasRemainingBits(size_t minCount) const = 0;

  // Get the next single bit from the stream.
  // @return 0 or 1
  virtual int getBit() = 0;

  // Get a numBits-sized unsigned integer from the stream, in MSB order
  // @return Unsigned value
  virtual int getBits(size_t numBits) = 0;

  // Get a numBits-sized signed integer from the stream, encoded as 2's complement
  // @return Signed value
  virtual int getSignedBits(size_t numBits) = 0;

  // Get the next numBits values as a numBits-sized array of booleans
  virtual std::vector<bool> getBitArray(size_t numBits) = 0;

  // Try to get a numBits-sized unsigned value, first checking whether the stream
  // has enough bits available.
  // @param numBits Size of the integer to get
  // @param outTarget Will be written with the result if successful
  // @return Whether successful
  virtual bool tryGetBits(size_t numBits, int& outTarget) = 0;
  virtual bool tryGetBits(size_t numBits, uint8_t& outTarget) = 0;

};

// Reads bits in MSB order first.
class BitstreamReader : public IBitstreamReader {
  public:
    BitstreamReader(const std::vector<uint8_t>& content);
    BitstreamReader(const uint8_t* content, size_t contentSize);
    BitstreamReader(BitstreamReader&&) = default;
    BitstreamReader(const BitstreamReader&) = default;

    size_t getByteOffset() const override;
    size_t getRemainingBits() const override;
    bool hasRemainingBits(size_t minCount) const override;
    int getBit() override;
    int getBits(size_t numBits) override;
    int getSignedBits(size_t numBits) override;
    std::vector<bool> getBitArray(size_t numBits) override;

    bool tryGetBits(size_t numBits, int& outTarget) override;
    bool tryGetBits(size_t numBits, uint8_t& outTarget) override;

  private:
    int getBitInternal();

    const uint8_t* content = nullptr;
    size_t totalBitSize = 0;
    size_t bitReadOffset = 0;
};
