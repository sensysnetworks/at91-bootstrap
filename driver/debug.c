/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support  -  ROUSSET  -
 * ----------------------------------------------------------------------------
 * Copyright (c) 2006, Atmel Corporation

 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaiimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 * File Name           : debug.c
 * Object              :
 * Creation            : ODi Apr 19th 2006
 *-----------------------------------------------------------------------------
 */
#include "../include/part.h"
#include "../include/main.h"

#ifdef CFG_DEBUG
//#define DEBUG_PORT AT91C_BASE_DBGU
#define DEBUG_PORT AT91C_BASE_US3

/* Write DBGU register */
static inline void write_dbgu(unsigned int offset, const unsigned int value)
{
	writel(value, offset + DEBUG_PORT);
}

/* Read DBGU registers */
static inline unsigned int read_dbgu( unsigned int offset)
{
	return readl(offset + DEBUG_PORT);
}


//*----------------------------------------------------------------------------
//* \fn    dbg_init
//* \brief This function is used to configure the DBGU COM port
//*----------------------------------------------------------------------------*/
void dbg_init(unsigned int baudrate)
{
	/* Disable interrupts */
	write_dbgu(US_IDR, -1);
	/* Reset the receiver and transmitter */
	write_dbgu(US_CR, AT91C_US_RSTRX | AT91C_US_RSTTX | AT91C_US_RXDIS  | AT91C_US_TXDIS);
	/* Configure the baudrate */
	write_dbgu(US_BRGR, baudrate);
	/* Configure USART in Asynchronous mode */
	write_dbgu(US_MR, (AT91C_US_CLKS_CLOCK | AT91C_US_CHRL_8_BITS | AT91C_US_PAR_NONE | AT91C_US_NBSTOP_1_BIT));
	/* Enable RX and Tx */
	write_dbgu(US_CR, AT91C_US_RXEN | AT91C_US_TXEN);
}

unsigned char dbg_getch(void)
{
	while ( !(read_dbgu(DBGU_CSR) & AT91C_US_RXRDY) );
	return (unsigned char) read_dbgu(DBGU_RHR);
}

void dbg_putch(unsigned char chr)
{
	while ( !(read_dbgu(DBGU_CSR) & AT91C_US_TXRDY) );
	write_dbgu(DBGU_THR, chr);	
}

void dbg_gets(char * str)
{
	while((*str=dbg_getch())!='\r') 
	{
		dbg_putch(*str);
		str++;
	}
	dbg_putch('\n');
	dbg_putch('\r');
	*str = 0;
}

int dbg_kbhit(void)
{
	return (read_dbgu(DBGU_CSR) & AT91C_US_RXRDY);
}

unsigned int dbg_atoi(char * str)
{
	int number = 0;

	do
	{
		if(*str>='0' && *str<='9')
			number += *str - '0';
		str++;
		if(*str != 0) number = number * 10;
	} while(*str);

	return number;
}

unsigned int dbg_atox(char * str)
{
	int number = 0;

	do
	{
		if(*str>='0' && *str<='9')
			number += *str - '0';
		if(*str>='a' && *str<='f')
			number += *str - 'a' + 0xa;
		if(*str>='A' && *str<='F')
			number += *str - 'A' + 0xa;
		str++;
		if(*str != 0) number = number << 4;
	} while(*str);

	return number;
}

//*----------------------------------------------------------------------------
//* \fn    dbg_print
//* \brief This function is used to configure the DBGU
//*----------------------------------------------------------------------------*/
void dbg_print(const char *ptr)
{
	int i=0;

	while (ptr[i] != '\0') {
		while ( !(read_dbgu(DBGU_CSR) & AT91C_US_TXRDY) );
		write_dbgu(DBGU_THR, ptr[i]);
		i++;
	}
}

void dbg_print_hex(const unsigned int number)
{
	unsigned int nibble;
	unsigned int mask;
	unsigned int shift;

	mask = 0xF0000000;
	shift = 28;
	do
	{
		nibble = (number&mask)>>shift;
		while ( !(read_dbgu(DBGU_CSR) & AT91C_US_TXRDY) );
		write_dbgu(DBGU_THR, (nibble <= 9)?(nibble+'0'):(nibble+'A'-0xA));
		mask = mask >> 4;
		shift = shift - 4;
	}while(mask);

}

void test_mem(void)
{
	unsigned long sdram_address = (0x20000000);
	unsigned long count = 0x2000000/4;
	unsigned long *p1;
	unsigned long i,j,k;
	unsigned long s = 2;
	char str[256];
	unsigned int img_addr;
	unsigned int *sdram_dest = (unsigned int*)(0x20FD0000);
	unsigned int *nor_source = (unsigned int*)(0x10040000);
	unsigned int index;
	unsigned char input = '\0';
	unsigned int img_size = 0x30000;

	
	while(1)
	{
		for (j = 0; j < 16; j++) 
		{
			p1 = (unsigned long *) sdram_address;
			dbg_print("Setting");
			for (i = 0; i < count; i++) 
			{
				k = ((j + i) % 2) == 0 ? (i << s) : ~(i << s);
				*p1 = k;

				if (*p1 != (((j + i) % 2) == 0 ? (i << s) : ~(i << s))) 
				{
					dbg_print("FAILURE 1 : possible bad address line at address 0x");
					dbg_print_hex(0x20000000 +(i << 2));
					dbg_print("\r\n");
					dbg_print_hex(*p1);
					dbg_print(" != ");
					dbg_print_hex((((j + i) % 2) == 0 ? (i << s) : ~(i << s)));
					dbg_print("\r\n");
				}
				p1++;
				if(dbg_kbhit()) goto outofhere;

			}

			p1 = (unsigned long *) sdram_address;
			dbg_print("Testing");
			for (i = 0; i < count; i++, p1++) 
			{
				if (*p1 != (((j + i) % 2) == 0 ? (i << s) : ~(i << s))) 
				{
					dbg_print("FAILURE 2 : possible bad address line at address 0x");
					dbg_print_hex(0x20000000 +(i << 2));
					dbg_print("\r\n");
					dbg_print_hex(*p1);
					dbg_print(" != ");
					dbg_print_hex((((j + i) % 2) == 0 ? (i << s) : ~(i << s)));
					dbg_print("\r\n");
				}
				if(dbg_kbhit()) goto outofhere;
			}
		}
	}
	
outofhere:

	dbg_print("\r\nEnter the starting address of image:0x");
	dbg_gets(str);		
	dbg_print_hex(dbg_atox(str));
	dbg_print("\n\r");
	
	nor_source = (unsigned int*) dbg_atox(str);
	img_addr = dbg_atox(str);
		
	dbg_print("\r\nCopying image to SDRAM\r\n");

	dbg_print("Display (y/n):\r\n");
	
	input = dbg_getch();
	
	for(index=0;index<img_size/4;index++)
	{
		if((input == 'y')||(input == 'Y'))
		{
			if((index % 4)==0) dbg_print_hex(index+img_addr);
			dbg_print(" : ");
			dbg_print_hex(*nor_source);
			if((index % 4)==3) dbg_print("\r\n");
			
			if((index % 40)==39)
			{
				dbg_print("Display (y/n):\r\n");
				input = dbg_getch();
			}
		}
		else if(input == 'r') goto outofhere;

		*sdram_dest = *nor_source;
		if((input == 'y')||(input == 'Y'))
		{
			dbg_print(",");
			dbg_print_hex(*sdram_dest);
		}
		sdram_dest++;
		nor_source++;
	}

}

#endif /* CFG_DEBUG */
