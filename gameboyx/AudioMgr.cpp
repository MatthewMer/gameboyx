#include "AudioMgr.h"

#include "AudioOpenAL.h"
#include "AudioSDL.h"

AudioMgr* AudioMgr::instance = nullptr;

AudioMgr* AudioMgr::getInstance() {
	if (instance == nullptr) {
		instance = new AudioSDL();
	}

	return instance;
}

void AudioMgr::resetInstance() {
	if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	}
}

/* 
FFT algorithm -> transform signal from time domain into frequency domain (application of fourier analysis)
	FT: X(f) = integral(t=-infinity,infinity) x(t) * e^(-i*2*pi*f*t) dt, where x(t) is the signal and e^(-i*2*pi*f*t) the complex exponential (polar form: e^(j*phi),
	which can be directly translated into the trigonometric form with eulers formula: e^(j*phi) = cos(phi) + j * sin(phi)) which contains the cosine component in the real part
	and the sine component in the imaginary part. The complex exponential can be viewed as a spiral coming out of the imaginary plane, rotating counter clockwise around the 
	t-axis with the radius omega*t (omega=2*pi*f) or amplitude, starting at cos(0)-i*sin(0) = 1. When plotting this spiral to the imaginary axis you end up with the sine component,
	when plotting it to the real axis you get the cosine component. These two components resemble each sinusoid in the frequency domain the input signal is made off. The convolution
	of these two components (amplitude) with the signal itself (multiply and integrate, as described by the formula above) delivers the area enclosed by the resulting signal, which also resembles its "impact". 
	Using Pythagoras on these two values (both components resemble a right triangle in the complex plane) you get the magnitude of the sinusoid, using the 4 quadrant inverse tangent 
	(atan2(opposite side / adjacent side)) you get its phase.
	
	FFT (which is an optimized version of DFT) splits the samples in half until each one contains 2 values.
	X[k] = Σ (n=0 bis N-1) x[n] * exp(-i*2*pi * k * n / N) = Σ (n=0 bis N-1) x[n] * (cos(2*pi*k*n/N)-i*sin(2*pi*k*n/N))

*/

// FFT based on Cooley and Turkey (need more research)
// returns DFT (size of N) with the phases, magnitudes and frequencies in cartesian form (e.g. 3.5+2.6i)
void fft(complex* _samples, const int& _N) {
	// only one sample
	if (_N == 1) {
		return;
	}

	// split in 2 arrays, one for all elements where n is odd and one where n is even
	complex* e0 = new complex[_N / 2];
	complex* e1 = new complex[_N / 2];
	for (int n = 0; n < _N / 2; n++) {
		e0[n] = _samples[n * 2];
		e1[n + 1] = _samples[n * 2 + 1];
	}

	// recursively call fft for all stages (with orders power of 2)
	fft(e0, _N / 2);
	fft(e1, _N / 2);

	// twiddel factors ( e^(i*2*pi*k/N) , where k = index and N = order (or in other words N = sampling rate and k = t ?)
	// used for shifting the signal
	float ang = (float)(2 * M_PI / _N);			// phase
	complex w, wn;
	w.real = 1;
	w.imaginary = 0;
	wn.real = cos(ang);
	wn.imaginary = sin(ang);

	for (int k = 0; 2 * k < _N; k++) {
		// shift phase and perform butterfly operation to calculate results
		complex t;
		t.real = w.real * e1[k].real - w.imaginary * e1[k].imaginary;
		t.imaginary = w.real * e1[k].imaginary + w.imaginary * e1[k].real;

		_samples[k].real = e0[k].real + t.real;
		_samples[k].imaginary = e0[k].imaginary + t.imaginary;
		_samples[k + _N / 2].real = e0[k].real - t.real;
		_samples[k + _N / 2].imaginary = e0[k].imaginary - t.imaginary;

		if (k < _N / 2) {
			float temp = w.real;
			w.real = w.real * wn.real - w.imaginary * wn.imaginary;
			w.imaginary = temp * wn.imaginary + w.imaginary * wn.real;
		}
	}

	delete[] e0;
	delete[] e1;
}