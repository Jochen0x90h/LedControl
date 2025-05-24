#include "EffectManager.hpp"
#include "E12.hpp"
#include <coco/BufferStorage.hpp>
#include <coco/debug.hpp>


AwaitableCoroutine EffectManager::load() {
    int result;
    //co_await storage.mount(result);

    // load preset directory
    /*co_await this->storage.read(DIRECTORY_ID, this->directory, MAX_PRESET_COUNT, result);
    this->presetCount = std::clamp(result, 0, MAX_PRESET_COUNT);
    this->directoryDirty = false;

    // load all presets
    for (int presetIndex = 0; presetIndex < this->presetCount; ++presetIndex) {
        int id = directory[presetIndex];
        auto &preset = this->presetList[presetIndex];
        co_await this->storage.read(id, &preset, sizeof(Preset), result);
        // todo: handle error

        // make sure effect index is valid
        preset.effectIndex = std::clamp(int(preset.effectIndex), 0, this->effectInfos.size() - 1);

        // todo: sanitize effect parameters

        preset.dirty = false;
    }

    // add default preset if necessary
    if (this->presetCount == 0) {
        addPreset();
    }

    */

    // load players
    int presetCount = 0;
    int ledCount = 0;
    for (int playerIndex = 0; playerIndex < PLAYER_COUNT; ++playerIndex) {
        auto &player = this->players[playerIndex];
        auto &config = player.config;

        int id = FIRST_PLAYER_ID + playerIndex * 2;
        co_await this->storage.read(id, &config, offsetof(PlayerConfig, presets[MAX_PLAYER_PRESET_COUNT]), result);
        int a = int((result - int(offsetof(PlayerConfig, presets))) / int(sizeof(Preset)));
        config.presetCount = std::max(int((result - int(offsetof(PlayerConfig, presets))) / int(sizeof(Preset))), 0);
        co_await this->storage.read(id + 1, config.data, MAX_PLAYER_DATA_SIZE, result);

        if (config.presetCount == 0) {
            config.ledCount = MAX_LEDSTRIP_LENGTH;

            // add default preset
            addPreset(playerIndex);
        }
        presetCount = std::max(int(config.presetCount), presetCount);

        // make sure the sum of the led counts does not exceed the strip data size
        config.ledCount = std::min(int(config.ledCount), this->stripData.size() - ledCount);
        ledCount += config.ledCount;
    }
    this->playerConfigsModified = 0;

    // update the list of player infos
    updatePlayerInfos();

    // gload global parameters
    co_await this->storage.read(GLOBAL_ID, &this->global, sizeof(Global), result);
    if (result == 3) {
        // sanitize global parametrs
        this->global.presetIndex = std::clamp(int(this->global.presetIndex), 0, presetCount - 1);
        this->global.brightness = std::clamp(int(this->global.brightness), 0, 24); // 0.1% - 100%
        this->global.duration = std::clamp(int(this->global.duration), 12, 72); // 100ms - 1000000s
    } else {
        // first-time initialization
        this->global.presetIndex = 0;
        this->global.brightness = 24; // 100%
        this->global.duration = 24; // 1s
    }

    // set this->brightness and this->duration
    updateGlobalParameter(0, 0);
    updateGlobalParameter(1, 0);
 /*
    // load LED strip configurations
    for (int i = 0; i < 3; ++i) {
        auto &stripConfig = this->stripConfigs[i];
        co_await this->storage.read(STRIP1_ID + i, &stripConfig, sizeof(StripConfig), result);
        stripConfig.sourceCount = std::max(int((result - offsetof(StripConfig, sources)) / sizeof(StripSource)), 0);
    }
    this->stripConfigDirty = 0;
*/
}

AwaitableCoroutine EffectManager::save() {
    int result;

    /*
    // save all dirty presets
    for (int presetIndex = 0; presetIndex < this->presetCount; ++presetIndex) {
        auto &preset = this->presetList[presetIndex];
        if (preset.dirty) {
            // determine size of preset
            int size = offsetof(Preset, data) + preset.nameLength + this->effectInfos[preset.effectIndex].parameterInfos.size();

            // save preset
            co_await this->storage.write(this->directory[presetIndex], &preset, size, result);
            preset.dirty = false;
        }
    }

    // save preset directory
    if (this->directoryDirty) {
        co_await this->storage.write(DIRECTORY_ID, this->directory, this->presetCount, result);
        this->directoryDirty = false;
    }
*/

    for (int playerIndex = 0; playerIndex < PLAYER_COUNT; ++playerIndex) {
        if (this->playerConfigsModified & (1 << playerIndex)) {
            auto &player = this->players[playerIndex];
            auto &config = player.config;

            int id = FIRST_PLAYER_ID + playerIndex * 2;
            co_await this->storage.write(id, &config, offsetof(PlayerConfig, presets[config.presetCount]), result);
            co_await this->storage.write(id + 1, config.data, config.getDataOffset(config.presetCount), result);
        }
    }
    this->playerConfigsModified = 0;
}
/*
AwaitableCoroutine EffectManager::saveStripConfigs() {
    int result;
    for (int i = 0; i < 3; ++i) {
        if (this->stripConfigDirty & (1 << i)) {
            auto &stripConfig = this->stripConfigs[i];
            int size = offsetof(StripConfig, sources[stripConfig.sourceCount]);
            co_await this->storage.write(STRIP1_ID + i, &stripConfig, size, result);
        }
    }
    this->stripConfigDirty = 0;
}*/

void EffectManager::setPresetIndex(int index) {
    if (index != this->global.presetIndex) {
        this->global.presetIndex = index;

        // start save timer
        this->timeout = this->loop.now() + SAVE_TIMEOUT;
        this->timerBarrier.doAll();
    }
}

int EffectManager::updateLedCount(int playerIndex, int delta) {
    auto &player = this->players[playerIndex];
    auto &config = player.config;

    if (delta != 0) {
        this->playerConfigsModified |= 1 << playerIndex;

        // determine maximum led count which is constrained by the strip data size
        int maxLedCount = this->stripData.size();
        for (int i = 0; i < PLAYER_COUNT; ++i) {
            auto &player = this->players[i];
            if (i != playerIndex)
                maxLedCount -= player.config.ledCount;
        }

        // also limit to maximum led strip length
        maxLedCount = std::min(maxLedCount, MAX_LEDSTRIP_LENGTH);

        // range is [1, maxLedCount]
        config.ledCount = (config.ledCount - 1 + delta + maxLedCount * 256) % maxLedCount + 1;

        // update the list of player infos
        updatePlayerInfos();
    }
    return config.ledCount;
}

int EffectManager::addPreset(int playerIndex) {
    /*int presetIndex = this->presetCount;

    // initialize preset
    auto &preset = this->presetList[presetIndex];
    preset.effectIndex = 0;
    preset.nameLength = 0;
    initParameters(presetIndex);
    preset.dirty = true;

    // find free id
    this->directory[presetIndex] = findFreeId();
    this->directoryDirty = true;
    ++this->presetCount;
*/
    auto &player = this->players[playerIndex];
    auto &config = player.config;

    int presetIndex = config.presetCount;
    ++config.presetCount;

    // initialize preset
    auto &preset = config.presets[presetIndex];
    preset.effectIndex = 0;
    preset.nameLength = 0;
    preset.parameterCount = initParameters(playerIndex, presetIndex);

    // mark as dirty
    this->playerConfigsModified |= 1 << playerIndex;

    return presetIndex;
}

void EffectManager::deletePreset(int playerIndex, int presetIndex) {
    /*for (int i = presetIndex + 1; i < this->presetCount; ++i) {
        this->directory[i - 1] = this->directory[i];
        this->presetList[i - 1] = this->presetList[i];
    }
    --this->presetCount;
    this->directoryDirty = true;*/

    auto &player = this->players[playerIndex];
    auto &config = player.config;

    for (int i = presetIndex + 1; i < config.presetCount; ++i) {
        config.presets[i - 1] = config.presets[i];
    }
    --config.presetCount;

    // mark player config as dirty
    this->playerConfigsModified |= 1 << playerIndex;
}

void EffectManager::updateEffect(int playerIndex, int presetIndex, int delta) {
    auto &player = this->players[playerIndex];
    auto &config = player.config;
    auto &preset = config.presets[presetIndex];
    //auto &preset = this->presetList[presetIndex];

    // set new effect index
    preset.effectIndex = (preset.effectIndex + this->effectInfos.size() * 256 + delta) % this->effectInfos.size();

    // set default parameters of new effect
    preset.parameterCount = initParameters(playerIndex, presetIndex);

    // mark preset as "dirty", to be saved on next save operation
    //preset.dirty = true;

    // mark player config as dirty
    this->playerConfigsModified |= 1 << playerIndex;
}

int EffectManager::initParameters(int playerIndex, int presetIndex) {
    //auto &preset = this->presetList[presetIndex];
    auto &player = this->players[playerIndex];
    auto &config = player.config;
    auto &preset = config.presets[presetIndex];
    auto &effectInfo = this->effectInfos[preset.effectIndex];

    // initialize effect parameters with default values
    effectInfo.init(player.effectParameters);

    // convert effect parameters (float) to preset parameters (uint8_t)
    auto &parameterInfos = effectInfo.parameterInfos;
    int parameterCount = parameterInfos.size();
    //int parametersOffset = preset.nameLength;// - 1;
    uint8_t *data = config.getParametersData(presetIndex);
    for (int parameterIndex = 0; parameterIndex < parameterCount; ++parameterIndex) {
        auto &parameterInfo = parameterInfos[parameterIndex];

        // get effect parameter
        float effectParameter = player.effectParameters[parameterIndex];

        // convert into preset parameter
        uint8_t &parameter = data[parameterIndex];//preset.data[parametersOffset + parameterIndex];
        switch (parameterInfo.type) {
        case ParameterInfo::Type::COUNT:
            parameter = int(effectParameter);
            break;
        case ParameterInfo::Type::DURATION_E12:
            parameter = toE12(int(effectParameter * 1000.0f));
            break;
        case ParameterInfo::Type::PERCENTAGE:
            parameter = int(effectParameter * 100.0f);
            break;
        case ParameterInfo::Type::PERCENTAGE_E12:
            parameter = toE12(int(effectParameter * 1000.0f));
            break;
        case ParameterInfo::Type::HUE:
            parameter = int(effectParameter * 24.0f);
            break;
        }
    }

    return parameterCount;
}

int EffectManager::getPresetCount() {
    int count = 0;
    for (int i = 0; i < PLAYER_COUNT; ++i) {
        count = std::max(count, int(this->players[i].config.presetCount));
    }
    return count;
}

// infos for the global parameters
const ParameterInfo globalParameterInfos[] = {
    {"Brightness", ParameterInfo::Type::PERCENTAGE_E12, 0, 24, 1},
    {"Duration", ParameterInfo::Type::DURATION_E12, 12, 72, 1},
};

String EffectManager::getGlobalParameterName(int parameterIndex) {
    return globalParameterInfos[parameterIndex].name;
}

String EffectManager::getParameterName(int playerIndex, int presetIndex, int parameterIndex) {
    auto &config = this->players[playerIndex].config;
    return this->effectInfos[config.presets[presetIndex].effectIndex].parameterInfos[parameterIndex].name;
}

EffectManager::ParameterValue EffectManager::updateGlobalParameter(int parameterIndex, int delta)
{
    // start save timer if global parameter gets changed
    if (delta != 0) {
        this->timeout = this->loop.now() + SAVE_TIMEOUT;
        this->timerBarrier.doAll();
    }

    uint8_t parameter;
    if (parameterIndex == 0) {
        // brightness percentage E12 (0-24 -> 1.0%-100.0%)
        parameter = this->global.brightness = std::clamp(this->global.brightness + delta, 0, 24);
        this->brightness = PercentageE12{parameter}.get() * 0.001f;
    } else {
        // duration E12 (12-72 -> 100ms - 10000s)
        parameter = this->global.duration = std::clamp(this->global.duration + delta, 12, 60);
        this->duration = MillisecondsE12{parameter}.get() * 1ms;
    }

    return {globalParameterInfos[parameterIndex], int(parameter)};
}

EffectManager::ParameterValue EffectManager::updateParameter(int playerIndex, int presetIndex, int parameterIndex,
    int delta)
{
    // parameter index 0 and 1 are the global parameters brighness and duration
    //if (parameterIndex < 2) {
   // }
    //parameterIndex -= 2;

    auto &player = this->players[playerIndex];
    auto &config = player.config;
    auto &preset = config.presets[presetIndex];
    //auto &preset = this->presetList[presetIndex];
    auto &parameterInfo = this->effectInfos[preset.effectIndex].parameterInfos[parameterIndex];

    if (delta != 0) {
        // mark preset as "dirty", to be saved on next save operation
        //preset.dirty = true;

        // mark player config as dirty
        this->playerConfigsModified |= 1 << playerIndex;
    }
    //int parametersOffset = preset.nameLength;// - 1;
    //uint8_t &parameter = preset.data[parametersOffset + parameterIndex];
    uint8_t &parameter = config.getParametersData(presetIndex)[parameterIndex];
    float &effectParameter = player.effectParameters[parameterIndex];

    // update parameter
    if (!parameterInfo.wrap) {
        parameter = std::clamp(parameter + delta * parameterInfo.step, int(parameterInfo.min), int(parameterInfo.max));
    } else {
        int m = parameterInfo.min;
        int range = parameterInfo.max + 1 - m;
        parameter = (parameter + delta * parameterInfo.step + range * 256 - m) % range + m;
    }

    // convert to effect parameter
    switch (parameterInfo.type) {
    case ParameterInfo::Type::COUNT:
        effectParameter = parameter;
        break;
    case ParameterInfo::Type::DURATION_E12:
        effectParameter = MillisecondsE12{parameter}.get() * 0.001f;
        break;
    case ParameterInfo::Type::PERCENTAGE:
        effectParameter = parameter * 0.01f;
        break;
    case ParameterInfo::Type::PERCENTAGE_E12:
        effectParameter = PercentageE12{parameter}.get() * 0.001f;
        break;
    case ParameterInfo::Type::HUE:
        effectParameter = parameter / 24.0f;
        break;
    }
    return {parameterInfo, int(parameter)};
}

void EffectManager::run(int presetIndex) {
    stop();
    int ledStart = 0;
    for (int i = 0; i < PLAYER_COUNT; ++i) {
        auto &player = this->players[i];
        auto &config = player.config;

        if (presetIndex < config.presetCount) {
            auto &effectInfo = this->effectInfos[config.presets[presetIndex].effectIndex];

            // transfer preset parameters into effect parameters
            for (int parameterIndex = 0; parameterIndex < effectInfo.parameterInfos.size(); ++parameterIndex)
                updateParameter(i, presetIndex, parameterIndex, 0);

            // start new effect
            player.effect = run(ledStart, player, effectInfo.end, effectInfo.run);
        }
        ledStart += config.ledCount;
    }
}

void EffectManager::run(int playerIndex, int presetIndex) {
    stop();
    int ledStart = 0;
    for (int i = 0; i < PLAYER_COUNT; ++i) {
        auto &player = this->players[i];
        auto &config = player.config;

        if (i == playerIndex && presetIndex < config.presetCount) {
            auto &effectInfo = this->effectInfos[config.presets[presetIndex].effectIndex];

            // transfer preset parameters into effect parameters
            for (int parameterIndex = 0; parameterIndex < effectInfo.parameterInfos.size(); ++parameterIndex)
                updateParameter(i, presetIndex, parameterIndex, 0);

            // start new effect
            player.effect = run(ledStart, player, effectInfo.end, effectInfo.run);
        }
        ledStart += config.ledCount;
    }
}

Coroutine EffectManager::run(int ledStart, Player &player, EndFunction end, CalcFunction calc) {
    auto start = this->loop.now();
    StripData stripData = this->stripData.range(ledStart, player.config.ledCount);
    while (true) {
        // get current time
        auto now = this->loop.now();

        // calc effect time
        float time = float(now - start) / float(this->duration);

        // check if effect time is at end
        if (end(time, player.effectParameters)) {
            // yes: restart
            start = now;
            time = 0;
        }

        // calculate the effect for the current effect time
        //calc(this->strip, this->brightness, time, this->effectParameters);
        calc(stripData, this->brightness, time, player.effectParameters);

        // wait on barrier
        co_await this->syncBarrier.untilResumed();
/*
        // wait until strip is ready
        co_await this->stripBuffer.untilReady();

        if (index == 0) {
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
            }

            // show effect
            //co_await strip.show();
            this->stripBuffer.startWrite(MAX_LEDSTRIP_LENGTH * 3);
        }*/
    }
}


void EffectManager::stop() {
    for (auto &player : this->players) {

        // stop current effect
        player.effect.destroy();

        // wait until strip is ready
        //co_await strip.untilReady();
        //co_await this->stripBuffer.untilReady();

        // turn off all LEDs
        this->stripData.fill(0);
        //strip.clear();
        //co_await strip.show();
        //this->stripBuffer.array<uint8_t>().fill(0);
        //co_await this->stripBuffer.write();
    }
}

void EffectManager::updatePlayerInfos() {
    uint16_t start = 0;
    for (int i = 0; i < PLAYER_COUNT; ++i) {
        uint16_t count = this->players[i].config.ledCount;
        this->playerInfos[i] = {start, count};
        start += count;
    }
}



/*
int EffectManager::findFreeId() {
    for (int id = FIRST_PRESET_ID; id <= LAST_PRESET_ID; ++id) {
        for (int i = 0; i < this->presetCount; ++i) {
            if (this->directory[i] == id)
                goto found;
        }
        return id;
    found:
        ;
    }
    return -1;
}*/

Coroutine EffectManager::saveTimer() {
    while (true) {
        // wait until timeout gets started
        co_await this->timerBarrier.untilResumed();

        // wait until timeout where time can be updated by read/write methods
        while (this->loop.now() < this->timeout)
            co_await this->loop.sleep(this->timeout);

        // timeout elapsed: save parameters
        int result;
        co_await storage.write(GLOBAL_ID, &this->global, sizeof(global), result);
        debug::toggleGreen();
    }
}
