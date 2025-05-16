#include "HuffmanTable.h"

HuffmanTable::HuffmanTable(const std::vector<HuffmanEntry>& entries) {
  init(entries);
}

void HuffmanTable::init(const std::vector<HuffmanEntry>& entries) {
  _entries = entries;
  _values.clear();
  int n = (int)entries.size();
  for (int i=0; i<n; ++i) {
    const HuffmanEntry& e = entries[i];
    _values[e.numBits][e.code] = e.symbol;
  }
}

bool HuffmanTable::contains(int code, int numBits, int& resultValue) const {
  const auto iter1 = _values.find(numBits);
  if (iter1 != _values.end()) {
    const std::map<int, int>& codes = iter1->second;
    const auto iter2 = codes.find(code);
    if (iter2 != codes.end()) {
      resultValue = iter2->second;
      return true;
    }
  }
  return false;
}

bool HuffmanTable::readCode(IBitstreamReader& bitstream, int& resultValue) const {
  constexpr int kMaxBits = 8;
  int code = 0;

  // Read the huffman code 1 bit at a time until it's a valid code
  for (int numBits=1; numBits <= kMaxBits; ++numBits) {
    code = (code << 1) + bitstream.getBit();
    if (contains(code, numBits, resultValue)) {
      return true;
    }
  }
  resultValue = 0;
  return false;
}

int HuffmanTable::readCode(IBitstreamReader& bitstream) const {
  constexpr int kMaxBits = 8;
  int resultValue = 0;
  int code = 0;

  // Read the huffman code 1 bit at a time until it's a valid code
  for (int numBits=1; numBits<=kMaxBits; ++numBits) {
    code = (code << 1) + bitstream.getBit();
    if (contains(code, numBits, resultValue)) {
      return resultValue;
    }
  }
  return 0;
}

bool HuffmanTable::readCodes(IBitstreamReader& bitstream, int numCodes, std::vector<int>& result) const {
  result.resize(numCodes);
  for (int i=0; i<numCodes; ++i) {
    if (!readCode(bitstream, result[i])) {
      return false;
    }
  }
  return true;
}
