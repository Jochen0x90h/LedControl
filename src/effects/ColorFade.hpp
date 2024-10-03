#pragma once

/*
 * Color fade effect, fades one color on and off
 * https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/#LEDStripEffectFadeInandFadeOutYourownColors
 */

#include "../EffectInfo.hpp"
#include "../math.hpp"


namespace ColorFade {

struct Parameters {
    float hue;
    float saturation;
};
const ParameterInfo parameterInfos[] = {
    {"Hue", ParameterInfo::Type::HUE, 0, 23, 1, true},
    {"Saturation", ParameterInfo::Type::PERCENTAGE, 0, 100, 5},
};


// initialize parameters with default values
void init(void *parameters) {
    auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.hue = 0; // red
    p.saturation = 1.0f;
}

bool end(float time, const void *parameters) {
    return time >= 1.0f;
}

void run(Strip &strip, float brightness, float time, const void *parameters) {
    int count = strip.size();
    auto &p = *reinterpret_cast<const Parameters *>(parameters);

    for (int ledIndex = 0; ledIndex < count; ++ledIndex) {
        float position = float(ledIndex);
        float3 color = {0, 0, 0};

        float fade = min(time, 1.0f - time) * 2.0f;
        color = hsv2rgb(float3(p.hue, p.saturation, brightness * fade));

        strip.set(ledIndex, color);
    }
}

constexpr EffectInfo info{"Color Fade", parameterInfos, &init, &end, &run};

} // namespace ColorFade
