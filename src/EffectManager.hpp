#pragma once

#include "EffectInfo.hpp"
#include "Strip.hpp"
#include <coco/Storage.hpp>
#include <coco/Buffer.hpp>
#include <coco/Loop.hpp>
#include <coco/Coroutine.hpp>


using namespace coco;

// preset (effect and parameters), gets stored in flash
/*struct Preset {
    uint8_t effectIndex;

    // length of name
    uint8_t nameLength;

    // name and parameters
    uint8_t data[30];

    bool dirty;
};*/

class EffectManager {
public:
    // storage ID for global parameters
    static constexpr int GLOBAL_ID = 0;

    /*static constexpr int DIRECTORY_ID = 1;
    static constexpr int FIRST_PRESET_ID = 2;
    static constexpr int LAST_PRESET_ID = 99;
*/

    // player configurations, each player needs 2 storage IDs
    static constexpr int FIRST_PLAYER_ID = 1;
    static constexpr int PLAYER_COUNT = 2;
    static constexpr int MAX_PLAYER_PRESET_COUNT = 32;
    static constexpr int MAX_PLAYER_DATA_SIZE = 300;

    // storage IDs for three LED strip configurations
    static constexpr int STRIP1_ID = 100;
    static constexpr int STRIP2_ID = 101;
    static constexpr int STRIP3_ID = 102;
    static constexpr int MAX_STRIP_SOURCE_COUNT = 10;

    static constexpr Milliseconds<> SAVE_TIMEOUT = 2s;

    // preset parameter info and value
    struct ParameterValue {
        const ParameterInfo &info;
        //int value;
        const uint8_t *values;
    };

    struct Preset {
        uint8_t effectIndex;
        uint8_t nameLength;
        uint8_t parameterCount;
    };

    // effect player configuration
    struct PlayerConfig {
        // LED count
        uint16_t ledCount;

        // list of presets
        Preset presets[MAX_PLAYER_PRESET_COUNT];
        uint8_t presetCount;

        // data for names and parameters
        uint8_t data[MAX_PLAYER_DATA_SIZE];

        int getDataOffset(int presetIndex) {
            int offset = 0;
            for (int i = 0; i < presetIndex; ++i) {
                auto &preset = this->presets[i];
                offset += preset.nameLength + preset.parameterCount;
            }
            return offset;
        }

        uint8_t *getParametersData(int presetIndex) {
            return this->data + this->getDataOffset(presetIndex) + this->presets[presetIndex].nameLength;
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
/*
    // source for the LED strip configuration
    struct StripSource {
        // index of effect player
        uint8_t playerIndex;

        // index of copy operation
        CopyOp copyOp;

        // start LED index and count
        uint16_t start;
        uint16_t count;
    };

    // LED strip configuration
    struct StripConfig {
        // type of LEDs
        uint8_t ledType;

        // number of LEDs
        uint16_t ledCount;

        // list of sources
        StripSource sources[MAX_STRIP_SOURCE_COUNT];
        uint8_t sourceCount;
    };*/

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
    bool modified() {return this->playerConfigsModified != 0;}


    /// @brief Set global preset index (for all players).
    /// @return Preset index
    int getPresetIndex() {return this->global.presetIndex;}

    /// @brief Get global preset index (for all players).
    /// @param index Preset index
    void setPresetIndex(int index);


    /// @brief Get LED offsets of all players
    /// @param offsets
    /*void getPlayerInfos(PlayerInfo *infos) {
        int start = 0;
        for (int i = 0; i < PLAYER_COUNT; ++i) {
            int count = this->players[i].config.ledCount;
            infos[i].ledStart = start;
            infos[i].ledCount = count;
            start += count;
        }
    }*/

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

    /// @brief Get the name of the given preset. The effect name is the default name
    ///
    String getPresetName(int playerIndex, int presetIndex) {
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


/*
    StripConfig &getStripConfig(int index) {
        this->stripConfigDirty |= 1 << index;
        return this->stripConfigs[index];
    }
*/

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

    // global preset parameters which get stored in flash automatically
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

    // effect players
    int playerCount = 0;
    Player players[PLAYER_COUNT];
    int playerConfigsModified = 0;

 /*
    // effect parameters (get converted from global and current preset)
    float effectParameters[32];

    Coroutine effect;
*/

    // configurations of the tree LED strips
    //StripConfig stripConfigs[3];
    //int stripConfigDirty = 0;


    // save timeout
    Loop::Time timeout;

    // timer waits on barrier until it gets started
    Barrier<> timerBarrier;

    // must be last
    Coroutine timerCoroutine;
};
