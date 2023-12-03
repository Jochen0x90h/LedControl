#pragma once

/*
 * Summer effect
 */

#include "../EffectInfo.hpp"
#include "lookupSummerPalette.hpp"
//#include "lookupSummerPaletteI.hpp"
#include "../math.hpp"


namespace Summer {

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

            strip.set(ledIndex, color * brightness);
        }
        co_await strip.show();
    }

/*
    Timer timer(loop);
    while (true) {
        int x = 0;//timer.get(loop, 8192000ms / p.speed, 1024 << 8);

        uint32_t step = (1 << 30) / (p.size * count);

        uint32_t cosOffset = x;
        uint32_t cosStep = step << 4;

        uint32_t noiseOffset1 = x;
        uint32_t noiseStep1 = step >> 1;

        uint32_t noiseOffset2 = -noiseOffset1;
        uint32_t noiseStep2 = step;

        for (int i = 0; i < count; ++i) {
            // noise as "sun rays"
            int n = noiseI16(noiseOffset1) * noiseI16(noiseOffset2) >> (4 + 12);

            // sine as envelope
            int s = cos8u10[(cosOffset >> 8) & 0x3ff];
            int x = n * s >> (8 + 4);

            // color lookup
            auto color = lookupSummerPaletteI(x & 0x3ff);

            strip.set(i, color * p.brightness >> 12);

            cosOffset += cosStep;
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
constexpr EffectInfo info{"Summer", parameterInfos, sizeof(Parameters), &init, &run};

} // namespace Summer
