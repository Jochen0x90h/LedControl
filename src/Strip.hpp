#pragma once

#include <coco/Buffer.hpp>
#include <coco/Color.hpp>


using namespace coco;


/**
 * LED strip with double buffering
 */
class Strip {
public:
	Strip(Buffer &buffer1, Buffer &buffer2) : buffers{&buffer1, &buffer2} {
		this->count = std::min(buffer1.capacity(), buffer2.capacity()) / sizeof(Color);
		this->buffer = &buffer1;
	}

	int size() {return this->count;}

	// get array of colors, only valid until show() is called
	Array<Color> array() {
		return {this->buffer->pointer<Color>(), this->count};
	}

	void set(int index, uint8_t red, uint8_t green, uint8_t blue) {
		array()[index] = Color{red, green, blue};
	}

	void set(int index, Color color) {
		array()[index] = color;
	}

	void clear() {
		array().fill(Color{0, 0, 0});
	}

	void fill(uint8_t red, uint8_t green, uint8_t blue) {
		array().fill(Color{red, green, blue});
	}

	void fill(Color color) {
		array().fill(color);
	}

	[[nodiscard]] Awaitable<> show() {
		// start transferring the buffer to the LED strip
		this->buffer->startWrite(this->count * sizeof(Color));

		// toggle buffer
		this->buffer = this->buffer == this->buffers[0] ? this->buffers[1] : this->buffers[0];

		// wait until other buffer is finished
		return this->buffer->untilReadyOrDisabled();
	}

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
