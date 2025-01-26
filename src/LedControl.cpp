#include <LedControl.hpp> // drivers
#include "E12.hpp"
#include "EffectManager.hpp"
#include "RemoteControl.hpp"
#include "effects/StaticColor.hpp"
#include "effects/ColorCycle3.hpp"
#include "effects/ColorFade.hpp"
#include "effects/ColorFade3.hpp"
#include "effects/Strobe.hpp"
#include "effects/CylonBounce.hpp"
#include "effects/MeteorRain.hpp"
#include "effects/SnowSparkle.hpp"
#include "effects/Spring.hpp"
#include "effects/Summer.hpp"
#include "effects/Autumn.hpp"
#include "effects/Winter.hpp"
#include "effects/Tetris.hpp"
#include <coco/nec.hpp>
#include <coco/nubert.hpp>
#include <coco/rc6.hpp>
#include <coco/BufferStorage.hpp>
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
    SnowSparkle::info,
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

AwaitableCoroutine remoteCommandMenu(Loop &loop, SSD130x &display, InputDevice &buttons, RemoteControl &remoteControl) {
    Menu menu(display, coco::tahoma8pt1bpp);

    while (true) {
        int seq2 = remoteControl.getSequenceNumber();

        // build menu
        menu.begin(buttons);

        auto stream = menu.stream();
        auto &command = remoteControl.getLearnCommand();
        switch (command.type) {
        case RemoteControl::Command::Type::NEC:
            stream << "NEC " << dec(command.necPacket.command);
            break;
        case RemoteControl::Command::Type::NUBERT:
            stream << "Nubert" << dec(command.nubertPacket);
            break;
        case RemoteControl::Command::Type::RC6:
            stream << "RC6" << dec(command.rc6Packet.data);
            break;
        default:
            stream << "None";
        }
        menu.label();

        if (menu.entry("Exit")) {
            co_return;
        }
        if (menu.entry("Clear")) {
            remoteControl.clear();
            co_return;
        }

        // show on display and wait for input
        co_await menu.show();
        co_await select(menu.untilInput(buttons), remoteControl.untilInput(seq2));
    }
}

AwaitableCoroutine remoteMenu(Loop &loop, SSD130x &display, InputDevice &buttons, RemoteControl &remoteControl) {
    // menu
    Menu menu(display, coco::tahoma8pt1bpp);
    while (true) {
        // build menu
        menu.begin(buttons);

        if (menu.entry("Brightness Up")) {
            remoteControl.setLearn(0);
            co_await remoteCommandMenu(loop, display, buttons, remoteControl);
            remoteControl.setNormal();
        }
        if (menu.entry("Brightness Down")) {
            remoteControl.setLearn(1);
            co_await remoteCommandMenu(loop, display, buttons, remoteControl);
            remoteControl.setNormal();
        }
        if (menu.entry("Duration Up")) {
            remoteControl.setLearn(2);
            co_await remoteCommandMenu(loop, display, buttons, remoteControl);
            remoteControl.setNormal();
        }
        if (menu.entry("Duration Down")) {
            remoteControl.setLearn(3);
            co_await remoteCommandMenu(loop, display, buttons, remoteControl);
            remoteControl.setNormal();
        }
        if (menu.entry("Next Preset")) {
            remoteControl.setLearn(4);
            co_await remoteCommandMenu(loop, display, buttons, remoteControl);
            remoteControl.setNormal();
        }

        if (menu.entry("Exit")) {
            co_return;
        }

        // show on display and wait for input
        co_await menu.show();
        co_await menu.untilInput(buttons);
    }
}

AwaitableCoroutine effectsMenu(Loop &loop, SSD130x &display, InputDevice &buttons, EffectManager &effectManager, RemoteControl &remoteControl) {
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
        menu.line();

        // stop effect when "New..." is selected
        if (menu.isSelected()) {
            if (currentPresetIndex != -1)
                co_await effectManager.stop();
            currentPresetIndex = -1;
        }
        if (effectManager.getPresetCount() < effectManager.MAX_PRESET_COUNT) {
            if (menu.entry("New Preset")) {
                int presetIndex = effectManager.addPreset();
                co_await effectManager.run(presetIndex);
                co_await parametersMenu(loop, display, buttons, effectManager, presetIndex);
            }
        }

        if (menu.entry("Remote Control")) {
            co_await remoteMenu(loop, display, buttons, remoteControl);
        }

        // stop effect when "Save" is selected
        if (menu.isSelected()) {
            if (currentPresetIndex != -1)
                co_await effectManager.stop();
            currentPresetIndex = -1;
        }
        if (menu.entry("Save")) {
            co_await effectManager.save();
            co_await remoteControl.save();
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
            co_await remoteControl.load();
            co_return;
        }

        // show on display and wait for input
        co_await menu.show();
        co_await menu.untilInput(buttons);
    }
}

Coroutine mainMenu(Loop &loop, SSD130x &display, InputDevice &input, Storage &storage, EffectManager &effectManager,
    RemoteControl &remoteControl)
{
    // initialize and enable the display
    co_await drivers.resetDisplay();
    co_await display.init();
    co_await display.enable();

    // mount flash storage
    int result;
    co_await storage.mount(result);
    if (result != Storage::Result::OK) {
    }

    // load all presets and IR commands
    co_await effectManager.load();
    co_await remoteControl.load();

    // run current effect
    int presetIndex = effectManager.getPresetIndex();
    co_await effectManager.run(presetIndex);

    int parameterIndex = 0;
    bool idle = true;
    bool showParameter = false;
    Loop::Time parameterTimeout;

    // init last state of input
    int8_t lastState[3];
    input.get(lastState);

    // init last state of remote control
    int lastSeq2 = remoteControl.getSequenceNumber();
    while (true) {
        // get button state
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
            } else {
                // update parameter
                effectManager.updateParameter(presetIndex, parameterIndex, delta);
            }
        }

        // button press: next parameter (increment parameter index)
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
            // enter effects menu
            co_await effectsMenu(loop, display, input, effectManager, remoteControl);

            // clamp preset index in case presets were deleted
            presetIndex = std::max(std::min(presetIndex, effectManager.getPresetCount() - 1), 0);

            // run preset
            co_await effectManager.run(presetIndex);

            // init variables
            parameterIndex = 0;
            showParameter = false;
            idle = true;

            // clear pending remote control command
            lastSeq2 = remoteControl.getSequenceNumber();
        }

        lastState[0] = state[0];
        lastState[1] = state[1];
        lastState[2] = state[2];


        // get remote control command
        int seq2 = remoteControl.getSequenceNumber();
        if (seq2 != lastSeq2) {
            lastSeq2 = seq2;
            int command = remoteControl.getCommand();

            showParameter = true;
            parameterTimeout = loop.now() + 5s;

            if (command < 4) {
                // update brightness or duration
                parameterIndex = command >> 1;
                effectManager.updateParameter(presetIndex, parameterIndex, (command & 1) == 0 ? 1 : -1);
            } else {
                // next preset
                parameterIndex = -1;
                presetIndex = (presetIndex + 1 + effectManager.getPresetCount()) % effectManager.getPresetCount();
                effectManager.setPresetIndex(presetIndex);
                co_await effectManager.run(presetIndex);
            }
            idle = false;
        }


        // draw
        auto bitmap = display.bitmap();
        bitmap.clear();

        if (!showParameter) {
            // idle: show preset name
            String name = effectManager.getPresetName(presetIndex);
            int w = coco::tahoma8pt1bpp.calcWidth(name);
            bitmap.drawText((128 - w) >> 1, 24, coco::tahoma8pt1bpp, name);

            // show bitmap on display
            co_await display.show();

            // wait for input
            co_await select(input.untilInput(seq), remoteControl.untilInput(seq2));
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
                barX = presetIndex * (124 - barW) / std::max(effectManager.getPresetCount() - 1, 1);

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

            // show bitmap on display
            co_await display.show();

            // wait for input
            int s = co_await select(input.untilInput(seq), remoteControl.untilInput(seq2), loop.sleep(parameterTimeout));
            if (s <= 2) {
                // user input from rotary knob: renew timeout
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


int main() {
    math::init();

    // flash storage
    BufferStorage storage(storageInfo, drivers.flashBuffer);

    // LED strip
    Strip strip(drivers.ledBuffer1, drivers.ledBuffer2);

    // effect manager
    EffectManager effectManager(drivers.loop, storage, strip, effectInfos);

    // IR remote control
    RemoteControl remoteControl(drivers.loop, storage, drivers.irDevice);

    // start idle display
    SSD130x display(drivers.displayBuffer, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_FLAGS);
    mainMenu(drivers.loop, display, drivers.input, storage, effectManager, remoteControl);

    drivers.loop.run();
    return 0;
}
