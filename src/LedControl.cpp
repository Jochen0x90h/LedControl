#include <LedControl.hpp> // drivers
#include "E12.hpp"
#include "EffectManager.hpp"
#include "StripManager.hpp"
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
#include <coco/ArrayBuffer.hpp>
#include <coco/BufferStorage.hpp>
#include <coco/BufferWriter.hpp>
#include <coco/Menu.hpp>
#include <coco/font/tahoma8pt1bpp.hpp>
#include <coco/PseudoRandom.hpp>
#include <coco/StreamOperators.hpp>
#include <coco/StringBuffer.hpp>
#include <coco/math.hpp>
#include <coco/debug.hpp>


/*
    Main progam

    Usage
    Power on: Show main screen and current preset (gets saved in flash)
    Wheel: Change brightness
    Button press: Cycle though parameters (speed, preset, effect parameters...)
    Wheel: Change current parameter
    Timeout: Switch back to main screen where wheel changes brightness
    Button long press: Enter configuration menu
*/

using namespace coco;


constexpr Seconds<> TIMEOUT = 10s;


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
    "Temp.",
};

static const String ledTypeNames[] = {
    "RGB",
    "GRB (WS1812)",
};

// Menu
// ----

// edit preset names
AwaitableCoroutine presetNamesMenu(Loop &loop, SSD130x &display, InputDevice &buttons, EffectManager &effectManager) {
    // menu
    Menu menu(display, coco::tahoma8pt1bpp);
    int bufferPresetIndex = -1;
    int buffer[EffectManager::MAX_PRESET_NAME_SIZE];
    int bufferSize = 0;
    bool bufferModified = false;
    int s = 0;
    int e = 0;
    while (true) {
        // build menu
        menu.begin(buttons);

        // list of preset names
        for (int presetIndex = 0; presetIndex < EffectManager::PRESET_COUNT; ++presetIndex) {
            auto& name = effectManager.getPresetName(presetIndex);

            if (menu.isSelected()) {
                // set UTF-32 buffer with current name
                if (bufferPresetIndex != presetIndex) {
                    bufferPresetIndex = presetIndex;

                    // convert from UTF-8
                    String n = name;
                    std::ranges::fill(buffer, 0);
                    bufferSize = 0;
                    while (!n.empty() && bufferSize < int(std::size(buffer))) {
                        auto result = utf8(n);
                        buffer[bufferSize++] = *result;
                        n = n.substring(result.length);
                    }
                }
            }

            int l = std::min(bufferSize + 1, int(std::size(buffer)));
            int edit = menu.edit(l);
            if (edit > 0) {
                int charIndex = edit - 1;

                // modify
                if (menu.delta() != 0) {
                    int code = buffer[charIndex];
                    if (menu.delta() > 0)
                        code = coco::tahoma8pt1bpp.nextCode(code, true);
                    else
                        code = coco::tahoma8pt1bpp.prevCode(code, true);
                    buffer[charIndex] = code;
                    bufferModified = true;
                }

                // to UTF-8
                name.clear();
                int i = 0;
                for (auto code : buffer) {
                    if (i <= charIndex)
                        s = name.size();
                    else if (code == 0)
                        break;

                    name << utf8(code);

                    if (i <= charIndex)
                        e = name.size();

                    if (code == 0)
                        break;

                    ++i;
                }
                bufferSize = i;
            } else {
                s = e = 0;

                // remove trailing zero
                if (name.size() > 0 && name[name.size() - 1] == 0) {
                    name.resize(name.size() - 1);
                    if (bufferModified) {
                        effectManager.setPresetNameModified(presetIndex);
                        bufferModified = false;
                    }
                }
            }

            // show preset name with underline between s and e
            menu.stream() << name.substring(0, s) << underline(name.substring(s, e)) << name.substring(e);
            menu.entry();
        }

        // check if something is modified
        if (effectManager.modified()) {
            if (menu.entry("Cancel")) {
                // undo all modifications
                co_await effectManager.load();
                co_return;
            }
            if (menu.entry("Save")) {
                // save all modified configs
                co_await effectManager.save();
                co_return;
            }
        } else {
            if (menu.entry("Exit")) {
                co_return;
            }
        }

        // show on display and wait for input
        co_await menu.show();
        co_await menu.untilInput(buttons);
    }
}

// edit preset parameters (effect and effect parameters)
AwaitableCoroutine parametersMenu(Loop &loop, SSD130x &display, InputDevice &buttons, EffectManager &effectManager,
    int playerIndex, int presetIndex)
{
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
                effectManager.updateEffect(playerIndex, presetIndex, menu.delta());
                effectManager.run(playerIndex, presetIndex);
            }

            auto stream = menu.stream();
            stream << "Effect: " << underline(effectManager.getEffectName(playerIndex, presetIndex), edit > 0);
            menu.entry();
        }

        // list of parameters
        int parameterCount = effectManager.getParameterCount(playerIndex, presetIndex);
        for (int parameterIndex = 0; parameterIndex < parameterCount; ++parameterIndex) {
            auto &info = effectManager.getParameterInfo(playerIndex, presetIndex, parameterIndex);
            int editCount = info.type == ParameterInfo::Type::COLOR ? 2 : 1;
            int edit = menu.edit(editCount);

            // print parameter name
            auto stream = menu.stream();
            if (editCount == 1 || edit == 0)
                stream << info.name << ": ";

            // edit parameter
            auto p = effectManager.updateParameter(playerIndex, presetIndex, parameterIndex, edit - 1, menu.delta());
            switch (info.type) {
            case ParameterInfo::Type::COUNT:
                stream << underline(dec(p.values[0]), edit > 0);
                break;
            case ParameterInfo::Type::DURATION_E12:
                stream << underline(MillisecondsE12(p.values[0]), edit > 0) << 's';
                break;
            case ParameterInfo::Type::PERCENTAGE:
                stream << underline(dec(p.values[0]), edit > 0) << '%';
                break;
            case ParameterInfo::Type::PERCENTAGE_E12:
                stream << underline(PercentageE12(p.values[0]), edit > 0) << '%';
                break;
            case ParameterInfo::Type::HUE:
                stream << underline(hueNames[p.values[0]], edit > 0);
                break;
            case ParameterInfo::Type::COLOR:
                stream << underline(hueNames[p.values[0]], edit == 1) << ' ';
                if (p.values[0] < 24)
                    stream << underline(dec(clampPercentage(p.values[1])), edit == 2) << '%';
                else
                    stream << underline(KelvinE12(clampColorTemperature(p.values[1])), edit == 2) << 'K';
                break;
            }
            menu.entry();
        }
        effectManager.applyParameters(playerIndex, presetIndex);

        // exit
        if (menu.entry("Exit")) {
            co_return;
        }

        // delete
        if (effectManager.getPresetCount(playerIndex) > 1) {
            if (menu.entry("Delete")) {
                // delete preset
                effectManager.stop();
                effectManager.deletePreset(playerIndex, presetIndex);

                // run preset at presetIndex
                if (presetIndex < effectManager.getPresetCount(playerIndex)) {
                    effectManager.run(playerIndex, presetIndex);
                }
                co_return;
            }
        }

        // show on display and wait for input
        co_await menu.show();
        co_await menu.untilInput(buttons);
    }
}

// edit an effect player (led count and list of presets)
AwaitableCoroutine effectPlayerMenu(Loop &loop, SSD130x &display, InputDevice &buttons, EffectManager &effectManager,
    int playerIndex)
{
    int currentPresetIndex = -2;

    // menu
    Menu menu(display, coco::tahoma8pt1bpp);
    while (true) {
        // build menu
        menu.begin(buttons);

        // led count
        if (menu.isSelected()) {
            // stop effect when LED count is selected
            if (currentPresetIndex != -1)
                effectManager.stop();
            currentPresetIndex = -1;
        }
        {
            int edit = menu.edit(1);
            int delta = edit > 0 ? menu.delta() : 0;
            int ledCount = effectManager.updateLedCount(playerIndex, delta);

            auto stream = menu.stream();
            stream << "LED Count: " << underline(dec(ledCount), edit > 0);
            menu.entry();
        }

        menu.line();

        // list of presets
        int presetCount = effectManager.getPresetCount(playerIndex);
        for (int presetIndex = 0; presetIndex < presetCount; ++presetIndex) {
            // run currently selected effect
            if (menu.isSelected()) {
                if (currentPresetIndex != presetIndex) {
                    currentPresetIndex = presetIndex;

                    // run preset only for the current player
                    effectManager.run(playerIndex, presetIndex);
                }
            }

            auto stream = menu.stream();
            stream << effectManager.getPresetName(presetIndex) << " (" << effectManager.getEffectName(playerIndex, presetIndex) << ')';
            if (menu.entry()) {
                // edit preset parameters (effect index and parameters)
                co_await parametersMenu(loop, display, buttons, effectManager, playerIndex, presetIndex);
            }
        }
        menu.line();

        // add preset
        if (menu.isSelected()) {
            // stop effect when "Add Preset" is selected
            if (currentPresetIndex != -1)
                effectManager.stop();
            currentPresetIndex = -1;
        }
        if (effectManager.getPresetCount(playerIndex) < EffectManager::PRESET_COUNT) {
            if (menu.entry("Add Preset")) {
                int presetIndex = effectManager.addPreset(playerIndex);
                effectManager.run(presetIndex);

                // edit preset parameters of new preset
                co_await parametersMenu(loop, display, buttons, effectManager, playerIndex, presetIndex);
            }
        }

        // exit
        if (menu.isSelected()) {
            // stop effect when "Exit" is selected
            if (currentPresetIndex != -1)
                effectManager.stop();
            currentPresetIndex = -1;
        }
        if (menu.entry("Exit")) {
            co_return;
        }

        // show on display and wait for input
        co_await menu.show();
        co_await menu.untilInput(buttons);
    }
}

// select an effect player to edit
AwaitableCoroutine effectPlayersMenu(Loop &loop, SSD130x &display, InputDevice &buttons, EffectManager &effectManager) {
    // menu
    Menu menu(display, coco::tahoma8pt1bpp);
    while (true) {
        // build menu
        menu.begin(buttons);

        // list of players
        for (int i = 0; i < EffectManager::PLAYER_COUNT; ++i) {
            menu.stream() << "Player " << dec(i + 1);
            if (menu.entry()) {
                // edit effect parameters
                co_await effectPlayerMenu(loop, display, buttons, effectManager, i);
            }
        }

        // check if something is modified
        if (effectManager.modified()) {
            if (menu.entry("Cancel")) {
                // undo all modifications
                co_await effectManager.load();
                co_return;
            }
            if (menu.entry("Save")) {
                // save all modified configs
                co_await effectManager.save();
                co_return;
            }
        } else {
            if (menu.entry("Exit")) {
                co_return;
            }
        }

        // show on display and wait for input
        co_await menu.show();
        co_await menu.untilInput(buttons);
    }
}


// edit led strip
AwaitableCoroutine ledStripMenu(Loop &loop, SSD130x &display, InputDevice &buttons,
    StripManager &stripManager, int stripIndex)
{
    // menu
    Menu menu(display, coco::tahoma8pt1bpp);
    while (true) {
        // build menu
        menu.begin(buttons);

        // LED type (e.g. WS1812)
        {
            int edit = menu.edit(1);
            auto ledType = stripManager.updateLedType(stripIndex, edit > 0 ? menu.delta() : 0);
            menu.stream() << "Type: " << underline(ledTypeNames[int(ledType)], edit > 0);
            menu.entry();
        }

        // LED count (number of LEDs in the strip)
        /*{
            int edit = menu.edit(1);
            auto ledCount = stripManager.updateLedCount(stripIndex, edit > 0 ? menu.delta() : 0);
            menu.stream() << "LED Count: " << underline(dec(ledCount), edit > 0);
            menu.entry();
        }*/

        // iterate over list of sources (players to copy data from)
        menu.line();
        int sourceCount = stripManager.getSourceCount(stripIndex);
        stripManager.setEditMode(-1, 0);
        for (int sourceIndex = 0; sourceIndex < sourceCount; ++sourceIndex) {
            {
                int edit = menu.edit(1);
                int playerIndex = stripManager.updatePlayerIndex(stripIndex, sourceIndex, edit > 0 ? menu.delta() : 0);
                menu.stream() << "Player: " << underline(dec(playerIndex), edit > 0);
                menu.entry();
            }
            {
                int edit = menu.edit(1);
                int ledStart = stripManager.updateLedStart(stripIndex, sourceIndex, edit > 0 ? menu.delta() : 0);
                menu.stream() << "Start: " << underline(dec(ledStart), edit > 0);
                menu.entry();
            }
            {
                if (menu.isSelected()) {
                    stripManager.setEditMode(stripIndex, sourceIndex);
                }
                int edit = menu.edit(1);
                int ledCount = stripManager.updateLedCount(stripIndex, sourceIndex, edit > 0 ? menu.delta() : 0);
                menu.stream() << "Count: " << underline(dec(ledCount), edit > 0);
                menu.entry();
            }
            menu.line();
        }

        // add entry
        if (sourceCount < StripManager::MAX_STRIP_SOURCE_COUNT
            && stripManager.getLedCount(stripIndex) < MAX_LEDSTRIP_LENGTH
            && menu.entry("Add Entry"))
        {
            stripManager.addSource(stripIndex);
        }

        // remove entry
        if (sourceCount > 1 && menu.entry("Remove Entry")) {
            stripManager.removeSource(stripIndex);
        }

        if (menu.entry("Exit")) {
            co_return;
        }

        // show on display and wait for input
        co_await menu.show();
        co_await menu.untilInput(buttons);
    }
}

// select led stip to edit
AwaitableCoroutine ledStripsMenu(Loop &loop, SSD130x &display, InputDevice &buttons, StripManager &stripManager) {
    // menu
    Menu menu(display, coco::tahoma8pt1bpp);
    while (true) {
        // build menu
        menu.begin(buttons);

        for (int i = 0; i < 3; ++i) {
            menu.stream() << "LED Strip " << dec(i + 1);
            if (menu.entry()) {
                co_await ledStripMenu(loop, display, buttons, stripManager, i);
            }
        }

        if (stripManager.modified()) {
            if (menu.entry("Cancel")) {
                // undo all modifications
                co_await stripManager.load();
                co_return;
            }
            if (menu.entry("Save")) {
                // save all modified configs
                co_await stripManager.save();
                co_return;
            }
        } else {
            if (menu.entry("Exit")) {
                co_return;
            }
        }

        // show on display and wait for input
        co_await menu.show();
        co_await menu.untilInput(buttons);
    }
}

// edit one command of the remote control
AwaitableCoroutine remoteCommandMenu(Loop &loop, SSD130x &display, InputDevice &buttons, RemoteControl &remoteControl) {
    Menu menu(display, coco::tahoma8pt1bpp);

    while (true) {
        int seq = remoteControl.getSequenceNumber();

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
        co_await select(menu.untilInput(buttons), remoteControl.untilInput(seq));
    }
}

// select remote control command to edit
AwaitableCoroutine remoteControlMenu(Loop &loop, SSD130x &display, InputDevice &buttons, RemoteControl &remoteControl) {
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

        if (!remoteControl.modified()) {
            if (menu.entry("Exit"))
                co_return;
        } else {
            if (menu.entry("Cancel")) {
                // undo all modifications
                co_await remoteControl.load();
                co_return;
            }
            if (menu.entry("Save")) {
                // save all modified commands
                co_await remoteControl.save();
                co_return;
            }
        }

        // show on display and wait for input
        co_await menu.show();
        co_await menu.untilInput(buttons);
    }
}

// main configuration menu
AwaitableCoroutine configurationMenu(Loop &loop, SSD130x &display, InputDevice &buttons,
    EffectManager &effectManager, StripManager &stripManager, RemoteControl &remoteControl)
{
    // initialize and enable the display
    co_await drivers.resetDisplay();
    co_await display.init();
    co_await display.enable();

    //int currentPresetIndex = -1;

    // menu
    Menu menu(display, coco::tahoma8pt1bpp);
    while (true) {
        // build menu
        menu.begin(buttons);
/*
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
*/
        // configure preset names
        if (menu.entry("Preset Names")) {
            co_await presetNamesMenu(loop, display, buttons, effectManager);
        }

        // configure effect players
        if (menu.entry("Effect Players")) {
            co_await effectPlayersMenu(loop, display, buttons, effectManager);
        }

        // configure the LED strips
        if (menu.entry("LED Strips")) {
            co_await ledStripsMenu(loop, display, buttons, stripManager);
        }

        // configure remote control
        if (menu.entry("Remote Control")) {
            co_await remoteControlMenu(loop, display, buttons, remoteControl);
        }

/*        // stop effect when "Save" is selected
        if (menu.isSelected()) {
            if (currentPresetIndex != -1)
                co_await effectManager.stop();
            currentPresetIndex = -1;
        }
        if (menu.entry("Save")) {
            co_await effectManager.savePresets();
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
*/
        if (menu.entry("Exit")) {
            co_return;
        }

        // show on display and wait for input
        co_await menu.show();
        co_await menu.untilInput(buttons);
    }
}

Coroutine mainMenu(Loop &loop, SSD130x &display, InputDevice &input, Storage &storage,
    EffectManager &effectManager, StripManager &stripManager, RemoteControl &remoteControl)
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
    co_await stripManager.load();
    co_await remoteControl.load();

    // copy player infos to strip manager
    //effectManager.getPlayerInfos(stripManager.playerInfos);

    // run strip manager
    stripManager.run();

    // run current effect
    int presetIndex = effectManager.getPresetIndex();
    effectManager.run(presetIndex);

    int parameterIndex = 0;
    //bool idle = true;
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
        int presetCount = effectManager.getPresetCount();
        bool hasMultiplePresets = presetCount > 1;

        // rotary knob: change parameter value or preset
        // parameter index: 0 brightness, 1 speed, 2 preset, 3 - N + 1 are the remaining effect parameters 2 - N
        int delta = int8_t(state[0] - lastState[0]);
        if (delta != 0) {
            showParameter = true;
            parameterTimeout = loop.now() + TIMEOUT;

            // changing of preset is inserted at parameterIndex 2 if there are multiple presets
            if (parameterIndex == 2 && hasMultiplePresets) {
                // change preset
                presetIndex = (presetIndex + presetCount * 256 + delta) % presetCount;
                effectManager.setPresetIndex(presetIndex);
                effectManager.run(presetIndex);
            } else {
                // update parameter
                effectManager.updateGlobalParameter(parameterIndex + int(parameterIndex > 2 && hasMultiplePresets), delta);
                //effectManager.updateParameter(i, presetIndex, parameterIndex + int(parameterIndex > 2 && hasMultiplePresets), delta);
            }
        }

        // button press: next parameter (increment parameter index)
        delta = int8_t(state[1] - lastState[1]);
        if (delta != 0) {
            showParameter = true;
            parameterTimeout = loop.now() + TIMEOUT;

            // select next parameter on button press
            ++parameterIndex;
            if (parameterIndex >= 2/*effectManager.getParameterCount(presetIndex)*/ + int(hasMultiplePresets)) {
                parameterIndex = 0;
            }
        }

        // long press: enter menu
        delta = int8_t(state[2] - lastState[2]);
        if (delta != 0) {
            // enter main configuration menu
            co_await configurationMenu(loop, display, input, effectManager, stripManager, remoteControl);

            // update presetCount
            presetCount = effectManager.getPresetCount();

            // clamp preset index in case presets were deleted
            presetIndex = std::max(std::min(presetIndex, presetCount - 1), 0);

            // run preset for all players
            effectManager.run(presetIndex);

            // init variables
            parameterIndex = 0;
            showParameter = false;
            //idle = true;

            // get current button state
            seq = input.get(state);

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
            parameterTimeout = loop.now() + TIMEOUT;

            if (command < 4) {
                // update brightness or duration (set parameterIndex to 0 or 1)
                parameterIndex = command >> 1;
                effectManager.updateGlobalParameter(parameterIndex, (command & 1) == 0 ? 1 : -1);
            } else {
                // next preset
                parameterIndex = 2;//-1;
                presetIndex = (presetIndex + 1 + effectManager.getPresetCount()) % effectManager.getPresetCount();
                effectManager.setPresetIndex(presetIndex);
                effectManager.run(presetIndex);
            }
            //idle = false;
        }


        // draw
        auto bitmap = display.bitmap();
        bitmap.clear();

        if (!showParameter) {
            // idle: show preset name

            // get player index of first strip for default preset name
            //int playerIndex = stripManager.updatePlayerIndex(0, 0, 0);

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
            if (parameterIndex == 2 && hasMultiplePresets) {
                // show "Preset", bar and preset name
                String name = "Preset";
                int w = coco::tahoma8pt1bpp.calcWidth(name);
                bitmap.drawText((128 - w) >> 1, 10, coco::tahoma8pt1bpp, name);

                barW = 124 / effectManager.getPresetCount();
                barX = presetIndex * (124 - barW) / std::max(effectManager.getPresetCount() - 1, 1);

                // get player index of first strip for default preset name
                //int playerIndex = stripManager.updatePlayerIndex(0, 0, 0);

                value << effectManager.getPresetName(presetIndex);
            } else {
                // show parameter name, bar and value
                //auto p = effectManager.updateParameter(0, presetIndex, parameterIndex - int(parameterIndex > 2 && hasMultiplePresets), 0);
                auto p = effectManager.updateGlobalParameter(parameterIndex, 0);

                // show parameter name
                String name = p.info.name;
                int w = coco::tahoma8pt1bpp.calcWidth(name);
                bitmap.drawText((128 - w) >> 1, 10, coco::tahoma8pt1bpp, name);

                // calc x-coordinate and width of display bar
                /*{
                    int value = p.value - p.info.min;
                    int range = p.info.max + p.info.min;
                    if (!p.info.wrap) {
                        barW = value * 124 / range;
                    } else {
                        barW = 124 / range + 1; // width of bar is at least 1 pixel
                        barX = value * (124 - barW) / range;
                    }
                }*/

                // create display value
                switch (p.info.type) {
                case ParameterInfo::Type::COUNT:
                    //barW = p.value * 124 / 20;
                    value << dec(p.values[0]);
                    break;
                case ParameterInfo::Type::DURATION_E12:
                    //barW = p.value * 124 / 36;
                    barW = (p.values[0] - p.info.value) * 124 / 60;
                    value << MillisecondsE12(p.values[0]) << "s";
                    break;
                case ParameterInfo::Type::PERCENTAGE:
                    //barW = p.value * 124 / 100;
                    value << dec(p.values[0]) << "%";
                    break;
                case ParameterInfo::Type::PERCENTAGE_E12:
                    barW = p.values[0] * 124 / 24;
                    value << PercentageE12(p.values[0]) << "%";
                    break;
                case ParameterInfo::Type::HUE:
                    //barW = 124 / 24;
                    //barX = p.value * (124 - barW) / 23;
                    value << hueNames[p.values[0]];
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
                parameterTimeout = loop.now() + TIMEOUT;
            } else {
                // timeout
                parameterIndex = 0;
                showParameter = false;
                //idle = true;
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

// size must be at least MAX_LEDSTRIP_LENGTH
uint32_t stripData[1000];

int main() {
    math::init();

    // flash storage
    BufferStorage storage(storageInfo, drivers.flashBuffer);

    // LED strip
    //Strip strip(drivers.ledBuffer1, drivers.ledBuffer2);

    // effect manager
    //EffectManager effectManager(drivers.loop, storage, strip, effectInfos);
    EffectManager effectManager(drivers.loop, storage, effectInfos, StripData(stripData));

    // strip manager
    StripManager stripManager(drivers.loop, storage, StripData(stripData),
        effectManager.syncBarrier, effectManager.playerInfos,
        drivers.ledBuffer1, drivers.ledBuffer2, drivers.ledBuffer3);

    // IR remote control
    RemoteControl remoteControl(drivers.loop, storage, drivers.irDevice);

    // start idle display
    SSD130x display(drivers.displayBuffer, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_FLAGS);
    mainMenu(drivers.loop, display, drivers.input, storage, effectManager, stripManager, remoteControl);

    drivers.loop.run();
    return 0;
}
