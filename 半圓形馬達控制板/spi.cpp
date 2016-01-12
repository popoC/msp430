#include <msp430x16x.h>
#include "main.h"
#include "spi.h"

/* Initialize and enable the SPI module */
void spi_initialize()
{

	P3SEL |= 0x00E;  // Setup P3 for SPI mode
	P3OUT |= 0x001; // Setup P3.0 as the SS signal, active
			// low. So, initialize it high.
	P3DIR |= 0x001; // Set up P3.0 as an output

	U0CTL = (CHAR | SYNC | MM | SWRST); 	// 8-bit, SPI, Master
	U0TCTL = (SSEL1 | STC | CKPH); 		// Normal polarity, 3-wire
	//U0TCTL = ( STC | CKPH); 		// Normal polarity, 3-wire UCLK
        U0BR0 = 0x002; 				// SPICLK = SMCLK/2 (2=Minimum divisor)
	U0BR1 = 0x000;
	U0MCTL = 0x000;
	ME1 |= USPIE0; 				// Module enable
	U0CTL &= ~SWRST; 			// SPI enable
}

/* Set the baud-rate divisor. The correct value is computed by dividing
the clock rate by the desired baud rate. The minimum divisor allowed
is 2. */
void spi_set_divisor(unsigned int divisor)
{
	U0CTL |= SWRST; // Temporarily disable the SPI module
	U0BR1 = divisor >> 8;
	U0BR0 = divisor;
	U0CTL &= ~SWRST; // Re-enable SPI
}

/* Assert the CS signal, active low (CS=0) */
void spi_cs_assert()
{
	// Pin 3.0, Pin 28
	P3OUT &= ~0x001;
}

/* Deassert the CS signal (CS=1) */
void spi_cs_deassert()
{
	// Pin 3.4, Pin 32
	P3OUT |= 0x001;
}

/* Send a single byte over the SPI port */
void spi_send_byte(unsigned char input)
{
	IFG1 &= ~URXIFG0;
	/* Send the byte */
	TXBUF0 = input;
	/* Wait for the byte to be sent */
	while ((IFG1 & URXIFG0) == 0) { }
}

/* Receive a byte. Output an 0xFF (the bus idles high) to receive the byte */
unsigned char spi_rcv_byte()
{
	unsigned char tmp;
	IFG1 &= ~URXIFG0;
	/* Send the byte */
	TXBUF0 = 0xFF;
	/* Wait for the byte to be received */
	while ((IFG1 & URXIFG0) == 0) { }
	tmp = U0RXBUF;
	return (tmp);
}

/* Disable the SPI module. This function assumes the module had
* already been initialized. */
void spi_disable()
{
	/* Put the SPI module in reset mode */
	U0CTL |= SWRST;
	/* Disable the USART module */
	ME1 &= ~USPIE0;
}

void spi_enable()
{
	/* Enable the USART module */
	ME1 |= USPIE0;
	/* Take the SPI module out of reset mode */
	U0CTL &= ~SWRST;
}
