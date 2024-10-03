#pragma once

/*
 * Color fade effect with 3 colors
 * https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/#LEDStripEffectFadeInandFadeOutRedGreenandBlue
 */

#include "../EffectInfo.hpp"
#include "../math.hpp"


namespace ColorFade3 {

struct Parameters {
    float hue1;
    float saturation1;
    float hue2;
    float saturation2;
    float hue3;
    float saturation3;
};
const ParameterInfo parameterInfos[] = {
    {"Hue 1", ParameterInfo::Type::HUE, 0, 23, 1, true},
    {"Saturation 1", ParameterInfo::Type::PERCENTAGE, 0, 100, 5},
    {"Hue 2", ParameterInfo::Type::HUE, 0, 23, 1, true},
    {"Saturation 2", ParameterInfo::Type::PERCENTAGE, 0, 100, 5},
    {"Hue 3", ParameterInfo::Type::HUE, 0, 23, 1, true},
    {"Saturation 3", ParameterInfo::Type::PERCENTAGE, 0, 100, 5},
};

// initialize parameters with default values
void init(void *parameters) {
	auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.hue1 = 0; p.saturation1 = 1.0f; // red
    p.hue2 = 0.3333f; p.saturation2 = 1.0f; // green
    p.hue3 = 0.6667f; p.saturation3 = 1.0f; // blue
}

bool end(float time, const void *parameters) {
    return time >= 3.0f;
}

void run(Strip &strip, float brightness, float time, const void *parameters) {
    int count = strip.size();
    auto &p = *reinterpret_cast<const Parameters *>(parameters);

    for (int ledIndex = 0; ledIndex < count; ++ledIndex) {
        float position = float(ledIndex);
        float3 color = {0, 0, 0};

        float t = fract(time);
        float iteration = trunc(time);

        float hue;
        float saturation;
        if (iteration == 0.0f) {
            hue = p.hue1;
            saturation = p.saturation1;
        } else if (iteration == 1.0f) {
            hue = p.hue2;
            saturation = p.saturation2;
        } else if (iteration == 2.0f) {
            hue = p.hue3;
            saturation = p.saturation3;
        }
        float fade = min(t, 1.0f - t) * 2.0f;
        color = hsv2rgb(float3(hue, saturation, brightness * fade));

        strip.set(ledIndex, color);
    }
}

constexpr EffectInfo info{"Color Fade 3", parameterInfos, &init, &end, &run};

} // namespace ColorFade3
