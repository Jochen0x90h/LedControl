#pragma once

#include "../EffectInfo.hpp"
#include "../Timer.hpp"


/*
 * Color fade effect
 * https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/#LEDStripEffectFadeInandFadeOutYourownColors
 */

namespace ColorFade {

struct Parameters {
    // brightness (0 - 4095)
    int brightness;

    // hue (0 - 1535)
    int hue;

    // saturation (0 - 4095)
    int saturation;

    // fade duration in milliseconds
    Milliseconds<> duration;
};

// initialize parameters with default values
void init(void *parameters) {
    auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.brightness = 4095; // 100%
    p.hue = 0; p.saturation = 4095; // red
    p.duration = 1s;
}
/*
// limit the parameters to valid range
void limit(void *parameters) {
    auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.brightness.value = std::clamp(p.brightness.value, 0, 24);
    p.duration.value = std::clamp(p.duration.value, 6, 48);
    p.hue = unsigned(p.hue + 240) % 24; // circular
    p.saturation = std::clamp(p.saturation, 0, 100);
}*/

Coroutine run(Loop &loop, Strip &strip, const void *parameters) {
    auto &p = *reinterpret_cast<const Parameters *>(parameters);
    while (true) {
        Timer timer(loop);
        while (true) {
            int x = timer.get(loop, p.duration, 8192);

            if (x >= 8192)
                break;
            int scale = x < 4096 ? x : 8191 - x;

            HSV hsv{p.hue, p.saturation, p.brightness * scale >> 12};

            strip.fill(toColor(hsv));
            co_await strip.show();
        }
    }
}

const ParameterInfo parameterInfos[] = {
    {"Brightness", ParameterInfo::Type::PERCENTAGE_E12, offsetof(Parameters, brightness)},
    {"Hue", ParameterInfo::Type::HUE, offsetof(Parameters, hue)},
    {"Saturation", ParameterInfo::Type::PERCENTAGE_5, offsetof(Parameters, saturation)},
    {"Duration", ParameterInfo::Type::LONG_DURATION_E12, offsetof(Parameters, duration)},
};
constexpr EffectInfo info{"Color Fade", parameterInfos, sizeof(Parameters), &init, &run};

} // namespace ColorFade
