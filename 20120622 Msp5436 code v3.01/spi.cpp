#include "msp430x54x.h"
#include "spi.h"


/* Initialize and enable the SPI module */
void spi_initialize()
{
  P3OUT |= 0x01; // Setup P3.0 as the SS signal, active
  	  	// low. So, initialize it high.  P3DIR |= 0x01;
  P3DIR |= 0x01;

  P3SEL |= 0x0E;

  UCB0CTL1 |= UCSWRST;                      // **Put state machine in reset**
  UCB0CTL0 |= UCMST+UCSYNC+UCCKPL+UCMSB;    // 3-pin, 8-bit SPI master
                                            // Clock polarity high, MSB
  UCB0CTL1 |= UCSSEL_2;                     // SMCLK
//  UCB0BR0 = 0x02;                           // /2
  UCB0BR0 = 0;                           // /2
  UCB0BR1 = 0;                              //

  //  UCB0MCTL = 0;                             // No modulation
  UCB0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
  UCB0IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt
 
}

/* Set the baud-rate divisor. The correct value is computed by dividing
the clock rate by the desired baud rate. The minimum divisor allowed
is 2. */
void spi_set_divisor(unsigned int divisor)
{
	UCB0CTL1 |= UCSWRST; // Temporarily disable the SPI module
	UCB0BR1 = divisor >> 8;
	UCB0BR0 = divisor;
	UCB0CTL1 &= ~UCSWRST; // Re-enable SPI
}

/* Assert the CS signal, active low (CS=0) */
void spi_cs_assert()
{
	// Pin 3.0, Pin 28
	P3OUT &= ~0x01;
}

/* Deassert the CS signal (CS=1) */
void spi_cs_deassert()
{
	// Pin 3.4, Pin 32
	P3OUT |= 0x01;
}

/* Send a single byte over the SPI port */
void spi_send_byte(unsigned char input)
{
        UCB0IFG &= ~UCRXIFG;
	/* Send the byte */
	UCB0TXBUF = input;
	/* Wait for the byte to be sent */
      	while ((UCB0IFG & UCRXIFG) == 0) { }

}

/* Receive a byte. Output an 0xFF (the bus idles high) to receive the byte */
unsigned char spi_rcv_byte()
{
	unsigned char tmp;
        UCB0IFG &= ~UCRXIFG;
	/* Send the byte */
	UCB0TXBUF = 0xFF;
	/* Wait for the byte to be received */
      	while ((UCB0IFG & UCRXIFG) == 0) { }
	tmp = UCB0RXBUF;
	return (tmp);
}

/* Disable the SPI module. This function assumes the module had
* already been initialized. */
void spi_disable()
{
	/* Put the SPI module in reset mode */
  	UCB0CTL1 |= UCSWRST; // Re-enable SPI
}

void spi_enable()
{

	/* Take the SPI module out of reset mode */
  	UCB0CTL1 &= ~UCSWRST;
}
