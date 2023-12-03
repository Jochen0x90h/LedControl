#pragma once

#include "Strip.hpp"
#include <coco/Loop.hpp>
#include <coco/Coroutine.hpp>
#include <coco/String.hpp>


using namespace coco;


struct ParameterInfo {
	enum class Type {
		// count 1 - 20
		COUNT_20,

		// short duration 10ms - 10s with E12 series steps (10ms, 12ms, 15ms ..., 0 - 36)
		SHORT_DURATION_E12,

		// long duration 100ms - 1000s with E12 series steps (100ms, 120ms, 150ms ..., 12 - 60)
		LONG_DURATION_E12,

	    // percentage 0 - 100% with E12 series steps (1.0%, 1.2%, 1.5% ... 82%, 100%, 0 - 24)
		PERCENTAGE_E12,

		// percentage 0 - 100% with 2% steps
		PERCENTAGE_2,

		// percentage 0 - 100% with 5% steps
		PERCENTAGE_5,

		// color hue 0 - 23
		HUE,
	};

	String name;
	Type type;
	int offset;
};

using Hue = int;

using InitFunction = void (*)(void *);
//using LimitFunction = void (*)(void *);
using EffectFunction = Coroutine (*)(Loop &, Strip &, const void *);

struct EffectInfo {
	String name;
	Array<const ParameterInfo> parameterInfos;
	int parametersSize;
	InitFunction init;
	//LimitFunction limit;
	EffectFunction run;
};
