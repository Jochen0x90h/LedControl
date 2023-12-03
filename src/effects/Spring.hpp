#pragma once

/*
 * Spring effect
 */

#include "../EffectInfo.hpp"
#include "lookupSpringPalette.hpp"
//#include "lookupSpringPaletteI.hpp"
#include "../math.hpp"


namespace Spring {

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

            float noiseOffset = -time + position * step;
            float cosOffset = time + position * step * 0.25f;

            float s1 = noise(noiseOffset) + 1.0f;
            float s2 = cos(cosOffset * 6.2831853f) + 1.0f;
            float s = s1 * s2 * 0.25f;

            // palette lookup
            color = lookupSpringPalette(fract(s));

            strip.set(ledIndex, color * brightness);
        }
        co_await strip.show();
    }

/*
    Timer timer(loop);
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
            auto color = lookupSpringPaletteI((s >> 6) & 0x3ff);

            //sendRGB(scale8RGB(color, brightness));
            strip.set(i, color * p.brightness >> 12);

            cosOffset += cosStep;
            noiseOffset += noiseStep;
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
constexpr EffectInfo info{"Spring", parameterInfos, sizeof(Parameters), &init, &run};

} // namespace Spring
