#pragma once

namespace FFT {

  // Perform a Fast Fourier Transform, modifying the signal in place.
  // @param signalReal The real-valued portion of the complex input, will be set
  //   with the real portion of the output
  // @param signalImag The imaginary-valued portion of the complex input, will be set
  //   with the imaginary portion of the output
  // @param n Number of complex values in the input and output
  // @param stride Interleaving amount for the signal arrays
  void forwardFFT(float* signalReal, float* signalImag, int n, int stride=1);

  // Perform a Fast Fourier Transform, modifying the signal in place. This
  // will post-scale the end result by 1/N to recreate the original input.
  // @param signalReal The real-valued portion of the complex input, will be set
  //   with the real portion of the output
  // @param signalImag The imaginary-valued portion of the complex input, will be set
  //   with the imaginary portion of the output
  // @param n Number of complex values in the input and output
  // @param stride Interleaving amount for the signal arrays
  void inverseFFT(float* signalReal, float* signalImag, int n, int stride=1);

}
