﻿#include "audio_helpers.h"

/*
FFT algorithm -> transform signal from time domain into frequency domain (application of fourier analysis)
	FT: X(f) = integral(t=-infinity,infinity) x(t) * e^(-i*2*pi*f*t) dt, where x(t) is the signal and e^(-i*2*pi*f*t) the complex exponential (polar form: e^(j*phi),
	which can be directly translated into the trigonometric form with eulers formula: e^(j*phi) = cos(phi) + j * sin(phi)) which contains the cosine component in the real part
	and the sine component in the imaginary part. The complex exponential can be viewed as a spiral coming out of the imaginary plane, rotating counter clockwise around the
	t-axis with the radius omega*t (omega=2*pi*f) or amplitude, starting at cos(0)+i*sin(0) = 1. When plotting this spiral to the imaginary axis you end up with the sine component,
	when plotting it to the real axis you get the cosine component. These two components resemble each sinusoid in the frequency domain the input signal is made off. The convolution
	of these two components (amplitude) with the signal itself (multiply and integrate, as described by the formula above) delivers the area enclosed by the resulting signal, which also resembles
	its "impact". Using Pythagoras on these two values (both components resemble a right triangle in the complex plane) you get the magnitude of the sinusoid, using the 4 quadrant inverse tangent
	(atan2(opposite side / adjacent side)) you get its phase. In the FFT this "spiral" rotates clockwise ((e^(i*2*pi*f*t))^-1 = e^(-i*2*pi*f*t)), which has the effekt of morroring the sine component
	on the t-axis (rotating the circle in the complex plane clockwise). The point of intersection of this spiral with the complex plane is therefore cos(0)-i*sin(0) and when adding this
	complex exponent to the signal (which has its imaginary part set to i*sin(omega*t)=i0) is where the phase shift along the t-axis happens.

	FFT (which is an optimized version of DFT) splits the samples in half until each one contains 2 values and processes recursively the DFT for each half. The regular FT is actually impossible
	to compute because it tests an infinite amount of waves (represented by the resulting complex numbers), starting at 0Hz (DC), to the 1st harmonic, the 2nd harmonic
	up to the Nyquist Frequency (Sampling Rate / 2, at least 2 samples  per period are required to reconstruct a wave).
	The Input for Cooley-Tukey FFT has to be a power of 2 and for the FFT in general contain samples of one full period of the Signal. There are more optimized versions that
	bypass the requirement for n^2 samples.
	FFT (optimized DFT):
	X[k] = Σ (n=0 bis N-1) x[n] * exp(-i*2*pi * k * n / N) = Σ (n=0 bis N-1) x[n] * (cos(2*pi*k*n/N)-i*sin(2*pi*k*n/N))
*/

// FFT based on Cooley-Tukey
// returns DFT (size of N) with the phases, magnitudes and frequencies in cartesian form (e.g. 3.5+2.6i)
// example in pseudo code: https://en.wikipedia.org/wiki/Cooley%E2%80%93Tukey_FFT_algorithm
void fft_cooley_tukey(complex* _samples, const int& _N) {
	// only one sample
	if (_N == 1) {
		return;
	}

	// split in 2 arrays, one for all elements where n is odd and one where n is even
	complex* e = new complex[_N / 2];
	complex* o = new complex[_N / 2];
	for (int n = 0; n < _N / 2; n++) {
		e[n] = _samples[n * 2];
		o[n] = _samples[n * 2 + 1];
	}

	// recursively call fft for all stages (with orders power of 2)
	fft_cooley_tukey(e, _N / 2);
	fft_cooley_tukey(o, _N / 2);

	// twiddel factors ( e^(-i*2*pi*k/N) , where k = index and N = order
	// used for shifting the signal
	for (int k = 0; k < _N / 2; k++) {
		float ang = (float)(2 * M_PI * k / _N);
		complex p = e[k];
		complex q = complex(cos(ang), -sin(ang)) * o[k];
		_samples[k] = p + q;
		_samples[k + _N / 2] = p - q;
	}

	delete[] e;
	delete[] o;
}

// inverse FFT: convert samples from frequency domain back to time domain (signal)
// Formulas can be found here: https://www.dsprelated.com/showarticle/800.php
// x[n] = 1/N * sum(m=0 to N-1) (X[m]*(cos(2*PI*m*n/N)+i*sin(2*PI*m*n/N)))		-> discarded normalization factor 1/N
void ifft_cooley_tukey(complex* _samples, const int& _N) {
	// only one sample
	if (_N == 1) {
		return;
	}

	// split in 2 arrays, one for all elements where n is odd and one where n is even
	complex* e = new complex[_N / 2];
	complex* o = new complex[_N / 2];
	for (int n = 0; n < _N / 2; n++) {
		e[n] = _samples[n * 2];
		o[n] = _samples[n * 2 + 1];
	}

	// recursively call fft for all stages (with orders power of 2)
	ifft_cooley_tukey(e, _N / 2);
	ifft_cooley_tukey(o, _N / 2);

	// twiddel factors ( e^(-i*2*pi*k/N) , where k = index and N = order
	// used for shifting the signal
	for (int k = 0; k < _N / 2; k++) {
		float ang = (float)(2 * M_PI * k / _N);
		complex p = e[k];
		complex q = complex(cos(ang), sin(ang)) * o[k];
		_samples[k] = p + q;
		_samples[k + _N / 2] = p - q;
	}

	delete[] e;
	delete[] o;
}

const float alpha = .5f;

// necessary, as it is impossible to assure that the sampled signal consists of exactly n periods (where n is an integer) and is continuous (doesn't matter in our application)
// could come in handy for something like a N64 where audio files (sort of) are played
void window_tukey(complex* _samples, const int& _N) {
	int N = _N - 1;
	int t_low = (int)(((alpha * N) / 2) + .5f);				// + .5f for rounding
	int t_high = (int)((N - (alpha * N) / 2) + .5f);

	for (int n = 0; n < t_low; n++) {
		_samples[n].real *= .5f - .5f * (float)cos(2 * M_PI * n / (alpha * N));
	}
	for (int n = t_high + 1; n < _N; n++) {
		_samples[n].real *= .5f - .5f * (float)cos(2 * M_PI * (N - n) / (alpha * N));
	}
}

void window_hamming(complex* _samples, const int& _N) {
	int N = _N - 1;
	for (int i = 0; i < _N; i++) {
		_samples[i].real *= (float)(0.54f - 0.46f * cos(2 * M_PI * i / N));
	}
}

void window_blackman(complex* _samples, const int& _N) {
	int N = _N - 1;
	for (int i = 0; i < _N; i++) {
		_samples[i].real *= (float)(0.42f - 0.5f * cos(2 * M_PI * i / N) * 0.08 * cos(4 * M_PI * i / N));
	}
}

// produces a window-sinc filter kernel for a given cutoff frequency and transition bandwidth
// _cutoff < _sampling_rate / 2 && _cutoff > 0
// _transition < _sampling_rate / 2 && _cutoff > 0
void fn_window_sinc(complex* _samples, const int& _N, const int& _sampling_rate, const int& _cutoff, const int& _transition) {
	float fc = (float)_cutoff / _sampling_rate;
	float BW = (float)_transition / _sampling_rate;
	int M = (int)(4 / BW);
	for (int i = 0; i < M; i++) {
		_samples[i].real = (float)(sin(2 * M_PI * fc * (i - (M / 2))) / (i - (M / 2)));
	}
	window_blackman(_samples, M);
}