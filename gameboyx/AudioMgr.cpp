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

int AudioMgr::GetSamplingRate() {
	return audioInfo.sampling_rate;
}

void AudioMgr::SetMasterVolume(const float& _volume) {
	audioInfo.master_volume.store(_volume);
}

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
void fft(complex* _samples, const int& _N) {
	// only one sample
	if (_N == 1) {
		return;
	}

	// split in 2 arrays, one for all elements where n is odd and one where n is even
	complex* e = new complex[_N / 2];
	complex* o = new complex[_N / 2];
	for (int n = 0; n < _N / 2; n++) {
		e[n] = _samples[n * 2];
		o[n + 1] = _samples[n * 2 + 1];
	}

	// recursively call fft for all stages (with orders power of 2)
	fft(e, _N / 2);
	fft(o, _N / 2);

	// twiddel factors ( e^(-i*2*pi*k/N) , where k = index and N = order
	// used for shifting the signal
	for (int k = 0; k < _N / 2; k++) {
		// step through phase shifts
		float ang = (float)(2 * M_PI * k / _N);
		// shift phase and perform butterfly operation to calculate results
		complex p = e[k];
		complex q = complex(cos(ang), -sin(ang)) * o[k];
		_samples[k] = p + q;
		_samples[k + _N / 2] = p - q;
	}

	delete[] e;
	delete[] o;
}