// DAC.c
// This software configures DAC output
// Lab 6 requires a minimum of 4 bits for the DAC, but you could have 5 or 6 bits
// Runs on TM4C123
// Program written by: Rick Hanish, Caleb Domingue
// Date Created: 3/6/17 
// Last Modified: 3/22/21 
// Lab number: 6

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
// Code files contain the actual implemenation for public functions
// this file also contains an private functions and private data

// **************DAC_Init*********************
// Initialize 4-bit or 6-bit DAC, called once 
// Input: none
// Output: none
void DAC_Init(void){
	SYSCTL_RCGCGPIO_R |= 0x02;
	while((SYSCTL_RCGCGPIO_R & 0x02) == 0);
	GPIO_PORTB_DIR_R |= 0x0F;
	GPIO_PORTB_DEN_R |= 0x0F;
	GPIO_PORTB_AMSEL_R &= ~0x0F;
	GPIO_PORTB_AFSEL_R &= ~0x0F;
	GPIO_PORTB_PCTL_R &= ~0x00000FFF;
}

// **************DAC_Out*********************
// output to DAC
// Input: 4-bit data, 0 to 15 
// or     6-bit data, 0 to 63
// Input=n is converted to n*3.3V/15
// or Input=n is converted to n*3.3V/63
// Output: none
void DAC_Out(uint32_t data){
	GPIO_PORTB_DATA_R = data;
}
