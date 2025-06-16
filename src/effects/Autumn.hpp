#pragma once

#include "../EffectInfo.hpp"
#include "lookupAutumnPalette.hpp"
#include "../math.hpp"


/// @brief Autumn effect
///
namespace Autumn {

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

        float noiseOffset1 = time + position * step;
        float noiseOffset2 = -time + position * step;
        float noiseOffset3 = time + position * step / 32.0f;
        float noiseOffset4 = -time + position * step / 32.0f;

        // "leaves"
        float l1 = noise(noiseOffset1);
        float l2 = noise(noiseOffset2);
        float l = l1 * l2 * 0.5f + 0.3125f;

        // "tree trunks"
        float t1 = noise(noiseOffset3);
        float t2 = noise(noiseOffset4);
        float t = max(t1 * t2 - 0.1f, 0.0f);

        color = lookupAutumnPalette(max(l - t, 0.0f));

        strip.set(ledIndex, color);
    }
}

constexpr EffectInfo info{"Autumn", parameterInfos, &init, &end, &run};

} // namespace Autumn
