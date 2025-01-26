#pragma once

#include "../EffectInfo.hpp"
#include "../math.hpp"


/// @brief Cylon bounce effect
/// https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/#LEDStripEffectCylon
namespace CylonBounce {

struct Parameters {
    float hue;
    float saturation;

    // size of "eye" as percentage of whole strip
    float size;
};
const ParameterInfo parameterInfos[] = {
    {"Hue", ParameterInfo::Type::HUE, 0, 23, 1, true},
    {"Saturation", ParameterInfo::Type::PERCENTAGE, 0, 100, 5},
    {"Size", ParameterInfo::Type::PERCENTAGE_E12, 0, 24, 1},
};

// initialize parameters with default values
void init(void *parameters) {
    auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.hue = 0; p.saturation = 1.0f; // red
    p.size = 0.12f; // 12%
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

        float w = count * p.size;
        float eyeStart = min(time, 1.0f - time) * 2.0f * (count - w);
        float eyeEnd = eyeStart + w;

        float b = 0;
        if (position < eyeStart - 1.0f) {
        } else if (position < eyeStart) {
            // interpolate start
            b = 1.0 - (eyeStart - position);
        } else if (position < eyeEnd) {
            // "eye"
            b = 1.0f;
        } else if (position < eyeEnd + 1.0f) {
            // interpolate end
            b = 1.0f - (position - eyeEnd);
        }
        color = hsv2rgb(float3(p.hue, p.saturation, brightness * b));

        strip.set(ledIndex, color);
    }
}

constexpr EffectInfo info{"Cylon Bounce", parameterInfos, &init, &end, &run};

} // namespace CylonBounce
