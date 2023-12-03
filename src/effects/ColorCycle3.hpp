#pragma once

#include "../EffectInfo.hpp"
#include "../Timer.hpp"


/*
 * Color cycle effect with 3 colors
 */

namespace ColorCycle3 {

struct Parameters {
    // brightness (0 - 4095)
    int brightness;

    int hue1;
    int saturation1;
    int hue2;
    int saturation2;
    int hue3;
    int saturation3;

    // fade duration in milliseconds
    Milliseconds<> duration;
};

// initialize parameters with default values
void init(void *parameters) {
	auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.brightness = 4095; // 100%
    p.hue1 = 0; p.saturation1 = 4095; // red
    p.hue2 = 512; p.saturation2 = 4095; // green
    p.hue3 = 1024; p.saturation3 = 4095; // blue
	p.duration = 1s;
}

// limit the parameters to valid range
/*void limit(void *parameters) {
	auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.brightness.value = std::clamp(p.brightness.value, 0, 24);
	p.duration.value = std::clamp(p.duration.value, 12, 60);
}*/

Coroutine run(Loop &loop, Strip &strip, const void *parameters) {
	auto &p = *reinterpret_cast<const Parameters *>(parameters);
	while (true) {
		for (int j = 0; j < 3; j++) {
			Timer timer(loop);
			while (true) {
				int x = timer.get(loop, p.duration, 4096);

				if (x >= 4096)
					break;

				// interpolate hue
				int hues[] = {p.hue1, p.hue2, p.hue3, p.hue1};
				int ha = hues[j];
				int hb = hues[j + 1];
				int hd = ((hb - ha) * 2796203) / 2796203;
				int hue = (ha + (hd * x >> 12) + 1536) % 1536;

				// interpolate saturation
				int saturations[] = {p.saturation1, p.saturation2, p.saturation3, p.saturation1};
				int sa = saturations[j];
				int sb = saturations[j + 1];
				int saturation = sa + ((sb - sa) * x >> 12);

				auto color = toColor({hue, saturation, p.brightness});
				strip.fill(color);
				co_await strip.show();
			}
		}
	}
}

const ParameterInfo parameterInfos[] = {
    {"Brightness", ParameterInfo::Type::PERCENTAGE_E12, offsetof(Parameters, brightness)},
    {"Hue 1", ParameterInfo::Type::HUE, offsetof(Parameters, hue1)},
    {"Saturation 1", ParameterInfo::Type::PERCENTAGE_5, offsetof(Parameters, saturation1)},
    {"Hue 2", ParameterInfo::Type::HUE, offsetof(Parameters, hue2)},
    {"Saturation 2", ParameterInfo::Type::PERCENTAGE_5, offsetof(Parameters, saturation2)},
    {"Hue 3", ParameterInfo::Type::HUE, offsetof(Parameters, hue3)},
    {"Saturation 3", ParameterInfo::Type::PERCENTAGE_5, offsetof(Parameters, saturation3)},
	{"Duration", ParameterInfo::Type::LONG_DURATION_E12, offsetof(Parameters, duration)},
};
constexpr EffectInfo info{"Color Cycle 3", parameterInfos, sizeof(Parameters), &init, &run};

} // namespace ColorCycle3
