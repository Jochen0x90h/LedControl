#include "EffectManager.hpp"
#include "E12.hpp"
#include <coco/BufferStorage.hpp>
#include <coco/Vector2.hpp>
#include <coco/debug.hpp>


static const Vector2<float> colorTemperature[] = {
    {0.0333333, 1}, // 1000K
    {0.0535948, 1}, // 1200K
    {0.072549, 1}, // 1500K
    {0.0849673, 1}, // 1800K
    {0.0864486, 0.839216}, // 2200K
    {0.0877193, 0.670588}, // 2700K
    {0.0903308, 0.513726}, // 3300K
    {0.0925926, 0.388235}, // 3900K
    {0.0899471, 0.247059}, // 4700K
    {0.0777778, 0.117647}, // 5600K
    {0.761905, 0.027451}, // 6800K
    {0.636905, 0.109804}, // 8200K
    {0.628472, 0.188235}, // 10000K
    {0.627778, 0.235294}, // 12000K
    {0.625, 0.282353}, // 15000K
    {0.624473, 0.309804}, // 18000K
    {0.626984, 0.329412}, // 22000K
    {0.626894, 0.345098}, // 27000K
    {0.626812, 0.360784}, // 33000K
    {0.62766, 0.368627}, // 39000K
};


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

        // read presets and data for preset names and preset parameters
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
            auto &p = this->players[i];
            if (i != playerIndex)
                maxLedCount -= p.config.ledCount;
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
    uint8_t *parameters = config.getParametersData(presetIndex);
    float *effectParameters = player.effectParameters;
    for (int parameterIndex = 0; parameterIndex < parameterCount; ++parameterIndex) {
        auto &parameterInfo = parameterInfos[parameterIndex];

        // convert into preset parameter
        switch (parameterInfo.type) {
        case ParameterInfo::Type::COUNT:
            parameters[0] = int(effectParameters[0]);
            break;
        case ParameterInfo::Type::DURATION_E12:
            parameters[0] = toE12(int(effectParameters[0] * 1000.0f));
            break;
        case ParameterInfo::Type::PERCENTAGE:
            parameters[0] = int(effectParameters[0] * 100.0f);
            break;
        case ParameterInfo::Type::PERCENTAGE_E12:
            parameters[0] = toE12(int(effectParameters[0] * 1000.0f));
            break;
        case ParameterInfo::Type::HUE:
            parameters[0] = int(effectParameters[0] * 24.0f);
            break;
        case ParameterInfo::Type::COLOR:
            // hue and saturation
            parameters[0] = int(effectParameters[0] * 24.0f);
            parameters[1] = int(effectParameters[1] * 100.0f);
            ++parameters;
            ++effectParameters;
            break;
        }
        ++parameters;
        ++effectParameters;
    }

    // return number of parameter values
    return effectParameters - player.effectParameters;
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
    {"Brightness", ParameterInfo::Type::PERCENTAGE_E12},//, 0, 24, 1},
    {"Duration", ParameterInfo::Type::DURATION_E12, 12},//, 12, 72, 1},
};

String EffectManager::getGlobalParameterName(int parameterIndex) {
    return globalParameterInfos[parameterIndex].name;
}

//String EffectManager::getParameterName(int playerIndex, int presetIndex, int parameterIndex) {
//    auto &config = this->players[playerIndex].config;
//    return this->effectInfos[config.presets[presetIndex].effectIndex].parameterInfos[parameterIndex].name;
//}
const ParameterInfo &EffectManager::getParameterInfo(int playerIndex, int presetIndex, int parameterIndex) {
    auto &config = this->players[playerIndex].config;
    return this->effectInfos[config.presets[presetIndex].effectIndex].parameterInfos[parameterIndex];
}

EffectManager::ParameterValue EffectManager::updateGlobalParameter(int parameterIndex, int delta)
{
    // start save timer if global parameter gets changed
    if (delta != 0) {
        this->timeout = this->loop.now() + SAVE_TIMEOUT;
        this->timerBarrier.doAll();
    }

    uint8_t *values;
    if (parameterIndex == 0) {
        // brightness percentage E12 (0-24 -> 1.0%-100.0%)
        this->global.brightness = std::clamp(this->global.brightness + delta, 0, 24);
        this->brightness = PercentageE12{this->global.brightness}.get() * 0.001f;
        values = &this->global.brightness;
    } else {
        // duration E12 (12-72 -> 100ms - 1000000s)
        this->global.duration = std::clamp(this->global.duration + delta, 12, 72);
        this->duration = MillisecondsE12{this->global.duration}.get() * 1ms;
        values = &this->global.duration;
    }

    return {globalParameterInfos[parameterIndex], values};
}

EffectManager::ParameterValue EffectManager::updateParameter(int playerIndex, int presetIndex, int parameterIndex,
    int componentIndex, int delta)
{
    auto &player = this->players[playerIndex];
    auto &config = player.config;
    auto &preset = config.presets[presetIndex];
/*
    auto &parameterInfo = this->effectInfos[preset.effectIndex].parameterInfos[parameterIndex];

    //if (delta != 0) {
    //    // mark player config as modified
    //    this->playerConfigsModified |= 1 << playerIndex;
    //}
    uint8_t *parameters = config.getParametersData(presetIndex);
    uint8_t &parameter = parameters[parameterIndex];
    //float *effectParameters = player.effectParameters[parameterIndex];
    float &effectParameter = player.effectParameters[parameterIndex];
*/
    // update parameter
    /*if (!parameterInfo.wrap) {
        parameter = std::clamp(parameter + delta * parameterInfo.step, int(parameterInfo.min), int(parameterInfo.max));
    } else {
        int m = parameterInfo.min;
        int range = parameterInfo.max + 1 - m;
        parameter = (parameter + delta * parameterInfo.step + range * 256 - m) % range + m;
    }

    // convert to effect parameter
    switch (parameterInfo.type) {
    case ParameterInfo::Type::COUNT:
        parameter = std::clamp(parameter + delta, 1, parameterInfo.value);

        effectParameter = parameter;
        break;
    case ParameterInfo::Type::DURATION_E12:
        parameter = std::clamp(parameter + delta, 12, 72); // 100ms - 1000000s

        effectParameter = MillisecondsE12{parameter}.get() * 0.001f;
        break;
    case ParameterInfo::Type::PERCENTAGE:
        parameter = std::clamp(parameter + delta * parameterInfo.value, 1, 100);

        effectParameter = parameter * 0.01f;
        break;
    case ParameterInfo::Type::PERCENTAGE_E12:
        parameter = std::clamp(parameter + delta, 0, 24); // 1.0% - 100%

        effectParameter = PercentageE12(parameter).get() * 0.001f; // 24 -> 1000 -> 100%
        break;
    case ParameterInfo::Type::HUE:
        parameter = (parameter + delta + 24 * 256) % 24;

        effectParameter = parameter / 24.0f;
        break;
    }

    */

    auto &effectInfo = this->effectInfos[preset.effectIndex];
    auto &info = effectInfo.parameterInfos[parameterIndex];
    int precedingValues = effectInfo.getPrecedingValueCount(parameterIndex);
    uint8_t *values = config.getParametersData(presetIndex) + precedingValues;

    if (delta != 0) {
        this->playerConfigsModified |= 1 << playerIndex;

        // modify parameter
        switch (info.type) {
        case ParameterInfo::Type::COUNT:
            if (componentIndex == 0)
                values[0] = clampCount(values[0] + delta, info.value);
            break;
        case ParameterInfo::Type::DURATION_E12:
            if (componentIndex == 0)
                values[0] = clampDurationE12(values[0] + delta, info.value); // 10ms - 1000000s
            break;
        case ParameterInfo::Type::PERCENTAGE:
            if (componentIndex == 0)
                values[0] = clampPercentage(values[0] + delta * info.value); // 1% - 100%
            break;
        case ParameterInfo::Type::PERCENTAGE_E12:
            if (componentIndex == 0)
                values[0] = clampPercentageE12(values[0] + delta); // 1.0% - 100%
            break;
        case ParameterInfo::Type::HUE:
            if (componentIndex == 0)
                values[0] = wrapHue(values[0] + delta);
            break;
        case ParameterInfo::Type::COLOR:
            if (componentIndex == 0)
                values[0] = wrapColorHue(values[0] + delta); // hue or temperature
            if (componentIndex == 1) {
                if (values[0] < 24)
                    values[1] = clampPercentage(values[1] + delta * info.value); // saturation
                else
                    values[1] = clampColorTemperature(values[1] + delta);
            }
            break;
        }
    }

    return {info, values};
}

void EffectManager::applyParameters(int playerIndex, int presetIndex) {
    auto &player = this->players[playerIndex];
    auto &config = player.config;
    auto &preset = config.presets[presetIndex];

    auto &parameterInfos = this->effectInfos[preset.effectIndex].parameterInfos;
    uint8_t *values = config.getParametersData(presetIndex);
    float *effectValues = player.effectParameters;

    for (int i = 0; i < parameterInfos.size(); ++i) {
        auto &info = parameterInfos[i];
        int value = values[0];
        switch (info.type) {
        case ParameterInfo::Type::COUNT:
            effectValues[0] = clampCount(value, info.value);
            break;
        case ParameterInfo::Type::DURATION_E12:
            effectValues[0] = MillisecondsE12(clampDurationE12(value, info.value)).get() * 0.001f;
            break;
        case ParameterInfo::Type::PERCENTAGE:
            effectValues[0] = clampPercentage(value) * 0.01f;
            break;
        case ParameterInfo::Type::PERCENTAGE_E12:
            effectValues[0] = PercentageE12(clampPercentageE12(value)).get() * 0.001f; // 24 -> 1000 -> 100%
            break;
        case ParameterInfo::Type::HUE:
            effectValues[0] = wrapHue(value) / 24.0f;
            break;
        case ParameterInfo::Type::COLOR:
            if (value < 24) {
                // hue and saturation
                effectValues[0] = value / 24.0f;
                effectValues[1] = clampPercentage(values[1]) * 0.01f;
            } else {
                // color temperature
                auto hs = colorTemperature[clampColorTemperature(values[1]) - 24];
                effectValues[0] = hs.x;
                effectValues[1] = hs.y;
            }
            ++values;
            ++effectValues;
            break;
        }
        ++values;
        ++effectValues;
    }
}

void EffectManager::run(int presetIndex) {
    stop();
    int ledStart = 0;
    for (int playerIndex = 0; playerIndex < PLAYER_COUNT; ++playerIndex) {
        auto &player = this->players[playerIndex];
        auto &config = player.config;

        if (presetIndex < config.presetCount) {
            auto &effectInfo = this->effectInfos[config.presets[presetIndex].effectIndex];

            // transfer preset parameters into effect parameters
            applyParameters(playerIndex, presetIndex);
            //for (int parameterIndex = 0; parameterIndex < effectInfo.parameterInfos.size(); ++parameterIndex)
            //    updateParameter(i, presetIndex, parameterIndex, 0);

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
            applyParameters(playerIndex, presetIndex);
            //for (int parameterIndex = 0; parameterIndex < effectInfo.parameterInfos.size(); ++parameterIndex)
            //    updateParameter(i, presetIndex, parameterIndex, 0);

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
