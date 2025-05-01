#include "EffectManager.hpp"
#include "E12.hpp"
#include <coco/BufferStorage.hpp>
#include <coco/debug.hpp>


AwaitableCoroutine EffectManager::load() {
    int result;
    //co_await storage.mount(result);

    // load preset directory
    co_await this->storage.read(DIRECTORY_ID, this->directory, MAX_PRESET_COUNT, result);
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

        // todo: sanitize parameters

        preset.dirty = false;
    }

    // add default preset if necessary
    if (this->presetCount == 0) {
        addPreset();
    }

    // gload global parameters
    co_await this->storage.read(GLOBAL_ID, &this->global, sizeof(Global), result);
    if (result == 3) {
        this->global.presetIndex = std::clamp(int(this->global.presetIndex), 0, this->presetCount - 1);
        this->global.brightness = std::clamp(int(this->global.brightness), 0, 24);
        this->global.duration = std::clamp(int(this->global.duration), 12, 72);
    } else {
        // first-time initialization
        this->global.presetIndex = 0;
        this->global.brightness = 24; // 100%
        this->global.duration = 24; // 1s
    }
}

AwaitableCoroutine EffectManager::save() {
    int result;

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
}

int EffectManager::addPreset() {
    int presetIndex = this->presetCount;

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

    return presetIndex;
}

void EffectManager::deletePreset(int presetIndex) {
    for (int i = presetIndex + 1; i < this->presetCount; ++i) {
        this->directory[i - 1] = this->directory[i];
        this->presetList[i - 1] = this->presetList[i];
    }
    --this->presetCount;
    this->directoryDirty = true;
}

void EffectManager::updateEffect(int presetIndex, int delta) {
    auto &preset = this->presetList[presetIndex];

    preset.effectIndex = (preset.effectIndex + this->effectInfos.size() * 256 + delta) % this->effectInfos.size();

    // set defaults
    initParameters(presetIndex);

    // mark preset as "dirty", to be saved on next save operation
    preset.dirty = true;
}

void EffectManager::initParameters(int presetIndex) {
    auto &preset = this->presetList[presetIndex];
    auto &effectInfo = this->effectInfos[preset.effectIndex];

    // initialize effect parameters with default values
    effectInfo.init(this->effectParameters);

    // convert to preset parameters
    auto &parameterInfos = effectInfo.parameterInfos;
    int parametersOffset = preset.nameLength;// - 1;
    for (int parameterIndex = 0; parameterIndex < parameterInfos.size(); ++parameterIndex) {
        auto &parameterInfo = parameterInfos[parameterIndex];
        uint8_t &parameter = preset.data[parametersOffset + parameterIndex];
        float effectParameter = this->effectParameters[parameterIndex];

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
}

const ParameterInfo globalParameterInfos[] = {
    {"Brightness", ParameterInfo::Type::PERCENTAGE_E12, 0, 24, 1},
    {"Duration", ParameterInfo::Type::DURATION_E12, 12, 72, 1},
};

String EffectManager::getParameterName(int presetIndex, int parameterIndex) {
    if (parameterIndex < 2)
        return globalParameterInfos[parameterIndex].name;
    return this->effectInfos[this->presetList[presetIndex].effectIndex].parameterInfos[parameterIndex - 2].name;
}

EffectManager::ParameterValue EffectManager::updateParameter(int presetIndex, int parameterIndex, int delta) {
    // parameter index 0 and 1 are the global parameters brighness and duration
    if (parameterIndex < 2) {
        if (delta != 0) {
            // start save timer
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

    parameterIndex -= 2;

    auto &preset = this->presetList[presetIndex];
    auto &parameterInfo = this->effectInfos[preset.effectIndex].parameterInfos[parameterIndex];

    if (delta != 0) {
        // mark preset as "dirty", to be saved on next save operation
        preset.dirty = true;
    }
    int parametersOffset = preset.nameLength;// - 1;
    uint8_t &parameter = preset.data[parametersOffset + parameterIndex];
    float &effectParameter = this->effectParameters[parameterIndex];

    // update parameter
    if (!parameterInfo.wrap) {
        parameter = std::clamp(parameter + delta * parameterInfo.step, int(parameterInfo.min), int(parameterInfo.max));
    } else {
        int m = parameterInfo.min;
        int range = parameterInfo.max + 1 - m;
        parameter = (parameter + delta * parameterInfo.step + (range << 16) - m) % range + m;
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

AwaitableCoroutine EffectManager::run(int presetIndex) {
    // stop current effect
    this->effect.destroy();

    // wait until strip is ready
    co_await this->strip.untilReady();

    auto &effectInfo = this->effectInfos[this->presetList[presetIndex].effectIndex];

    // transfer effect parameters from preset
    for (int parameterIndex = 0; parameterIndex < effectInfo.parameterInfos.size() + 2; ++parameterIndex)
        updateParameter(presetIndex, parameterIndex, 0);

    // start new effect
    this->effect = run(effectInfo.end, effectInfo.run);
}

Coroutine EffectManager::run(EndFunction end, RunFunction calc) {
    auto start = this->loop.now();
    while (true) {
        // get current time
        auto now = this->loop.now();

        // calc effect time
        float time = float(now - start) / float(this->duration);

        // check if effect time is at end
        if (end(time, this->effectParameters)) {
            // yes: restart
            start = now;
            time = 0;
        }

        // calculate the effect for the current effect time
        calc(this->strip, this->brightness, time, this->effectParameters);

        // copy into actual strip buffers
        // todo

        // show effect
        co_await strip.show();
    }
}


AwaitableCoroutine EffectManager::stop() {
    // stop current effect
    this->effect.destroy();

    // wait until strip is ready
    co_await strip.untilReady();

    // turn off all LEDs
    strip.clear();
    co_await strip.show();
}

void EffectManager::setPresetIndex(int index) {
    if (index != this->global.presetIndex) {
        this->global.presetIndex = index;

        // start save timer
        this->timeout = this->loop.now() + SAVE_TIMEOUT;
        this->timerBarrier.doAll();
    }
}

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
}

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
