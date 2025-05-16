#include "Bitstream.h"
#include "util/Logging.h"
#include "io/IO.h"
#include "util/MathUtil.h"

namespace {
  void logBits(int numBits, int value) {
    //LogVerbose("BitstreamReader", "(%d bits = %d)", numBits, value);
    //printf("(%d bits = %d)", numBits, value);
  }
}

BitstreamReader::BitstreamReader(const std::vector<uint8_t>& content)
  : BitstreamReader(content.data(), content.size()) {}

BitstreamReader::BitstreamReader(const uint8_t* contentPtr, size_t contentSize)
  : content(contentPtr) {
  totalBitSize = contentSize * 8;
  }

size_t BitstreamReader::getByteOffset() const {
  return (bitReadOffset / 8);
}

size_t BitstreamReader::getRemainingBits() const {
  return (totalBitSize - bitReadOffset);
}

bool BitstreamReader::hasRemainingBits(size_t minCount) const {
  return (bitReadOffset + minCount <= totalBitSize);
}

int BitstreamReader::getBit() {
  int result = getBitInternal();
  logBits(1, result);
  return result;
}

int BitstreamReader::getBits(size_t numBits) {
  int result = 0;
  for (size_t i=0; i<numBits; ++i) {
    result = (result<<1) + getBitInternal();
  }
  logBits(numBits, result);
  return result;
}

int BitstreamReader::getSignedBits(size_t numBits) {
  int encoded = getBits(numBits);
  return twosComplementToSigned(encoded, numBits);
}

bool BitstreamReader::tryGetBits(size_t numBits, int& outTarget) {
  if (!hasRemainingBits(numBits)) {
    return false;
  }
  outTarget = getBits(numBits);
  return true;
}

bool BitstreamReader::tryGetBits(size_t numBits, uint8_t& outTarget) {
  if (!hasRemainingBits(numBits)) {
    return false;
  }
  outTarget = getBits(numBits);
  return true;
}


std::vector<bool> BitstreamReader::getBitArray(size_t numBits) {
  int mask = 0;
  std::vector<bool> result(numBits);
  for (size_t i=0; i<numBits; ++i) {
    int bit = getBitInternal();
    result[i] = bit;
    mask = (mask<<1)+bit;
  }
  logBits(numBits, mask);
  return result;
}

int BitstreamReader::getBitInternal() {
  if (bitReadOffset >= totalBitSize) {
    return 0;
  }
  size_t byteOffset = bitReadOffset / 8;
  size_t whichBit = bitReadOffset % 8;
  ++bitReadOffset;
  int result = (content[byteOffset] >> (7-whichBit)) & 0x1;
  return result;
}