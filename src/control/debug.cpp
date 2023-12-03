#include <coco/Time.hpp>
#include <coco/platform/gpio.hpp>


namespace coco {
namespace debug {

void init() {
}

void set(uint32_t bits, uint32_t function) {
}

void sleep(Microseconds<> time) {
	int64_t count = int64_t(23) * time.value;
	for (int64_t i = 0; i < count; ++i) {
		__NOP();
	}
}

} // debug
} // coco
