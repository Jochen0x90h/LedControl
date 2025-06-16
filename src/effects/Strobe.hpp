#pragma once

#include "../EffectInfo.hpp"
#include "../math.hpp"


/// @param Strobe effect
/// https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/#LEDStripEffectStrobe
namespace Strobe {

struct Parameters {
    float hue;
    float saturation;

    // number of flashes
    float strobeCount;

    // percentage of strobe time, rest is pause
    float strobePercentage;
};
const ParameterInfo parameterInfos[] = {
    {"Hue", ParameterInfo::Type::HUE},//, 0, 23, 1, true},
    {"Saturation", ParameterInfo::Type::PERCENTAGE, 5},//, 0, 100, 5},
    {"Strobe Count", ParameterInfo::Type::COUNT, 100},//1, 100, 1},
    {"Percentage", ParameterInfo::Type::PERCENTAGE, 5},//0, 100, 5},
};

// initialize parameters with default values
void init(void *parameters) {
    auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.hue = 0; p.saturation = 1.0f; // red
    p.strobeCount = 10.0f;
    p.strobePercentage = 1.0f;
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

        if (time < p.strobePercentage) {
            float t = time / p.strobePercentage * p.strobeCount;
            if (fract(t) < 0.5f)
                color = hsv2rgb(float3(p.hue, p.saturation, brightness));
        }

        strip.set(ledIndex, color);
    }
}

constexpr EffectInfo info{"Strobe", parameterInfos, &init, &end, &run};

} // namespace Strobe
