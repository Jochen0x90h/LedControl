#pragma once

#include "EffectManager.hpp"


using namespace coco;


class StripManager {
public:
    // storage IDs for three LED strip configurations
    static constexpr int FIRST_STRIP_ID = 100;

    // number of LED strips
    static constexpr int STRIP_COUNT = 3;

    // maximum number of sources per strip (i.e. how many effect players can be displayed on one strip)
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
        //uint16_t ledCount;

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

    int getLedCount(int stripIndex);
    int getSourceCount(int stripIndex) {return this->strips[stripIndex].config.sourceCount;}
    void addSource(int stripIndex);
    void removeSource(int stripIndex);
    LedType updateLedType(int stripIndex, int delta);
    int updatePlayerIndex(int stripIndex, int sourceIndex, int delta);
    int updateLedStart(int stripIndex, int sourceIndex, int delta);
    int updateLedCount(int stripIndex, int sourceIndex, int delta);

    void setEditMode(int stripIndex, int sourceIndex) {this->editStripIndex = stripIndex; this->editSourceIndex = sourceIndex;}

    Coroutine run();

protected:

    Loop &loop;
    Storage &storage;

    StripData stripData;
    Barrier<> &syncBarrier;

    // points to EffectManager::playerInfos
    const EffectManager::PlayerInfo *playerInfos;

    // three LED strips
    Strip strips[3];
    int stripConfigsModified = 0;

    int editStripIndex = -1;
    int editSourceIndex;
};
