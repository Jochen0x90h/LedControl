#pragma once

#include "../EffectInfo.hpp"
#include "../math.hpp"


/// @brief Meteor rain effect
/// https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/#LEDStripEffectMeteorRain
namespace MeteorRain {

struct Parameters {
    // color
    float hue;
    float saturation;

    // size of meteor as percentage of whole strip
    float size;

    // random decay percent
    float randomDecay;
};
const ParameterInfo parameterInfos[] = {
    {"Hue", ParameterInfo::Type::HUE, 0, 23, 1, true},
    {"Saturation", ParameterInfo::Type::PERCENTAGE, 0, 100, 5},
    {"Size", ParameterInfo::Type::PERCENTAGE_E12, 0, 24, 1},
    {"Random Decay", ParameterInfo::Type::PERCENTAGE_E12, 0, 24, 1},
};

// initialize parameters with default values
void init(void *parameters) {
    auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.hue = 0; p.saturation = 1.0f; // red
    p.size = 0.1f; // 10%
    p.randomDecay = 1.0f; // 100%
}

bool end(float time, const void *parameters) {
    // 256 iterations so that meteor looks different in every iteration
    return time >= 256.0f;
}

void run(Strip &strip, float brightness, float time, const void *parameters) {
    int count = strip.size();
    auto &p = *reinterpret_cast<const Parameters *>(parameters);

    for (int ledIndex = 0; ledIndex < count; ++ledIndex) {
        float position = float(ledIndex);
        float3 color = {0, 0, 0};

        float t = fract(time);
        float iteration = trunc(time);

        // half the LEDs are 100% size
        float s = 0.5f * p.size * count;

        float meteorEnd = t * count * 3.0f;
        float meteorStart = meteorEnd - s;
        float b;
        if (position < meteorStart) {
            // trail

            // "age" of the current "particle"
            float age = meteorStart - position;

            // calc noise using randomness and age
            float o = iteration * 20.0f;
            b = clamp(p.randomDecay * noise(o + position * 0.25f) + 1.0f - age / 255.0f, 0.0f, 1.0f);
        } else if (position < meteorEnd + 1.0f) {
            // meteor
            b = min(1.0f - (position - meteorEnd), 1.0f);
        } else {
            // space
            b = 0;
        }
        color = hsv2rgb(float3(p.hue, p.saturation, brightness * b));

        strip.set(ledIndex, color);
    }
}

constexpr EffectInfo info{"Meteor Rain", parameterInfos, &init, &end, &run};

} // namespace MeteorRain
