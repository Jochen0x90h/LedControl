#pragma once

#include "config.hpp"
#include <coco/SSD130x.hpp>
#include <coco/BufferStorage.hpp>
#include <coco/DummyOutputPort.hpp>
#include <coco/platform/SSD130x_emu.hpp>
#include <coco/platform/RotaryKnob_emu.hpp>
#include <coco/platform/LedStrip_emu.hpp>
#include <coco/platform/IrReceiver_emu.hpp>
#include <coco/platform/Newline_emu.hpp>
#include <coco/platform/Flash_File.hpp>


using namespace coco;

// emulated display
constexpr int DISPLAY_WIDTH = 128;
constexpr int DISPLAY_HEIGHT = 64;
constexpr SSD130x::Flags DISPLAY_FLAGS = SSD130x::Flags::SSD1306;

// emulated Näve lighting remote control
const uint8_t timesStar[] = {0,181,88,12,32,12,10,12,10,12,10,12,10,12,10,12,32,12,10,12,10,12,32,12,10,12,32,12,10,12,32,12,32,12,32,12,10,12,10,12,10,12,32,12,10,12,10,12,32,12,10,12,32,12,32,12,32,12,10,12,32,12,32,12,10,12,32,12};
const uint8_t timesHotter[] = {0,180,89,11,33,11,11,11,11,11,11,11,11,11,11,11,33,11,11,11,11,11,33,11,11,11,33,11,11,11,33,11,33,12,33,11,33,11,11,11,11,11,11,11,33,11,11,12,10,11,11,11,11,12,33,11,33,11,33,11,11,11,33,11,33,11,33,11};
const uint8_t timesColder[] = {0,180,89,11,33,11,11,11,11,11,11,11,11,11,11,11,33,11,11,11,11,11,33,11,11,11,33,11,11,11,33,11,33,11,33,11,11,11,11,11,11,11,11,11,34,11,10,11,11,11,11,11,33,11,33,11,33,11,33,11,11,11,33,11,33,11,33,11};
const uint8_t timesPlus[] = {0,181,88,12,32,12,10,12,10,12,10,12,10,12,10,12,32,12,10,12,10,12,32,12,9,12,32,12,10,12,32,12,32,12,32,12,10,12,32,12,10,12,9,12,32,12,9,12,10,12,10,12,32,12,10,12,32,12,32,12,10,12,32,12,32,12,32,12};
const uint8_t timesMinus[] = {0,181,88,12,32,12,10,12,10,12,9,12,10,12,9,12,32,12,9,12,9,12,32,12,10,12,32,12,9,12,32,12,32,12,32,12,32,12,32,12,9,12,9,12,32,12,9,12,10,12,10,12,9,12,10,12,32,12,32,12,9,12,32,12,32,12,32,12};
IrReceiver_emu::Config config {
    {
        timesHotter, // left
        timesPlus, // up
        timesMinus, // down
        timesColder, // right
        timesStar // center
    }
};

// emulated flash
constexpr int BLOCK_SIZE = 8;
constexpr int PAGE_SIZE = 2048;
constexpr BufferStorage::Info storageInfo {
    0, // address
    BLOCK_SIZE,
    PAGE_SIZE,
    8192, // sector size
    2, // sector count
    BufferStorage::Type::MEM_4N // file
};


// drivers for LedControl
struct Drivers {
    Loop_emu loop;

    // 3 LED strips
    LedStrip_emu strip1{loop};
    LedStrip_emu::Buffer stripBuffer1{MAX_LEDSTRIP_LENGTH, strip1};

    Newline_emu newline2{loop}; // start a new line in the emulateor gui
    LedStrip_emu strip2{loop};
    LedStrip_emu::Buffer stripBuffer2{MAX_LEDSTRIP_LENGTH, strip2};

    //Newline_emu newline3{loop}; // start a new line in the emulateor gui
    //LedStrip_emu strip3{loop};
    //LedStrip_emu::Buffer stripBuffer3{MAX_LEDSTRIP_LENGTH, strip3};

    Buffer *stripBuffers[LEDSTRIP_COUNT] {&stripBuffer1, &stripBuffer2};//, &stripBuffer3};

    // display
    Newline_emu newlineDisplay{loop}; // start a new line in the emulateor gui
    SSD130x_emu displayBuffer{loop, DISPLAY_WIDTH, DISPLAY_HEIGHT};
    //AwaitableCoroutine resetDisplay() {co_return;}
    DummyOutputPort resetPin;

    // rotary knob
    RotaryKnob_emu input{loop, true, 100}; // gui id

    // ir receiver
    using IrReceiver = IrReceiver_emu;
    IrReceiver irDevice{loop, config, 101}; // gui id
    IrReceiver::Buffer irBuffer1{80, irDevice};
    IrReceiver::Buffer irBuffer2{80, irDevice};

    // flash
    Flash_File flash{"flash.bin", 16384, PAGE_SIZE, BLOCK_SIZE};
    Flash_File::Buffer flashBuffer{256, flash};
};

Drivers drivers;
