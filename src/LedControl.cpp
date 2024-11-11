#include <LedControl.hpp> // drivers
#include "E12.hpp"
#include "EffectManager.hpp"
#include "effects/StaticColor.hpp"
#include "effects/ColorCycle3.hpp"
#include "effects/ColorFade.hpp"
#include "effects/ColorFade3.hpp"
#include "effects/Strobe.hpp"
#include "effects/CylonBounce.hpp"
#include "effects/MeteorRain.hpp"
#include "effects/Spring.hpp"
#include "effects/Summer.hpp"
#include "effects/Autumn.hpp"
#include "effects/Winter.hpp"
#include "effects/Tetris.hpp"
#include <coco/Menu.hpp>
#include <coco/font/tahoma8pt1bpp.hpp>
#include <coco/PseudoRandom.hpp>
#include <coco/StreamOperators.hpp>
#include <coco/StringBuffer.hpp>
#include <coco/math.hpp>
#include <coco/debug.hpp>


/*
	Main progam
	LED strip effects
	https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/

	Usage
	Power on: Current effect
	Wheel: Change brightness (change first parameter)
	Button press: Cycle though parameters where first parameter is the effect
	Timeout: Switch back to brightness
	Button long press: Enter configuration menu
*/

using namespace coco;




// list of all effects
const EffectInfo effectInfos[] = {
	StaticColor::info,
	ColorCycle3::info,
	ColorFade::info,
	ColorFade3::info,
	Strobe::info,
	CylonBounce::info,
	MeteorRain::info,
	Spring::info,
	Summer::info,
	Autumn::info,
	Winter::info,
	Tetris::info,
};

static const String hueNames[] = {
	"Red",
	"Warm Red",
	"Orange",
	"Warm Yellow",
	"Yellow",
	"Cool Yellow",
	"Yellow Green",
	"Warm Green",
	"Green",
	"Cool Green",
	"Green Cyan",
	"Warm Cyan",
	"Cyan",
	"Cool Cyan",
	"Blue Cyan",
	"Cool Blue",
	"Blue",
	"Warm Blue",
	"Violet",
	"Cool Magenta",
	"Magenta",
	"Warm Magenta",
	"Red Magenta",
	"Cool Red",
};


// Menu
// ----

AwaitableCoroutine parametersMenu(Loop &loop, SSD130x &display, InputDevice &buttons, EffectManager &effectManager,
	int presetIndex)
{
	//int effectIndex = effectManager.parameters.base.effectIndex;

	// menu
	Menu menu(display, coco::tahoma8pt1bpp);
	while (true) {
		// build menu
		menu.begin(buttons);

		// select effect
		{
			int edit = menu.edit(1);
			if (edit > 0 && menu.delta() != 0) {
				// change effect
				effectManager.updateEffect(presetIndex, menu.delta());
				co_await effectManager.run(presetIndex);
			}

			auto stream = menu.stream();
			stream << "Effect: " << underline(effectManager.getPresetName(presetIndex)/*effectInfos[effectIndex].name*/, edit > 0);
			menu.entry();
		}

		// edit parameters
		int parameterCount = effectManager.getParameterCount(presetIndex);
		for (int parameterIndex = 0; parameterIndex < parameterCount; ++parameterIndex) {
			// print parameter name
			auto stream = menu.stream();
			stream << effectManager.getParameterName(presetIndex, parameterIndex) << ": ";

			int edit = menu.edit(1);
			int delta = edit > 0 ? menu.delta() : 0;
			auto p = effectManager.updateParameter(presetIndex, parameterIndex, delta);
			switch (p.info.type) {
			case ParameterInfo::Type::COUNT:
				stream << underline(dec(p.value), edit > 0);
				break;
			case ParameterInfo::Type::DURATION_E12:
				stream << underline(MillisecondsE12(p.value), edit > 0) << 's';
				break;
			case ParameterInfo::Type::PERCENTAGE:
				stream << underline(dec(p.value), edit > 0) << '%';
				break;
			case ParameterInfo::Type::PERCENTAGE_E12:
				stream << underline(PercentageE12(p.value), edit > 0) << '%';
				break;
			case ParameterInfo::Type::HUE:
				stream << underline(hueNames[p.value], edit > 0);
				break;
			}
			menu.entry();
		}

		/*if (menu.entry("Save")) {
			// save preset to flash
			co_await effectManager.savePreset(presetIndex);
			co_return;
		}

		if (menu.entry("Cancel")) {
			// reload preset from flash
			co_await effectManager.stop();
			if (presetIndex < effectManager.getPresetCount()) {
				co_await effectManager.loadPreset(presetIndex);
				co_await effectManager.run(presetIndex);
			}
			co_return;
		}*/
		if (menu.entry("Exit")) {
			co_return;
		}

		if (effectManager.getPresetCount() > 1) {
			if (menu.entry("Delete")) {
				// delete preset
				co_await effectManager.stop();
				/*co_await*/ effectManager.deletePreset(presetIndex);
				if (presetIndex < effectManager.getPresetCount()) {
					co_await effectManager.run(presetIndex);
				}
				co_return;
			}
		}

		// show on display and wait for input
		co_await menu.show();
		co_await menu.untilInput(buttons);
	}
}

AwaitableCoroutine effectsMenu(Loop &loop, SSD130x &display, InputDevice &buttons, EffectManager &effectManager) {
	// initialize and enable the display
	co_await drivers.resetDisplay();
	co_await display.init();
	co_await display.enable();

	int currentPresetIndex = -1;

	// menu
	Menu menu(display, coco::tahoma8pt1bpp);
	while (true) {
		// build menu
		menu.begin(buttons);

		// list presets
		for (int presetIndex = 0; presetIndex < effectManager.getPresetCount(); ++presetIndex) {
			// run currently selected effect
			if (menu.isSelected()) {
				if (currentPresetIndex != presetIndex) {
					currentPresetIndex = presetIndex;
					co_await effectManager.run(presetIndex);
				}
			}

			auto stream = menu.stream();
			stream << effectManager.getPresetName(presetIndex);
			if (menu.entry()) {
				// edit effect parameters
				co_await parametersMenu(loop, display, buttons, effectManager, presetIndex);
			}
		}

		// stop effect when "New..." is selected
		if (menu.isSelected()) {
			if (currentPresetIndex != -1)
				co_await effectManager.stop();
			currentPresetIndex = -1;
		}
		if (effectManager.getPresetCount() < effectManager.MAX_PRESET_COUNT) {
			if (menu.entry("New...")) {
				int presetIndex = effectManager.addPreset();
				co_await effectManager.run(presetIndex);
				co_await parametersMenu(loop, display, buttons, effectManager, presetIndex);
			}
		}

		// stop effect when "Exit" is selected
/*		if (menu.isSelected()) {
			if (currentPresetIndex != -1)
				co_await effectManager.stop();
			currentPresetIndex = -1;
		}
		if (menu.entry("Exit")) {
			co_return;
		}*/

		// stop effect when "Save" is selected
		if (menu.isSelected()) {
			if (currentPresetIndex != -1)
				co_await effectManager.stop();
			currentPresetIndex = -1;
		}
		if (menu.entry("Save")) {
			co_await effectManager.save();
			co_return;
		}

		// stop effect when "Cancel" is selected
		if (menu.isSelected()) {
			if (currentPresetIndex != -1)
				co_await effectManager.stop();
			currentPresetIndex = -1;
		}
		if (menu.entry("Cancel")) {
			co_await effectManager.load();
			co_return;
		}

		// show on display and wait for input
		co_await menu.show();
		co_await menu.untilInput(buttons);
	}
}

Coroutine mainMenu(Loop &loop, SSD130x &display, InputDevice &input, EffectManager &effectManager) {
	// initialize and enable the display
	co_await drivers.resetDisplay();
	co_await display.init();
	co_await display.enable();

	// load all presets
	co_await effectManager.load();

	// load and run current effect

	int presetIndex = effectManager.getPresetIndex();
	co_await effectManager.run(presetIndex);

	bool redraw = true;

	int parameterIndex = 0;
	bool idle = true;
	bool showParameter = false;
	Loop::Time parameterTimeout;

	int8_t lastState[3];
	while (true) {
		// get button state
		if (redraw) {
			redraw = false;
			input.get(lastState);
		}
		int8_t state[3];
		int seq = input.get(state);

		// rotary knob: change parameter value
		int delta = state[0] - lastState[0];
		if (delta != 0) {
			showParameter = true;
			parameterTimeout = loop.now() + 5s;

			if (parameterIndex < 0) {
				// change preset
				presetIndex = (presetIndex + delta + effectManager.getPresetCount()) % effectManager.getPresetCount();
				effectManager.setPresetIndex(presetIndex);
				co_await effectManager.run(presetIndex);

				// redraw and continue to refresh effectInfo
				redraw = true;
				continue;
			}

			// update parameter
			effectManager.updateParameter(presetIndex, parameterIndex, delta);
		}

		// button press: change parameter index
		delta = state[1] - lastState[1];
		if (delta != 0) {
			showParameter = true;
			parameterTimeout = loop.now() + 5s;

			// select next parameter on button press
			++parameterIndex;
			if (idle || parameterIndex >= effectManager.getParameterCount(presetIndex)) {
				// select preset (parameterIndex = -1) only if there are at least two presets
				parameterIndex = effectManager.getPresetCount() >= 2 ? -1 : int(idle);
				idle = false;
			}
		}

		// long press: enter menu
		delta = state[2] - lastState[2];
		if (delta != 0) {
			co_await effectsMenu(loop, display, input, effectManager);
			presetIndex = std::max(std::min(presetIndex, effectManager.getPresetCount() - 1), 0);
			co_await effectManager.run(presetIndex);
			parameterIndex = 0;
			showParameter = false;
			redraw = true;
			idle = true;
		}

		lastState[0] = state[0];
		lastState[1] = state[1];
		lastState[2] = state[2];

		auto bitmap = display.bitmap();
		bitmap.clear();

		if (!showParameter) {
			// idle: show preset name
			String name = effectManager.getPresetName(presetIndex);
			int w = coco::tahoma8pt1bpp.calcWidth(name);
			bitmap.drawText((128 - w) >> 1, 24, coco::tahoma8pt1bpp, name);

			if (!redraw) {
				// show bitmap on display
				co_await display.show();

				// wait for input
				co_await input.untilInput(seq);
			}
		} else {
			// show parameter with bar
			int barX = 0;
			int barW;
			StringBuffer<16> value;
			if (parameterIndex < 0) {
				// show preset name
				String name = "Preset";//effectManager.getPresetName(presetIndex);
				int w = coco::tahoma8pt1bpp.calcWidth(name);
				bitmap.drawText((128 - w) >> 1, 10, coco::tahoma8pt1bpp, name);

				barW = 124 / effectManager.getPresetCount();
				barX = presetIndex * (124 - barW) / (effectManager.getPresetCount() - 1);

				value << effectManager.getPresetName(presetIndex);
			} else {
				// get parameter info and value
				auto p = effectManager.updateParameter(presetIndex, parameterIndex, 0);

				// show parameter name
				String name = p.info.name;//effectManager.getParameterName(presetIndex, parameterIndex);
				int w = coco::tahoma8pt1bpp.calcWidth(name);
				bitmap.drawText((128 - w) >> 1, 10, coco::tahoma8pt1bpp, name);

				// calc x-coordinate and widht of display bar
				{
					int value = p.value - p.info.min;
					int range = p.info.max + p.info.min;
					if (!p.info.wrap) {
						barW = value * 124 / range;
					} else {
						barW = 124 / range + 1; // width of bar is at least 1 pixel
						barX = value * (124 - barW) / range;
					}
				}

				// create display value
				switch (p.info.type) {
				case ParameterInfo::Type::COUNT:
					//barW = p.value * 124 / 20;
					value << dec(p.value);
					break;
				case ParameterInfo::Type::DURATION_E12:
					//barW = p.value * 124 / 36;
					//barW = (p.value - 12) * 124 / 48;
					value << MillisecondsE12(p.value) << "s";
					break;
				case ParameterInfo::Type::PERCENTAGE:
					//barW = p.value * 124 / 100;
					value << dec(p.value) << "%";
					break;
				case ParameterInfo::Type::PERCENTAGE_E12:
					//barW = p.value * 124 / 24;
					value << PercentageE12(p.value) << "%";
					break;
				case ParameterInfo::Type::HUE:
					//barW = 124 / 24;
					//barX = p.value * (124 - barW) / 23;
					value << hueNames[p.value];
					break;
				}
			}
			bitmap.drawRectangle(0, 32, 128, 10);
			bitmap.fillRectangle(2 + barX, 34, barW, 6);
			int w = coco::tahoma8pt1bpp.calcWidth(value);
			bitmap.drawText((128 - w) >> 1, 48, coco::tahoma8pt1bpp, value);

			if (!redraw) {
				// show bitmap on display
				co_await display.show();

				// wait for input
				int s = co_await select(input.untilInput(seq), loop.sleep(parameterTimeout));
				if (s == 1) {
					// user input: renew timeout
					parameterTimeout = loop.now() + 5s;
				} else {
					// timeout
					parameterIndex = 0;
					showParameter = false;
					idle = true;
				}
			}
		}
	}
}




//------------

/*
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

Coroutine snowSparkle(Loop &loop, Strip &strip, Color color, / *int pixelCount,* / Milliseconds<> sparkleDelay, Milliseconds<> minSpeedDelay, Milliseconds<> maxSpeedDelay) {
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
*/


/*
Coroutine draw(Loop &loop, Buffer &buffer) {
	SSD130x display(buffer, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_FLAGS);
	co_await drivers.resetDisplay();
	co_await display.init();
	co_await display.enable();

	int x = 0;
	int y = 0;
	while (true) {
		Bitmap bitmap = display.bitmap();
		bitmap.clear();
		bitmap.drawText(0, 0, tahoma8pt1bpp, "Hello World!");
		bitmap.drawText(50, 50, tahoma8pt1bpp, "SSD1309 SPI");
		bitmap.drawRectangle(x, y, 10, 10);
		x = (x + 1) & (DISPLAY_WIDTH - 1);
		y = (y + 1) & (DISPLAY_HEIGHT - 1);

		co_await display.show();
		co_await loop.sleep(200ms);

		debug::toggleRed();
		debug::toggleGreen();
	}
}*/



int main() {
	math::init();
	//draw(drivers.loop, drivers.displayBuffer);


	Strip strip(drivers.buffer1, drivers.buffer2);

	// run effects manager in separate coroutine
	EffectManager effectManager(drivers.loop, storageInfo, drivers.flashBuffer, strip, effectInfos);

	// start idle display
	SSD130x display(drivers.displayBuffer, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_FLAGS);
	//effectsMenu(drivers.loop, display, drivers.buttons, effectManager);
	mainMenu(drivers.loop, display, drivers.input, effectManager);

	drivers.loop.run();
	return 0;
}
