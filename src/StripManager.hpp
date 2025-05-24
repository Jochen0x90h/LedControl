#pragma once

#include "EffectManager.hpp"


using namespace coco;



class StripManager {
public:
    // storage IDs for three LED strip configurations
    static constexpr int FIRST_STRIP_ID = 100;
    static constexpr int STRIP_COUNT = 3;
    static constexpr int MAX_STRIP_SOURCE_COUNT = 10;


    enum class LedType : uint8_t {
        // standard RGB
        RGB,

        // WS2812
        GRB,
    };

    // source for the LED strip configuration
    struct StripSource {
        // index of effect player
        uint8_t playerIndex;

        // index of copy operation
        //CopyOp copyOp;

        // start LED index (relative to player) and count
        uint16_t ledStart;
        uint16_t ledCount;
    };

    // LED strip configuration
    struct StripConfig {
        // type of LEDs
        LedType ledType;

        // number of LEDs
        uint16_t ledCount;

        // list of sources
        StripSource sources[MAX_STRIP_SOURCE_COUNT];
        uint8_t sourceCount;
    };

    struct Strip {
        Buffer &buffer;

        StripConfig config;


        Strip(Buffer &buffer) : buffer(buffer) {}
    };


    StripManager(Loop &loop, Storage &storage, StripData stripData,
        Barrier<> &syncBarrier, const EffectManager::PlayerInfo *playerInfos,
        Buffer &stripBuffer1, Buffer &stripBuffer2, Buffer &stripBuffer3)
        : loop(loop)
        , storage(storage)
        , stripData(stripData)
        , syncBarrier(syncBarrier)
        , playerInfos(playerInfos)
        , strips{{stripBuffer1}, {stripBuffer2}, {stripBuffer3}}
    {
        // start coroutine
        //run();
    }

    ~StripManager() {
    }

    /// @brief Load configuration from flash
    ///
    [[nodiscard]] AwaitableCoroutine load();

    /// @brief Save modified configuration to flash
    ///
    [[nodiscard]] AwaitableCoroutine save();

    /// @brief Check if the configuration is modified and needs to be saved
    /// @return true if modified
    bool modified() {return this->stripConfigsModified != 0;}


    const StripConfig &getStripConfig(int index) {
        //this->stripConfigDirty |= 1 << index;
        return this->strips[index].config;
    }

    void setStripConfig(int index, StripConfig &newConfig);

    Coroutine run();

protected:

    Loop &loop;
    Storage &storage;

    StripData stripData;
    Barrier<> &syncBarrier;
    const EffectManager::PlayerInfo *playerInfos;

    // three LED strips
    Strip strips[3];
    int stripConfigsModified = 0;
};
