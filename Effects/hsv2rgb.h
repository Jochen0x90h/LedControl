#pragma once

#include "types.h"
#include "generated/hue.h"


// hsv to rgb using rainbow scheme
// see https://github.com/FastLED/FastLED/wiki/FastLED-HSV-Colors
inline RGB hsv2rgb(HSV hsv) {
	// convert hue to rgb
	RGB color = hue(hsv.hue);
	
	// now apply saturation and value
	// rgb = (color * saturation + (1 - saturation)) * value
	// => rgb = color * saturation * value + value - saturation * value
	uint8_t sat = hsv.saturation;
	uint8_t val = hsv.value;
	uint8_t sat_val = scale8u(sat, val);

	color.red = scale8u(color.red, sat_val) + val - sat_val;
	color.green = scale8u(color.green, sat_val) + val - sat_val;
	color.blue = scale8u(color.blue, sat_val) + val - sat_val;
	return color;
}
