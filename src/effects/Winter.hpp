#pragma once

#include "../EffectInfo.hpp"
#include "lookupWinterPalette.hpp"
#include "cos8u10.hpp"
#include <coco/noise.hpp>


/*
 * Summer effect
 */

namespace Winter {

struct Parameters {
    // brightness (0 - 4095)
    int brightness;

    // size of color changes in percent (0 - 4095)
    int size;

    // speed in percent (0 - 4095)
    int speed;
};

// initialize parameters with default values
void init(void *parameters) {
    auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.brightness = 4095; // 100%
    p.size = 5 * 4095 / 100; // 5%
    p.speed = 5 * 4095 / 100; // 5%;
}

Coroutine run(Loop &loop, Strip &strip, const void *parameters) {
    auto &p = *reinterpret_cast<const Parameters *>(parameters);

    Timer timer(loop);
    int count = strip.size();
    while (true) {
        int x = timer.get(loop, 8192000ms / p.speed, 1024 << 8);

        uint32_t step = (1 << 30) / (p.size * count);

        uint32_t noiseOffset1 = x;
        uint32_t noiseStep1 = step;

        uint32_t noiseOffset2 = -noiseOffset1;
        uint32_t noiseStep2 = step >> 1;

        for (int i = 0; i < count; ++i) {
            int n = noiseI16(noiseOffset1) * noiseI16(noiseOffset2) >> (4 + 16);

            auto color = lookupWinterPalette(n & 0x3ff);
            strip.set(i, color * p.brightness >> 12);

            noiseOffset1 += noiseStep1;
            noiseOffset2 += noiseStep2;
        }

        co_await strip.show();
    }
}

const ParameterInfo parameterInfos[] = {
    {"Brightness", ParameterInfo::Type::PERCENTAGE_E12, offsetof(Parameters, brightness)},
    {"Size", ParameterInfo::Type::PERCENTAGE_E12, offsetof(Parameters, size)},
    {"Speed", ParameterInfo::Type::PERCENTAGE_E12, offsetof(Parameters, speed)},
};
constexpr EffectInfo info{"Winter", parameterInfos, sizeof(Parameters), &init, &run};

} // namespace Winter
