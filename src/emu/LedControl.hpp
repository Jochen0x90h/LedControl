#pragma once

#include <coco/SSD130x.hpp>
#include <coco/BufferStorage.hpp>
#include <coco/platform/SSD130x_emu.hpp>
#include <coco/platform/RotaryKnob_emu.hpp>
#include <coco/platform/LedStrip_emu.hpp>
#include <coco/platform/Newline_emu.hpp>
#include <coco/platform/Flash_File.hpp>


using namespace coco;

constexpr int LEDSTRIP_LENGTH = 300;

constexpr int DISPLAY_WIDTH = 128;
constexpr int DISPLAY_HEIGHT = 64;
constexpr SSD130x::Flags DISPLAY_FLAGS = SSD130x::Flags::SSD1306;

constexpr int PAGE_SIZE = 2048;
constexpr int BLOCK_SIZE = 8;
constexpr BufferStorage::Info storageInfo {
	0, // address
	BLOCK_SIZE,
	PAGE_SIZE,
	8192, // sector size
	2, // sector count
	BufferStorage::Type::MEM_4N
};


// drivers for LedControl
struct Drivers {
	Loop_emu loop;

	// LED strip
	LedStrip_emu ledStrip{loop};
	LedStrip_emu::Buffer buffer1{LEDSTRIP_LENGTH, ledStrip};
	LedStrip_emu::Buffer buffer2{LEDSTRIP_LENGTH, ledStrip};
	Newline_emu newline1{loop}; // start a new line in the emulateor gui

	// display
	SSD130x_emu displayBuffer{loop, DISPLAY_WIDTH, DISPLAY_HEIGHT};
	AwaitableCoroutine resetDisplay() {co_return;}

	// rotary knob
	RotaryKnob_emu input{loop, true, 100}; // id, long press delay

	// flash
	Flash_File flash{"flash.bin", 16384, PAGE_SIZE, BLOCK_SIZE};
	Flash_File::Buffer flashBuffer{256, flash};
};

Drivers drivers;
