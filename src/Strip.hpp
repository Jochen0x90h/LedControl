#pragma once

#include <coco/Buffer.hpp>
#include "../Color.hpp"


using namespace coco;


/**
 * LED strip with double buffering
 */
class Strip {
public:
	Strip(Buffer &buffer1, Buffer &buffer2) : buffers{&buffer1, &buffer2} {
		this->count = std::min(buffer1.capacity(), buffer2.capacity()) / sizeof(Color<uint8_t>);
		this->buffer = &buffer1;
	}

	int size() {return this->count;}

	// get array of colors, only valid until show() is called
	Array<Color<uint8_t>> array() {
		return {this->buffer->pointer<Color<uint8_t>>(), this->count};
	}

	void clear() {
		array().fill(Color<uint8_t>{0, 0, 0});
	}

	void clear(int startIndex, int endIndex) {
		auto a = array();
		for (int i = startIndex; i < endIndex; ++i)
			a[i] = {0, 0, 0};
	}

	//void set(int index, uint8_t red, uint8_t green, uint8_t blue) {
	//	array()[index] = Color<uint8_t>{red, green, blue};
	//}

	/**
	 * Set 8 bit color (0 - 255)
	 */
	void set(int index, Color<uint8_t> color) {
		array()[index] = color;
	}

	/**
	 * Set 12 bit color (0 - 4095)
	 */
	void set(int index, Color<int> color) {
		array()[index] = Color<uint8_t>(color >> 4);
	}

	void setSafe(int index, Color<int> color) {
		auto a = array();
		if (unsigned(index) < unsigned(a.size()))
			array()[index] = Color<uint8_t>(color >> 4);
	}

	/**
	 * Set 8 bit color (0 - 255)
	 */
	void set(int startIndex, int endIndex, Color<uint8_t> color) {
		auto a = array();
		for (int i = startIndex; i < endIndex; ++i)
			a[i] = color;
	}

	/**
	 * Set 12 bit color (0 - 4095)
	 */
	void set(int startIndex, int endIndex, Color<int> color) {
		auto a = array();
		Color<uint8_t> c(color >> 4);
		for (int i = startIndex; i < endIndex; ++i)
			a[i] = c;
	}

	void setSafe(int startIndex, int endIndex, Color<int> color) {
		auto a = array();
		Color<uint8_t> c(color >> 4);
		int s = std::max(startIndex, 0);
		int e = std::min(endIndex, a.size());
		for (int i = s; i < e; ++i)
			a[i] = c;
	}

	//void fill(uint8_t red, uint8_t green, uint8_t blue) {
	//	array().fill(Color<uint8_t>{red, green, blue});
	//}

	/**
	 * Fill 8 bit color (0 - 255)
	 */
	void fill(Color<uint8_t> color) {
		array().fill(color);
	}

	/**
	 * Fill 12 bit color (0 - 4095)
	 */
	void fill(Color<int> color) {
		Color<uint8_t> c(color >> 4);
		array().fill(c);
	}

	/**
	 * Show contents of strip on LEDs
	 */
	[[nodiscard]] Awaitable<> show() {
		// WS2812: swap first and second byte of RGB colors
		auto a = array();
		for (Color<uint8_t> &c : a) {
			std::swap(c.r, c.g);
		}

		// start transferring the buffer to the LED strip
		this->buffer->startWrite(this->count * sizeof(Color<uint8_t>));

		// toggle buffer
		this->buffer = this->buffer == this->buffers[0] ? this->buffers[1] : this->buffers[0];

		// wait until other buffer is finished
		return this->buffer->untilReadyOrDisabled();
	}

	/**
	 * Use co_await to wait until the strip is ready again
	 */
	[[nodiscard]] Awaitable<> wait() {
		return this->buffer->untilReadyOrDisabled();
	}

protected:
	// number of LEDs
	int count;

	// double buffer
	Buffer *buffers[2];
	Buffer *buffer;
};
