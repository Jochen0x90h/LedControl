#pragma once

#include <coco/Frequency.hpp>


using namespace coco::literals;

// make sure the board specific SystemInit() configures the clock to the given frequency
constexpr auto SYS_CLOCK = 160MHz;
constexpr auto AHB_CLOCK = SYS_CLOCK;

constexpr auto APB1_CLOCK = AHB_CLOCK / 4;
constexpr auto APB1_TIMER_CLOCK = APB1_CLOCK * 2;

constexpr auto APB2_CLOCK = AHB_CLOCK / 4;
constexpr auto APB2_TIMER_CLOCK = APB2_CLOCK * 2;

constexpr auto USART1_CLOCK = APB2_CLOCK;
constexpr auto USART2_CLOCK = APB1_CLOCK;
constexpr auto USART3_CLOCK = APB1_CLOCK;
constexpr auto UART5_CLOCK = APB1_CLOCK;
constexpr auto I2C1_CLOCK = APB1_CLOCK;
constexpr auto I2C2_CLOCK = APB1_CLOCK;
constexpr auto I2C3_CLOCK = APB1_CLOCK;


// flash start address and size
constexpr int FLASH_ADDRESS = 0x8000000;
constexpr int FLASH_SIZE = 0x20000; // 128K, use upper 64K for storage


constexpr int MAX_LEDSTRIP_LENGTH = 300;

#define SWAP_R_G
#define DEFAULT_LED_TYPE LedType::GRB;
