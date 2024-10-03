#pragma once

/*
 * Spring effect
 */

#include "../EffectInfo.hpp"
#include "lookupSpringPalette.hpp"
#include "../math.hpp"


namespace Spring {

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

void run(Strip &strip, float brightness, float time, const void *parameters) {
    int count = strip.size();
    auto &p = *reinterpret_cast<const Parameters *>(parameters);

    for (int ledIndex = 0; ledIndex < count; ++ledIndex) {
        float position = float(ledIndex);
        float3 color = {0, 0, 0};

        float step = 4.0f / (p.size * count);

        float noiseOffset = -time + position * step;
        float cosOffset = time + position * step * 0.25f;

        float s1 = noise(noiseOffset) + 1.0f;
        float s2 = cos(cosOffset * 6.2831853f) + 1.0f;
        float s = s1 * s2 * 0.25f;

        // palette lookup
        color = lookupSpringPalette(fract(s));

        strip.set(ledIndex, color);
    }
}

constexpr EffectInfo info{"Spring", parameterInfos, &init, &end, &run};

} // namespace Spring
