#pragma once

/*
 * Tetris effect
 */

#include "../EffectInfo.hpp"
#include "../math.hpp"


namespace Tetris {

struct Parameters {
    // brightness (0 - 4095)
    int brightness;

    // duration for movement over whole strip
    Milliseconds<> duration;
};

// initialize parameters with default values
void init(void *parameters) {
    auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.brightness = 4095; // 100%
    p.duration = 5s;
}

float3 block(float time, float endTime, float size, float position, float hue) {
    float e = min(time, endTime);
    float s = e - size;
    if (position < s || position > e + 1.0f) {
        return {0, 0, 0};
    } else {
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
        return hsv2rgb({hue, 1.0f, value});
    }
}

Coroutine run(Loop &loop, Strip &strip, const void *parameters) {
    auto &p = *reinterpret_cast<const Parameters *>(parameters);

    int count = strip.size();
    auto start = loop.now();
    while (true) {
        auto now = loop.now();
        auto t = now - start;
        if (t > p.duration * 8) {
            start = now;
            t = 0s;
        }
        float time = float(t) / float(p.duration);

        float brightness = p.brightness / 4096.0f;

        for (int ledIndex = 0; ledIndex < count; ++ledIndex) {
            float position = float(ledIndex);
            //float coordinate = position / float(count - 1);
            float3 color = {0, 0, 0};

            float t = (time - 1.0f) * count;
            float size = count / 10.0f;

            for (int i = 0; i < 10; ++i) {
                float endTime = (10 - i) * 0.1f * count;
                color += block(t, endTime, size, position, noise(i * 0.2f));

                t -= endTime;
            }

            strip.set(ledIndex, color);
        }
        co_await strip.show();
    }
}

const ParameterInfo parameterInfos[] = {
    {"Brightness", ParameterInfo::Type::PERCENTAGE_E12, offsetof(Parameters, brightness)},
    {"Duration", ParameterInfo::Type::LONG_DURATION_E12, offsetof(Parameters, duration)},
};
constexpr EffectInfo info{"Tetris", parameterInfos, sizeof(Parameters), &init, &run};

} // namespace Tetris
