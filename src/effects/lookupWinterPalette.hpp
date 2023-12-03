#pragma once

inline Color<int> lookupWinterPalette(int x) {
	int r, g, b;
	if (x < 0x200) {
		if (x < 0x100) {
			// 0x0 - 0xff
			r = 0;
			g = 0 + ((x - 0x0) * 3358 >> 8);
			b = 2252 + ((x - 0x0) * 123 >> 8);
		} else {
			// 0x100 - 0x1ff
			r = 0 + ((x - 0x100) * 1597 >> 8);
			g = 3358 - ((x - 0x100) * 2089 >> 8);
			b = 2375 - ((x - 0x100) * 450 >> 8);
		}
	} else {
		if (x < 0x300) {
			// 0x200 - 0x2ff
			r = 1597;
			g = 1269 - ((x - 0x200) * 1269 >> 8);
			b = 1925 + ((x - 0x200) * 163 >> 8);
		} else {
			// 0x300 - 0x3ff
			r = 1597 - ((x - 0x300) * 1597 >> 8);
			g = 0;
			b = 2088 + ((x - 0x300) * 164 >> 8);
		}
	}
	return {r, g, b};
}
