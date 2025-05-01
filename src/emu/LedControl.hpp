#pragma once

#include <coco/SSD130x.hpp>
#include <coco/BufferStorage.hpp>
#include <coco/platform/SSD130x_emu.hpp>
#include <coco/platform/RotaryKnob_emu.hpp>
#include <coco/platform/LedStrip_emu.hpp>
#include <coco/platform/IrReceiver_emu.hpp>
#include <coco/platform/Newline_emu.hpp>
#include <coco/platform/Flash_File.hpp>


using namespace coco;

constexpr int LEDSTRIP_LENGTH = 300;

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

	// LED strip
	LedStrip_emu ledStrip{loop};
	LedStrip_emu::Buffer ledBuffer1{LEDSTRIP_LENGTH, ledStrip};
	LedStrip_emu::Buffer ledBuffer2{LEDSTRIP_LENGTH, ledStrip};
	Newline_emu newline1{loop}; // start a new line in the emulateor gui

	// display
	SSD130x_emu displayBuffer{loop, DISPLAY_WIDTH, DISPLAY_HEIGHT};
	AwaitableCoroutine resetDisplay() {co_return;}

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
