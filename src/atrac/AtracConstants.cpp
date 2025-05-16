#include "AtracConstants.h"

namespace Atrac3 {

  Atrac3Constants::Atrac3Constants() {

    // For a single encoding/decoding mDCT window, the perfect reconstruction constraint
    // is that the sum of squares must equal 1:
    //     sqr(w[i])+sqr(w[i+N/2]) = 1
    // When the encoding window does not meet the perfect reconstruction constraint, it
    // has a mutual constraint with the complementary decoding window:
    //     e[i]*d[i] + e[i+N/2]*d[i+N/2] = 1
    // This mathematically simplifies to the approach used here:
    //     d[i] = 1 / (sqr(e[i]) + sqr(e[i+N/2]))
    encodingScalingWindow = initArray(512, [](int i){
      // An ease-in ease-out cosine curve offset by 1/2 sample
      float t = (i + 0.5f) / 512.0f;
      return (1.0f - std::cosf(t * kTwoPi)) * 0.5f;
      // TODO: is this supposed to be sin(t*Math.PI) ?
      // for the same encoding/decoding window, the constraint is: sqr(w[i])+sqr(w[i+halfN])==1
    });

    // The decoding window is derived from the encoding window, so must
    // be initialized afterwards. To allow for proper TDAC (Time Domain
    // Aliasing Cancellation) with 50% neighboring window overlap,
    // each sample must scale based on its 256-offset 
    decodingScalingWindow = initArray(512, [&](int i){
        float a = encodingScalingWindow[i];
        float b = encodingScalingWindow[(i+256)%512];
        return a / (a*a + b*b);
    });
    
    scaleFactors = initArray(64, [](int i){
      return std::powf(2.0f, -5 + (static_cast<float>(i) / 3.0f));
    });

    gainCompensationLevelTable = initArray(16, [](int i){
      return std::powf(2.0f, 4-i);
    });

    // Initialize Huffman tables of HuffmanEntry {bits, code, symbol}
    huffmanTables.resize(8); // Note: Table 0 remains empty
    huffmanTables[1].init({ // Table index 1,size 9
      {1,0,0}, {3,4,1}, {3,5,2}, {4,12,3}, {4,13,4}, {5,28,5},
      {5,29,6}, {5,30,7}, {5,31,8}, });
    huffmanTables[2].init({ // Table index 2,size 5
      {1,0,0}, {3,4,1}, {3,5,-1}, {3,6,2}, {3,7,-2}, });
    huffmanTables[3].init({ // Table index 3,size 7
      {1,0,0}, {3,4,1}, {3,5,-1}, {4,12,2}, {4,13,-2}, {4,14,3}, {4,15,-3}, });
    huffmanTables[4].init({ // Table index 4,size 9
      {1,0,0}, {3,4,1}, {3,5,-1}, {4,12,2}, {4,13,-2},
      {5,28,3}, {5,29,-3}, {5,30,4}, {5,31,-4}, });
    huffmanTables[5].init({ // Table index 5,size 15
      {2,0,0}, {3,2,1}, {3,3,-1}, {4,8,2}, {4,9,-2}, {4,10,3}, {4,11,-3},
      {4,12,7}, {4,13,-7}, {5,28,4}, {5,29,-4}, {6,60,5}, {6,61,-5},
      {6,62,6}, {6,63,-6}, });
    huffmanTables[6].init({ // Table index 6,size 31
      {3,0,0}, {4,2,1}, {4,3,-1}, {4,4,2}, {4,5,-2}, {4,6,3}, {4,7,-3}, {4,8,15},
      {4,9,-15}, {5,20,4}, {5,21,-4}, {5,22,5}, {5,23,-5}, {5,24,6}, {5,25,-6},
      {6,52,7}, {6,53,-7}, {6,54,8}, {6,55,-8}, {6,56,9}, {6,57,-9}, {6,58,10},
      {6,59,-10}, {7,120,11}, {7,121,-11}, {7,122,12}, {7,123,-12}, {7,124,13},
      {7,125,-13}, {7,126,14}, {7,127,-14}, });
    huffmanTables[7].init({ // Table index 7,size 63
      {3,0,0}, {4,2,31}, {4,3,-31}, {5,8,1}, {5,9,-1}, {5,10,2}, {5,11,-2},
      {5,12,3}, {5,13,-3}, {5,14,4}, {5,15,-4}, {5,16,5}, {5,17,-5}, {6,36,6},
      {6,37,-6}, {6,38,7}, {6,39,-7}, {6,40,8}, {6,41,-8}, {6,42,9}, {6,43,-9},
      {6,44,10}, {6,45,-10}, {6,46,11}, {6,47,-11}, {6,48,12}, {6,49,-12},
      {6,50,13}, {6,51,-13}, {7,104,14}, {7,105,-14}, {7,106,15}, {7,107,-15},
      {7,108,16}, {7,109,-16}, {7,110,17}, {7,111,-17}, {7,112,18}, {7,113,-18},
      {7,114,19}, {7,115,-19}, {7,116,20}, {7,117,-20}, {8,236,21}, {8,237,-21},
      {8,238,22}, {8,239,-22}, {8,240,23}, {8,241,-23}, {8,242,24}, {8,243,-24},
      {8,244,25}, {8,245,-25}, {8,246,26}, {8,247,-26}, {8,248,27}, {8,249,-27},
      {8,250,28}, {8,251,-28}, {8,252,29}, {8,253,-29}, {8,254,30}, {8,255,-30}, });
  }

  float Atrac3Constants::getScaleFactor(int scaleFactorIndex) const {
    int n = static_cast<int>(scaleFactors.size());
    return (scaleFactorIndex < n ? scaleFactors[scaleFactorIndex] : 1.0f);
  }

  bool Atrac3Constants::getSpectralSubbandOffsets(int index,
        int& resultStart, int& resultSize) const {
    if (index >= 0 && index < (int)bfuSubbandOffsets.size() - 1) {
      resultStart = bfuSubbandOffsets[index];
      resultSize = bfuSubbandOffsets[index+1] - resultStart;
      return true;
    } else {
      resultStart = 0;
      resultSize = 0;
      return false;
    }
  }

}