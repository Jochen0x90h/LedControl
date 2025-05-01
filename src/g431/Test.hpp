#pragma once

#include "config.hpp"
#include <coco/platform/LedStrip_UART_DMA.hpp>
#include <coco/platform/Loop_TIM2.hpp>
#include <coco/platform/platform.hpp>
#include <coco/platform/gpio.hpp>
#include <coco/debug.hpp>


using namespace coco;
using namespace coco::literals;

constexpr int LEDSTRIP_LENGTH = 300;


/*
	Drivers for LedControl
	Uncheck (clear) nSWBOOT0 option byte to disable the built-in boot loader
*/
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
	LedStrip::Buffer<LEDSTRIP_LENGTH * 3> buffer1{ledStrip};
	LedStrip::Buffer<LEDSTRIP_LENGTH * 3> buffer2{ledStrip};

	Drivers() {
		// set RS485_DE high
		gpio::configureOutput(gpio::Config::PB0, true);
	}
};

Drivers drivers;

extern "C" {

// LED strip
void USART1_IRQHandler() {
	drivers.ledStrip.UARTx_IRQHandler();
}
void DMA1_Channel1_IRQHandler() {
	drivers.ledStrip.DMAx_IRQHandler();
}

}
