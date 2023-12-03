#pragma once

/*
 * Autumn effect
 */

#include "../EffectInfo.hpp"
#include "lookupAutumnPalette.hpp"
//#include "lookupAutumnPaletteI.hpp"
#include "../math.hpp"


namespace Autumn {

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

        uint32_t noiseOffset2 = -noiseOffset1 >> 2;
        uint32_t noiseStep2 = step;

        uint32_t noiseOffset3 = x >> 2;
        uint32_t noiseStep3 = step >> 5;

        uint32_t noiseOffset4 = -noiseOffset3;
        uint32_t noiseStep4 = step >> 5;

        for (int i = 0; i < count; ++i) {
            // "tree trunks"
		    int t = std::max((noiseI16(noiseOffset3) * noiseI16(noiseOffset4) >> (4 + 16)) - 100, 0);

            // "leaves"
            int l = (noiseI16(noiseOffset1) * noiseI16(noiseOffset2) >> (5 + 16)) + 80*4;

            auto color = lookupAutumnPaletteI(std::max(l - t, 0) & 0x3ff);
            strip.set(i, color * p.brightness >> 12);

            noiseOffset1 += noiseStep1;
            noiseOffset2 += noiseStep2;
            noiseOffset3 += noiseStep3;
            noiseOffset4 += noiseStep4;
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
constexpr EffectInfo info{"Autumn", parameterInfos, sizeof(Parameters), &init, &run};

} // namespace Autumn
