#pragma once

#include "Strip.hpp"
#include <coco/Loop.hpp>
#include <coco/Coroutine.hpp>
#include <coco/String.hpp>


using namespace coco;


struct ParameterInfo {
	enum class Type {
		// count
		COUNT,

		// duration in E12 series steps (10ms, 12ms, 15ms ..., 0 - 72)
		DURATION_E12,

		// percentage (0% - 100%, 0 - 100)
		PERCENTAGE,

	    // percentage in E12 series steps (1.0%, 1.2%, 1.5% ... 82%, 100%, 0 - 24)
		PERCENTAGE_E12,

		// color hue (0 - 23)
		HUE,
	};

	String name;
	Type type;
	uint8_t min;
	uint8_t max;
	uint8_t step;
	bool wrap;
};

using Hue = int;

using InitFunction = void (*)(void *parameters);
using EndFunction = bool (*)(float time, const void *parameters);
using RunFunction = void (*)(Strip &strip, float brightness, float time, const void *parameters);

struct EffectInfo {
	String name;
	Array<const ParameterInfo> parameterInfos;
	//int parametersSize;
	InitFunction init;
	EndFunction end;
	RunFunction run;
};
