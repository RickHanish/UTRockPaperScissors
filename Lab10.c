// Lab10.c
// Runs on TM4C123
// Jonathan Valvano and Daniel Valvano
// This is a starter project for the EE319K Lab 10

// Last Modified: 4/30/2021
// http://www.spaceinvaders.de/
// sounds at http://www.classicgaming.cc/classics/spaceinvaders/sounds.php
// http://www.classicgaming.cc/classics/spaceinvaders/playguide.php
/* 
 Copyright 2021 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */
// ******* Possible Hardware I/O connections*******************
// Slide pot pin 1 connected to ground
// Slide pot pin 2 connected to PD2/AIN5
// Slide pot pin 3 connected to +3.3V 

// Button 1 connected to PE0
// Button 2 connected to PE1
// Button 3 connected to PE2

// 8*R resistor DAC bit 0 on PB0 (least significant bit)
// 4*R resistor DAC bit 1 on PB1
// 2*R resistor DAC bit 2 on PB2
// 1*R resistor DAC bit 3 on PB3 (most significant bit)

// VCC   3.3V power to OLED
// GND   ground
// SCL   PD0 I2C clock (add 1.5k resistor from SCL to 3.3V)
// SDA   PD1 I2C data

//************WARNING***********
// The LaunchPad has PB7 connected to PD1, PB6 connected to PD0
// Option 1) do not use PB7 and PB6
// Option 2) remove 0-ohm resistors R9 R10 on LaunchPad
//******************************

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/CortexM.h"
#include "SSD1306.h"
#include "Print.h"
#include "Random.h"
#include "ADC.h"
#include "Images.h"
#include "Sound.h"
#include "Timer0.h"
#include "Timer1.h"
#include "TExaS.h"
#include "Switch.h"
#include "DAC.h"
//********************************************************************************
// debuging profile, pick up to 7 unused bits and send to Logic Analyzer
#define PB54                  (*((volatile uint32_t *)0x400050C0)) // bits 5-4
#define PF321                 (*((volatile uint32_t *)0x40025038)) // bits 3-1
// use for debugging profile
#define PF0			  (*((volatile uint32_t *)0x40025004))
#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))
#define PF4       (*((volatile uint32_t *)0x40025040))
#define PB5       (*((volatile uint32_t *)0x40005080)) 
#define PB4       (*((volatile uint32_t *)0x40005040)) 
#define PE0				(*((volatile uint32_t *)0x40024004)) 
#define PE1				(*((volatile uint32_t *)0x40024008)) 
#define PE2				(*((volatile uint32_t *)0x40024010))
#define PE3				(*((volatile uint32_t *)0x40024020))
#define PE3210    (*((volatile uint32_t *)0x4002403C))


uint32_t points;
int MailStatus;
uint32_t MailValue;

void SSD1306_DrawDec(int16_t x, int16_t y, uint32_t value, uint16_t color);
uint16_t getX(void);
int main(void);

// TExaSdisplay logic analyzer shows 7 bits 0,PB5,PB4,PF3,PF2,PF1,0 
// edit this to output which pins you use for profiling
// you can output up to 7 pins
void LogicAnalyzerTask(void){
  UART0_DR_R = 0x80|PF321|PB54; // sends at 10kHz
}

void ScopeTask(void){  // called 10k/sec
  UART0_DR_R = (ADC1_SSFIFO3_R>>4); // send ADC to TExaSdisplay
}

void SysTick_Init(unsigned long period){
  NVIC_ST_CTRL_R = 0;
	NVIC_ST_RELOAD_R = period - 1;
	NVIC_ST_CURRENT_R = 0;
	NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R & 0x00FFFFFF) | 0x20000000;
  NVIC_ST_CTRL_R = 0x0007;
}

uint32_t Convert(uint32_t convertIn){
	uint32_t ret = ((convertIn + 20) * points) / 4095;
	if(ret > points) ret = points;
	return ret;
}


//int NeedToDraw;
typedef enum {dead,alive} status_t;
struct sprite {
	 int32_t x; 	//x 0-127
	 int32_t y; 	//y 0-63
	 int32_t vy;	//pixels/50ms
	 const uint8_t *image; //ptr->image
	 status_t life; //dead/alive
};
typedef struct sprite sprite_t;
sprite_t Bevo; //player sprites
sprite_t Student;
sprite_t Eng;

sprite_t BevoCom; //computer sprites
sprite_t StudentCom;
sprite_t EngCom;


void SetPlayer_Init(void) {
	
		Bevo.x = 50; //location set for player sprites
		Bevo.y = 0;
		Bevo.image = Bevo3;
		Bevo.life = dead;

		Student.x =50;
		Student.y = 0;
		Student.image = Student1;
		Student.life = dead;
	
		Eng.x = 50;
		Eng.y = 0;
		Eng.image = Eng0;
		Eng.life = dead;
	
		BevoCom.x = 50; //location set for computer sprites
		BevoCom.y = 70;
		BevoCom.image = Bevo3;
		BevoCom.life = dead;

		StudentCom.x =50;
		StudentCom.y = 70;
		StudentCom.image = Student1;
		StudentCom.life = dead;
	
		EngCom.x = 50;
		EngCom.y = 70;
		EngCom.image = Eng0;
		EngCom.life = dead;

}


void Draw(void){
		SSD1306_ClearBuffer();
		if(Bevo.life == alive)
			SSD1306_DrawBMP(Bevo.x, Bevo.y, 		//draw all sprites
			Bevo.image, 0, SSD1306_WHITE);
	
		if(Student.life == alive)
			SSD1306_DrawBMP(Student.x, Student.y, 
			Student.image, 0, SSD1306_WHITE);
	
		if(Eng.life == alive)
			SSD1306_DrawBMP(Eng.x, Eng.y, 
			Eng.image, 0, SSD1306_WHITE);
	
		if(BevoCom.life == alive)
			SSD1306_DrawBMP(BevoCom.x, BevoCom.y, 
			BevoCom.image, 0, SSD1306_WHITE);
	
		if(StudentCom.life == alive)
			SSD1306_DrawBMP(StudentCom.x, StudentCom.y, 
			StudentCom.image, 0, SSD1306_WHITE);
	
		if(EngCom.life == alive)
			SSD1306_DrawBMP(EngCom.x, EngCom.y, 
			EngCom.image, 0, SSD1306_WHITE);
	
		SSD1306_OutBuffer();
}



// edit this to initialize which pins you use for profiling
void Profile_Init(void){
  SYSCTL_RCGCGPIO_R |= 0x32;      // activate port B,E,F
  while((SYSCTL_PRGPIO_R&0x20) != 0x20){};
  GPIO_PORTF_DIR_R |=  0x0E;   // output on PF3,2,1 
	//GPIO_PORTF_DIR_R &= !0x11;	 // input  on PF4,1
  GPIO_PORTF_DEN_R |=  0x0E;   // enable digital I/O on PF3,2,1
  GPIO_PORTB_DIR_R |=  0x30;   // output on PB4 PB5
  GPIO_PORTB_DEN_R |=  0x30;   // enable on PB4 PB5  
	GPIO_PORTE_DIR_R &=  ~0x0F;  // input on PE3-0
	GPIO_PORTE_DEN_R |=  0x0F;	 // enable on PE3-0
}
//********************************************************************************
/*
volatile uint32_t FallingEdges;
void Edge_Init(void){
	FallingEdges = 0;
	GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;
	GPIO_PORTF_CR_R |= 0x11;
	GPIO_PORTF_DIR_R &= ~0x11;
	GPIO_PORTF_AFSEL_R &= ~0x11;
	GPIO_PORTF_DEN_R |= 0x11;
	GPIO_PORTF_PCTL_R &= ~0x000F0000;
	GPIO_PORTF_AMSEL_R &= ~0x11;
	GPIO_PORTF_PUR_R |= 0x11;
	GPIO_PORTF_IS_R &= ~0x10;
	GPIO_PORTF_IBE_R &= ~0x10;
	GPIO_PORTF_IEV_R &= ~0x10;
	GPIO_PORTF_ICR_R = 0x10;
	GPIO_PORTF_IM_R |= 0x10;
	NVIC_PRI7_R = (NVIC_PRI7_R & 0xFF00FFFF) | 0x00A00000;
	NVIC_EN0_R = 0x40000000;
}*/
 
void Delay100ms(uint32_t count); // time delay in 0.1 seconds

typedef enum {English, Spanish} Language_t;
Language_t myLanguage = English;

void gameOver(){
	SSD1306_OutClear();
	SSD1306_SetCursor(0,1);
	switch(myLanguage){
		case English:
			SSD1306_OutString("You cashed out!\n");
			SSD1306_OutString("Your score was ");
			LCD_OutDec(points);
			SSD1306_OutString("!\n\nPress 1 to start a\nnew game with\n500 points!");
			break;
		case Spanish:
			SSD1306_OutString("Lo cobraste.\nTu puntuacion fue 500\n\nPresione 1 para\ncomenzar un nuevo\njuego con 500 puntos.");
			break;
	}
	uint32_t again = 0;
	while(!again){
		again = PE0;
	}
	main();
}

uint8_t firstInit = 1;
int main(void){
  //uint32_t time=0;
  DisableInterrupts();
  // pick one of the following three lines, all three set to 80 MHz
  //PLL_Init();                   // 1) call to have no TExaS debugging
  TExaS_Init(&LogicAnalyzerTask); // 2) call to activate logic analyzer
  //TExaS_Init(&ScopeTask);       // or 3) call to activate analog scope PD2
  SSD1306_Init(SSD1306_SWITCHCAPVCC);
  Profile_Init(); // PB5,PB4,PF3,PF2,PF1
  Delay100ms(1);
	//TExaS_Init();    // bus clock at 80 MHz
	SysTick_Init(8000000);
  ADC_Init();  // turn on ADC
	DAC_Init();
	Sound_Init();
	SetPlayer_Init();
	//Edge_Init();
	Random_Init(ADC_In());
	EnableInterrupts();
	points = 500;
	if(firstInit){
		SSD1306_OutString("Press 1 for English,\n2 for Spanish");
		uint8_t eng = 0, spa = 0;
		while(!eng && !spa){
			eng = PE0;
			spa = PE1>>1;
		}
		if(eng) myLanguage = English;
		else if(spa) myLanguage = Spanish;
		Delay100ms(10);
	}
	firstInit = 0;
  while(1){
    //SSD1306_ClearBuffer();
		//SSD1306_OutClear();
		//SSD1306_SetCursor(0,0);
		//SSD1306_OutString("Use slider to adjust\nbet and press 1 to\nlock in.");
		uint32_t lockedIn = 0, bet;
		SSD1306_SetCursor(0,1);
		/*SSD1306_OutString("You have ");
		LCD_OutDec(points);
		SSD1306_OutString(" points.\nPress 1 to lock bet.\n");*/
		while(!lockedIn){ //bust-wait for user to press 1 and lock in bet
			Clock_Delay1ms(33);
			SSD1306_SetCursor(0,0);
			lockedIn = PE0;
			uint32_t i = 10, j = 54;
			while(!MailStatus){
			}
			MailStatus = 0;
			SSD1306_ClearBuffer();
			//SSD1306_OutClear();
			bet = Convert(MailValue);
			uint32_t barAmount = (bet * 116) / points;
			for(; i < 118; i++){ //outer box top
				SSD1306_DrawPixel(i, j, SSD1306_WHITE);
			}
			for(; j < 63; j++){ //outer box right
				SSD1306_DrawPixel(i, j, SSD1306_WHITE);
			}
			for(; i > 9; i--){ //outer box bottom
				SSD1306_DrawPixel(i, j, SSD1306_WHITE);
			}
			for(; j > 53; j--){ //outer box left
				SSD1306_DrawPixel(i, j, SSD1306_WHITE);
			}
			for(i = 12; i < barAmount; i++){ //inner box
				for(j = 56; j < 61; j++){
					SSD1306_DrawPixel(i, j, SSD1306_WHITE);
				}
			}
			switch(myLanguage){
				case English:
					SSD1306_DrawString(0, 0, "You have ", SSD1306_WHITE);
					SSD1306_DrawDec(54, 0, points, SSD1306_WHITE);
					
					uint8_t xOffset;
					if(points >=1000) xOffset = 30;
					else if(points >= 100) xOffset = 24;
					else if(points >= 10) xOffset = 18;
					else xOffset = 12;
					SSD1306_DrawString(54 + xOffset, 0, "points.", SSD1306_WHITE);
					SSD1306_DrawString(0, 10, "Press 1 to lock bet.", SSD1306_WHITE);
					
					SSD1306_DrawString(0, 44, "Bet: ", SSD1306_WHITE);
					SSD1306_DrawDec(30, 44, bet, SSD1306_WHITE);
					if(bet >=1000) xOffset = 30;
					else if(bet >= 100) xOffset = 24;
					else if(bet >= 10) xOffset = 18;
					else xOffset = 12;
					SSD1306_DrawChar(30+xOffset-6, 44, '/', SSD1306_WHITE);
					SSD1306_DrawDec(30+xOffset, 44, points, SSD1306_WHITE);
					break;
				
				case Spanish:
					SSD1306_DrawString(0, 0, "Tienes ", SSD1306_WHITE);
					SSD1306_DrawDec(42, 0, points, SSD1306_WHITE);
					
					if(points >=1000) xOffset = 30;
					else if(points >= 100) xOffset = 24;
					else if(points >= 10) xOffset = 18;
					else xOffset = 12;
					SSD1306_DrawString(42 + xOffset, 0, "puntos.", SSD1306_WHITE);
					SSD1306_DrawString(0, 10, "Presione 1 para", SSD1306_WHITE);
					SSD1306_DrawString(0, 20, "bloquear la apuesta.", SSD1306_WHITE);
					
					SSD1306_DrawString(0, 44, "Apuesta: ", SSD1306_WHITE);
					SSD1306_DrawDec(54, 44, bet, SSD1306_WHITE);
					if(bet >=1000) xOffset = 30;
					else if(bet >= 100) xOffset = 24;
					else if(bet >= 10) xOffset = 18;
					else xOffset = 12;
					SSD1306_DrawChar(54+xOffset-6, 44, '/', SSD1306_WHITE);
					SSD1306_DrawDec(54+xOffset, 44, points, SSD1306_WHITE);
					break;
			}
			SSD1306_OutBuffer();
		}
		Delay100ms(5);
		SSD1306_OutClear();
		SSD1306_ClearBuffer();
		SSD1306_SetCursor(0,1);
		switch(myLanguage){
			case English:
				SSD1306_OutString("Your bet is ");
				LCD_OutDec(bet);
				SSD1306_OutString(" out\nof ");
				LCD_OutDec(points);
				SSD1306_OutString(" points.\n\nPress 1 to continue.");
				break;
			case Spanish:
				SSD1306_OutString("Su apuesta es\n");
				LCD_OutDec(bet);
				SSD1306_OutString(" de ");
				LCD_OutDec(points);
				SSD1306_OutString(" puntos.\n\nPresione 1 para\ncontinuar.");
				break;
		}
		Delay100ms(5);
		points -= bet;
		while(!PE0){
		}
		SSD1306_OutClear();
		SSD1306_ClearBuffer();
		SSD1306_SetCursor(0,1);
		switch(myLanguage){
			case English:
				SSD1306_OutString("Choose Bevo (1),\nStudent (2),\nor Engineering (3).");
				break;
			case Spanish:
				SSD1306_OutString("Elija Bevo (1),\nEstudiante(2),\no ingenieria(3).");
				break;
		}
		Delay100ms(10);
		uint32_t selection = 0, compSelection = 0, result = 0;
		while(!selection){
			selection = PE3210;
		}
		
		switch(selection){
			case 1: selection = 0; break;
			case 2: selection = 1; break;
			case 4: selection = 2; break;
		}
		
		compSelection = Random()>>6;
		compSelection = compSelection % 3;
		//compSelection = 2;
		//TODO: display sprites moving in from top and bottom
			Eng.y = 18;
			Bevo.y = 18;
			Student.y = 18;
			EngCom.y = 63;
			BevoCom.y = 63;
			StudentCom.y = 63;
			SSD1306_ClearBuffer();
		if(selection == 0){
			Bevo.life = alive;
			Bevo.vy = 1;
			if(compSelection == 0){
				BevoCom.vy = -1;
				BevoCom.life = alive;
				for(int i = 0; i<13; i++){
					Draw(); //draw the sprites
					Bevo.y += Bevo.vy;	//move the player's sprite down one
					//SSD1306_DrawBMP(Bevo.x, Bevo.y,  Bevo.image, 0, SSD1306_WHITE); //print the new sprite location
					BevoCom.y += BevoCom.vy;	//move the computer's sprite up 1
					//SSD1306_DrawBMP(BevoCom.x, BevoCom.y,  BevoCom.image, 0, SSD1306_WHITE);// print the new sprite location
					//SSD1306_OutBuffer();
					Clock_Delay1ms(33); //delay
				}
				BevoCom.life = dead;
				BevoCom.vy = 0;
				result = 0;
			}
			else if(compSelection == 1){
				StudentCom.life = alive;
				StudentCom.vy = -1;
				for(int i = 0; i<22; i++){
					Draw();
					Bevo.y += Bevo.vy;	//move the player's sprite down one
					//SSD1306_DrawBMP(Bevo.x, Bevo.y,  Bevo.image, 0, SSD1306_WHITE); //print the new sprite location
					StudentCom.y += StudentCom.vy;	//move the computer's sprite up 1
					//SSD1306_DrawBMP(StudentCom.x, StudentCom.y,  StudentCom.image, 0, SSD1306_WHITE);
					//SSD1306_OutBuffer();
					Clock_Delay1ms(33);
					if(i == 13){
						Bevo.vy = -5;
						SoundHIT();
					}
				}
				result = 2;
				StudentCom.life = dead;
				StudentCom.vy = 0;
			}
			else if(compSelection == 2){
				EngCom.life = alive;
				EngCom.vy = -1;
				for(int i = 0; i<22; i++){
					Draw();
					Bevo.y += Bevo.vy;	//move the player's sprite down one
					//SSD1306_DrawBMP(Bevo.x, Bevo.y,  Bevo.image, 0, SSD1306_WHITE); //print the new sprite location
					EngCom.y += EngCom.vy;	//move the computer's sprite up 1
					//SSD1306_DrawBMP(EngCom.x, EngCom.y,  EngCom.image, 0, SSD1306_WHITE);
					//SSD1306_OutBuffer();
					Clock_Delay1ms(33);
					if(i == 13){
						EngCom.vy = 5;
						SoundHIT();
					}
				}
				EngCom.life = dead;
				EngCom.vy = 0;
				result = 1;
			}
			Bevo.life = dead;
			Bevo.vy = 0;
		}
		else if(selection == 1){
			Student.life = alive;
			Student.vy = 1;
			if(compSelection == 0){
				BevoCom.life = alive;
				BevoCom.vy = -1;
				for(int i = 0; i<22; i++){
					Draw();
					Student.y += Student.vy;	//move the player's sprite down one
					//SSD1306_DrawBMP(Bevo.x, Bevo.y,  Bevo.image, 0, SSD1306_WHITE); //print the new sprite location
					BevoCom.y += BevoCom.vy;	//move the computer's sprite up 1
					//SSD1306_DrawBMP(BevoCom.x, BevoCom.y,  BevoCom.image, 0, SSD1306_WHITE);
					//SSD1306_OutBuffer();
					Clock_Delay1ms(33);
					if(i == 13){
						BevoCom.vy = 5;
						SoundHIT();
					}
				}
				BevoCom.life = dead;
				BevoCom.vy = 0;
				result = 1;
			}
			else if(compSelection == 1){
				StudentCom.life = alive;
				StudentCom.vy = -1;
				for(int i = 0; i<13; i++){
					Draw();
					Student.y += Student.vy;	//move the player's sprite down one
					//SSD1306_DrawBMP(Bevo.x, Bevo.y,  Bevo.image, 0, SSD1306_WHITE); //print the new sprite location
					StudentCom.y += StudentCom.vy;	//move the computer's sprite up 1
					//SSD1306_DrawBMP(StudentCom.x, StudentCom.y,  StudentCom.image, 0, SSD1306_WHITE);
					//SSD1306_OutBuffer();
					Clock_Delay1ms(33);
				}
				result = 0;
				StudentCom.life = dead;
				StudentCom.vy = 0;
			}
			else if(compSelection == 2){
				EngCom.life = alive;
				EngCom.vy = -1;
				for(int i = 0; i<22; i++){
					Draw();
					Student.y += Student.vy;	//move the player's sprite down one
					//SSD1306_DrawBMP(Bevo.x, Bevo.y,  Bevo.image, 0, SSD1306_WHITE); //print the new sprite location
					EngCom.y += EngCom.vy;	//move the computer's sprite up 1
					//SSD1306_DrawBMP(EngCom.x, EngCom.y,  EngCom.image, 0, SSD1306_WHITE);
					//SSD1306_OutBuffer();
					Clock_Delay1ms(33);
					if(i == 13){
						Student.vy = -5;
						SoundHIT();
					}
				}
				EngCom.life = dead;
				EngCom.vy = 0;
				result = 2;
			}
			Student.life = dead;
			Student.vy = 0;
		}
		else if(selection == 2){
			Eng.life = alive;
			Eng.vy = 1;
			if(compSelection == 0){
				BevoCom.life = alive;
				BevoCom.vy = -1;
				for(int i = 0; i<22; i++){
					Draw();
					Eng.y += Eng.vy;	//move the player's sprite down one
					//SSD1306_DrawBMP(Bevo.x, Bevo.y,  Bevo.image, 0, SSD1306_WHITE); //print the new sprite location
					BevoCom.y += BevoCom.vy;	//move the computer's sprite up 1
					//SSD1306_DrawBMP(BevoCom.x, BevoCom.y,  BevoCom.image, 0, SSD1306_WHITE);
					//SSD1306_OutBuffer();
					Clock_Delay1ms(33);
					if(i == 13){
						Eng.vy = -5;
						SoundHIT();
					}
				}
				BevoCom.life = dead;
				BevoCom.vy = 0;
				result = 2;
			}
			else if(compSelection == 1){
				StudentCom.life = alive;
				StudentCom.vy = -1;
				for(int i = 0; i<22; i++){
					Draw();
					Eng.y += Eng.vy;	//move the player's sprite down one
					//SSD1306_DrawBMP(Bevo.x, Bevo.y,  Bevo.image, 0, SSD1306_WHITE); //print the new sprite location
					StudentCom.y += StudentCom.vy;	//move the computer's sprite up 1
					//SSD1306_DrawBMP(StudentCom.x, StudentCom.y,  StudentCom.image, 0, SSD1306_WHITE);
					//SSD1306_OutBuffer();
					Clock_Delay1ms(33);
					if(i == 13){
						StudentCom.vy = 5;
						SoundHIT();
					}
				}
				StudentCom.life = dead;
				StudentCom.vy = 0;
				result = 1;
			}
			else if(compSelection == 2){
				EngCom.life = alive;
				EngCom.vy = -1;
				for(int i = 0; i<13; i++){
					Draw();
					Eng.y += Eng.vy;	//move the player's sprite down one
					//SSD1306_DrawBMP(Bevo.x, Bevo.y,  Bevo.image, 0, SSD1306_WHITE); //print the new sprite location
					EngCom.y += EngCom.vy;	//move the computer's sprite up 1
					//SSD1306_DrawBMP(EngCom.x, EngCom.y,  EngCom.image, 0, SSD1306_WHITE);
					//SSD1306_OutBuffer();
					Clock_Delay1ms(33);
				}
				EngCom.life = dead;
				EngCom.vy = 0;
				result = 0;
			}
			Eng.life = dead;
			Eng.vy = 0;
		}
		Delay100ms(10);
		SSD1306_OutClear();
		SSD1306_ClearBuffer();
		SSD1306_SetCursor(0,1);
		switch(result){
			case 1: 
				points += (bet * 3);
				switch(myLanguage){
					case English:
						SSD1306_OutString("You won ");
						LCD_OutDec(bet * 3);
						SSD1306_OutString(" points!");
						break;
					case Spanish:
						SSD1306_OutString("Ganaste ");
						LCD_OutDec(bet * 3);
						SSD1306_OutString(" puntos.");
						break;
				}
				//TODO: output winning sound with DAC
				SoundWIN();
				break;
			case 0: 
				points += bet;
				switch(myLanguage){
					case English:
						SSD1306_OutString("You drew!");
						break;
					case Spanish:
						SSD1306_OutString("Te ataste");
						break;
				}
				break;
			case 2:
				switch(myLanguage){
					case English:
						SSD1306_OutString("You lost!");
						break;
					case Spanish:
						SSD1306_OutString("Perdiste");
						break;
				}
				//TODO: output losing sound with DAC
				SoundLOSE();
				break;
		}
		SSD1306_SetCursor(0,2);
		switch(myLanguage){
			case English:
				SSD1306_OutString("You have ");
				LCD_OutDec(points);
				SSD1306_OutString(" points.");
				break;
			case Spanish:
				SSD1306_OutString("Tienes ");
				LCD_OutDec(points);
				SSD1306_OutString(" puntos");
				break;
		}
		uint32_t playAgain = 0, cashOut = 0;
		if(!points){
			SSD1306_SetCursor(0,4);
			switch(myLanguage){
				case English:
					SSD1306_OutString("Game over!\n");
					SSD1306_OutString("Press 1 to play again");
					break;
				case Spanish:
					SSD1306_OutString("Juego terminado.\n");
					SSD1306_OutString("Presione 1 para\nvolver a jugar.");
					break;
			}
			while(!playAgain){
				playAgain = PE0;
			}
			points = 500;
		}
		else{
			SSD1306_SetCursor(0,4);
			switch(myLanguage){
				case English:
					SSD1306_OutString("Press 1 to play again\nor 2 to cash out!");
					break;
				case Spanish:
					SSD1306_OutString("Presione 1 para\nvolver a jugar de\nnuevo o 2 para cobrar");
					break;
			}
			
			while(!playAgain && !cashOut){
				playAgain = PE0;
				cashOut = PE1;
			}
			if(cashOut) gameOver();
		}
		Delay100ms(10);
	}
}

// You can't use this timer, it is here for starter code only 
// you must use interrupts to perform delays
void Delay100ms(uint32_t count){uint32_t volatile time;
  while(count>0){
    time = 727240;  // 0.1sec at 80 MHz
    while(time){
	  	time--;
    }
    count--;
  }
}

void SysTick_Handler(void){
	//PF1 ^= 0x02;     // Heartbeat
  MailValue = ADC_In();
	MailStatus = 1;
}
/*
void GPIOPortF_Handler(void){
	GPIO_PORTF_ICR_R = 0x10;
	FallingEdges++;
	//Clock_Delay1ms(500);
	while(PF0){}
}
*/
