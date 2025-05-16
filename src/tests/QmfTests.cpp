#include "TestRunner.h"
#include "util/ArrayUtil.h"
#include "audio/QMF.h"
#include "atrac/AtracConstants.h"
#include <cmath>

namespace {
  constexpr float kTolerance = 0.0001f; // the precision we have from outputting the known qmf values

  TestResult testKnownQmfStep() {
    // Set up the known input and output values (48 samples per input array)
    FloatArray lowpass, highpass;
    for (int i=0; i<48; ++i) {
      lowpass.push_back(std::sinf(i*0.1f));
      highpass.push_back(std::cosf(i*0.021f));
    }
    FloatArray knownOutput = { // 96 samples
      0.000029f, -0.000184f, 0.000139f, 0.000400f, -0.000360f, -0.001264f, 0.000716f, 0.002676f,
      -0.000921f, -0.005477f, 0.000679f, 0.009639f, 0.000737f, -0.016240f, -0.004265f, 0.025548f,
      0.011756f, -0.039968f, -0.027008f, 0.064528f, 0.062806f, -0.127528f, -0.207460f, 0.787796f,
      -1.115605f, 1.131432f, -0.803671f, 1.148974f, -0.819833f, 1.280418f, -0.650084f, 1.359252f,
      -0.601558f, 1.452955f, -0.482009f, 1.537095f, -0.412282f, 1.613962f, -0.322284f, 1.687076f,
      -0.251817f, 1.750313f, -0.182075f, 1.806869f, -0.122359f, 1.854175f, -0.069904f, 1.892334f,
      -0.026239f, 1.920874f, 0.008452f, 1.939431f, 0.033876f, 1.947764f, 0.049836f, 1.945731f,
      0.056233f, 1.933291f, 0.053070f, 1.910498f, 0.040445f, 1.877511f, 0.018559f, 1.834584f,
      -0.012293f, 1.782067f, -0.051722f, 1.720402f, -0.099249f, 1.650119f, -0.154310f, 1.571828f,
      -0.216264f, 1.486220f, -0.284396f, 1.394051f, -0.357925f, 1.296140f, -0.436013f, 1.193362f,
      -0.517773f, 1.086633f, -0.602278f, 0.976910f, -0.688569f, 0.865171f, -0.775669f, 0.752415f,
      -0.862583f, 0.639647f, -0.948322f, 0.527866f, -1.031901f, 0.418062f, -1.112354f, 0.311198f
    };

    Atrac3::Atrac3Constants constants;
    FloatArray coefficients = Qmf::mirrorCoefficients(constants.qmfHalfCoefficients, 2.0f);
    HistoryBuffer history;

    FloatArray output;
    Qmf::qmfCombineUpsample(coefficients, lowpass, highpass, history, output);
    /*
    printf("expected: %f, %f, %f, %f, %f, %f, %f, %f\n",
      knownOutput[0], knownOutput[1], knownOutput[2], knownOutput[3],
      knownOutput[4], knownOutput[5], knownOutput[6], knownOutput[7]);
    printf("actual:   %f, %f, %f, %f, %f, %f, %f, %f\n",
      output[0], output[1], output[2], output[3],
      output[4], output[5], output[6], output[7]);
    printf("\n\n");
    */


    return isClose(output, knownOutput, kTolerance);

  }

}

void addQmfTests(TestRunner& runner) {
  runner.add("QMF decode should match known data", testKnownQmfStep);
}
