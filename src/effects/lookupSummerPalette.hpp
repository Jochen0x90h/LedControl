#pragma once

inline Color<int> lookupSummerPalette(int x) {
	int r, g, b;
	if (x < 0x200) {
		if (x < 0x100) {
			// 0x0 - 0xff
			r = 1597 + ((x - 0x0) * 2498 >> 8);
			g = 1597 + ((x - 0x0) * 2498 >> 8);
			b = 4095 - ((x - 0x0) * 2498 >> 8);
		} else {
			// 0x100 - 0x1ff
			r = 4095;
			g = 4095;
			b = 1597 + ((x - 0x100) * 1597 >> 8);
		}
	} else {
		if (x < 0x300) {
			// 0x200 - 0x2ff
			r = 4095 - ((x - 0x200) * 4095 >> 8);
			g = 4095 - ((x - 0x200) * 4095 >> 8);
			b = 3194 + ((x - 0x200) * 901 >> 8);
		} else {
			// 0x300 - 0x3ff
			r = 0 + ((x - 0x300) * 1597 >> 8);
			g = 0 + ((x - 0x300) * 1597 >> 8);
			b = 4095;
		}
	}
	return {r, g, b};
}
