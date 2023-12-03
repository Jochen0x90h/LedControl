#pragma once

#include "EffectInfo.hpp"
#include "Strip.hpp"
#include <coco/BufferStorage.hpp>
#include <coco/Buffer.hpp>
#include <coco/Loop.hpp>
#include <coco/Coroutine.hpp>


using namespace coco;

// preset (effect and parameters), gets stored in flash
struct Preset {
	uint8_t effectIndex;

	// length of name
	uint8_t nameLength;

	// name and parameters
	uint8_t data[30];

	bool dirty;
};

class EffectManager {
public:
	static constexpr int GLOBAL_ID = 0;
	static constexpr int DIRECTORY_ID = 100;
	static constexpr int FIRST_PRESET_ID = 101;
	static constexpr int LAST_PRESET_ID = FIRST_PRESET_ID + 98;
	static constexpr int MAX_PRESET_COUNT = 32;
	static constexpr Milliseconds<> SAVE_TIMEOUT = 2s;


	EffectManager(Loop &loop, const BufferStorage::Info &storageInfo, Buffer &flashBuffer, Strip &strip,
		Array<const EffectInfo> effectInfos)
		: loop(loop)
		, storage(storageInfo, flashBuffer)
		, strip(strip)
		, effectInfos(effectInfos)
		, timerCoroutine(saveTimer())
	{}

	~EffectManager() {
		this->timerCoroutine.destroy();
	}

	/**
	 * Load all presets from flash
	 */
	[[nodiscard]] AwaitableCoroutine load();

	/**
	 * Save all presets with dirty flag to flash
	 */
	[[nodiscard]] AwaitableCoroutine save();

	/**
	 * Add a new preset at the end of the list. Make sure getPresetCount() is < MAX_PRESET_COUNT
	 * @return index of new preset
	 */
	int addPreset();

	/**
	 * Delete a preset
	 */
	void deletePreset(int presetIndex);


	//[[nodiscard]] AwaitableCoroutine loadPreset(int presetInex);

	/**
	 * Save a preset, also makes newPreset() permanent
	 */
	//[[nodiscard]] AwaitableCoroutine savePreset(int presetIndex);

	/**
	 * Delete a preset
	 */
	//[[nodiscard]] AwaitableCoroutine deletePreset(int presetIndex);

	/**
	 * Add a delta to an effect index of a preset
	 */
	void updateEffect(int presetIndex, int delta);

	/**
	 * Set default values for the parameters
	 */
	void initParameters(int presetIndex);

	struct ParameterValue {
		ParameterInfo::Type type;
		int value;
	};

	/**
	 * Add a delta to a parameter of a preset, clamp to valid range and return its current value
	 */
	ParameterValue updateParameter(int presetIndex, int parameterIndex, int delta);

	/**
	 * Run the given preset
	 */
	[[nodiscard]] AwaitableCoroutine run(int presetIndex);

	/**
	 * Stop the current preset
	 */
	[[nodiscard]] AwaitableCoroutine stop();



	/**
	 * Size of presets list
	 */
	int getPresetCount() {return this->presetCount;}

	/**
	 * Get the name of the given preset. The effect name is the default name
	 */
	String getPresetName(int presetIndex) {
		// use effect name for now
		return this->effectInfos[this->presetList[presetIndex].effectIndex].name;
	}

	/**
	 * Number of effect parameters in the given preset
	 */
	int getParameterCount(int presetIndex) {
		return this->effectInfos[this->presetList[presetIndex].effectIndex].parameterInfos.size();
	}

	/**
	 * Get the name of the given parameter of the given preset
	 */
	String getParameterName(int presetIndex, int parameterIndex) {
		return this->effectInfos[this->presetList[presetIndex].effectIndex].parameterInfos[parameterIndex].name;
	}


	int getPresetIndex() {return this->global.presetIndex;}
	void setPresetIndex(int index);
protected:

	int findFreeId();

	/**
	 * Timer that saves parameter changes to flash after some time
	 */
	Coroutine saveTimer();


	Loop &loop;
	BufferStorage storage;
 	Strip &strip;

	// list of all effects
	Array<const EffectInfo> effectInfos;

	// preset directory in flash
	uint8_t directory[MAX_PRESET_COUNT];
	bool directoryDirty = false;

	// list of all presets
	int presetCount = 0;
	Preset presetList[MAX_PRESET_COUNT];

	// current preset
	//int presetIndex = -1;

	//uint8_t globalBrightness = 24;

	struct Global {
		uint8_t presetIndex;
		uint8_t brightness;
	};
	Global global;

	// effect parameters (get converted from current preset)
	alignas(4) uint8_t effectParameters[128];

	Coroutine effect;

	// save timeout
	Loop::Time timeout;

	// timer waits on barrier until it gets started
	Barrier<> timerBarrier;

	// must be last
	Coroutine timerCoroutine;
};
