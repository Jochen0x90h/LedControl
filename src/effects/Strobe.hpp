#pragma once

#include "../EffectInfo.hpp"
#include "../E12.hpp"
#include <coco/Coroutine.hpp>


/*
 * Strobe effect
 * https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/#LEDStripEffectStrobe
 */

namespace Strobe {

struct Parameters {
	Color color;
	int strobeCount;
	E12Milliseconds flashDelay;
	E12Milliseconds endPause;
};

void init(void *parameters) {
	auto &p = *reinterpret_cast<Parameters *>(parameters);

	p.color = Color(255, 0, 0);
	p.strobeCount = 10;
	p.flashDelay.value = 12 + 8; // 50ms
	p.endPause.value = 24; // 1s
}

void limit(void *parameters) {
	auto &p = *reinterpret_cast<Parameters *>(parameters);

	p.strobeCount = std::clamp(p.strobeCount, 1, 50);
	p.flashDelay.value = std::clamp(p.flashDelay.value, 6, 48);
	p.endPause.value = std::clamp(p.endPause.value, 6, 48);
}

Coroutine run(Loop &loop, Strip &strip, void *parameters) {
	auto &p = *reinterpret_cast<Parameters *>(parameters);
	while (true) {
		for (int j = 0; j < p.strobeCount; j++) {
			strip.fill(p.color);
			co_await strip.show();
			co_await sleep(loop, p.flashDelay);

			strip.clear();
			co_await strip.show();
			co_await sleep(loop, p.flashDelay);
		}
		co_await loop.sleep(p.endPause);
	}
}

const ParameterInfo parameterInfos[] = {
	{"Color", ParameterInfo::Type::COLOR, offsetof(Parameters, color)},
	{"Strobe Count", ParameterInfo::Type::INTEGER, offsetof(Parameters, strobeCount)},
	{"Flash Delay", ParameterInfo::Type::E12_MILLISECONDS, offsetof(Parameters, flashDelay)},
	{"End Pause", ParameterInfo::Type::E12_MILLISECONDS, offsetof(Parameters, endPause)},
};
constexpr EffectInfo info{"Strobe", std::size(parameterInfos), parameterInfos, sizeof(Parameters), &init, &limit, &run};

}
