#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>
#include <vector>
#include "logger.h"

using std::swap;

size_t to_power_of_two(const size_t& _in);

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

	float magnitude() {
		return (float)sqrt(pow(real, 2) + pow(imaginary, 2));
	}

	// the phase offset in rad
	float phase() {
		return atan2(imaginary, real);
	}

	float frequency(const int& _sampling_rate, const int& _index, const int& _size) {
		return (float)_sampling_rate * _index / _size;
	}

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

void fft_cooley_tukey(std::vector<complex>& _samples);
void ifft_cooley_tukey(std::vector<complex>& _samples);
void window_tukey(std::vector<complex>& _samples);
void window_hamming(std::vector<complex>& _samples);
void window_blackman(std::vector<complex>& _samples);
void fn_window_sinc(std::vector<complex>& _samples, const int& _sampling_rate, const int& _cutoff, const int& _transition);

// convolution in time domain corresponds to multiplication in frequency domain: as the convolution cancels out frequencies in the time domain that are not present (or have a very small magnitude) in the resulting signal, this logically is the same result as multiplying the frequencies in the frequency domain
struct fir_filter {
	std::vector<complex> impulse_response;

	int f_transition;
	int f_cutoff;
	int sampling_rate;

	fir_filter(const int& _sampling_rate, const int& _f_cutoff, const int& _f_transition) : sampling_rate(_sampling_rate), f_cutoff(_f_cutoff), f_transition(_f_transition) {
		fn_window_sinc(impulse_response, _sampling_rate, _f_cutoff, _f_transition);
	}

	void apply(std::vector<complex>& _samples) {
		size_t size = to_power_of_two(impulse_response.size() + _samples.size() - 1);
		std::vector<complex> tmp = std::vector<complex>(impulse_response);
		tmp.resize(size);
		_samples.resize(size);

		fft_cooley_tukey(tmp);
		fft_cooley_tukey(_samples);

		for (int i = 0; i < _samples.size(); i++) {
			_samples[i] = _samples[i] * tmp[i];
		}

		ifft_cooley_tukey(_samples);
	}
};

struct iir_filter {

};