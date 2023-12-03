#pragma once

#include <cstdint>
#include <coco/assert.hpp>


namespace coco {


template <typename T>
struct Color {
	T r;
	T g;
	T b;

	Color() = default;
	Color(T r, T g, T b) : r(r), g(g), b(b) {}

	template <typename T2>
	explicit Color(Color<T2> color) : r(color.r), g(color.g), b(color.b) {}
};

template <typename T>
Color<T> operator <<(Color<T> color, int x) {
	return {color.r << x, color.g << x, color.b << x};
}

template <typename T>
Color<T> operator >>(Color<T> color, int x) {
	return {color.r >> x, color.g >> x, color.b >> x};
}

template <typename T>
Color<T> operator *(Color<T> color, int x) {
	return {color.r * x, color.g * x, color.b * x};
}


struct HSV {
	// 6 * 8 bit hue (0 - 1535)
	int h;

	// 12 bit saturation (0 - 4096)
	int s;

	// 12 bit brightness (0 - 4096, use 4095 as maximum to prevent overflow wenn converting to 8 bit)
	int v;
};


// conversion functions


// convert HSV to color with 12 bit resolution
inline Color<int> toColor(HSV hsv) {
	// https://www.rapidtables.com/convert/color/hsv-to-rgb.html
	int H = hsv.h;
	int V = hsv.v;
	int S = hsv.s;

	int C = (V * S) >> 12;
	int X = C * (256 - std::abs((H & 511) - 256)) >> 8;
	int m = V - C;

	int R_, G_, B_;
	switch (H >> 8) {
	case 0:
		R_ = C;
		G_ = X;
		B_ = 0;
		break;
	case 1:
		R_ = X;
		G_ = C;
		B_ = 0;
		break;
	case 2:
		R_ = 0;
		G_ = C;
		B_ = X;
		break;
	case 3:
		R_ = 0;
		G_ = X;
		B_ = C;
		break;
	case 4:
		R_ = X;
		G_ = 0;
		B_ = C;
		break;
	default:
		R_ = C;
		G_ = 0;
		B_ = X;
		break;
	}

	return {R_ + m, G_ + m, B_ + m};
}




/**
 * Fixed point number
 * @tparam F fractional bits (e.g. for F = 8, a value of 256 represents 1.0)
 */
/*
template <int F>
struct Fixed {
	int value;
};


struct Color {
	uint8_t r;
	uint8_t g;
	uint8_t b;

	/ **
		Scale by 24.8 fixed point value without overflow handling
		@param a color to scale
		@param b scale factor in 24.8 fixed point (256 represents 1.0)
		@return scaled color
	* /
	//Color scale(int s) const {
	//	return {uint8_t(this->r * s >> 8), uint8_t(this->g * s >> 8), uint8_t(this->b * s >> 8)};
	//}

	uint8_t &operator[](int index) {
		assert(unsigned(index) < 3);
		return (&r)[index];
	}
};
*/

/**
	Multiply color with fixed point value without overflow handling
	@tparam F fractional bits
	@param color color to multiply
	@param factor multiplication factor
	@return multiplied color
*/
/*template <int F>
Color operator *(const Color &color, Fixed<F> factor) {
	return {uint8_t(color.r * factor.value >> F), uint8_t(color.g * factor.value >> F), uint8_t(color.b * factor.value >> F)};
}*/


/*
struct ColorWhite {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t w;

	uint8_t &operator[](int index) {
		assert(unsigned(index) < 4);
		return (&r)[index];
	}
};

/ **
	Multiply color with fixed point value without overflow handling
	@tparam F fractional bits
	@param color color to multiply
	@param factor multiplication factor
	@return multiplied color
* /
template <int F>
ColorWhite operator *(const ColorWhite &color, Fixed<F> factor) {
	return {uint8_t(color.r * factor.value >> F), uint8_t(color.g * factor.value >> F), uint8_t(color.b * factor.value >> F), uint8_t(color.w * factor.value >> F)};
}*/

} // namespace coco
