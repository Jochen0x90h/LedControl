#pragma once

/*
 * Color cycle effect with 3 colors, interpolates between three colors
 */

#include "../EffectInfo.hpp"
#include "../Timer.hpp"
#include "../math.hpp"


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

 	int count = strip.size();
    auto start = loop.now();
    while (true) {
        auto now = loop.now();
        auto t = now - start;
        if (t > p.duration * 3) {
            start = now;
            t = 0s;
        }
        float time = float(t) / float(p.duration);

        float brightness = p.brightness / 4096.0f;
        float hue1 = p.hue1 / 1536.0f;
        float saturation1 = p.saturation1 / 4096.0f;
        float hue2 = p.hue2 / 1536.0f;
        float saturation2 = p.saturation2 / 4096.0f;
        float hue3 = p.hue3 / 1536.0f;
        float saturation3 = p.saturation3 / 4096.0f;

        for (int ledIndex = 0; ledIndex < count; ++ledIndex) {
            float position = float(ledIndex);
            //float coordinate = position / float(count - 1);
            float3 color = {0, 0, 0};

            float t = fract(time);
            float iteration = trunc(time);

			// select hue and saturation
			float h1, h2;
            float s1, s2;
            if (iteration == 0.0f) {
    			h1 = hue1;
				h2 = hue2;
				s1 = saturation1;
				s2 = saturation2;
            } else if (iteration == 1.0f) {
    			h1 = hue2;
				h2 = hue3;
				s1 = saturation2;
				s2 = saturation3;
			} else if (iteration == 2.0f) {
    			h1 = hue3;
				h2 = hue1;
				s1 = saturation3;
				s2 = saturation1;
			}

			// interpolate hue (on "shortest path") and saturation
    		float hue = mix(h1, h2 - round(h2 - h1), t);
			float saturation = mix(s1, s2, t);

			// convert hsv to rgb color
			color = hsv2rgb(float3(hue, saturation, brightness));

            strip.set(ledIndex, color);
        }
        co_await strip.show();
    }

/*
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
*/
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
