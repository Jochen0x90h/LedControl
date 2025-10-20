#pragma once

#include "Strip.hpp"
#include <coco/Loop.hpp>
#include <coco/Coroutine.hpp>
#include <coco/String.hpp>


using namespace coco;


struct ParameterInfo {
    enum class Type {
        // count (1 - 255), value is maximum
        COUNT,

        // duration in E12 series steps (0 - 72 -> 10ms, 12ms, 15ms ... 1000000s), value is minimum (e.g. 12 for 100ms)
        DURATION_E12,

        // percentage (0 - 100 -> 0% - 100%), value is step (e.g. 5)
        PERCENTAGE,

        // percentage in E12 series steps (0 - 24 -> 1.0%, 1.2%, 1.5% ... 82%, 100%), no value
        PERCENTAGE_E12,

        // color hue (0 - 23), no value
        HUE,

        // saturation in percent (0 - 100 -> 0% - 100%)
        // or color temperature in E12 series steps (24 - 43 -> 1000K, 1200K, 1500K ... 33000K, 39000K)
        // depending on the previous hue value
        //SATURATION_OR_COLOR_TEMPERATURE_E12,

        // color has two values and can be set as hue/saturation or color temperature
        // [0] hue (0 - 23) or temperature (24)
        // [1] saturation (0 - 100 -> 0% - 100%) or temperature in E12 series steps (24 - 43 -> 1000K, 1200K, 1500K ... 33000K, 39000K)
        COLOR
    };

    String name;
    Type type;
    int value;
    /*uint8_t min;
    uint8_t max;
    uint8_t step;
    bool wrap;*/
};

inline int getValueCount(ParameterInfo::Type type) {
    return type == ParameterInfo::Type::COLOR ? 2 : 1;
}

inline int clampCount(int value, int max) {
    return std::clamp(value, 1, max);
}

inline int clampDurationE12(int value, int min) {
    return std::clamp(value, min, 72);
}

inline int clampPercentage(int value) {
    return std::clamp(value, 1, 100);
}

inline int clampPercentageE12(int value) {
    return std::clamp(value, 0, 24);
}

inline int wrapHue(int value) {
    return (value + 24 * 256) % 24;
}

inline int wrapColorHue(int value) {
    return (value + 25 * 256) % 25;
}

inline int clampColorTemperature(int value) {
    return std::clamp(value, 24, 43);
}


//using Hue = int;

/// @brief Initialize function
///
using InitFunction = void (*)(void *parameters);

/// @brief Function that determines if the end time was reached and the effect should be restarted
///
using EndFunction = bool (*)(float time, const void *parameters);

/// @brief Function that caluclates the effect at the given time
using CalcFunction = void (*)(StripData strip, float brightness, float time, const void *parameters);

struct EffectInfo {
    String name;
    Array<const ParameterInfo> parameterInfos;
    InitFunction init;
    EndFunction end;
    CalcFunction run;

    /// @brief Get number of parameter values that precede the parameter at the given index
    /// @param index Parameter index
    /// @return Number of preceding parameter values
    int getPrecedingValueCount(int index) const {
        int count = 0;
        for (int i = 0; i < index; ++i) {
            auto &info = this->parameterInfos[i];
            count += ::getValueCount(info.type);
        }
        return count;
    }

    /// @brief Get number of values, a parameter can have more than one value (e.g. COLOR)
    /// @return number of values
    int getValueCount() const {
        return this->getPrecedingValueCount(this->parameterInfos.size());
    }
};
