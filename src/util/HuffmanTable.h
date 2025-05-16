#pragma once

#include <map>
#include <vector>
#include "io/Bitstream.h"

struct HuffmanEntry {
  int numBits; // number of bits
  int code; // bitcode for the entry
  int symbol; // output value
};

class HuffmanTable {
public:
  HuffmanTable() = default;
  HuffmanTable(const std::vector<HuffmanEntry>& entries);

  const std::vector<HuffmanEntry>& getEntries() const { return _entries;}

  void init(const std::vector<HuffmanEntry>& entries);
  bool contains(int code, int numBits, int& resultValue) const;

  bool readCode(IBitstreamReader& bitstream, int& outResult) const;
  int readCode(IBitstreamReader& bitstream) const;

  // Read consecutive values, and resize and fill the result array.
  // @return Whether all values were read successfully
  bool readCodes(IBitstreamReader& bitstream, int numCodes, std::vector<int>& result) const;
private:
  // numBits -> code -> value
  std::map<int, std::map<int, int>> _values;
  std::vector<HuffmanEntry> _entries;
};
