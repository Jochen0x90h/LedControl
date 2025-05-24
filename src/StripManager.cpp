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

        if (config.sourceCount == 0) {
            // add default source
            config.ledType = DEFAULT_LED_TYPE;
            config.ledCount = MAX_LEDSTRIP_LENGTH;
            auto &source = config.sources[0];
            source.ledStart = 0;
            source.ledCount = std::min(this->stripData.size(), MAX_LEDSTRIP_LENGTH);
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

void StripManager::setStripConfig(int index, StripConfig &newConfig) {
    int flag = 1 << index;
    auto &config = this->strips[index].config;

    if (config.ledType != newConfig.ledType
        || config.ledCount != newConfig.ledCount
        || config.sourceCount != newConfig.sourceCount)
    {
        this->stripConfigsModified |= flag;
    }
    config.ledType = newConfig.ledType;
    config.ledCount = newConfig.ledCount;
    config.sourceCount = newConfig.sourceCount;

    for (int i = 0; i < config.sourceCount; ++i) {
        auto &source = config.sources[i];
        auto &newSource = newConfig.sources[i];

        if (source.playerIndex != newSource.playerIndex
            || source.ledStart != newSource.ledStart
            || source.ledCount != newSource.ledCount)
        {
            this->stripConfigsModified |= flag;
        }
        source.playerIndex = newSource.playerIndex;
        source.ledStart = newSource.ledStart;
        source.ledCount = newSource.ledCount;
    }
}

Coroutine StripManager::run() {
    while (true) {
        // calculate all effects
        this->syncBarrier.doAll();

        for (auto &strip : this->strips) {
            auto &config = strip.config;

            // wait until strip buffer is ready
            co_await strip.buffer.untilReady();

            // copy strip data into strip buffer
            uint8_t *dst = strip.buffer.data();
            for (int i = 0; i < config.sourceCount; ++i) {
                auto &source = config.sources[i];
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
            }

            /*
            // copy strip data into strip buffer
            uint32_t *src = this->stripData.begin();
            uint32_t *end = this->stripData.end();
            uint8_t *dst = this->stripBuffer.data();
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
            }*/

            // show
            strip.buffer.startWrite(strip.config.ledCount * 3);
        }

        // show effect
        //this->stripBuffer.startWrite(MAX_LEDSTRIP_LENGTH * 3);
    }
}
