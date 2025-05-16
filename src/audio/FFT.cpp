#include "FFT.h"
#include <math.h>
#include <vector>
 
namespace FFTImpl {

  // A basic Complex number container
  struct Complex {
    float re = 0.0f;
    float im = 0.0f;
    Complex(float r=0.0f, float i=0.0f) : re(r), im(i) {}
    void set(float r, float i) {re=r; im=i;}
    static void resizeAtLeast(std::vector<Complex>& values, size_t minSize) {
      if (values.size() < minSize) {
        values.resize(minSize);
      }
    }
  };

  // A sparse-indexing Complex array
  struct ComplexSpan {
  private:
    Complex* _values;
    int _offset;
    int _stride;
  public:
    ComplexSpan(Complex* values, int offset=0, int stride=1)
      :_values(values), _offset(offset), _stride(stride) {}

    Complex& operator[](int index) {return _values[_offset + index*_stride];}
    ComplexSpan even() {return ComplexSpan(_values, _offset, _stride*2);}
    ComplexSpan odd() {return ComplexSpan(_values, _offset+_stride, _stride*2);}
    ComplexSpan shifted(int amount) {return ComplexSpan(_values, _offset+amount*_stride, _stride);}

    void copyTo(ComplexSpan& target, int n) {
      for (int i=0; i<n; ++i) {
        target[i] = operator[](i);
      }
    }
  };

  // Perform a basic FFT in-place
  // @param signal The input signal (size N), which will be set in place with the output result (also size N)
  // @param temp Temporary working space (size N*4)
  // @param N Number of samples in the signal
  void forwardFFTInPlace(ComplexSpan signal, ComplexSpan temp, int N) {
    if (N == 1) {
      return;
    }
    const int halfN = N >> 1;

    // Use the temp array as scratch space to calculate intermediate even and odd FFT
    signal.copyTo(temp, N);
    ComplexSpan evenFft = temp.even();
    ComplexSpan oddFft = temp.odd();
    forwardFFTInPlace(evenFft, temp.shifted(N), halfN);
    forwardFFTInPlace(oddFft, temp.shifted(N+halfN), halfN);

    const float twoPiOverN = 2.0f * M_PI / static_cast<float>(N);
    Complex tempK;
    for (int k = 0; k < halfN; ++k) {
      const float cosK = cosf(twoPiOverN * k);
      const float sinK = sinf(twoPiOverN * k);
      const Complex& oddK = oddFft[k];
      tempK.set(oddK.re*cosK + oddK.im*sinK, oddK.im*cosK - oddK.re*sinK);

      const Complex& evenK = evenFft[k];
      signal[k].set(evenK.re + tempK.re, evenK.im + tempK.im);
      signal[k+halfN].set(evenK.re - tempK.re, evenK.im - tempK.im);
    }
  }

}

namespace FFT {

  void forwardFFT(float* signalReal, float* signalImag, int n, int stride) {
    // Reserve temporary space (static to reduce reallocations, but this is not threadsafe)
    static std::vector<FFTImpl::Complex> signal;
    static std::vector<FFTImpl::Complex> temp;
    FFTImpl::Complex::resizeAtLeast(temp, n*4);
    FFTImpl::Complex::resizeAtLeast(signal, n);

    // Copy input values into the working buffer
    for (int i=0; i<n; ++i) {
      signal[i].set(signalReal[i*stride], signalImag[i*stride]);
    }

    // Perform FFT
    FFTImpl::forwardFFTInPlace(FFTImpl::ComplexSpan(signal.data()), FFTImpl::ComplexSpan(temp.data()), n);

    // Copy back to the signal for output
    for (int i=0; i<n; ++i) {
      const FFTImpl::Complex& c = signal[i];
      signalReal[i*stride] = c.re;
      signalImag[i*stride] = c.im;
    }
  }

  void inverseFFT(float* signalReal, float* signalImag, int n, int stride) {
    if (n == 0) {
      return;
    }
    std::swap(signalReal, signalImag); // pre-swap
    forwardFFT(signalReal, signalImag, n, stride);
    std::swap(signalReal, signalImag); // post-swap
    float oneOverN = 1.0f / static_cast<float>(n);
    for (int i=0; i<n; ++i) {
      signalReal[i*stride] *= oneOverN;
      signalImag[i*stride] *= oneOverN;
    }
  }

}
