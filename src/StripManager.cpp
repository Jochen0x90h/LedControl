#include "StripManager.hpp"
#include <coco/BufferStorage.hpp>
//#include <coco/debug.hpp>


AwaitableCoroutine StripManager::load() {
    int result;
    for (int stripIndex = 0; stripIndex < STRIP_COUNT; ++stripIndex) {
        auto &strip = this->strips[stripIndex];
        auto &config = strip.config;
        co_await this->storage.read(FIRST_STRIP_ID + stripIndex, &config, sizeof(StripConfig), result);
        config.sourceCount = std::max(int((result - int(offsetof(StripConfig, sources))) / int(sizeof(StripSource))), 0);

        // sanitize parameters
        //config.ledCount = std::clamp(int(config.ledCount), 1, MAX_LEDSTRIP_LENGTH);
        for (int i = 0; i < config.sourceCount; ++i) {
            auto &source = config.sources[i];
            source.playerIndex = std::clamp(int(source.playerIndex), 0, EffectManager::PLAYER_COUNT - 1);
            source.ledStart = std::clamp(int(source.ledStart), 0, this->stripData.size());
            source.ledCount = std::clamp(int(source.ledCount), 1, MAX_LEDSTRIP_LENGTH);
        }

        // add default source
        if (config.sourceCount == 0) {
            config.ledType = DEFAULT_LED_TYPE;
            auto &source = config.sources[0];
            source.ledStart = 0;
            source.ledCount = MAX_LEDSTRIP_LENGTH;
            config.sourceCount = 1;
        }
    }
    this->stripConfigsModified = 0;
}

AwaitableCoroutine StripManager::save() {
    int result;
    for (int stripIndex = 0; stripIndex < STRIP_COUNT; ++stripIndex) {
        if (this->stripConfigsModified & (1 << stripIndex)) {
            auto &strip = this->strips[stripIndex];
            auto &config = strip.config;
            int size = offsetof(StripConfig, sources[config.sourceCount]);
            co_await this->storage.write(FIRST_STRIP_ID + stripIndex, &config, size, result);
        }
    }
    this->stripConfigsModified = 0;
}

int StripManager::getLedCount(int stripIndex) {
    auto &config = this->strips[stripIndex].config;
    int ledCount = 0;
    for (int i = 0; i < config.sourceCount; ++i) {
        auto &source = config.sources[i];
        ledCount += source.ledCount;
    }
    return ledCount;
}

void StripManager::addSource(int stripIndex) {
    this->stripConfigsModified |= 1 << stripIndex;
    auto &config = this->strips[stripIndex].config;
    auto &source = config.sources[config.sourceCount];
    source.playerIndex = 0;
    source.ledStart = 0;
    source.ledCount = MAX_LEDSTRIP_LENGTH - getLedCount(stripIndex); // remaining number of LEDs
    ++config.sourceCount;
}

void StripManager::removeSource(int stripIndex) {
    this->stripConfigsModified |= 1 << stripIndex;
    this->strips[stripIndex].config.sourceCount--;
}

StripManager::LedType StripManager::updateLedType(int stripIndex, int delta) {
    auto &config = this->strips[stripIndex].config;
    if (delta != 0) {
        this->stripConfigsModified |= 1 << stripIndex;

        config.ledType = LedType((int(config.ledType) + delta + 2 * 256) % 2);
    }
    return config.ledType;
}

int StripManager::updatePlayerIndex(int stripIndex, int sourceIndex, int delta) {
    auto &config = this->strips[stripIndex].config;
    auto &source = config.sources[sourceIndex];
    if (delta != 0) {
        this->stripConfigsModified |= 1 << stripIndex;

        // range is [0, PLAYER_COUNT)
        source.playerIndex = (source.playerIndex + delta + EffectManager::PLAYER_COUNT * 256) % EffectManager::PLAYER_COUNT;

    }
    return source.playerIndex;
}

int StripManager::updateLedStart(int stripIndex, int sourceIndex, int delta) {
    auto &config = this->strips[stripIndex].config;
    auto &source = config.sources[sourceIndex];
    if (delta != 0) {
        this->stripConfigsModified |= 1 << stripIndex;

        int ledCount = this->playerInfos[source.playerIndex].ledCount;

        // range is [0, player.ledCount)
        source.ledStart = (source.ledStart + delta + ledCount * 256) % ledCount;
    }
    return source.ledStart;
}

int StripManager::updateLedCount(int stripIndex, int sourceIndex, int delta) {
    auto &config = this->strips[stripIndex].config;
    auto &source = config.sources[sourceIndex];
    if (delta != 0) {
        this->stripConfigsModified |= 1 << stripIndex;

        // determine maximum led count which is constrained by the strip length
        int maxLedCount = MAX_LEDSTRIP_LENGTH - getLedCount(stripIndex) + source.ledCount;

        // range is [1, maxLedCount]
        source.ledCount = (source.ledCount - 1 + delta + maxLedCount * 256) % maxLedCount + 1;
    }
    return source.ledCount;
}

Coroutine StripManager::run() {
    while (true) {
        // calculate all effects
        this->syncBarrier.doAll();

        for (int stripIndex = 0; stripIndex < std::size(this->strips); ++stripIndex) {
            auto &strip = this->strips[stripIndex];
            auto &config = strip.config;

            // wait until strip buffer is ready
            co_await strip.buffer.untilReady();

            // copy strip data into strip buffer
            uint8_t *dst = strip.buffer.data();
            for (int sourceIndex = 0; sourceIndex < config.sourceCount; ++sourceIndex) {
                auto &source = config.sources[sourceIndex];
                if (this->editStripIndex == -1) {
                    // normal mode
                    const auto &playerInfo = this->playerInfos[source.playerIndex];
                    uint32_t *playerData = this->stripData.begin() + playerInfo.ledStart;
                    uint32_t *src = playerData + std::min(source.ledStart, playerInfo.ledCount);
                    uint32_t *end = playerData + std::min(source.ledStart + source.ledCount, int(playerInfo.ledCount));
                    int clearCount = source.ledCount - (end - src);

                    switch (config.ledType) {
                    case LedType::RGB:
                        while (src != end) {
                            uint32_t color = *src;
                            int r = color >> 3 & 255;
                            int g = color >> 14 & 255;
                            int b = color >> 24 & 255;

                            dst[0] = r;
                            dst[1] = g;
                            dst[2] = b;

                            ++src;
                            dst += 3;
                        }
                        break;
                    case LedType::GRB:
                        while (src != end) {
                            uint32_t color = *src;
                            int r = color >> 3 & 255;
                            int g = color >> 14 & 255;
                            int b = color >> 24 & 255;

                            dst[0] = g;
                            dst[1] = r;
                            dst[2] = b;

                            ++src;
                            dst += 3;
                        }
                        break;
                    }

                    for (int i = 0; i < clearCount; ++i) {
                        dst[0] = 0;
                        dst[1] = 0;
                        dst[2] = 0;
                        dst += 3;
                    }
                } else {
                    uint8_t r = 0;
                    uint8_t g = 0;
                    uint8_t b = 0;
                    if (this->editStripIndex == stripIndex && this->editSourceIndex == sourceIndex) {
                        r = 128;
                        g = 128;
                        b = 128;
                    }
                    for (int i = 0; i < source.ledCount; ++i) {
                        dst[0] = r;
                        dst[1] = g;
                        dst[2] = b;
                        dst += 3;
                    }
                }
            }

            // show on led strip
            strip.buffer.startWrite(dst);
        }
    }
}
