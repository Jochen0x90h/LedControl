#pragma once

#include "../EffectInfo.hpp"
#include "../Timer.hpp"
#include <coco/noise.hpp>


/*
 * Meteor rain effect
 * https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/#LEDStripEffectMeteorRain
 */

namespace MeteorRain {

struct Parameters {
    // brightness (0 - 4095)
    int brightness;

    // hue (0 - 1535)
    int hue;

    // saturation (0 - 4095)
    int saturation;

    // size of meteor in percent (0 - 4095)
    int size;

    // duration for movement over whole strip
    Milliseconds<> duration;

    // random decay percent (0 - 4095)
    int randomDecay;
};

// initialize parameters with default values
void init(void *parameters) {
    auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.brightness = 4095; // 100%
    p.hue = 0; p.saturation = 4095; // red
    p.size = 10 * 4095 / 100; // 10%
    p.duration = 5s;
    p.randomDecay = 4095; // 100%
}

// limit the parameters to valid range
/*void limit(void *parameters) {
    auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.brightness.value = std::clamp(p.brightness.value, 0, 24);
    p.hue = unsigned(p.hue + 240) % 24; // circular
    p.saturation = std::clamp(p.saturation, 0, 100);
    p.size = std::clamp(p.size, 1, 100);
    //p.trailDecay = std::clamp(p.size, 1, 100);
    p.randomDecay = std::clamp(p.randomDecay, 0, 8);
}*/

Coroutine run(Loop &loop, Strip &strip, const void *parameters) {
    auto &p = *reinterpret_cast<const Parameters *>(parameters);

    int count = strip.size();
    int o = 0;
    while (true) {
        Timer timer(loop);
        while (true) {
            // get size, half the LEDs are 100% size
            int size = p.size * count >> 13;

            int maxValue = (count + size + 350) * 256;
            int x = timer.get(loop, p.duration, maxValue);
            if (unsigned(x) >= unsigned(maxValue))
                break;

            // start/end
            int s = (x >> 8) - size;
            int e = (x >> 8);

            // fractional part
            int f = x & 255;

            // train (left part)
            {
                int randomDecay = p.randomDecay * 100 >> 12; // convert to 0 - 100
                int ms = std::min(s, count);
                for (int i = 0; i < ms; ++i) {
                    // "age" of the current "particle"
                    int age = s - i;

                    // calc noise using randomness and age
                    int n = std::clamp((randomDecay * noiseI8(o + (i << 6)) / 100) + 255 - age, 0, 256);

                    // calc and set color
                    HSV hsv{p.hue, p.saturation, p.brightness * n >> 8};
                    Color<int> color = toColor(hsv);
                    strip.set(i, color);
                }
            }

            // meteor
            {
                // calc and set color
                HSV hsv{p.hue, p.saturation, p.brightness};
                Color<int> color = toColor(hsv);
                strip.setSafe(s + 1, e, color);

                // fade in first LED
                strip.setSafe(e, color * f >> 8);
            }

            // clear right part
            strip.clear(e + 1, count);

            co_await strip.show();
        }
        o += 3000;
    }
}

const ParameterInfo parameterInfos[] = {
    {"Brightness", ParameterInfo::Type::PERCENTAGE_E12, offsetof(Parameters, brightness)},
    {"Hue", ParameterInfo::Type::HUE, offsetof(Parameters, hue)},
    {"Saturation", ParameterInfo::Type::PERCENTAGE_5, offsetof(Parameters, saturation)},
    {"Size", ParameterInfo::Type::PERCENTAGE_5, offsetof(Parameters, size)},
    {"Duration", ParameterInfo::Type::LONG_DURATION_E12, offsetof(Parameters, duration)},
    //{"Trail Decay", ParameterInfo::Type::INTEGER, offsetof(Parameters, trailDecay)},
    {"Random Decay", ParameterInfo::Type::PERCENTAGE_5, offsetof(Parameters, randomDecay)},
};
constexpr EffectInfo info{"Meteor Rain", parameterInfos, sizeof(Parameters), &init, &run};

} // namespace MeteorRain
