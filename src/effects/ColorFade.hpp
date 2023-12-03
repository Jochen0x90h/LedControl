#pragma once

#include "../EffectInfo.hpp"
#include "../E12.hpp"
#include <coco/Color.hpp>
#include <coco/Coroutine.hpp>
#include <algorithm>


/*
 * Color Fade
 * https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/#LEDStripEffectFadeInandFadeOutYourownColors
 */

namespace ColorFade {

struct Parameters {
    E12Milliseconds duration;
    Color color;
};

void init(void *parameters) {
    auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.duration.value = 24; // 1s
    p.color = Color(255, 0, 0);
}

void limit(void *parameters) {
    auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.duration.value = std::clamp(p.duration.value, 6, 48);
}

Coroutine run(Loop &loop, Strip &strip, void *parameters) {
    auto &p = *reinterpret_cast<Parameters *>(parameters);
    while (true) {
        Timer timer(loop);
        while (true) {
            int x = timer.get(loop, p.duration, 512);

            if (x >= 512)
                break;
            int k = x < 256 ? x : 512 - x;

            Color color = p.color * Fixed<8>{k};//.scale(k);
            strip.fill(color);
            co_await strip.show();
        }
    }
}

const ParameterInfo parameterInfos[] = {
    {"Duration", ParameterInfo::Type::E12_MILLISECONDS, offsetof(Parameters, duration)},
    {"Color", ParameterInfo::Type::COLOR, offsetof(Parameters, color)},
};
constexpr EffectInfo info{"Color Fade", std::size(parameterInfos), parameterInfos, sizeof(Parameters), &init, &limit, &run};

}
