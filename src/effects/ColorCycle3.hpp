#pragma once

#include "../EffectInfo.hpp"
#include "../math.hpp"


/// @brief Color cycle effect with 3 colors, interpolates between three colors
///
namespace ColorCycle3 {

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

void run(StripData strip, float brightness, float time, const void *parameters) {
    int count = strip.size();
    auto &p = *reinterpret_cast<const Parameters *>(parameters);

    for (int ledIndex = 0; ledIndex < count; ++ledIndex) {
        //float position = float(ledIndex);
        float3 color = {0, 0, 0};

        float t = fract(time);
        float iteration = trunc(time);

        // select hue and saturation
        float h1, h2;
        float s1, s2;
        if (iteration == 0.0f) {
            h1 = p.hue1;
            h2 = p.hue2;
            s1 = p.saturation1;
            s2 = p.saturation2;
        } else if (iteration == 1.0f) {
            h1 = p.hue2;
            h2 = p.hue3;
            s1 = p.saturation2;
            s2 = p.saturation3;
        } else {
            h1 = p.hue3;
            h2 = p.hue1;
            s1 = p.saturation3;
            s2 = p.saturation1;
        }

        // interpolate hue (on "shortest path") and saturation
        float hue = mix(h1, h2 - round(h2 - h1), t);
        float saturation = mix(s1, s2, t);

        // convert hsv to rgb color
        color = hsv2rgb(float3(hue, saturation, brightness));

        strip.set(ledIndex, color);
    }
}

constexpr EffectInfo info{"Color Cycle 3", parameterInfos, &init, &end, &run};

} // namespace ColorCycle3
