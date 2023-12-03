#pragma once

#include "../EffectInfo.hpp"
#include "../E12.hpp"
#include <coco/Coroutine.hpp>


/*
 * RGB Fade effect
 * https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/#LEDStripEffectFadeInandFadeOutRedGreenandBlue
 */

namespace RgbFade {

struct Parameters {
	E12Milliseconds duration;
};

void init(void *parameters) {
	auto &p = *reinterpret_cast<Parameters *>(parameters);

	p.duration.value = 24; // 1s
}

void limit(void *parameters) {
	auto &p = *reinterpret_cast<Parameters *>(parameters);

	p.duration.value = std::clamp(p.duration.value, 12, 60);
}

Coroutine run(Loop &loop, Strip &strip, void *parameters) {
	auto &p = *reinterpret_cast<Parameters *>(parameters);
	while (true) {
		for (int j = 0; j < 3; j++) {
			Timer timer(loop);
			while (true) {
				int x = timer.get(loop, p.duration, 512);

				if (x >= 512)
					break;
				int k = x < 256 ? x : 511 - x;

				switch(j) {
				case 0: strip.fill(k, 0, 0); break;
				case 1: strip.fill(0, k, 0); break;
				case 2: strip.fill(0, 0, k); break;
				}
				co_await strip.show();
			}
		}
	}
}

const ParameterInfo parameterInfos[] = {
	{"Duration", ParameterInfo::Type::E12_MILLISECONDS, offsetof(Parameters, duration)},
};
constexpr EffectInfo info{"RGB Fade", std::size(parameterInfos), parameterInfos, sizeof(Parameters), &init, &limit, &run};

}
