#pragma once

#include "EffectInfo.hpp"
#include "Strip.hpp"
#include <coco/Storage.hpp>
#include <coco/Buffer.hpp>
#include <coco/Coroutine.hpp>
#include <coco/Loop.hpp>
#include <coco/StringBuffer.hpp>

#define getOffset(type, member) intptr_t(&((type *)nullptr)->member)


using namespace coco;


class EffectManager {
public:
    // storage ID for global parameters
    static constexpr int GLOBAL_ID = 0;

    // id of first preset name
    static constexpr int FIRST_PRESET_NAME_ID = 10;

    // id of first player configuration, each player needs 2 storage IDs (presets and data)
    static constexpr int FIRST_PLAYER_ID = 60;

    // maximum preset name size
    static constexpr int MAX_PRESET_NAME_SIZE = 24;

    // number of presets
    static constexpr int PRESET_COUNT = 32;

    // number of players
    static constexpr int PLAYER_COUNT = 10;

    // maximum player data size
    static constexpr int MAX_PLAYER_DATA_SIZE = PRESET_COUNT * 8;


    // timeout for saving brightness, speed and preset index to flash
    static constexpr Milliseconds<> SAVE_TIMEOUT = 2s;

    // preset parameter info and value
    struct ParameterValue {
        const ParameterInfo &info;
        //int value;
        const uint8_t *values;
    };

    struct Preset {
        uint8_t effectIndex;
        uint8_t parameterCount;
    };

    // effect player configuration
    struct PlayerConfig {
        // LED count
        uint16_t ledCount;

        // list of presets
        Preset presets[PRESET_COUNT];
        uint8_t presetCount;

        // data for preset parameters
        uint8_t data[MAX_PLAYER_DATA_SIZE];

        int getDataOffset(int presetIndex) {
            int offset = 0;
            for (int i = 0; i < presetIndex; ++i) {
                auto &preset = this->presets[i];
                offset += preset.parameterCount;
            }
            return offset;
        }

        uint8_t *getParametersData(int presetIndex) {
            return this->data + this->getDataOffset(presetIndex);
        }
    };

    struct Player {
        PlayerConfig config;

        // effect parameters (get converted from current preset)
        //float brightness;
        //Milliseconds<> duration;
        float effectParameters[32];

        Coroutine effect;
    };

    struct PlayerInfo {
        uint16_t ledStart;
        uint16_t ledCount;
    };


    /// @brief Constructor
    /// @param loop Event loop
    /// @param storage Flash storage
    /// @param effectInfos List of effects
    /// @param stripData LED strip data
    /// @param syncBarrier Barrier for frame synchronization
    EffectManager(Loop &loop, Storage &storage, Array<const EffectInfo> effectInfos,
        StripData stripData) //Buffer &stripBuffer, //Strip &strip,
        : loop(loop)
        , storage(storage)
        //, stripBuffer(stripBuffer)
        //, strip(strip)
        , effectInfos(effectInfos)
        , stripData(stripData)
        , timerCoroutine(saveTimer()) // start coroutine
    {}

    ~EffectManager() {
        this->timerCoroutine.destroy();
    }

    /// @brief Load configuration from flash
    ///
    [[nodiscard]] AwaitableCoroutine load();

    /// @brief Save modified configuration to flash
    ///
    [[nodiscard]] AwaitableCoroutine save();

    /// @brief Check if the configuration is modified and needs to be saved
    /// @return true if modified
    bool modified() {return (this->presetNamesModified | this->playerConfigsModified) != 0;}


    /// @brief Set global preset index (for all players).
    /// @return Preset index
    int getPresetIndex() {return this->global.presetIndex;}

    /// @brief Get global preset index (for all players).
    /// @param index Preset index
    void setPresetIndex(int index);


    int updateLedCount(int playerIndex, int delta);

    /// @brief Add a new preset at the end of the list. Make sure getPresetCount() is < MAX_PRESET_COUNT
    /// @return index of new preset
    int addPreset(int playerIndex);

    /// @brief Delete a preset
    ///
    void deletePreset(int playerIndex, int presetIndex);

    /// @brief Add a delta to the effect index of a preset
    ///
    void updateEffect(int playerIndex, int presetIndex, int delta);

    /// @brief Set default values for the parameters
    /// @param playerIndex Index of the player
    /// @param presetIndex Index of the preset
    /// @return Number of parameters
    int initParameters(int playerIndex, int presetIndex);

    int getPresetCount();

    /// @brief Get number of presets of a player
    ///
    int getPresetCount(int playerIndex) {return this->players[playerIndex].config.presetCount;}

    /// @brief Get preset name.
    /// @param presetIndex Preset index
    /// @return Reference to StringBuffer containing the preset name
    StringBuffer<MAX_PRESET_NAME_SIZE> &getPresetName(int presetIndex) {
        return this->presetNames[presetIndex];
    }

    /// @brief Set preset name modified.
    /// @param presetIndex Preset index
    void setPresetNameModified(int presetIndex) {
        this->presetNamesModified |= (1 << presetIndex);
    }

    /// @brief Get the name of an effect. The effect name is the default name
    /// @param playerIndex Player index
    /// @param presetIndex Preset index
    String getEffectName(int playerIndex, int presetIndex) {
        // use effect name for now
        auto &config = this->players[playerIndex].config;
        return this->effectInfos[config.presets[presetIndex].effectIndex].name;
    }

    /// @brief Get the number of effect parameters in the given preset including the 2 global parameters
    ///
    int getParameterCount(int playerIndex, int presetIndex) {
        auto &config = this->players[playerIndex].config;
        return this->effectInfos[config.presets[presetIndex].effectIndex].parameterInfos.size();
    }

    /// @brief Get the name of a global parameter
    /// @param parameterIndex Global parameter index (0: Brightness, 1: Speed)
    /// @return Name of global parameter
    String getGlobalParameterName(int parameterIndex);

    /// @brief Get the name of the given parameter of the given preset
    ///
    //String getParameterName(int playerIndex, int presetIndex, int parameterIndex);
    const ParameterInfo &getParameterInfo(int playerIndex, int presetIndex, int parameterIndex);

    /// @brief Add a delta to a global parameter, clamp to valid range and return its current value
    /// @return Preset parameter info and value
    ParameterValue updateGlobalParameter(int parameterIndex, int delta);

    /// @brief Add a delta to a parameter of a preset, clamp to valid range and return its current value
    /// @return Preset parameter info and value
    ParameterValue updateParameter(int playerIndex, int presetIndex, int parameterIndex, int componentIndex, int delta);


    /// @brief Apply the parameters of a preset so that they are used by the effect.
    /// @param playerIndex Index of the player
    /// @param presetIndex Index of the preset
    void applyParameters(int playerIndex, int presetIndex);

    /// @brief Run the preset with given index on all players.
    /// @param presetIndex Index of the preset (for all players)
    void run(int presetIndex);

    /// @brief Run the preset with given index on one player.
    /// @param playerIndex Index of the player
    /// @param presetIndex Index of the preset
    void run(int playerIndex, int presetIndex);

    /// @brief Stop the current preset
    ///
    void stop();


    Barrier<> syncBarrier;
    PlayerInfo playerInfos[PLAYER_COUNT];

protected:
    Coroutine run(int ledStart, Player &player, EndFunction end, CalcFunction function);

    void updatePlayerInfos();

    //int findFreeId();

    /// @brief Timer that saves parameter changes to flash after some time
    ///
    Coroutine saveTimer();


    Loop &loop;
    Storage &storage;

    // list of all effects
    Array<const EffectInfo> effectInfos;

    //Strip &strip;
    StripData stripData;
    //Buffer &stripBuffer;

 /*
    // preset directory in flash
    uint8_t directory[MAX_PRESET_COUNT];
    bool directoryDirty = false;

    // list of all presets
    int presetCount = 0;
    Preset presetList[MAX_PRESET_COUNT];
*/


    // global preset parameters which get stored to flash automatically
    struct Global {
        // global brightness
        uint8_t brightness;

        // duration as E12 value
        uint8_t duration;

        // index of preset in all players
        uint8_t presetIndex;
    };
    Global global;

    // global effect parameters
    float brightness;
    Milliseconds<> duration;

    // preset names
    StringBuffer<MAX_PRESET_NAME_SIZE> presetNames[PRESET_COUNT];
    //char presetNames[PRESET_COUNT][MAX_PRESET_NAME_SIZE];
    int presetNamesModified = 0;

    // effect players
    int playerCount = 0;
    Player players[PLAYER_COUNT];
    int playerConfigsModified = 0;

     // save timeout
    Loop::Time timeout;

    // timer waits on barrier until it gets started
    Barrier<> timerBarrier;

    // must be last
    Coroutine timerCoroutine;
};
