#pragma once

#include "../EffectInfo.hpp"
#include "../E12.hpp"
#include "../Timer.hpp"
#include <coco/Color.hpp>
#include <coco/Coroutine.hpp>


/*
 * Cylon Bounce
 * https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/#LEDStripEffectCylon
 */

namespace CylonBounce {

struct Parameters {
    // color of "eye"
    Color color;

    // size of "eye"
    int size;

    // duration for movement over whole strip
    E12Milliseconds duration;

    // delay at one end
    E12Milliseconds returnDelay;
};

void init(void *parameters) {
    auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.color = Color(255, 0, 0);
    p.size = 10;
    p.duration.value = 24; // 1s
}

void limit(void *parameters) {
    auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.size = std::clamp(p.size, 1, 100);
    p.duration.value = std::clamp(p.duration.value, 6, 48);
    p.returnDelay.value = std::clamp(p.returnDelay.value, 6, 48);
}

Coroutine run(Loop &loop, Strip &strip, void *parameters) {
    auto &p = *reinterpret_cast<Parameters *>(parameters);

    int count = strip.size();
    while (true) {
        {
            Timer timer(loop);
            while (true) {
                int maxValue = (count - p.size) * 256;
                int x = timer.get(loop, p.duration, maxValue * 2);
                if (unsigned(x) >= unsigned(maxValue * 2))
                    break;
                if (x >= maxValue)
                    x = (maxValue * 2 - 1) - x;

                // start/end
                int s = x >> 8;
                int e = (x >> 8) + p.size;

                // fractional part
                int f = x & 255;

                // clear left part
                for (int i = 0; i < s; ++i) {
                    strip.set(i, 0, 0, 0);
                }

                // "eye"
                strip.set(s, p.color * Fixed<8>{255 - f});
                for (int i = s + 1; i < e; i++) {
                    strip.set(i, p.color);
                }
                strip.set(e, p.color * Fixed<8>{f});

            // clear right part
            for (int i = e + 1; i < count; ++i) {
                strip.set(i, 0, 0, 0);
            }

                co_await strip.show();
            }
        }
    }
}

const ParameterInfo parameterInfos[] = {
    {"Color", ParameterInfo::Type::COLOR, offsetof(Parameters, color)},
    {"Size", ParameterInfo::Type::INTEGER, offsetof(Parameters, size)},
    {"Duration", ParameterInfo::Type::E12_MILLISECONDS, offsetof(Parameters, duration)},
};
constexpr EffectInfo info{"Cylon Bounce", std::size(parameterInfos), parameterInfos, sizeof(Parameters), &init, &limit, &run};

}
