#pragma once

#include "config.hpp"
#include <coco/platform/Flash_flash.hpp>
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

constexpr int LEDSTRIP_LENGTH = 300;

// flash
constexpr int BLOCK_SIZE = flash::BLOCK_SIZE;
constexpr int PAGE_SIZE = flash::PAGE_SIZE;
constexpr int SECTOR_COUNT = 4;
constexpr int SECTOR_PAGE_COUNT = STORAGE_SIZE / (SECTOR_COUNT * PAGE_SIZE); // number of pages in one sector       FLASH_SIZE / (2 * PAGE_SIZE * SECTOR_COUNT); // use 1/4 of available flash which is 64K, also configure in link.ld
constexpr int SECTOR_SIZE = SECTOR_PAGE_COUNT * PAGE_SIZE; // size of one sector
constexpr BufferStorage::Info storageInfo {
    STORAGE_ADDRESS, //FLASH_ADDRESS + FLASH_SIZE - SECTOR_COUNT * SECTOR_SIZE, // address
    BLOCK_SIZE,
    PAGE_SIZE,
    SECTOR_SIZE,
    SECTOR_COUNT,
    BufferStorage::Type::FLASH_4N // internal flash
};


// drivers for LedControl
struct Drivers {
    Loop_TIM2 loop{APB1_TIMER_CLOCK};

    // LED strip 1
    using LedStrip = LedStrip_UART_DMA;
    LedStrip ledStrip1{loop,
        gpio::PC4 | gpio::AF7 | gpio::Config::SPEED_HIGH, // USART1 TX (RS485_1_TX)
        usart::USART1_INFO,
        dma::DMA1_CH1_INFO,
        USART1_CLOCK,
        1125ns, // bit time T
        75us}; // reset time
    LedStrip::Buffer<MAX_LEDSTRIP_LENGTH * 3> ledBuffer1{ledStrip1};

    // LED strip 2
    LedStrip ledStrip2{loop,
        gpio::PB3 | gpio::AF7 | gpio::Config::SPEED_HIGH, // USART2 TX (RS485_2_TX)
        usart::USART2_INFO,
        dma::DMA1_CH2_INFO,
        USART2_CLOCK,
        1125ns, // bit time T
        75us}; // reset time
    LedStrip::Buffer<MAX_LEDSTRIP_LENGTH * 3> ledBuffer2{ledStrip2};

    // LED strip 3
    /*LedStrip ledStrip3{loop,
        gpio::PC10 | gpio::AF7 | gpio::Config::SPEED_HIGH, // USART3 TX (RS485_3_TX)
        usart::USART3_INFO,
        dma::DMA1_CH3_INFO,
        USART3_CLOCK,
        1125ns, // bit time T
        75us}; // reset time*/
    LedStrip::Buffer<MAX_LEDSTRIP_LENGTH * 3> ledBuffer3{ledStrip2}; //!

    // display
    using SpiMaster = SpiMaster_SPI_DMA;
    SpiMaster displaySpi{loop,
        gpio::PA5 | gpio::AF5, // SPI1 SCK (DISP_SCK)
        gpio::NONE, // no MISO, send only
        gpio::PA7 | gpio::AF5, // SPI1 MOSI (DISP_MOSI)
        gpio::PA0, // DC (DISP_D/nC)
        spi::SPI1_INFO,
        dma::DMA1_CH2_CH3_INFO,
        spi::ClockConfig::DIV32, spi::Config::PHA1_POL1};
    SpiMaster::Channel displayChannel{displaySpi, gpio::PA4 | gpio::Config::INVERT, true}; // nCS (DISP_nCS)
    SpiMaster::Buffer<0, DISPLAY_WIDTH * DISPLAY_HEIGHT / 8> displayBuffer{displayChannel};

    // display reset pin
    static constexpr OutputPort_GPIO::Config outputConfig[] {
        {gpio::PA2 | gpio::Config::SPEED_MEDIUM | gpio::Config::INVERT, false}, // display reset (DISP_nRST)
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
        gpio::PC13 | gpio::Config::PULL_DOWN, // rotary knob A (ENC_A)
        gpio::PC14 | gpio::Config::PULL_DOWN, // rotary knob B (ENC_B)
        gpio::PC15 | gpio::Config::PULL_DOWN, // push button (SW)
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
        APB1_TIMER_CLOCK};

    // IR receiver
    using IrReceiver = IrReceiver_TIM;
    IrReceiver irDevice{loop,
        gpio::PC6 | gpio::AF2 | gpio::Config::PULL_UP, // data pin TIM3_CH1 (does not need to be connected, therefore pull-up)
        timer::TIM3_INFO,
        1, // channel 1
        APB1_TIMER_CLOCK};

    // flash storage
    Flash_flash flash;
    Flash_flash::Buffer<256> flashBuffer{flash};
    BufferStorage storage{storageInfo, flashBuffer};

    Drivers() {
        // set RS485_1_DE high (enable output)
        gpio::enableOutput(gpio::PB0, true);

        // set RS485_2_DE high (enable output)
        gpio::enableOutput(gpio::PA6, true);

        // set RS485_3_DE high (enable output)
        gpio::enableOutput(gpio::PB5, true);
    }
};

Drivers drivers;

extern "C" {

// LED strip 1
void USART1_IRQHandler() {
    drivers.ledStrip1.UART_IRQHandler();
}
void DMA1_Channel1_IRQHandler() {
    drivers.ledStrip1.DMA_IRQHandler();
}

// LED strip 2


// LED strip 3


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
