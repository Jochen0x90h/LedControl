#include "EffectManager.hpp"
#include "E12.hpp"
#include <coco/BufferStorage.hpp>
#include <coco/debug.hpp>


AwaitableCoroutine EffectManager::load() {
	int result;
	co_await storage.mount(result);

	// load preset directory
	co_await storage.read(DIRECTORY_ID, this->directory, MAX_PRESET_COUNT, result);
	this->presetCount = std::clamp(result, 0, MAX_PRESET_COUNT);
	this->directoryDirty = false;

	// load all presets
	for (int presetIndex = 0; presetIndex < this->presetCount; ++presetIndex) {
		int id = directory[presetIndex];
		auto &preset = this->presetList[presetIndex];
		co_await storage.read(id, &preset, sizeof(Preset), result);
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
	co_await storage.read(GLOBAL_ID, &this->global, sizeof(Global), result);
	this->global.brightness = std::clamp(int(this->global.brightness), 0, 24);
	this->global.presetIndex = std::clamp(int(this->global.presetIndex), 0, this->presetCount - 1);
}

AwaitableCoroutine EffectManager::save() {
	int result;

	// save all presets
	for (int presetIndex = 0; presetIndex < this->presetCount; ++presetIndex) {
		auto &preset = this->presetList[presetIndex];
		if (preset.dirty) {
			// determine size of preset
			int size = offsetof(Preset, data) + preset.nameLength + this->effectInfos[preset.effectIndex].parameterInfos.size() - 1;

			// save preset
			co_await storage.write(this->directory[presetIndex], &preset, size, result);
			preset.dirty = false;
		}
	}

	// save preset directory
	if (this->directoryDirty) {
		co_await storage.write(DIRECTORY_ID, this->directory, this->presetCount, result);
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


/*
AwaitableCoroutine EffectManager::loadPreset(int presetInex) {
	int result;
	int id = directory[presetIndex];
	auto &preset = this->presetList[presetInex];

	// load preset
	co_await storage.read(directory[presetInex], &preset, sizeof(Preset), result);

	// make sure effect index is valid
	preset.effectIndex = std::clamp(int(preset.effectIndex), 0, this->effectInfos.size() - 1);

	// todo: sanitize parameters
}

int EffectManager::newPreset() {
	int presetIndex = this->presetCount;
	auto &preset = this->presetList[presetIndex];

	// initialize preset
	preset.effectIndex = 0;
	initParameters(presetIndex);

	// find free id
	this->directory[presetIndex] = findFreeId();

	return presetIndex;
}

AwaitableCoroutine EffectManager::savePreset(int presetIndex) {
	int result;
	auto &preset = this->presetList[presetIndex];

	// determine size of preset
	int size = offsetof(Preset, parameters) + this->effectInfos[preset.effectIndex].parameterInfos.size();

	// save effect parameters
	co_await storage.write(this->directory[presetIndex], &preset, size, result);

	// save directory if it was a new effect
	if (presetIndex >= this->presetCount) {
		this->presetCount = presetIndex + 1;
		co_await storage.write(DIRECTORY_ID, this->directory, this->presetCount, result);
	}
}

AwaitableCoroutine EffectManager::deletePreset(int presetIndex) {
	int result;
	for (int i = presetIndex + 1; i < this->presetCount; ++i) {
		this->directory[i - 1] = this->directory[i];
		this->presetList[i - 1] = this->presetList[i];
	}
	--this->presetCount;

	// save directory
	co_await storage.write(DIRECTORY_ID, this->directory, this->presetCount, result);
}
*/

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
	int parametersOffset = preset.nameLength - 1;
	for (int parameterIndex = 1; parameterIndex < parameterInfos.size(); ++parameterIndex) {
		auto &parameterInfo = parameterInfos[parameterIndex];
		uint8_t &parameter = preset.data[parametersOffset + parameterIndex];
		void *effectParameter = this->effectParameters + parameterInfo.offset;

		switch (parameterInfo.type) {
		case ParameterInfo::Type::COUNT_20:
			parameter = *reinterpret_cast<int *>(effectParameter);
			break;
		case ParameterInfo::Type::SHORT_DURATION_E12:
		case ParameterInfo::Type::LONG_DURATION_E12:
			parameter = toE12(reinterpret_cast<Milliseconds<> *>(effectParameter)->value);
			break;
		case ParameterInfo::Type::PERCENTAGE_E12:
			parameter = toE12(*reinterpret_cast<int *>(effectParameter) * 1000 / 4095);
			break;
		case ParameterInfo::Type::PERCENTAGE_2:
		case ParameterInfo::Type::PERCENTAGE_5:
			parameter = *reinterpret_cast<int *>(effectParameter) * 100 / 4095;
			break;
		case ParameterInfo::Type::HUE:
			parameter = *reinterpret_cast<int *>(effectParameter) >> 6;
			break;
		}
	}
}

EffectManager::ParameterValue EffectManager::updateParameter(int presetIndex, int parameterIndex, int delta) {
	auto &preset = this->presetList[presetIndex];
	auto &parameterInfo = this->effectInfos[preset.effectIndex].parameterInfos[parameterIndex];

	if (delta != 0) {
		if (parameterIndex == 0) {
			// start save timer
			this->timeout = this->loop.now() + SAVE_TIMEOUT;
			this->timerBarrier.doAll();
		} else {
			// mark preset as "dirty", to be saved on next save operation
			preset.dirty = true;
		}
	}
	int parametersOffset = preset.nameLength - 1;
	uint8_t &parameter = parameterIndex == 0 ? this->global.brightness : preset.data[parametersOffset + parameterIndex];
	void *effectParameter = this->effectParameters + parameterInfo.offset;

	// update parameter and convert/assign to effect parameter
	switch (parameterInfo.type) {
	case ParameterInfo::Type::COUNT_20:
		parameter = std::clamp(parameter + delta, 1, 20);
		*reinterpret_cast<int *>(effectParameter) = parameter;
		break;
	case ParameterInfo::Type::SHORT_DURATION_E12:
		parameter = std::clamp(parameter + delta, 0, 36);
		*reinterpret_cast<Milliseconds<> *>(effectParameter) = MillisecondsE12{parameter}.get() * 1ms;
		break;
	case ParameterInfo::Type::LONG_DURATION_E12:
		parameter = std::clamp(parameter + delta, 12, 60);
		*reinterpret_cast<Milliseconds<> *>(effectParameter) = MillisecondsE12{parameter}.get() * 1ms;
		break;
	case ParameterInfo::Type::PERCENTAGE_E12:
		parameter = std::clamp(parameter + delta, 0, 24);
		*reinterpret_cast<int *>(effectParameter) = PercentageE12{parameter}.get() * 4095 / 1000;
		break;
	case ParameterInfo::Type::PERCENTAGE_2:
		parameter = std::clamp(parameter + delta * 2, 0, 100);
		*reinterpret_cast<int *>(effectParameter) = parameter * 4095 / 100;
		break;
	case ParameterInfo::Type::PERCENTAGE_5:
		parameter = std::clamp(parameter + delta * 5, 0, 100);
		*reinterpret_cast<int *>(effectParameter) = parameter * 4095 / 100;
		break;
	case ParameterInfo::Type::HUE:
		parameter = (parameter + delta + 240) % 24;
		*reinterpret_cast<int *>(effectParameter) = parameter << 6;
		break;
	}
	return {parameterInfo.type, int(parameter)};
}

AwaitableCoroutine EffectManager::run(int presetIndex) {
	// stop current effect
	this->effect.destroy();

	// wait until strip is ready
	co_await strip.wait();


	auto &effectInfo = this->effectInfos[this->presetList[presetIndex].effectIndex];

	// transfer effect parameters from preset
	for (int parameterIndex = 0; parameterIndex < effectInfo.parameterInfos.size(); ++parameterIndex)
		updateParameter(presetIndex, parameterIndex, 0);

	// start new effect
	this->effect = effectInfo.run(loop, strip, this->effectParameters);
}

AwaitableCoroutine EffectManager::stop() {
	// stop current effect
	this->effect.destroy();

	// wait until strip is ready
	co_await strip.wait();

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
