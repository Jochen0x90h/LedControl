#pragma once

#include <coco/Loop.hpp>
#include <coco/Time.hpp>
#include "E12.hpp"


using namespace coco;


/**
 * Sleep for effects that prevents "getting stuck" in long delays while editing the delay
 */
[[nodiscard]] inline AwaitableCoroutine sleep(Loop &loop, const Milliseconds<> &delay) {
	auto start = loop.now();
	auto time = start;
	while (true) {
		//Milliseconds<> d(delay.get());
		auto end = start + delay;
		time += 1s;
		if (time >= end) {
			co_await loop.sleep(end);
			break;
		}
		co_await loop.sleep(time);
	}
}



/**
	Timer for effect animations which is "immune" to on-the-fly changes of the duration
*/
class Timer {
public:
	Timer(Loop &loop) : start(loop.now()), residual(0) {}

	/**
		Get timer value
		@param loop event loop
		@param duration duration
		@param value to reach after given duration
	*/
	int get(Loop &loop, const Milliseconds<> &duration, int value) {
		auto now = loop.now();
		auto d = (now - this->start) * value + this->residual;
		this->start = now;

		this->residual = d % duration;
		this->x += (d / duration).value;

		return x;
	}

protected:
	Loop::Time start;
	Milliseconds<> residual;
	int x = 0;
};
