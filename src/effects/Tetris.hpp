#pragma once

#include "../EffectInfo.hpp"


/// @brief Tetris effect
///
namespace Tetris {

const uint8_t tetrisRandom[32] = {0, 3, 1, 4, 2, 4, 0, 2, 1, 3, 1, 0, 2, 1, 3, 4, 2, 4, 1, 3, 4, 0, 1, 0, 3, 4, 1, 4, 3, 1, 4, 0};

float lookupTetrisRandom(float index) {
    return tetrisRandom[int(index) & 31];
}

struct Parameters {
    // number of blocks (may not be accurate due to random size of blocks)
    float count;
};
const ParameterInfo parameterInfos[] = {
    {"Count", ParameterInfo::Type::COUNT, 1, 50, 1},
};


// initialize parameters with default values
void init(void *parameters) {
    auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.count = 10.0f;
}

bool end(float time, const void *parameters) {
    auto &p = *reinterpret_cast<const Parameters *>(parameters);

    float count = 1.0f;

    float t = (time - 1.0f) * count;

    float endTime = count;
    float randomIndex = 0;
    while (endTime > 0) {
        // random value 0 - 4
        float randomValue = lookupTetrisRandom(randomIndex);

        // size and hue of block
        float size = (1.0f + randomValue) * count / (p.count * 3.0f);

        t -= endTime;
        endTime -= size;
        randomIndex += 1.3f;
    }

    return t >= 1.0f;
}

void run(Strip &strip, float brightness, float time, const void *parameters) {
    int count = strip.size();
    auto &p = *reinterpret_cast<const Parameters *>(parameters);

    for (int ledIndex = 0; ledIndex < count; ++ledIndex) {
        float position = float(ledIndex);
        float3 color = {0, 0, 0};

        // position and end position of block
        float blockPosition = (time - 1.0f) * count;
        float endPosition = count;

        float randomIndex = 0;
        while (endPosition > 0) {
            // random value 0 - 4
            float randomValue = lookupTetrisRandom(randomIndex);

            // size and hue of block
            float size = (1.0f + randomValue) * count / (p.count * 3.0f);
            float hue = randomValue * 0.2f;

            // block
            float e = min(blockPosition, endPosition);
            float s = e - size;
            if (position >= s && position <= e + 1.0f) {
                float value;
                if (position < s + 1) {
                    // ramp up
                    value = position - s;
                } else if (position < e) {
                    value = 1.0f;
                } else {
                    // ramp down
                    value = 1.0f - (position - e);
                }
                color += hsv2rgb({hue, 1.0f, value});
            }

            // update position and end position for next block
            blockPosition -= endPosition;
            endPosition -= size;

            randomIndex += 1.0f;
        }

        color *= brightness;

        strip.set(ledIndex, color);
    }
}

constexpr EffectInfo info{"Tetris", parameterInfos, &init, &end, &run};

} // namespace Tetris
