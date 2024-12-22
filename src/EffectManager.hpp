#pragma once

#include "EffectInfo.hpp"
#include "Strip.hpp"
#include <coco/Storage.hpp>
#include <coco/Buffer.hpp>
#include <coco/Loop.hpp>
#include <coco/Coroutine.hpp>


using namespace coco;

// preset (effect and parameters), gets stored in flash
struct Preset {
    uint8_t effectIndex;

    // length of name
    uint8_t nameLength;

    // name and parameters
    uint8_t data[30];

    bool dirty;
};

class EffectManager {
public:
    static constexpr int GLOBAL_ID = 0;
    static constexpr int DIRECTORY_ID = 1;
    static constexpr int FIRST_PRESET_ID = 2;
    static constexpr int LAST_PRESET_ID = 99;
    static constexpr int MAX_PRESET_COUNT = 32;

    static constexpr Milliseconds<> SAVE_TIMEOUT = 2s;


    EffectManager(Loop &loop, Storage &storage, Strip &strip,
        Array<const EffectInfo> effectInfos)
        : loop(loop)
        , storage(storage)
        , strip(strip)
        , effectInfos(effectInfos)
        , timerCoroutine(saveTimer())
    {}

    ~EffectManager() {
        this->timerCoroutine.destroy();
    }

    /// @brief Load all presets from flash
    ///
    [[nodiscard]] AwaitableCoroutine load();

    /// @brief Save all presets with dirty flag to flash
    ///
    [[nodiscard]] AwaitableCoroutine save();

    /// @brief Add a new preset at the end of the list. Make sure getPresetCount() is < MAX_PRESET_COUNT
    /// @return index of new preset
    int addPreset();

    /// @brief Delete a preset
    ///
    void deletePreset(int presetIndex);

    /// @brief Add a delta to an effect index of a preset
    ///
    void updateEffect(int presetIndex, int delta);

    /// @brief Set default values for the parameters
    ///
    void initParameters(int presetIndex);

    struct ParameterValue {
        const ParameterInfo &info;
        int value;
    };



    /// @brief Get number of presets in the presets list
    ///
    int getPresetCount() {return this->presetCount;}

    /// @brief Get the name of the given preset. The effect name is the default name
    ///
    String getPresetName(int presetIndex) {
        // use effect name for now
        return this->effectInfos[this->presetList[presetIndex].effectIndex].name;
    }

    int getPresetIndex() {return this->global.presetIndex;}
    void setPresetIndex(int index);

    /// @brief Number of effect parameters in the given preset
    ///
    int getParameterCount(int presetIndex) {
        return 2 + this->effectInfos[this->presetList[presetIndex].effectIndex].parameterInfos.size();
    }

    /// @brief Get the name of the given parameter of the given preset
    ///
    String getParameterName(int presetIndex, int parameterIndex);

    /// @brief Add a delta to a parameter of a preset, clamp to valid range and return its current value
    ///
    ParameterValue updateParameter(int presetIndex, int parameterIndex, int delta);


    /// @brief Run the given preset
    ///
    [[nodiscard]] AwaitableCoroutine run(int presetIndex);

    /// @brief Stop the current preset
    ///
    [[nodiscard]] AwaitableCoroutine stop();

protected:
    Coroutine run(EndFunction end, RunFunction function);

    int findFreeId();

    /**
     * Timer that saves parameter changes to flash after some time
     */
    Coroutine saveTimer();


    Loop &loop;
    Storage &storage;
    Strip &strip;

    // list of all effects
    Array<const EffectInfo> effectInfos;

    // preset directory in flash
    uint8_t directory[MAX_PRESET_COUNT];
    bool directoryDirty = false;

    // list of all presets
    int presetCount = 0;
    Preset presetList[MAX_PRESET_COUNT];

    // global parameters which get stored in flash automatically
    struct Global {
        uint8_t presetIndex;
        uint8_t brightness;
        uint8_t duration;
    };
    Global global;

    // effect parameters (get converted from global and current preset)
    float brightness;
    Milliseconds<> duration;
    float effectParameters[32];

    Coroutine effect;

    // save timeout
    Loop::Time timeout;

    // timer waits on barrier until it gets started
    Barrier<> timerBarrier;

    // must be last
    Coroutine timerCoroutine;
};
