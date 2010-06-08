/**************************************************************************
 * arch/arm/src/lpc17xx/lpc17_lowputc.c
 *
 *   Copyright (C) 2010 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 **************************************************************************/

/**************************************************************************
 * Included Files
 **************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

#include <arch/irq.h>
#include <arch/board/board.h>

#include "up_internal.h"
#include "up_arch.h"

#include "lpc17_internal.h"
#include "lpc17_uart.h"
#include "lpc17_serial.h"

/**************************************************************************
 * Private Definitions
 **************************************************************************/
     
/* Baud calculations

BAUD = PCLK / (16 x (256 x DLM + DLL) x (1 + DIVADDVAL/MULVAL))

Where PCLK is the peripheral clock, DLM and DLL are the standard
UART baud rate divider registers, and DIVADDVAL and MULVAL are UART
fractional baud rate generator specific parameters.

The value of MULVAL and DIVADDVAL should comply to the following conditions:

1. 1 <= MULVAL <= 15
2. 0 <= DIVADDVAL <= 14
3. DIVADDVAL < MULVAL

The peripheral clock is controlled by:

#define SYSCON_PCLKSET_CCLK4 PCLK_peripheral = CCLK/4
#define SYSCON_PCLKSET_CCLK  PCLK_peripheral = CCLK
#define SYSCON_PCLKSET_CCLK2 PCLK_peripheral = CCLK/2
#define SYSCON_PCLKSET_CCLK6 PCLK_peripheral = CCLK/8 (except CAN1, CAN2, and CAN)
#define SYSCON_PCLKSET_CCLK8 PCLK_peripheral = CCLK/6 (CAN1, CAN2, and CAN)
 */

/**************************************************************************
 * Private Types
 **************************************************************************/

/**************************************************************************
 * Private Function Prototypes
 **************************************************************************/

/**************************************************************************
 * Global Variables
 **************************************************************************/

/**************************************************************************
 * Private Variables
 **************************************************************************/

/**************************************************************************
 * Private Functions
 **************************************************************************/

/**************************************************************************
 * Public Functions
 **************************************************************************/

/**************************************************************************
 * Name: up_lowputc
 *
 * Description:
 *   Output one byte on the serial console
 *
 **************************************************************************/

void up_lowputc(char ch)
{
  /* Wait for the transmitter to be available */

  while ((getreg32(CONSOLE_BASE+LPC17_UART_LSR_OFFSET) & UART_LSR_THRE) == 0);

  /* Send the character */

  putreg32((uint32_t)ch, CONSOLE_BASE+LPC17_UART_THR_OFFSET);
}

/**************************************************************************
 * Name: lpc17_lowsetup
 *
 * Description:
 *   This performs basic initialization of the UART used for the serial
 *   console.  Its purpose is to get the console output availabe as soon
 *   as possible.
 *
 *   The UART0/2/3 peripherals are configured using the following registers:
 *   1. Power: In the PCONP register, set bits PCUART0/1/2/3.
 *      On reset, UART0 and UART 1 are enabled (PCUART0 = 1 and PCUART1 = 1)
 *      and UART2/3 are disabled (PCUART1 = 0 and PCUART3 = 0).
 *   2. Peripheral clock: In the PCLKSEL0 register, select PCLK_UART0 and
 *      PCLK_UART1; in the PCLKSEL1 register, select PCLK_UART2 and PCLK_UART3.
 *   3. Baud rate: In the LCR register, set bit DLAB = 1. This enables access
 *      to registers DLL and DLM for setting the baud rate. Also, if needed,
 *      set the fractional baud rate in the fractional divider 
 *   4. UART FIFO: Use bit FIFO enable (bit 0) in FCR register to
 *      enable FIFO.
 *   5. Pins: Select UART pins through the PINSEL registers and pin modes
 *      through the PINMODE registers. UART receive pins should not have
 *      pull-down resistors enabled.
 *   6. Interrupts: To enable UART interrupts set bit DLAB = 0 in the LCRF
 *      register. This enables access to IER. Interrupts are enabled
 *      in the NVIC using the appropriate Interrupt Set Enable register.
 *   7. DMA: UART transmit and receive functions can operate with the
 *      GPDMA controller.
 *
 **************************************************************************/

void lpc17_lowsetup(void)
{
#if 0
  uint32_t regval;

  /* Step 1: Enable power for all selected UARTs */

  regval = getreg32(LPC17_SYSCON_PCONP);
  regval &= ~(SYSCON_PCONP_PCUART0|SYSCON_PCONP_PCUART1|SYSCON_PCONP_PCUART2|SYSCON_PCONP_PCUART3)
#ifdef CONFIG_LPC17_UART0
  regval |= SYSCON_PCONP_PCUART0;
#endif
#ifdef CONFIG_LPC17_UART1
  regval |= SYSCON_PCONP_PCUART1;
#endif
#ifdef CONFIG_LPC17_UART2
  regval |= SYSCON_PCONP_PCUART2;
#endif
#ifdef CONFIG_LPC17_UART3
  regval |= SYSCON_PCONP_PCUART3;
#endif
  putreg32(regval, LPC17_SYSCON_PCONP);

/* Step 2: Enable peripheral clocking for all selected UARTs */

#define SYSCON_PCLKSET_MASK           (3)

#define SYSCON_PCLKSEL0_UART0_SHIFT   (6)         /* Bits 6-7: Peripheral clock UART0 */
#define SYSCON_PCLKSEL0_UART0_MASK    (3 << SYSCON_PCLKSEL0_UART0_MASK)
#define SYSCON_PCLKSEL0_UART1_SHIFT   (8)         /* Bits 8-9: Peripheral clock UART1 */
#define SYSCON_PCLKSEL0_UART1_MASK    (3 << SYSCON_PCLKSEL0_UART1_SHIFT)


#define SYSCON_PCLKSEL1_UART2_SHIFT   (16)        /* Bits 16-17: Peripheral clock UART2 */
#define SYSCON_PCLKSEL1_UART2_MASK    (3 << SYSCON_PCLKSEL1_UART2_SHIFT)
#define SYSCON_PCLKSEL1_UART3_SHIFT   (18)        /* Bits 18-19: Peripheral clock UART3 */
#define SYSCON_PCLKSEL1_UART3_MASK    (3 << SYSCON_PCLKSEL1_UART3_SHIFT)

  /* Configure UART pins for all selected UARTs */

#define GPIO_UART0_TXD     (GPIO_ALT1 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN2)
#define GPIO_UART0_RXD     (GPIO_ALT1 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN3

#define GPIO_UART1_TXD_1   (GPIO_ALT1 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN15)
#define GPIO_UART1_RXD_1   (GPIO_ALT1 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN16)
#define GPIO_UART1_CTS_1   (GPIO_ALT1 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN17)
#define GPIO_UART1_DCD_1   (GPIO_ALT1 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN18)
#define GPIO_UART1_DSR_1   (GPIO_ALT1 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN19)
#define GPIO_UART1_DTR_1   (GPIO_ALT1 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN20)
#define GPIO_UART1_RI_1    (GPIO_ALT1 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN21)
#define GPIO_UART1_RTS_1   (GPIO_ALT1 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN22)

#define GPIO_UART1_TXD_2   (GPIO_ALT2 | GPIO_PULLUP | GPIO_PORT2 | GPIO_PIN0)
#define GPIO_UART1_RXD_2   (GPIO_ALT2 | GPIO_PULLUP | GPIO_PORT2 | GPIO_PIN1)
#define GPIO_UART1_CTS_2   (GPIO_ALT2 | GPIO_PULLUP | GPIO_PORT2 | GPIO_PIN2)
#define GPIO_UART1_DCD_2   (GPIO_ALT2 | GPIO_PULLUP | GPIO_PORT2 | GPIO_PIN3)
#define GPIO_UART1_DSR_2   (GPIO_ALT2 | GPIO_PULLUP | GPIO_PORT2 | GPIO_PIN4)
#define GPIO_UART1_DTR_2   (GPIO_ALT2 | GPIO_PULLUP | GPIO_PORT2 | GPIO_PIN5)
#define GPIO_UART1_RI_2    (GPIO_ALT2 | GPIO_PULLUP | GPIO_PORT2 | GPIO_PIN6)
#define GPIO_UART1_RTS_2   (GPIO_ALT2 | GPIO_PULLUP | GPIO_PORT2 | GPIO_PIN7)

#define GPIO_UART2_TXD_1   (GPIO_ALT1 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN10)
#define GPIO_UART2_RXD_1   (GPIO_ALT1 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN11)
#define GPIO_UART2_TXD_2   (GPIO_ALT2 | GPIO_PULLUP | GPIO_PORT2 | GPIO_PIN8)
#define GPIO_UART2_RXD_2   (GPIO_ALT2 | GPIO_PULLUP | GPIO_PORT2 | GPIO_PIN9)

#define GPIO_UART3_TXD_1   (GPIO_ALT2 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN0)
#define GPIO_UART3_RXD_1   (GPIO_ALT2 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN1)
#define GPIO_UART3_TXD_2   (GPIO_ALT3 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN25)
#define GPIO_UART3_RXD_2   (GPIO_ALT3 | GPIO_PULLUP | GPIO_PORT0 | GPIO_PIN26)
#define GPIO_UART3_TXD_3   (GPIO_ALT3 | GPIO_PULLUP | GPIO_PORT4 | GPIO_PIN28)
#define GPIO_UART3_RXD_3     (GPIO_ALT3 | GPIO_PULLUP | GPIO_PORT4 | GPIO_PIN29)


#ifdef CONFIG_LPC17_UART0
  (void)lpc17_configgpio(GPIO_UART0_RXD);
  (void)lpc17_configgpio(GPIO_UART0_TXD);
  (void)lpc17_configgpio(GPIO_UART0_CTS);
  (void)lpc17_configgpio(GPIO_UART0_RTS);
#endif
#ifdef CONFIG_LPC17_UART1
  (void)lpc17_configgpio(GPIO_UART1_RXD);
  (void)lpc17_configgpio(GPIO_UART1_TXD);
  (void)lpc17_configgpio(GPIO_UART1_CTS);
  (void)lpc17_configgpio(GPIO_UART1_RTS);
#endif
#ifdef CONFIG_LPC17_UART2
  (void)lpc17_configgpio(GPIO_UART2_RXD);
  (void)lpc17_configgpio(GPIO_UART2_TXD);
  (void)lpc17_configgpio(GPIO_UART2_CTS);
  (void)lpc17_configgpio(GPIO_UART2_RTS);
#endif
#ifdef CONFIG_LPC17_UART3
  (void)lpc17_configgpio(GPIO_UART3_RXD);
  (void)lpc17_configgpio(GPIO_UART3_TXD);
  (void)lpc17_configgpio(GPIO_UART3_CTS);
  (void)lpc17_configgpio(GPIO_UART3_RTS);
#endif

#ifdef GPIO_CONSOLE_RXD
#endif
#ifdef GPIO_CONSOLE_TXD
  (void)lpc17_configgpio(GPIO_CONSOLE_TXD);
#endif
#ifdef GPIO_CONSOLE_CTS
  (void)lpc17_configgpio(GPIO_CONSOLE_CTS);
#endif
#ifdef GPIO_CONSOLE_RTS
  (void)lpc17_configgpio(GPIO_CONSOLE_RTS);
#endif

  /* Configure the console (only) */
#if defined(HAVE_CONSOLE) && !defined(CONFIG_SUPPRESS_UART_CONFIG)
  /* Reset and disable receiver and transmitter */

  putreg32((UART_CR_RSTRX|UART_CR_RSTTX|UART_CR_RXDIS|UART_CR_TXDIS),
           CONSOLE_BASE+LPC17_UART_CR_OFFSET);

  /* Disable all interrupts */

  putreg32(0xffffffff, CONSOLE_BASE+LPC17_UART_IDR_OFFSET);

  /* Set up the mode register */

  putreg32(MR_VALUE, CONSOLE_BASE+LPC17_UART_MR_OFFSET);

  /* Configure the console baud */

  putreg32(((LPC17_MCK_FREQUENCY + (LPC17_CONSOLE_BAUD << 3))/(LPC17_CONSOLE_BAUD << 4)),
           CONSOLE_BASE+LPC17_UART_BRGR_OFFSET);

  /* Enable receiver & transmitter */

  putreg32((UART_CR_RXEN|UART_CR_TXEN),
           CONSOLE_BASE+LPC17_UART_CR_OFFSET);
#endif
#endif /* 0 */
}


