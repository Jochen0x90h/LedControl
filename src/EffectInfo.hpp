#pragma once

#include <coco/String.hpp>


using namespace coco;


struct ParameterInfo {
	enum class Type {
		// integer
		INTEGER,

		// logarithmic duration in milliseconds using E12 series
		E12_MILLISECONDS,

		// color
		COLOR,
	};

	String name;
	Type type;
	int offset;
};

using InitFunction = void (*)(void *);
using LimitFunction = void (*)(void *);
using EffectFunction = Coroutine (*)(Loop &, Strip &, void *);

struct EffectInfo {
	String name;
	int parameterCount;
	const ParameterInfo *parameterInfos;
	int parametersSize;
	InitFunction init;
	LimitFunction limit;
	EffectFunction run;
};
