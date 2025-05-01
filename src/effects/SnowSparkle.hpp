#pragma once

#include "../EffectInfo.hpp"
#include "../math.hpp"


/// @brief Snow sparkle effect
/// https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/#LEDStripEffectSnowSparkle
namespace SnowSparkle {

struct Parameters {
    // sparkle color
    float sparkleHue;
    float sparkleSaturation;

    // background color
    float backgroundHue;
    float backgroundSaturation;
    float backgroundBrightness;

    // percentage of time interval that a sparkle is visible
    float percentage;
};
const ParameterInfo parameterInfos[] = {
    {"Sparkle Hue", ParameterInfo::Type::HUE, 0, 23, 1, true},
    {"Sparkle Saturation", ParameterInfo::Type::PERCENTAGE, 0, 100, 5},
    {"Back Hue", ParameterInfo::Type::HUE, 0, 23, 1, true},
    {"Back Saturation", ParameterInfo::Type::PERCENTAGE, 0, 100, 5},
    {"Back Brightness", ParameterInfo::Type::PERCENTAGE_E12, 0, 24, 1},
    {"Percentage", ParameterInfo::Type::PERCENTAGE, 0, 100, 2},
};

// initialize parameters with default values
void init(void *parameters) {
    auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.sparkleHue = 0; p.sparkleSaturation = 0; // white
    p.backgroundHue = 0; p.backgroundSaturation = 0; // white
    p.backgroundBrightness = 0.05f; // 5%
    p.percentage = 0.1f; // 10%
}

bool end(float time, const void *parameters) {
    return time >= 256.0f;
}

// https://gist.github.com/badboy/6267743
int hash32shiftmult(uint32_t key) {
    int c2 = 0x27d4eb2d; // a prime or an odd constant
    key = (key ^ 61) ^ (key >> 16);
    key = key + (key << 3);
    key = key ^ (key >> 4);
    key = key * c2;
    key = key ^ (key >> 15);
    return key;
}

int hash(int x, int y) {
    return hash32shiftmult(x) * 53 + hash32shiftmult(y);
}

void run(Strip &strip, float brightness, float time, const void *parameters) {
    int count = strip.size();
    auto &p = *reinterpret_cast<const Parameters *>(parameters);

    for (int ledIndex = 0; ledIndex < count; ++ledIndex) {
        float position = float(ledIndex);
        float3 color = {0, 0, 0};

        float t = fract(time);
        //float iteration = trunc(time);

        int h = hash(int(position), int(time));

        float3 c1(p.sparkleHue, p.sparkleSaturation, brightness);
        float3 c2(p.backgroundHue, p.backgroundSaturation, brightness * p.backgroundBrightness);
        color = hsv2rgb(t < p.percentage && h > 0x7f000000 ? c1 : c2);

        strip.set(ledIndex, color);
    }
}

constexpr EffectInfo info{"Snow Sparkle", parameterInfos, &init, &end, &run};

} // namespace MeteorRain
