#pragma once

/*
 * Strobe effect
 * https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/#LEDStripEffectStrobe
 */

#include "../EffectInfo.hpp"
#include "../math.hpp"


namespace Strobe {

struct Parameters {
    // brightness (0 - 4095)
    int brightness;

    // hue (0 - 1535)
    int hue;

    // saturation (0 - 4095)
    int saturation;

    // number of flashes
    int strobeCount;

    // percentage of strobe time, rest is pause (0 - 4095)
    int strobePercentage;

    // duration for movement over whole strip
    Milliseconds<> duration;



    // duration of one flash
    //Milliseconds<> flashDelay;

    // pause after number of flashes
    //Milliseconds<> endPause;
};

// initialize parameters with default values
void init(void *parameters) {
    auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.brightness = 4095; // 100%
    p.hue = 0; p.saturation = 4095; // red
    p.strobeCount = 10;
    p.strobePercentage = 4095;
    p.duration = 5s;


    //p.flashDelay = 50ms;
    //p.endPause = 1s;
}

// limit the parameters to valid range
/*void limit(void *parameters) {
    auto &p = *reinterpret_cast<Parameters *>(parameters);

    p.brightness.value = std::clamp(p.brightness.value, 0, 24);
    p.hue = unsigned(p.hue + 240) % 24; // circular
    p.saturation = std::clamp(p.saturation, 0, 100);
    p.strobeCount = std::clamp(p.strobeCount, 1, 50);
    p.flashDelay.value = std::clamp(p.flashDelay.value, 6, 48);
    p.endPause.value = std::clamp(p.endPause.value, 6, 48);
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
        float strobeCount = p.strobeCount;
        float strobePercentage = p.strobePercentage / 4095.0f;

        for (int ledIndex = 0; ledIndex < count; ++ledIndex) {
            float position = float(ledIndex);
            //float coordinate = position / float(count - 1);
            float3 color = {0, 0, 0};

            if (time < strobePercentage) {
                float t = time / strobePercentage * strobeCount;
                if (fract(t) < 0.5f)
                     color = hsv2rgb(float3(hue, saturation, brightness));
            }

            strip.set(ledIndex, color);
        }
        co_await strip.show();
    }

/*
    while (true) {
        for (int j = 0; j < p.strobeCount; j++) {
            // calc and set color
            HSV hsv{p.hue, p.saturation, p.brightness};
            strip.fill(toColor(hsv));
            co_await strip.show();
            co_await sleep(loop, p.flashDelay);

            // set black
            strip.clear();
            co_await strip.show();
            co_await sleep(loop, p.flashDelay);
        }
        co_await sleep(loop, p.endPause);
    }
*/
}

const ParameterInfo parameterInfos[] = {
    {"Brightness", ParameterInfo::Type::PERCENTAGE_E12, offsetof(Parameters, brightness)},
    {"Hue", ParameterInfo::Type::HUE, offsetof(Parameters, hue)},
    {"Saturation", ParameterInfo::Type::PERCENTAGE_5, offsetof(Parameters, saturation)},
    {"Strobe Count", ParameterInfo::Type::COUNT_20, offsetof(Parameters, strobeCount)},
    {"Percentage", ParameterInfo::Type::PERCENTAGE_5, offsetof(Parameters, strobePercentage)},
    {"Duration", ParameterInfo::Type::LONG_DURATION_E12, offsetof(Parameters, duration)},


    //{"Flash Delay", ParameterInfo::Type::SHORT_DURATION_E12, offsetof(Parameters, flashDelay)},
    //{"End Pause", ParameterInfo::Type::LONG_DURATION_E12, offsetof(Parameters, endPause)},
};
constexpr EffectInfo info{"Strobe", parameterInfos, sizeof(Parameters), &init, &run};

} // namespace Strobe
