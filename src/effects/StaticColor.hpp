#pragma once

#include "../EffectInfo.hpp"
#include "../math.hpp"


/// @brief Static color effect
/// Displays an uniform color without animation
namespace StaticColor {

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

    p.hue = 0; p.saturation = 1.0f; // red
}

bool end(float time, const void *parameters) {
    return time >= 1.0f;
}

void run(StripData strip, float brightness, float time, const void *parameters) {
    int count = strip.size();
    auto &p = *reinterpret_cast<const Parameters *>(parameters);

    for (int ledIndex = 0; ledIndex < count; ++ledIndex) {
        //float position = float(ledIndex);
        float3 color = {0, 0, 0};

        color = hsv2rgb(float3(p.hue, p.saturation, brightness));

        strip.set(ledIndex, color);
    }
}

constexpr EffectInfo info{"Static Color", parameterInfos, &init, &end, &run};

} // namespace StaticColor
