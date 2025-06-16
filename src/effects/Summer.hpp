#pragma once

#include "../EffectInfo.hpp"
#include "lookupSummerPalette.hpp"
#include "../math.hpp"


/// @brief Summer effect
///
namespace Summer {

struct Parameters {
    // size of color changes in percent
    float size;
};
const ParameterInfo parameterInfos[] = {
    {"Size", ParameterInfo::Type::PERCENTAGE_E12},//, 0, 24, 1},
};

// initialize parameters with default values
void init(void *parameters) {
    auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.size = 0.05f; // 5%
}

bool end(float time, const void *parameters) {
    return time >= 256.0f;
}

void run(StripData strip, float brightness, float time, const void *parameters) {
    int count = strip.size();
    auto &p = *reinterpret_cast<const Parameters *>(parameters);

    for (int ledIndex = 0; ledIndex < count; ++ledIndex) {
        float position = float(ledIndex);
        float3 color = {0, 0, 0};

        float step = 4.0f / (p.size * count);

        float sinOffset = time + position * step * 0.25f;// * 4.0f;
        float noiseOffset1 = time + position * step * 0.5f;
        float noiseOffset2 = -time + position * step;

        // noise as "sun rays"
        float n = noise(noiseOffset1) * noise(noiseOffset2);

        // sine as envelope
        float s = sin(sinOffset * 6.2831853f) * 0.5f + 0.5f;

        float x = n * s;

        // color lookup
        color = lookupSummerPalette(fract(x));

        strip.set(ledIndex, color);
    }
}

constexpr EffectInfo info{"Summer", parameterInfos, &init, &end, &run};

} // namespace Summer
