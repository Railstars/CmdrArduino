#ifndef __DCCHARDWARE_H__
#define __DCCHARDWARE_H__

#if defined(LPC1769)
#include "LPC17xx.h"
#include "lpc17xx_mcpwm.h"
#include "lpc17xx_clkpwr.h"
#include "lpc17xx_pinsel.h"
#elif defined(LPC1343)
#include "LPC13xx.h"
#include "lpc13xx_tmr.h"
#include "lpc13xx_syscon.h"
#include "lpc13xx_iocon.h"
#include "lpc13xx_gpio.h"
#elif defined(__AVR__)
#include "Arduino.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

void setup_DCC_waveform_generator(void);
void DCC_waveform_generation_start(void);
void DCC_IRQ(void);

void DCC_DisableOutput(void);
void DCC_EnableOutput(void);
uint8_t DCC_OutputState(void);
void setDC(int8_t new_val);

#if defined(LPC1769)
#define ENABLE_INTERRUPTS NVIC_EnableIRQ(MCPWM_IRQn)
#define DISABLE_INTERRUPTS NVIC_DisableIRQ(MCPWM_IRQn)
#elif defined(LPC1343)
#define ENABLE_INTERRUPTS NVIC_EnableIRQ(TIMER_32_1_IRQn)
#define DISABLE_INTERRUPTS NVIC_DisableIRQ(TIMER_32_1_IRQn)
#endif

#ifdef __cplusplus
}
#endif

#endif //__DCCHARDWARE_H__
