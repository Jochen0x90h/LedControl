#pragma once

#include "config.hpp"
#include <coco/platform/Flash_FLASH.hpp>
#include <coco/platform/InputDevice_EXTI_TIM.hpp>
#include <coco/platform/IrReceiver_TIM.hpp>
#include <coco/platform/LedStrip_UART_DMA.hpp>
#include <coco/platform/Loop_TIM2.hpp>
#include <coco/platform/SpiMaster_SPI_DMA.hpp>
#include <coco/platform/OutputPort_GPIO.hpp>
#include <coco/BufferStorage.hpp>
#include <coco/SSD130x.hpp>
#include <coco/platform/platform.hpp>
#include <coco/platform/gpio.hpp>


using namespace coco;
using namespace coco::literals;

constexpr int DISPLAY_WIDTH = 128;
constexpr int DISPLAY_HEIGHT = 64;
constexpr SSD130x::Flags DISPLAY_FLAGS = SSD130x::Flags::UG_2864ASWPG01_SPI;


// flash
constexpr int BLOCK_SIZE = Flash_FLASH::BLOCK_SIZE;
constexpr int PAGE_SIZE = Flash_FLASH::PAGE_SIZE;
constexpr int SECTOR_COUNT = 4;
constexpr int SECTOR_PAGE_COUNT = FLASH_SIZE / (2 * PAGE_SIZE * SECTOR_COUNT); // use half of available flash which is 64K, also configure in link.ld
constexpr int SECTOR_SIZE = SECTOR_PAGE_COUNT * PAGE_SIZE;
constexpr BufferStorage::Info storageInfo {
    FLASH_ADDRESS + FLASH_SIZE - SECTOR_COUNT * SECTOR_SIZE, // address
    BLOCK_SIZE,
    PAGE_SIZE,
    SECTOR_SIZE,
    SECTOR_COUNT,
    BufferStorage::Type::FLASH_4N // internal flash
};


// drivers for LedControl
struct Drivers {
    Loop_TIM2 loop{APB1_TIMER_CLOCK};

    // LED strip
    using LedStrip = LedStrip_UART_DMA;
    LedStrip ledStrip{loop,
        gpio::Config::PC4 | gpio::Config::AF7 | gpio::Config::SPEED_HIGH, // USART1 TX (RS485_TX)
        usart::USART1_INFO,
        dma::DMA1_CH1_INFO,
        USART1_CLOCK,
        1125ns, // bit time T
        75us}; // reset time
    LedStrip::Buffer<MAX_LEDSTRIP_LENGTH * 3> ledBuffer1{ledStrip};
    LedStrip::Buffer<MAX_LEDSTRIP_LENGTH * 3> ledBuffer2{ledStrip};

    // display
    using SpiMaster = SpiMaster_SPI_DMA;
    SpiMaster displaySpi{loop,
        gpio::Config::PA5 | gpio::Config::AF5, // SPI1 SCK (DISP_SCK)
        gpio::Config::NONE, // no MISO, send only
        gpio::Config::PA7 | gpio::Config::AF5, // SPI1 MOSI (DISP_MOSI)
        gpio::Config::PA0, // DC (DISP_D/nC)
        spi::SPI1_INFO,
        dma::DMA1_CH2_CH3_INFO,
        spi::ClockConfig::DIV32, spi::Config::PHA1_POL1};
    SpiMaster::Channel displayChannel{displaySpi, gpio::Config::PA4 | gpio::Config::INVERT, true}; // nCS (DISP_nCS)
    SpiMaster::Buffer<DISPLAY_WIDTH * DISPLAY_HEIGHT / 8> displayBuffer{displayChannel};

    // display reset pin
    static constexpr OutputPort_GPIO::Config outputConfig[] {
        {gpio::Config::PA2 | gpio::Config::SPEED_MEDIUM | gpio::Config::INVERT, false}, // display reset (DISP_nRST)
    };
    OutputPort_GPIO resetPin{outputConfig};

    // display reset method
    AwaitableCoroutine resetDisplay() {
        resetPin.set(1, 1);
        co_await loop.sleep(10ms);
        resetPin.set(0, 1);
        co_return;
    }

    // rotary knob with push button
    using InputDevice = InputDevice_EXTI_TIM;
    static constexpr gpio::Config inputPinConfigs[] {
        gpio::Config::PC13 | gpio::Config::PULL_DOWN, // rotary knob A (ENC_A)
        gpio::Config::PC14 | gpio::Config::PULL_DOWN, // rotary knob B (ENC_B)
        gpio::Config::PC15 | gpio::Config::PULL_DOWN, // push button (SW)
    };
    static constexpr InputDevice::Config inputConfigs[] {
        {0, 0, InputDevice::Action::DECREMENT_WHEN_ENABLED, InputDevice::Action::INCREMENT_WHEN_ENABLED, 1ms, 1ms}, // quadrature decoder (inputs 0 and 1, counter 0)
        {2, 1, InputDevice::Action::INCREMENT, InputDevice::Action::NONE, 10ms, 10ms}, // button press (input 2, counter 1)
        {2, 2, InputDevice::Action::INCREMENT, InputDevice::Action::NONE, 3s, 10ms}, // button long press (input 2, counter 2)
    };
    InputDevice input{loop,
        inputPinConfigs,
        inputConfigs,
        timer::TIM4_INFO,
        APB1_TIMER_CLOCK,
        0}; // CC index

    // IR receiver
    using IrReceiver = IrReceiver_TIM;
    IrReceiver irDevice{loop,
        gpio::Config::PC6 | gpio::Config::AF2 | gpio::Config::PULL_UP, // data pin TIM3_CH1 (does not need to be connected, therefore pull-up)
        timer::TIM3_INFO,
        1, // channel 1
        APB1_TIMER_CLOCK};

    // flash
    Flash_FLASH::Buffer<256> flashBuffer;

    Drivers() {
        // set RS485_DE high
        gpio::configureOutput(gpio::Config::PB0, true);
    }
};

Drivers drivers;

extern "C" {

// LED strip
void USART1_IRQHandler() {
    drivers.ledStrip.UART_IRQHandler();
}
void DMA1_Channel1_IRQHandler() {
    drivers.ledStrip.DMA_IRQHandler();
}

// display
void DMA1_Channel2_IRQHandler() {
    drivers.displaySpi.DMA_Rx_IRQHandler();
}

// rotary knob with push button
void EXTI15_10_IRQHandler() {
    drivers.input.EXTI_IRQHandler();
}
void TIM3_IRQHandler() {
    drivers.input.TIM_IRQHandler();
}

}
