#pragma once

#include "../EffectInfo.hpp"
#include "../E12.hpp"
#include "../Timer.hpp"
#include <coco/Color.hpp>
#include <coco/Coroutine.hpp>
#include <coco/noise.hpp>


/*
 * Meteor Rain
 * https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/#LEDStripEffectMeteorRain
 */

namespace MeteorRain {

struct Parameters {
    // color of meteor rain
    Color color;

    // size of meteor
    int size;

    // duration for movement over whole strip
    E12Milliseconds duration;

    // trail decay
    int trailDecay;

    // random decay
    int randomDecay;
};

void init(void *parameters) {
    auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.color = Color(255, 255, 255);
    p.size = 10;
    p.duration.value = std::clamp(p.duration.value, 6, 48);
    p.trailDecay = 64;
    p.randomDecay = 8;
}

void limit(void *parameters) {
    auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.size = std::clamp(p.size, 1, 100);
    p.trailDecay = std::clamp(p.size, 1, 100);
    p.randomDecay = std::clamp(p.randomDecay, 0, 8);
}

Coroutine run(Loop &loop, Strip &strip, void *parameters) {
    auto &p = *reinterpret_cast<Parameters *>(parameters);

    int count = strip.size();
    int o = 0;
    while (true) {
        Timer timer(loop);
        while (true) {
            int maxValue = (count + 350) * 256;
            int x = timer.get(loop, p.duration, maxValue);
            if (unsigned(x) >= unsigned(maxValue))
                break;

            // start/end
            int s = x >> 8;
            int e = (x >> 8) + p.size;

            // fractional part
            int f = x & 255;

            // decay left part
            int ms = std::min(s, count);
            for (int i = 0; i < ms; ++i) {
                int age = s - i;
                int n = std::clamp((p.randomDecay * noiseS8(o + (i << 6)) >> 3) + 255 - age, 0, 255);
                auto color = p.color * Fixed<8>(n);
                strip.set(i, color);
            }

            // meteor
            int me = std::min(e, count);
            for (int i = s + 1; i < me; i++) {
                strip.set(i, p.color);
            }
            if (e < count)
                strip.set(e, p.color * Fixed<8>{f});

            // clear right part
            for (int i = e + 1; i < count; ++i) {
                strip.set(i, 0, 0, 0);
            }

            co_await strip.show();
        }
        o += 3000;
    }
}

const ParameterInfo parameterInfos[] = {
    {"Color", ParameterInfo::Type::COLOR, offsetof(Parameters, color)},
    {"Size", ParameterInfo::Type::INTEGER, offsetof(Parameters, size)},
    {"Duration", ParameterInfo::Type::E12_MILLISECONDS, offsetof(Parameters, duration)},
    {"Trail Decay", ParameterInfo::Type::INTEGER, offsetof(Parameters, trailDecay)},
    {"Random Decay", ParameterInfo::Type::INTEGER, offsetof(Parameters, randomDecay)},
};
constexpr EffectInfo info{"Meteor Rain", std::size(parameterInfos), parameterInfos, sizeof(Parameters), &init, &limit, &run};

}
