#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>
#include <vector>

using std::swap;

struct complex {
	float real;
	float imaginary;

	complex() {
		this->real = 0;
		this->imaginary = 0;
	}

	complex(float real, float imaginary) {
		this->real = real;
		this->imaginary = imaginary;
	}

	complex(const complex& other) : real(other.real), imaginary(other.imaginary) {}

	complex& operator=(const complex& rhs) {
		complex tmp(rhs);
		std::swap(*this, tmp);
		return *this;
	}

	complex& operator+=(const complex& rhs) {
		real += rhs.real;
		imaginary += rhs.imaginary;
		return *this;
	}

	complex& operator-=(const complex& rhs) {
		real -= rhs.real;
		imaginary -= rhs.imaginary;
		return *this;
	}

	complex& operator*=(const complex& rhs) {
		real = real * rhs.real - imaginary * rhs.imaginary;
		imaginary = real * rhs.imaginary + imaginary * rhs.real;
		return *this;
	}

	complex& operator/=(const complex& rhs) {
		float divisor = (float)(pow(rhs.real, 2) + pow(rhs.imaginary, 2));
		real = (real * rhs.real + imaginary * rhs.imaginary) / divisor;
		imaginary = (imaginary * rhs.real - real * rhs.imaginary) / divisor;
		return *this;
	}

	complex operator!() const {
		return complex(
			real,
			-imaginary
		);
	}
};

inline complex operator+(const complex& lhs, const complex& rhs) {
	complex res = lhs;
	res += rhs;
	return res;
}

inline complex operator-(const complex& lhs, const complex& rhs) {
	complex res = lhs;
	res -= rhs;
	return res;
}

inline complex operator*(const complex& lhs, const complex& rhs) {
	complex res = lhs;
	res *= rhs;
	return res;
}

inline complex operator/(const complex& lhs, const complex& rhs) {
	complex res = lhs;
	res /= rhs;
	return res;
}

void fft_cooley_tukey(complex* _samples, const int& _N);
void ifft_cooley_tukey(complex* _samples, const int& _N);
void window_tukey(complex* _samples, const int& _N);
void window_hamming(complex* _samples, const int& _N);
void window_blackman(complex* _samples, const int& _N);
void fn_window_sinc(complex* _samples, const int& _N, const int& _sampling_rate, const int& _cutoff, const int& _transition);

// convolution in time domain corresponds to multiplication in frequency domain: as the convolution cancels out frequencies in the time domain that are not present (or have a very small magnitude) in the resulting signal, this logically is the same result as multiplying the frequencies in the frequency domain
struct fir_filter {
	std::vector<complex> buffer;

	int f_transition;
	int f_cutoff;
	int sampling_rate;

	fir_filter(const int& _sampling_rate, const int& _f_cutoff, const int& _f_transition) : sampling_rate(_sampling_rate), f_cutoff(_f_cutoff), f_transition(_f_transition) {
		buffer.assign(_sampling_rate / 2, {});
		fn_window_sinc(buffer.data(), (int)buffer.size(), _sampling_rate, _f_cutoff, _f_transition);
	}
};

struct iir_filter {

};