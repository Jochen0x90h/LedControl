#pragma once

#include "../EffectInfo.hpp"
#include "lookupSpringPalette.hpp"
#include "cos8u10.hpp"
#include <coco/noise.hpp>


/*
 * Spring effect
 */

namespace Spring {

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

        uint32_t noiseOffset = -x;
        uint32_t noiseStep = step;

        uint32_t cosOffset = x;
        uint32_t cosStep = step;

        for (int i = 0; i < count; ++i) {
            //int s1 = noiseU8(noiseOffset >> 8);
            int s1 = noiseU16(noiseOffset);
            int s2 = cos8u10[(cosOffset >> 8) & 0x3ff];
            int s = s1 * s2 >> 8;

            // palette lookup
            auto color = lookupSpringPalette((s >> 6) & 0x3ff);

            //sendRGB(scale8RGB(color, brightness));
            strip.set(i, color * p.brightness >> 12);

            cosOffset += cosStep;
            noiseOffset += noiseStep;
        }

        co_await strip.show();
    }
}

const ParameterInfo parameterInfos[] = {
    {"Brightness", ParameterInfo::Type::PERCENTAGE_E12, offsetof(Parameters, brightness)},
    {"Size", ParameterInfo::Type::PERCENTAGE_E12, offsetof(Parameters, size)},
    {"Speed", ParameterInfo::Type::PERCENTAGE_E12, offsetof(Parameters, speed)},
};
constexpr EffectInfo info{"Spring", parameterInfos, sizeof(Parameters), &init, &run};

} // namespace Spring
