#pragma once

#include <coco/Buffer.hpp>
#include "math.hpp"
#include <config.hpp> // platform dependent defines (e.g. SWAP_R_G)


using namespace coco;


/// @brief LED strip with double buffering
///
class Strip {
public:
    Strip(Buffer &buffer1, Buffer &buffer2) : buffers{&buffer1, &buffer2} {
        this->count = std::min(buffer1.capacity(), buffer2.capacity()) / 3;
        this->buffer = &buffer1;
    }

    int size() {return this->count;}

	/// @brief Clear the buffer
    ///
	void clear() {
        this->buffer->array<uint8_t>().fill(0);
    }

    /// @brief Set float color (0.0f - 1.0f)
    /// @param index index of LED
    /// @param color color of LED
    void set(int index, float3 color) {
        uint8_t *b = this->buffer->data() + index * 3;
        b[0] = clamp(int(color.x * 255.0f + 0.5f), 0, 255);
        b[1] = clamp(int(color.y * 255.0f + 0.5f), 0, 255);
        b[2] = clamp(int(color.z * 255.0f + 0.5f), 0, 255);
    }

    /// @brief Show contents of strip on LEDs
    /// @return use co_await to wait until show() is done
    [[nodiscard]] Awaitable<Buffer::Events> show() {
        // WS2812: swap first and second byte of RGB colors
#ifdef SWAP_R_G
        auto a = this->buffer->array<Vector3<uint8_t>>();
        for (Vector3<uint8_t> &c : a) {
            std::swap(c.x, c.y);
        }
#endif

        // start transferring the buffer to the LED strip
        this->buffer->startWrite(this->count * 3);

        // toggle buffer
        this->buffer = this->buffer == this->buffers[0] ? this->buffers[1] : this->buffers[0];

        // wait until other buffer is finished
        return this->buffer->untilReadyOrDisabled();
    }

    /// @brief Wait until strip is ready
    /// @return Use co_await to wait until the strip is ready again
    [[nodiscard]] Awaitable<Buffer::Events> untilReady() {
        return this->buffer->untilReadyOrDisabled();
    }

protected:
    // number of LEDs
    int count;

    // double buffer
    Buffer *buffers[2];
    Buffer *buffer;
};
