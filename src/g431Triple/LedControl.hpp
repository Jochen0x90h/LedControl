#pragma once

#include "config.hpp"
#include <coco/platform/Flash_flash.hpp>
#include <coco/platform/InputDevice_EXTI_TIM.hpp>
#include <coco/platform/IrReceiver_TIM.hpp>
#include <coco/platform/LedStrip_UART_DMA.hpp>
#include <coco/platform/Loop_TIM2.hpp>
#include <coco/platform/SpiMaster_SPI_DMA.hpp>
#include <coco/platform/SpiDisplayChannel_SPI_DMA.hpp>
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
    LedStrip strip1{loop,
        gpio::PC4 | gpio::AF7 | gpio::Config::SPEED_HIGH, // USART1 TX (RS485_1_TX)
        uart::USART1_INFO,
        dma::DMA1_CH1_INFO,
        USART1_CLOCK,
        1125ns, // bit time T
        75us}; // reset time
    LedStrip::Buffer<MAX_LEDSTRIP_LENGTH * 3> stripBuffer1{strip1};

    // LED strip 2
    LedStrip strip2{loop,
        gpio::PB3 | gpio::AF7 | gpio::Config::SPEED_HIGH, // USART2 TX (RS485_2_TX)
        uart::USART2_INFO,
        dma::DMA1_CH2_INFO,
        USART2_CLOCK,
        1125ns, // bit time T
        75us}; // reset time
    LedStrip::Buffer<MAX_LEDSTRIP_LENGTH * 3> stripBuffer2{strip2};

    // LED strip 3
    /*LedStrip strip3{loop,
        gpio::PC10 | gpio::AF7 | gpio::Config::SPEED_HIGH, // USART3 TX (RS485_3_TX)
        uart::USART3_INFO,
        dma::DMA1_CH3_INFO,
        USART3_CLOCK,
        1125ns, // bit time T
        75us}; // reset time*/
    //LedStrip::Buffer<MAX_LEDSTRIP_LENGTH * 3> stripBuffer3{strip3};

    Buffer *stripBuffers[LEDSTRIP_COUNT] {&stripBuffer1, &stripBuffer2};//, &stripBuffer3};

    // display
    using SpiMaster = SpiMaster_SPI_DMA;
    SpiMaster displaySpi{loop,
        gpio::PA5 | gpio::AF5 | gpio::Config::SPEED_MEDIUM, // SPI1 SCK (DISP_SCK)
        gpio::PA7 | gpio::AF5 | gpio::Config::SPEED_MEDIUM, // SPI1 MOSI (DISP_MOSI)
        gpio::NONE, // no MISO, send only
        //gpio::PA0, // DC (DISP_D/nC)
        spi::SPI1_INFO,
        dma::DMA1_CH4_CH5_INFO};
        //spi::ClockConfig::DIV32, spi::Config::PHA1_POL1};
    //SpiMaster::Channel displayChannel{displaySpi, gpio::PA4 | gpio::Config::INVERT, true}; // nCS (DISP_nCS)
    SpiDisplayChannel_SPI_DMA displayChannel{displaySpi,
        gpio::PA4 | gpio::Config::SPEED_MEDIUM | gpio::Config::INVERT, // DISP_nCS
        gpio::PA0 | gpio::Config::SPEED_MEDIUM, false, 0x40, // DISP_D/nC
        spi::Format::CLOCK_DIV_32 | spi::Format::PHA1_POL1 | spi::Format::DATA_8};
    SpiMaster::Buffer<1, DISPLAY_WIDTH * DISPLAY_HEIGHT / 8> displayBuffer{displayChannel};

    // display reset pin
    static constexpr OutputPort_GPIO::Config outputConfig[] {
        {gpio::PC15 | gpio::Config::SPEED_MEDIUM | gpio::Config::INVERT, false}, // DISP_nRST
    };
    OutputPort_GPIO resetPin{outputConfig};


    // rotary knob with push button
    using InputDevice = InputDevice_EXTI_TIM;
    static constexpr gpio::Config inputPinConfigs[] {
        gpio::PC13 | gpio::Config::PULL_DOWN, // rotary knob A (ENC_A)
        gpio::PC14 | gpio::Config::PULL_DOWN, // rotary knob B (ENC_B)
        gpio::PB8 | gpio::Config::PULL_DOWN, // push button (SW)`
    };
    static constexpr InputDevice::Config inputConfigs[] {
        {0, 0, InputDevice::Init::INPUT, InputDevice::Action::DECREMENT_WHEN_ENABLED, InputDevice::Action::INCREMENT_WHEN_ENABLED, 1ms, 1ms}, // quadrature decoder (inputs 0 and 1, counter 0)
        {2, 1, InputDevice::Init::LOW, InputDevice::Action::INCREMENT, InputDevice::Action::NONE, 10ms, 10ms}, // button press (input 2, counter 1)
        {2, 2, InputDevice::Init::LOW, InputDevice::Action::INCREMENT, InputDevice::Action::NONE, 3s, 10ms}, // button long press (input 2, counter 2)
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
    IrReceiver::Buffer<80> irBuffer1{irDevice};
    IrReceiver::Buffer<80> irBuffer2{irDevice};

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
    drivers.strip1.UART_IRQHandler();
}
void DMA1_Channel1_IRQHandler() {
    drivers.strip1.DMA_IRQHandler();
}

// LED strip 2
void USART2_IRQHandler() {
    drivers.strip2.UART_IRQHandler();
}
void DMA1_Channel2_IRQHandler() {
    drivers.strip2.DMA_IRQHandler();
}

// LED strip 3


// display
void DMA1_Channel4_IRQHandler() {
    drivers.displaySpi.DMA_Rx_IRQHandler();
}

// rotary knob with push button
void EXTI9_5_IRQHandler() {
    drivers.input.EXTI_IRQHandler();
}
void EXTI15_10_IRQHandler() {
    drivers.input.EXTI_IRQHandler();
}
void TIM4_IRQHandler() {
    drivers.input.TIM_IRQHandler();
}

// IR receiver
void TIM3_IRQHandler() {
    drivers.irDevice.TIM_IRQHandler();
}

}
