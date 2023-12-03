#pragma once

/*
 * Cylon bounce effect
 * https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/#LEDStripEffectCylon
 */

#include "../EffectInfo.hpp"
#include "../Timer.hpp"
#include "../math.hpp"


namespace CylonBounce {

struct Parameters {
    // brightness (0 - 4095)
    int brightness;

    // hue (0 - 1535)
    int hue;

    // saturation (0 - 4095)
    int saturation;

    // size of "eye" as percentage (0 - 4095)
    int size;

    // duration for movement over whole strip
    Milliseconds<> duration;
};

// initialize parameters with default values
void init(void *parameters) {
    auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.brightness = 4095; // 100%
    p.hue = 0; p.saturation = 4095; // red
    p.size = 12 * 4095 / 100; // 12%
    p.duration = 1s;
}

// limit the parameters to valid range
/*void limit(void *parameters) {
    auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.brightness.value = std::clamp(p.brightness.value, 0, 24);
    p.hue = unsigned(p.hue + 240) % 24; // circular
    p.saturation = std::clamp(p.saturation, 0, 100);
    p.size = std::clamp(p.size, 1, 100);
    p.duration.value = std::clamp(p.duration.value, 6, 48);
    //p.returnDelay.value = std::clamp(p.returnDelay.value, 6, 48);
}*/

Coroutine run(Loop &loop, Strip &strip, const void *parameters) {
    auto &p = *reinterpret_cast<const Parameters *>(parameters);

 	int count = strip.size();
    auto start = loop.now();
    while (true) {
        auto now = loop.now();
        auto t = now - start;
        if (t > p.duration) {
            start = now;
            t = 0s;
        }
        float time = float(t) / float(p.duration);

        float brightness = p.brightness / 4096.0f;
        float hue = p.hue / 1536.0f;
        float saturation = p.saturation / 4096.0f;
        float size = p.size / 4095.0f;

        for (int ledIndex = 0; ledIndex < count; ++ledIndex) {
            float position = float(ledIndex);
            //float coordinate = position / float(count - 1);
            float3 color = {0, 0, 0};

            float w = count * size;
            float eyeStart = min(time, 1.0f - time) * 2.0f * (count - w);
            float eyeEnd = eyeStart + w;

            float b = 0;
            if (position < eyeStart - 1.0f) {
            } else if (position < eyeStart) {
                // interpolate start
                b = 1.0 - (eyeStart - position);
            } else if (position < eyeEnd) {
                // "eye"
                b = 1.0f;
            } else if (position < eyeEnd + 1.0f) {
                // interpolate end
                b = 1.0f - (position - eyeEnd);
            }
            color = hsv2rgb(float3(hue, saturation, brightness * b));

            strip.set(ledIndex, color);
        }
        co_await strip.show();
    }

/*
    while (true) {
        {
            Timer timer(loop);
            while (true) {
                // 100% is half strip, size has 8 bit fractional part
                int size = strip.size() * p.size >> 5;//.get() * 256 / 2000;

                int maxValue = count * 256 - size;
                int x = timer.get(loop, p.duration, maxValue * 2);
                if (unsigned(x) >= unsigned(maxValue * 2))
                    break;
                if (x >= maxValue)
                    x = (maxValue * 2 - 1) - x;

                int y = x + size;

                // start/end
                int s = x >> 8;
                int e = y >> 8;

                // fractional parts
                int sf = x & 255;
                int ef = y & 255;

                // clear left part
                strip.clear(0, s);

                // color
                //HSV hsv{p.hue << 6, p.saturation * 4096 / 100, p.brightness.get() * 4095 / 1000};
                HSV hsv{p.hue, p.saturation, p.brightness};
                Color<int> color = toColor(hsv);

                // "eye"
                strip.set(s, color * (255 - sf) >> 8);
                strip.set(s + 1, e, color);
                strip.set(e, color * ef >> 8);

                // clear right part
                strip.clear(e + 1, count);

                co_await strip.show();
            }
        }
    }
*/
}

const ParameterInfo parameterInfos[] = {
    {"Brightness", ParameterInfo::Type::PERCENTAGE_E12, offsetof(Parameters, brightness)},
    {"Hue", ParameterInfo::Type::HUE, offsetof(Parameters, hue)},
    {"Saturation", ParameterInfo::Type::PERCENTAGE_E12, offsetof(Parameters, saturation)},
    {"Size", ParameterInfo::Type::PERCENTAGE_E12, offsetof(Parameters, size)},
    {"Duration", ParameterInfo::Type::LONG_DURATION_E12, offsetof(Parameters, duration)},
};
constexpr EffectInfo info{"Cylon Bounce", parameterInfos, sizeof(Parameters), &init, &run};

} // namespace CylonBounce
