#pragma once

/*
 * Winter effect
 */

#include "../EffectInfo.hpp"
#include "lookupWinterPalette.hpp"
//#include "lookupWinterPaletteI.hpp"
#include "../math.hpp"


namespace Winter {

struct Parameters {
    // brightness (0 - 4095)
    int brightness;

    // size of color changes in percent (0 - 4095)
    int size;

    // speed in percent (0 - 4095)
    //int speed;

    // duration for movement over whole strip
    Milliseconds<> duration;
};

// initialize parameters with default values
void init(void *parameters) {
    auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.brightness = 4095; // 100%
    p.size = 5 * 4095 / 100; // 5%
    //p.speed = 5 * 4095 / 100; // 5%;
    p.duration = 5s;
}

Coroutine run(Loop &loop, Strip &strip, const void *parameters) {
    auto &p = *reinterpret_cast<const Parameters *>(parameters);

    int count = strip.size();
    auto start = loop.now();
    while (true) {
        auto now = loop.now();
        auto t = now - start;
        if (t > p.duration * 256) {
            start = now;
            t = 0s;
        }
        float time = float(t) / float(p.duration);

        float brightness = p.brightness / 4096.0f;
        float size = p.size / 4096.0f;

        for (int ledIndex = 0; ledIndex < count; ++ledIndex) {
            float position = float(ledIndex);
            //float coordinate = position / float(count - 1);
            float3 color = {0, 0, 0};

            float step = 4.0f / (size * count);

            float noiseOffset1 = time + position * step;
            float noiseOffset2 = -time + position * step * 0.5f;

            float n1 = noise(noiseOffset1);

            float n2 = noise(noiseOffset2);

            float x = n1 * n2;

            // color lookup
            color = lookupWinterPalette(fract(x));

            strip.set(ledIndex, color * brightness);
        }
        co_await strip.show();
    }

/*
    Timer timer(loop);
    while (true) {
        int x = timer.get(loop, 8192000ms / p.speed, 1024 << 8);

        uint32_t step = (1 << 30) / (p.size * count);

        uint32_t noiseOffset1 = x;
        uint32_t noiseStep1 = step;

        uint32_t noiseOffset2 = -noiseOffset1;
        uint32_t noiseStep2 = step >> 1;

        for (int i = 0; i < count; ++i) {
            int n = noiseI16(noiseOffset1) * noiseI16(noiseOffset2) >> (4 + 16);

            auto color = lookupWinterPaletteI(n & 0x3ff);
            strip.set(i, color * p.brightness >> 12);

            noiseOffset1 += noiseStep1;
            noiseOffset2 += noiseStep2;
        }

        co_await strip.show();
    }
*/
}

const ParameterInfo parameterInfos[] = {
    {"Brightness", ParameterInfo::Type::PERCENTAGE_E12, offsetof(Parameters, brightness)},
    {"Size", ParameterInfo::Type::PERCENTAGE_E12, offsetof(Parameters, size)},
    //{"Speed", ParameterInfo::Type::PERCENTAGE_E12, offsetof(Parameters, speed)},
    {"Duration", ParameterInfo::Type::LONG_DURATION_E12, offsetof(Parameters, duration)},
};
constexpr EffectInfo info{"Winter", parameterInfos, sizeof(Parameters), &init, &run};

} // namespace Winter
