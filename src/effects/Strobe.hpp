#pragma once

#include "../EffectInfo.hpp"


/*
 * Strobe effect
 * https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/#LEDStripEffectStrobe
 */

namespace Strobe {

struct Parameters {
    // brightness (0 - 4095)
    int brightness;

    // hue (0 - 1535)
    int hue;

    // saturation (0 - 4095)
    int saturation;

	// number of flashes
	int strobeCount;

	// duration of one flash
	Milliseconds<> flashDelay;

	// pause after number of flashes
	Milliseconds<> endPause;
};

// initialize parameters with default values
void init(void *parameters) {
	auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.brightness = 4095; // 100%
    p.hue = 0; p.saturation = 4095; // red
	p.strobeCount = 10;
	p.flashDelay = 50ms;
	p.endPause = 1s;
}

// limit the parameters to valid range
/*void limit(void *parameters) {
	auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.brightness.value = std::clamp(p.brightness.value, 0, 24);
    p.hue = unsigned(p.hue + 240) % 24; // circular
    p.saturation = std::clamp(p.saturation, 0, 100);
	p.strobeCount = std::clamp(p.strobeCount, 1, 50);
	p.flashDelay.value = std::clamp(p.flashDelay.value, 6, 48);
	p.endPause.value = std::clamp(p.endPause.value, 6, 48);
}*/

Coroutine run(Loop &loop, Strip &strip, const void *parameters) {
	auto &p = *reinterpret_cast<const Parameters *>(parameters);
	while (true) {
		for (int j = 0; j < p.strobeCount; j++) {
			// calc and set color
            HSV hsv{p.hue, p.saturation, p.brightness};
			strip.fill(toColor(hsv));
			co_await strip.show();
			co_await sleep(loop, p.flashDelay);

			// set black
			strip.clear();
			co_await strip.show();
			co_await sleep(loop, p.flashDelay);
		}
		co_await sleep(loop, p.endPause);
	}
}

const ParameterInfo parameterInfos[] = {
    {"Brightness", ParameterInfo::Type::PERCENTAGE_E12, offsetof(Parameters, brightness)},
    {"Hue", ParameterInfo::Type::HUE, offsetof(Parameters, hue)},
    {"Saturation", ParameterInfo::Type::PERCENTAGE_5, offsetof(Parameters, saturation)},
	{"Strobe Count", ParameterInfo::Type::COUNT_20, offsetof(Parameters, strobeCount)},
	{"Flash Delay", ParameterInfo::Type::SHORT_DURATION_E12, offsetof(Parameters, flashDelay)},
	{"End Pause", ParameterInfo::Type::LONG_DURATION_E12, offsetof(Parameters, endPause)},
};
constexpr EffectInfo info{"Strobe", parameterInfos, sizeof(Parameters), &init, &run};

} // namespace Strobe
