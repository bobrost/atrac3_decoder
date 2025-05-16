#include "TestRunner.h"
#include "util/ArrayUtil.h"
#include "audio/DCT.h"
#include <cmath>

namespace {
  constexpr float kTolerance = 0.00001f;

  TestResult testBruteMDCT() {
    FloatArray input = {1,2,3,4,5,6,7,8};
    FloatArray expected = {
      -25.42111462625002f,
      -4.775004699494127f,
      4.111055037636232f,
      3.17261528654083f};
    FloatArray output(4);
    DCT::MDCT_Brute(input.data(), 8, output.data());
    return isClose(output, expected, kTolerance);
  }

  // Verify that our inverse mdct matches ffmpeg's output for a reference atrac decoder.
  // Testing against known ffmpeg iMDCT values
  TestResult testBasicInverseMdct() {
    FloatArray input(8);
    for (int i=0; i<8; ++i) {
      input[i] = (i==0 ? 1 : 0);
    }

    // I used ffmpeg to set up an mdct context and output the imdct
    // of the input array [1,0,0,0,0,0,0,0]. This is the output value.
    FloatArray expectedOutput = {
      -0.634393f, -0.471397f, -0.290285f, -0.0980171f,
      0.0980171f, 0.290285f, 0.471397f, 0.634393f,
      0.77301f, 0.881921f, 0.95694f, 0.995185f,
      0.995185f, 0.95694f, 0.881921f, 0.77301f
    };
    FloatArray output(16);

    // Through experimentation, I've discovered that ffmpeg scales the
    // output by -1 compared to the formal definition. Their internal
    // atrac implementation scales again by 1/32768, converting from
    // 16-bit audio range (signed 15 bits) to normalized [-1,+1] range.
    constexpr float kOutputScale = -1.0f;
    DCT::MDCT_Inverse_Brute(input.data(), 8, output.data(), kOutputScale);
    float maxError = getMaxDifference(output, expectedOutput);

    if (maxError > kTolerance) {
      printf("Oops\n");
      for (int i=0; i<16; ++i) {
        printf("  [%d]  %g   %g\n", i, output[i], expectedOutput[i]);
      }
      return "Oops";
    }
    return true;
  }

  TestResult testFastInverseMdct() {
    constexpr float kTolerance = 0.0001f;
    FloatArray input = {1,2,3,4,5,6,7,8};
    FloatArray bruteOutput(16);
    FloatArray fastOutput(16);
    DCT::MDCT_Inverse_Brute(input.data(), 8, bruteOutput.data(), 1.0f);
    DCT::MDCT_Inverse_Fast(input.data(), 8, fastOutput.data(), 1.0f);
    return isClose(bruteOutput, fastOutput, kTolerance);
  }

} // namespace

void addDctTests(TestRunner& runner) {
  runner.add("brute MDCT", testBruteMDCT);
  runner.add("inverse MDCT (known values)", testBasicInverseMdct);
  runner.add("inverse MDCT fast", testFastInverseMdct);
}
