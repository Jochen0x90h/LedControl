#pragma once

#include "../EffectInfo.hpp"
#include "../math.hpp"


/*
 * Static color
 */

namespace StaticColor {

struct Parameters {
    // brightness (0 - 4095)
    int brightness;

    // hue (0 - 1535)
    int hue;

    // saturation (0 - 4095)
    int saturation;
};

// initialize parameters with default values
void init(void *parameters) {
    auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.brightness = 4095; // 100%
    p.hue = 0; p.saturation = 4095; // red
}

// limit the parameters to valid range
/*void limit(void *parameters) {
    auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.brightness.value = std::clamp(p.brightness.value, 0, 24);
    p.hue = unsigned(p.hue + 240) % 24; // circular
    p.saturation = std::clamp(p.saturation, 0, 100);
}*/

Coroutine run(Loop &loop, Strip &strip, const void *parameters) {
    auto &p = *reinterpret_cast<const Parameters *>(parameters);

    int count = strip.size();
    auto start = loop.now();
    while (true) {
        float brightness = p.brightness / 4096.0f;
        float hue = p.hue / 1536.0f;
        float saturation = p.saturation / 4096.0f;

        for (int ledIndex = 0; ledIndex < count; ++ledIndex) {
            float position = float(ledIndex);
            //float coordinate = position / float(count - 1);
            float3 color = {0, 0, 0};

            color = hsv2rgb(float3(hue, saturation, brightness));

            strip.set(ledIndex, color);
        }
        co_await strip.show();
    }

/*
    while (true) {
        while (true) {
            HSV hsv{p.hue, p.saturation, p.brightness};
            strip.fill(toColor(hsv));
            / *Color<uint8_t> color1 = {0, 255, 0};
            Color<uint8_t> color2 = {0, 0, 0};
            auto a = strip.array();
            for (int i = 0; i < a.size(); ++i) {
                a[i] = (i & 8) ? color1 : color2;
            }
            strip.fill(color);* /
            co_await strip.show();
        }
    }
*/
}

const ParameterInfo parameterInfos[] = {
    {"Brightness", ParameterInfo::Type::PERCENTAGE_E12, offsetof(Parameters, brightness)},
    {"Hue", ParameterInfo::Type::HUE, offsetof(Parameters, hue)},
    {"Saturation", ParameterInfo::Type::PERCENTAGE_5, offsetof(Parameters, saturation)},
};
constexpr EffectInfo info{"Static Color", parameterInfos, sizeof(Parameters), &init, &run};

} // namespace StaticColor
