#include "Strip.hpp"
#include "Timer.hpp"
#include "effects/ColorFade.hpp"
#include "effects/RgbFade.hpp"
#include "effects/Strobe.hpp"
#include "effects/CylonBounce.hpp"
#include "effects/MeteorRain.hpp"
#include <coco/Menu.hpp>
#include <coco/font/tahoma8pt.hpp>
#include <coco/Color.hpp>
#include <coco/PseudoRandom.hpp>
#include <coco/StreamOperators.hpp>
#include <coco/noise.hpp>
#include <Drivers.hpp>
#include <coco/debug.hpp>


/*
	LED strip effects
	https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/
*/

using namespace coco;




// list of all effects
const EffectInfo effectInfos[] = {
	RgbFade::info,
	ColorFade::info,
	Strobe::info,
	CylonBounce::info,
	MeteorRain::info
};



struct BaseParameters {
	uint8_t effectIndex;
	//uint8_t id;

	// name
};

struct Parameters : public BaseParameters {
	alignas(4) uint8_t buffer[128];
};

class EffectManager {
public:
	// commands
	static constexpr int NEW = 0x01;
	static constexpr int ERASE = 0x02;
	static constexpr int LOAD = 0x04;
	static constexpr int SAVE = 0x08;
	static constexpr int STOP = 0x10;
	static constexpr int RUN = 0x20;


	// list of effects
	BaseParameters effectList[32];
	int effectCount = 0;

	// parameters of current effect
	Parameters parameters;

	Barrier<> barrier;


	void doCommand(int flags) {
		this->flags = flags;
		this->commandBarrier.doFirst();
	}

	void doCommand(int flags, int index) {
		this->flags = flags;
		this->index = index;
		this->commandBarrier.doFirst();
	}

	Coroutine run(Loop &loop, Buffer &flashBuffer, Strip &strip) {
		Storage_Buffer storage(storageInfo, flashBuffer);
		int result;

		uint8_t directory[32];

		co_await storage.mount(result);

		// load directory
		co_await storage.read(0, directory, sizeof(directory), result);

		int count = std::clamp(result, 0, int(sizeof(directory)));
		this->effectCount = count;

		// load effect list
		for (int i = 0; i < this->effectCount; ++i) {
			int id = directory[i];
			co_await storage.read(id, &this->effectList[i], sizeof(BaseParameters), result);
		}


		Coroutine effect;
		while (true) {
			// notify application that we are finished
			if (this->flags == 0) {
				this->barrier.doAll();

				// wait until a new command arrives
				do {
					co_await this->commandBarrier.wait();
				} while (this->flags == 0);
			}

			int flags = this->flags;
			int flag = flags & ~(flags - 1);
			this->flags = flags & ~flag;
			int index = this->index;
			switch (flag) {
			case NEW:
				// create new effect
				this->parameters.effectIndex = 0;
				effectInfos[0].init(this->parameters.buffer);
				/*memset(&this->parameters, 0, sizeof(Parameters));
				{
					auto &p = reinterpret_cast<RgbFadeParameters &>(parameters.buffer);
					p.duration = 20;
				}*/

				// find free id
				directory[count] = findFreeId(directory, count);

				this->index = count;
				break;
			case ERASE:
				// erase effect from directory and effectList
				for (int i = index + 1; i < count; ++i) {
					directory[i - 1] = directory[i];
					this->effectList[i - 1] = this->effectList[i];
				}
				--count;
				this->effectCount = count;

				// save directory
				co_await storage.write(0, directory, count, result);
				break;
			case LOAD:
				// load effect
				co_await storage.read(directory[index], &this->parameters, sizeof(Parameters), result);
				this->parameters.effectIndex = std::min(this->parameters.effectIndex, uint8_t(std::size(effectInfos) - 1));
				break;
			case SAVE:
				// save effect
				{
					int size = effectInfos[this->parameters.effectIndex].parametersSize;

					// save effect parameters
					co_await storage.write(directory[index], &this->parameters, offsetof(Parameters, buffer) + size, result);

					// save directory if it was a new effect
					if (index >= count) {
						this->effectCount = count = index + 1;
						co_await storage.write(0, directory, count, result);
					}

					this->effectList[index] = this->parameters;
				}
				break;
			case STOP:
				effect.destroy();
				co_await strip.wait();
				strip.clear();
				co_await strip.show();
				break;
			case RUN:
				effect.destroy();
				co_await strip.wait();
				effect = effectInfos[this->parameters.effectIndex].run(loop, strip, this->parameters.buffer);
				break;
			}
		}
	}


protected:

	int findFreeId(uint8_t *directory, int count) {
		for (int j = 1; j < 256; ++j) {
			for (int i = 0; i < count; ++i) {
				if (directory[i] == j)
					goto found;
			}
			return j;
		found:
			;
		}
		return -1;
	}

	int flags = 0;
	int index = 0;
	Barrier<> commandBarrier;
};



// Menu
// ----

AwaitableCoroutine parametersMenu(Loop &loop, SSD130x &display, Input &buttons, EffectManager &effectManager) {
	int effectIndex = effectManager.parameters.effectIndex;

	// menu
	Menu menu(display, coco::tahoma8pt);
	while (true) {
		// build menu
		menu.begin(buttons);

		// select effect
		{
			int edit = menu.edit(1);
			if (edit > 0 && menu.delta() != 0) {
				// change effect
				effectIndex = (effectIndex + std::size(effectInfos) * 256 + menu.delta()) % std::size(effectInfos);
				effectManager.parameters.effectIndex = effectIndex;

				// set defaults
				effectInfos[effectIndex].init(effectManager.parameters.buffer);

				effectManager.doCommand(EffectManager::RUN);
			}

			auto stream = menu.stream();
			stream << "Effect: " << underline(effectInfos[effectIndex].name, edit > 0);
			menu.entry();
		}

		// edit parameters
		auto &effectInfo = effectInfos[effectIndex];
		for (int i = 0; i < effectInfo.parameterCount; ++i) {
			auto &parameterInfo = effectInfo.parameterInfos[i];

			auto stream = menu.stream();
			stream << parameterInfo.name << ": ";
			switch (parameterInfo.type) {
			case ParameterInfo::Type::INTEGER:
				{
					int &value = *reinterpret_cast<int *>(effectManager.parameters.buffer + parameterInfo.offset);
					int edit = menu.edit(1);
					if (edit > 0) {
						value += menu.delta();
						effectInfo.limit(effectManager.parameters.buffer);
					}
					stream << underline(dec(value), edit > 0);
				}
				break;
			case ParameterInfo::Type::E12_MILLISECONDS:
				{
					E12Milliseconds &duration = *reinterpret_cast<E12Milliseconds *>(effectManager.parameters.buffer + parameterInfo.offset);
					int edit = menu.edit(1);
					if (edit > 0) {
						duration.value += menu.delta();
						effectInfo.limit(effectManager.parameters.buffer);
					}
					stream << underline(duration, edit > 0) << 's';
				}
				break;
			case ParameterInfo::Type::COLOR:
				{
					Color &color = *reinterpret_cast<Color *>(effectManager.parameters.buffer + parameterInfo.offset);
					int edit = menu.edit(3);
					if (edit > 0) {
						color[edit - 1] = std::clamp(int(color[edit - 1] + menu.delta()), 0, 255);
					}
					stream << underline(dec(color.r), edit == 1) << ' ' << underline(dec(color.g), edit == 2) << ' ' << underline(dec(color.b), edit == 3);
				}
				break;
			}
			menu.entry();
		}

		if (menu.entry("Save")) {
			effectManager.doCommand(EffectManager::SAVE);
			co_return;
		}

		if (menu.entry("Cancel")) {
			co_return;
		}

		// show on display and wait for input
		co_await select(menu.show(), buttons.stateChange());
	}
}

Coroutine effectsMenu(Loop &loop, SSD130x &display, Input &buttons, EffectManager &effectManager) {
	// initialize and enable the display
	co_await drivers.resetDisplay();
	co_await display.init();
	co_await display.enable();

	int effectIndex = -1;

	// menu
	Menu menu(display, coco::tahoma8pt);
	while (true) {
		// build menu
		menu.begin(buttons);

		// list effects
		for (int i = 0; i < effectManager.effectCount; ++i) {
			if (menu.isSelected()) {
				// run currently selected effect
				if (effectIndex != i) {
					effectIndex = i;
					effectManager.doCommand(EffectManager::LOAD | EffectManager::RUN, i);
				}
			}

			auto stream = menu.stream();
			int effectIndex = effectManager.effectList[i].effectIndex;
			stream << effectInfos[effectIndex].name;//"Effect " << dec(i);
			if (menu.entry()) {
				co_await parametersMenu(loop, display, buttons, effectManager);
				effectIndex = -1;
			}
		}


		if (menu.isSelected()) {
			// stop effect
			if (effectIndex != effectManager.effectCount) {
				effectIndex = effectManager.effectCount;
				effectManager.doCommand(EffectManager::STOP);
			}
		}
		if (menu.entry("New...")) {
			effectManager.doCommand(EffectManager::NEW | EffectManager::RUN);
			co_await parametersMenu(loop, display, buttons, effectManager);
			effectIndex = -1;
		}

		// show on display and wait for input
		co_await select(menu.show(), buttons.stateChange());
	}
}






//------------
/*

enum class Command {
	NEXT,
	PREV,
	SAVE,

	CANCEL,
};


struct RgbFade {
	int duration = 0;

	Coroutine run(Loop &loop, Strip &strip) {
		while (true) {
			for (int j = 0; j < 3; j++) {
				Timer timer(loop);
				while (true) {
					int x = timer.get(loop, MilliSeconds(e12<-3>(this->duration)), 512);

					if (x >= 512)
						break;
					int k = x < 256 ? x : 511 - x;

					switch(j) {
					case 0: strip.fill(k, 0, 0); break;
					case 1: strip.fill(0, k, 0); break;
					case 2: strip.fill(0, 0, k); break;
					}
					co_await strip.show();
				}
			}
		}
	}

	// configure menu
	AwaitableCoroutine configure(Loop &loop, SSD130x &display, Input &buttons, Strip &strip, Command &command) {
		// load from flash

		// run effect
		Coroutine effect = run(loop, strip);

		Menu menu(display, coco::tahoma8pt);
		while (true) {
			menu.begin(buttons);

			// select effect
			{
				int edit = menu.edit(1);
				if (edit > 0 && menu.delta() != 0) {
					command = menu.delta() > 0 ? Command::NEXT : Command::PREV;
					break;
				}
				auto stream = menu.stream();
				stream << underline("RGB Fade", edit > 0);
				menu.entry();
			}

			// name

			// duration
			{
				int edit = menu.edit(1);
				if (edit > 0) {
					this->duration = std::clamp(this->duration + menu.delta(), 0, 100);
				}
				auto stream = menu.stream();
				stream << "Duration: " << underline(e12<-3>(this->duration), edit > 0) << 's';
				menu.entry();
			}

			if (menu.entry("Cancel")) {
				command = Command::CANCEL;
				break;
			}
			if (menu.entry("Save")) {
				command = Command::SAVE;
				break;
			}

			// show on display and wait for input
			co_await select(menu.show(), buttons.stateChange());
		}

		// destroy effect
		effect.destroy();

		// store to flash

	}

	// control menu
	AwaitableCoroutine control(Loop &loop, SSD130x &display, Input &buttons, Strip &strip) {
	}
};


struct ColorFade {
	Color color = {255, 255, 255};
	int duration = 0;

	Coroutine run(Loop &loop, Strip &strip) {
		while (true) {
			Timer timer(loop);
			while (true) {
				int x = timer.get(loop, MilliSeconds(e12<-3>(this->duration)), 512);

				if (x >= 512)
					break;
				int k = x < 256 ? x : 511 - x;

				strip.fill(this->color);
				co_await strip.show();
			}
		}
	}

	// configure menu
	AwaitableCoroutine configure(Loop &loop, SSD130x &display, Input &buttons, Strip &strip, Command &command) {
		// load from flash

		// run effect
		Coroutine effect = run(loop, strip);

		Menu menu(display, coco::tahoma8pt);
		while (true) {
			menu.begin(buttons);

			// select effect
			{
				int edit = menu.edit(1);
				if (edit > 0 && menu.delta() != 0) {
					command = menu.delta() > 0 ? Command::NEXT : Command::PREV;
					break;
				}
				auto stream = menu.stream();
				stream << underline("Color Fade", edit > 0);
				menu.entry();
			}

			// name

			// color
			{
				int edit = menu.edit(3);
				if (edit > 0) {
					this->color[edit - 1] = std::clamp(int(this->color[edit - 1] + menu.delta()), 0, 255);
				}
				auto stream = menu.stream();
				stream << "Color: " << underline(dec(this->color.r), edit == 1) << ' ' << underline(dec(this->color.g), edit == 2) << ' ' << underline(dec(this->color.b), edit == 3);
				menu.entry();
			}

			// duration
			{
				int edit = menu.edit(1);
				if (edit > 0) {
					this->duration = std::clamp(this->duration + menu.delta(), 0, 100);
				}
				auto stream = menu.stream();
				stream << "Duration: " << underline(e12<-3>(this->duration), edit > 0) << 's';
				menu.entry();
			}

			if (menu.entry("Cancel")) {
				command = Command::CANCEL;
				break;
			}
			if (menu.entry("Save")) {
				command = Command::SAVE;
				break;
			}

			// show on display and wait for input
			co_await select(menu.show(), buttons.stateChange());
		}

		// destroy effect
		effect.destroy();

		// store to flash

	}

	// control menu
	AwaitableCoroutine control(Loop &loop, SSD130x &display, Input &buttons, Strip &strip) {
	}
};




Coroutine mainMenu(Loop &loop, SSD130x &display, Input &buttons, Strip &strip, Buffer &flashBuffer) {
	Storage_Buffer storage(storageInfo, flashBuffer);

	// wait for reset circuit (APX803) on the display board
	co_await loop.sleep(300ms);

	// initialize and enable the display
	co_await display.init();
	co_await display.enable();

	// load effect list

	//int effectIndex = 0;
	int effectTypes[2] = {0, 1};
	int effectCount = 2;

	// menu
	Menu menu(display, coco::tahoma8pt);
	while (true) {
		// build menu
		menu.begin(buttons);

		for (int i = 0; i < effectCount; ++i) {
			auto stream = menu.stream();
			stream << "Effect " << dec(i);
			if (menu.entry()) {
				Command command;
				while (true) {
					int &t = effectTypes[i];
					co_await strip.wait();
					if (t == 0) {
						RgbFade effect;
						co_await effect.configure(loop, display, buttons, strip, command);
					} else if (t == 1) {
						ColorFade effect;
						co_await effect.configure(loop, display, buttons, strip, command);
					}
					if (command == Command::NEXT) {
						t = (t + 1) % effectCount;
					} else if (command == Command::PREV) {
						t = (t + effectCount - 1) % effectCount;
					} else {
						break;
					}
				}
			}
		}

		//if (menu.entry("Color Fade"))
		//	;//co_await menu1(display, buttons);

		// show on display and wait for input
		co_await select(menu.show(), buttons.stateChange());
	}
}
*/



/*
Coroutine rgbFade(Loop &loop, Strip &strip, MilliSeconds duration) {
	while (true) {
		for (int j = 0; j < 3; j++) {
			auto start = loop.now();
			while (true) {
				auto now = loop.now() - start;
				int k = (now * 512) / duration;
				if (k >= 512)
					break;
				if (k >= 256)
					k = 511 - k;

				switch(j) {
				case 0: strip.fill(k, 0, 0); break;
				case 1: strip.fill(0, k, 0); break;
				case 2: strip.fill(0, 0, k); break;
				}
				co_await strip.show();
			}
		}
	}
}

Coroutine colorFade(Loop &loop, Strip &strip, Color color, MilliSeconds duration) {
	while (true) {
		auto start = loop.now();
		while (true) {
			auto now = loop.now() - start;
			int k = (now * 512) / duration;
			if (k >= 512)
				break;
			if (k >= 256)
				k = 511 - k;

			strip.fill(k * color.r >> 8, k * color.g >> 8, k * color.b >> 8);

			co_await strip.show();
		}
	}
}

Coroutine strobe(Loop &loop, Strip &strip, Color color, int strobeCount, MilliSeconds<> flashDelay, MilliSeconds<> endPause) {
	while (true) {
		for (int j = 0; j < strobeCount; j++) {
			strip.fill(color);
			co_await strip.show();
			co_await loop.sleep(flashDelay);

			strip.clear();
			co_await strip.show();
			co_await loop.sleep(flashDelay);
		}
		co_await loop.sleep(endPause);
	}
}

Coroutine cylonBounce(Loop &loop, Strip &strip, Color color, int eyeSize, MilliSeconds<> speedDelay, MilliSeconds<> returnDelay) {
	int count = strip.size();
	while (true) {
		for (int i = 0; i < count - eyeSize - 2; i++) {
			strip.clear();
			strip.set(i, color.r / 10, color.g / 10, color.b / 10);
			for (int j = 1; j <= eyeSize; j++) {
				strip.set(i+j, color);
			}
			strip.set(i + eyeSize + 1, color.r / 10, color.g / 10, color.b / 10);

			co_await strip.show();
			co_await loop.sleep(speedDelay);
		}

		co_await loop.sleep(returnDelay);

		for (int i = count - eyeSize - 2; i > 0; i--) {
			strip.clear();
			strip.set(i, color.r / 10, color.g / 10, color.b / 10);
			for(int j = 1; j <= eyeSize; j++) {
				strip.set(i+j, color);
			}
			strip.set(i + eyeSize + 1, color.r / 10, color.g / 10, color.b / 10);

			co_await strip.show();
			co_await loop.sleep(speedDelay);
		}

		co_await loop.sleep(returnDelay);
	}
}*/

Coroutine sparkle(Loop &loop, Strip &strip, Color color, int pixelCount, Milliseconds<> speedDelay) {
	XorShiftRandom random;
	int count = strip.size();
	while (true) {
		strip.clear();
		for (int i = 0; i < pixelCount; ++i) {
			int pixel = random.draw() % count;
  			strip.set(pixel, color);
		}
		co_await strip.show();
		co_await loop.sleep(speedDelay);
	}
}

Coroutine snowSparkle(Loop &loop, Strip &strip, Color color, /*int pixelCount,*/ Milliseconds<> sparkleDelay, Milliseconds<> minSpeedDelay, Milliseconds<> maxSpeedDelay) {
	XorShiftRandom random;
	int count = strip.size();
	while (true) {
		strip.fill(color);

		int pixel = random.draw() % count;
		strip.set(pixel, Color{255, 255, 255});
		co_await strip.show();
		co_await loop.sleep(sparkleDelay);


		strip.fill(color);
		co_await strip.show();
		auto speedDelay = minSpeedDelay + (maxSpeedDelay - minSpeedDelay) * int(random.draw() & 0xff) / 255;
		co_await loop.sleep(speedDelay);
	}
}

Coroutine noiseTest(Loop &loop, Strip &strip, Color color) {
	int count = strip.size();
	while (true) {
		for (int i = 0; i < count; ++i) {
			strip.set(i, color * Fixed<8>{noiseU8((i << 8) + 128)});
		}

		co_await strip.show();
	}
}

Coroutine meteorRain(Loop &loop, Strip &strip, Color color, int meteorSize, int speed) {
	int count = strip.size();
	auto start = loop.now();
	while (true) {
		// meteor position
		int p = int((loop.now() - start) * speed / 1000ms) % (count * 2);

		for (int i = 0; i < count; ++i) {
			if (i >= p - meteorSize && i < p) {
				strip.set(i, color);
			} else {
				int age = (i < p ? 0 : count * 2) + (p - meteorSize) - i;

				//int decay = age * (128 + int(random.draw() & 0x7f)) >> 8;
				int decay = age * (128 + int(noiseU8((i << 5) + 128) >> 1)) >> 8;
				int s = std::max(255 - decay, 0);

				strip.set(i, color * Fixed<8>{s});
			}
		}

		co_await strip.show();
	}
}



int main() {
	Strip strip(drivers.buffer1, drivers.buffer2);

	//fade1(drivers.loop, strip, 1s);
	//fade2(drivers.loop, strip, Color{128, 0, 255}, 1s);
	//strobe(drivers.loop, strip, Color{255, 255, 255}, 10, 50ms, 1s);
	//cylonBounce(drivers.loop, strip, Color{255, 0, 0}, 10, 10ms, 50ms);
	//sparkle(drivers.loop, strip, Color{255, 255, 255}, 5, 0ms);
	//snowSparkle(drivers.loop, strip, Color{16, 16, 16}, 20ms, 100ms, 1s);
	//noiseTest(drivers.loop, strip, Color{255, 255, 255});
	//meteorRain(drivers.loop, strip, Color{255, 255, 255}, 10, 100);

	// start effects manager and run in separate coroutine
	EffectManager effectManager;
	effectManager.run(drivers.loop, drivers.flashBuffer, strip);

	// start idle display
	SSD130x display(drivers.displayBuffer, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_FLAGS);
	effectsMenu(drivers.loop, display, drivers.buttons, effectManager);

	drivers.loop.run();
	return 0;
}
