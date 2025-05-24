#pragma once

#include "../EffectInfo.hpp"
#include "lookupWinterPalette.hpp"
#include "../math.hpp"


/// @brief Winter effect
///
namespace Winter {

struct Parameters {
    // size of color changes in percent
    float size;
};
const ParameterInfo parameterInfos[] = {
    {"Size", ParameterInfo::Type::PERCENTAGE_E12, 0, 24, 1},
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

        float noiseOffset1 = time + position * step;
        float noiseOffset2 = -time + position * step * 0.5f;

        float n1 = noise(noiseOffset1);

        float n2 = noise(noiseOffset2);

        float x = n1 * n2;

        // color lookup
        color = lookupWinterPalette(fract(x));

        strip.set(ledIndex, color);
    }
}

constexpr EffectInfo info{"Winter", parameterInfos, &init, &end, &run};

} // namespace Winter
