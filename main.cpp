/*
 * Real clock.cpp
 *
 * Created: 08-Jun-17 11:17:36 AM
 * Author : Emancipator
 */ 

/*
  NOTICE:
1. To set time at any point in time, press A
2. User-input data are two digits ONLY
3. 500ms delay in-between keypad presses
4. 500ms timer interrupt
5. 1000ms total delay in-between keypad presses
6. "A","B","C","D","*","#" keypad presses are "ZERO DIGITS",
   except otherwise stated
7. Time can be updated only once per session
8. To update time for the second time in a session, please RESET
9. HOUR data is entered in 24hours format
   NB: * A session being the duration during which the system is powered
       * ENTER DAY: 1-SUNDAY, 2-MONDAY, ... 7-SATURDAY
*/

#define  F_CPU 16000000
#include <stdio.h>
#include <avr/io.h>
#include <stdlib.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define usingGeneralAVR
#define usingSoftI2c
#define usingLcd
#define usingKeypad4X4

/**************************************
******** PIN CONFIGURATIONS ***********
**************************************/

/******  SHIFT_REGISTER   ******/
#define HC595_DDR 	DDRB
#define HC595_4_DDR DDRD
#define HC595_PORT 	 PORTB
#define HC595_4_PORT PORTD

#define HC595_DS1_POS       PB0
#define HC595_DS2_POS       PB1
#define HC595_DS3_POS       PB2
#define HC595_DS4_POS       PB3	
#define HC595_SH1_CP_POS    PB4
#define HC595_SH2_CP_POS    PB5
#define HC595_SH3_CP_POS    PB6
#define HC595_ST_CP_POS     PB7
#define HC595_4_SH4_CP_POS  PD7

/*******   RTC   *********/
#define i2cSclDir DDRD_B5
#define i2cSdaDir DDRD_B4
#define i2cSdaPin PIND_B4
#define i2cScl   PORTD_B5
#define i2cSda   PORTD_B4

/********   LCD   ********/
#define lcdD4Dir DDRA_B0
#define lcdD5Dir DDRA_B1
#define lcdD6Dir DDRA_B2
#define lcdD7Dir DDRA_B3
#define lcdRsDir DDRA_B7
#define lcdEnDir DDRA_B5

#define lcdD4 PORTA_B0
#define lcdD5 PORTA_B1
#define lcdD6 PORTA_B2
#define lcdD7 PORTA_B3
#define lcdRs PORTA_B7
#define lcdEn PORTA_B5

/************ KEYPAD *************/
#define keypadPort           PORTC
#define keypadPortDirection  DDRC

#define keypadRow0 PINC_B0
#define keypadRow1 PINC_B1
#define keypadRow2 PINC_B2
#define keypadRow3 PINC_B3
#define keypadCol0 PINC_B4
#define keypadCol1 PINC_B5
#define keypadCol2 PINC_B6
#define keypadCol3 PINC_B7

#include "c://Emancipator/Emancipator.h"

lcd display (16,2);             //LCD resolution 16x2
i2c rtcDs1307(0x68, I2C_8Bit);  //Address, bit size (RTC)

/*****************************
**** VARIABLE DECLARATION ****
*****************************/
int key;
char press;
uint8_t t = 0;
int RTC[17];
int pad[17] = {1,2,3,0,4,5,6,0,7,8,9,0,0,0,0,0};
char second[5], minute[5], hour[5], day[5], date[5], month[5], year[5];
int second1, minute1, hour1, day1, date1, month1, year1;
int second2, minute2, hour2, day2, date2, month2, year2;
////////////////////////////////////////////////////////
uint8_t rtcHour;
uint8_t rtcData[8];
uint8_t rtcTime[15];
uint8_t Year            = 20;
uint8_t led_pattern[10] = {63,6,91,79,102,109,125,7,127,111};
const char* DOW[][8]    = {"        ","  SUNDAY   ","  MONDAY   ","  TUESDAY   "," WEDNESDAY  "," THURSDAY   ","  FRIDAY   "," SATURDAY    "};
const char* MONTH[][13] = {"        "," JANUARY ","    FEB. ","   MARCH ","   APRIL ","     MAY ","    JUNE ","    JULY ","  AUGUST ","   SEPT. "," OCTOBER ","NOVEMBER ","DECEMBER "};

/**********************************
****** FUNCTION DECLARATION *******
**********************************/
void SetTime(void)
{
		
	/*** Converts Decimal to BCD *****/
	second2 =  ((second1/10*16) + (second1%10));       //___second
	minute2 =  ((minute1/10*16) + (minute1%10));      //___minute
	hour2   =  ((hour1/10*16)   +   (hour1%10));     //___hour
	day2    =  ((day1/10*16)    +    (day1%10));    //___day
	date2   =  ((date1/10*16)   +   (date1%10));   //___date
	month2  =  ((month1/10*16)  +  (month1%10));  //___month
	year2   =  ((year1/10*16)   +   (year1%10)); //___year


	/*** Writes time(in BCD) to the
    appropriate addresses of the RTC *****/
	rtcDs1307.writeByte(0x0000, second2);
	rtcDs1307.writeByte(0x0001, minute2);
	rtcDs1307.writeByte(0x0002, hour2);
	rtcDs1307.writeByte(0x0003, day2);
	rtcDs1307.writeByte(0x0004, date2);
	rtcDs1307.writeByte(0x0005, month2);
	rtcDs1307.writeByte(0x0006, year2);
}

void SetProtocol(void)
{
	key = readKeypad4X4();

	if (t == 0 || t == 1)
	{
		display.writeText(0,0,"ENTER SECOND:   ");
		if (key != 255)
		{
			press = pad[key];
			RTC[t] = press;
			display.writeDec(1,t,RTC[t]);
			display.writeText(1,2,"              ");
			t++;
			_delay_ms(500);
			if(t == 2) display.clear();
			key = readKeypad4X4();
		}
	}
	if (t == 2 || t == 3)
	{
		display.writeText(0,0,"ENTER MINUTE: ");
		if (key != 255)
		{
			press = pad[key];
			RTC[t] = press;
			display.writeDec(1,t,RTC[t]);
			t++;
			_delay_ms(500);
			if(t == 4) display.clear();
			key = readKeypad4X4();
		}
	}
	if (t == 4 || t == 5)
	{
		display.writeText(0,0,"ENTER HOUR:    ");
		if (key != 255)
		{
			press = pad[key];
			RTC[t] = press;
			display.writeDec(1,t,RTC[t]);
			t++;
			_delay_ms(500);
			if(t == 6) display.clear();
			key = readKeypad4X4();
		}
	}
	if (t == 6 || t == 7)
	{
		display.writeText(0,0,"ENTER DAY:    ");
		if (key != 255)
		{
			press = pad[key];
			RTC[t] = press;
			display.writeDec(1,t,RTC[t]);
			t++;
			_delay_ms(500);
			if(t == 8) display.clear();
			key = readKeypad4X4();
		}
	}
	if (t == 8 || t == 9)
	{
		display.writeText(0,0,"ENTER DATE:    ");
		if (key != 255)
		{
			press = pad[key];
			RTC[t] = press;
			display.writeDec(1,t,RTC[t]);
			t++;
			_delay_ms(500);
			if(t == 10) display.clear();
			key = readKeypad4X4();
		}
	}
	if (t == 10 || t == 11)
	{
		display.writeText(0,0,"ENTER MONTH:  ");
		if (key != 255)
		{
			press = pad[key];
			RTC[t] = press;
			display.writeDec(1,t,RTC[t]);
			t++;
			_delay_ms(500);
			if(t == 12) display.clear();
			key = readKeypad4X4();
		}
	}
	if (t == 12 || t == 13)
	{
		display.writeText(0,0,"ENTER YEAR:   ");
		if (key != 255)
		{
			press = pad[key];
			RTC[t] = press;
			display.writeDec(1,t,RTC[t]);
			t++;
			_delay_ms(500);
			if(t == 14) display.clear();
			key = readKeypad4X4();
		}
	}
	if (t == 14)
	{
		display.writeText(0,0,"TAP # TO LOAD");
		display.writeText(1,0,"TAP * TO CANCEL");
		if(key == 14)
		{
			t = 20;
			display.clear();
		}
		if(key == 12)
		{
			display.clear();
			_delay_ms(500);
			t = 0;
			key = readKeypad4X4();
		}
	}
	
	if(t == 20)
	{
		for (uint8_t j =0;j<101;j++)
		{
			display.writeText(0,0,"UPDATING TIME...");
			display.writeDec(1,6,j);
			display.writeText(1,9,"%");
			_delay_ms(50);
			if(j == 100)
			{
				_delay_ms(1000);
				display.clear();
				_delay_ms(500);
				 t = 21;
			}
		}
	}
	if(t == 21)
	{
		display.writeText(0,0,"TIME UPDATED!   ");
		display.writeText(1,0,"    Thank You...");
		sprintf(second, "%d%d",  RTC[0],RTC[1]);
		sprintf(minute, "%d%d",  RTC[2],RTC[3]);
		sprintf(hour,   "%d%d",  RTC[4],RTC[5]);
		sprintf(day,    "%d%d",  RTC[6],RTC[7]);
		sprintf(date,   "%d%d",  RTC[8],RTC[9]);
		sprintf(month,  "%d%d",  RTC[10],RTC[11]);
		sprintf(year,   "%d%d",  RTC[12],RTC[13]);
		
		second1 = atoi(second);
		minute1 = atoi(minute);
		hour1   = atoi(hour);
		day1    = atoi(day);
		date1   = atoi(date);
		month1  = atoi(month);
		year1   = atoi(year);
		
		SetTime();
		_delay_ms(2000);
		display.clear();
		_delay_ms(500);
		t = 25;
	}
}

void HC595Init()
{
	//Make the Data(DS), Shift clock (SH_CP), Store Clock (ST_CP) lines output
	HC595_DDR|=((1<<HC595_SH1_CP_POS)|(1<<HC595_SH2_CP_POS)|(1<<HC595_SH3_CP_POS)|(1<<HC595_ST_CP_POS)|(1<<HC595_DS1_POS)|(1<<HC595_DS2_POS)|(1<<HC595_DS3_POS)|(1<<HC595_DS4_POS));
	HC595_4_DDR|=(1<<HC595_4_SH4_CP_POS);
}

//Low level macros to change data (DS)lines
#define HC595Data1High() (HC595_PORT|=(1<<HC595_DS1_POS))
#define HC595Data1Low() (HC595_PORT&=(~(1<<HC595_DS1_POS)))

#define HC595Data2High() (HC595_PORT|=(1<<HC595_DS2_POS))
#define HC595Data2Low() (HC595_PORT&=(~(1<<HC595_DS2_POS)))

#define HC595Data3High() (HC595_PORT|=(1<<HC595_DS3_POS))
#define HC595Data3Low() (HC595_PORT&=(~(1<<HC595_DS3_POS)))

#define HC595Data4High() (HC595_PORT|=(1<<HC595_DS4_POS))
#define HC595Data4Low() (HC595_PORT&=(~(1<<HC595_DS4_POS)))

void HC595Latch()
{
	//Common Latch for all Shift Registers
	HC595_PORT|=(1<<HC595_ST_CP_POS);//HIGH
	_delay_loop_1(1);

	HC595_PORT&=(~(1<<HC595_ST_CP_POS));//LOW
	_delay_loop_1(1);
}

void HC595Pulse1()
{
	//Pulse the 1st Shift Clock
	HC595_PORT|=(1<<HC595_SH1_CP_POS);//HIGH

	HC595_PORT&=(~(1<<HC595_SH1_CP_POS));//LOW
}

void HC595Pulse2()
{
	//Pulse the 2nd Shift Clock

	HC595_PORT|=(1<<HC595_SH2_CP_POS);//HIGH

	HC595_PORT&=(~(1<<HC595_SH2_CP_POS));//LOW

}

void HC595Pulse3()
{
	//Pulse the 3rd Shift Clock

	HC595_PORT|=(1<<HC595_SH3_CP_POS);//HIGH

	HC595_PORT&=(~(1<<HC595_SH3_CP_POS));//LOW

}

void HC595Pulse4()
{
	//Pulse the 4th Shift Clock

	HC595_4_PORT|=(1<<HC595_4_SH4_CP_POS);//HIGH

	HC595_4_PORT&=(~(1<<HC595_4_SH4_CP_POS));//LOW

}


void Write1(uint8_t data1)
{
	//Shifts in the 8 bits,one after the other
	//Order is MSB first
	for(uint8_t i=0;i<8;i++)
	{
		//Output the data on DS line according to the
		//Value of MSB
		if(data1 & 0b10000000)
		{
			HC595Data1High();
		}
		else
		{
			HC595Data1Low();
		}
		HC595Pulse1();
		data1=data1<<1;
	}
	//This function writes the 1st shift register
}

void Write2(uint8_t data2)
{
	for(uint8_t j=0;j<8;j++)
	{
		if(data2 & 0b10000000)
		{
			HC595Data2High();
		}
		else
		{
			HC595Data2Low();
		}
		HC595Pulse2();
		data2=data2<<1;
	}
	//This function writes the 2nd shift register
}

void Write3(uint8_t data3)
{
	for(uint8_t k=0;k<8;k++)
	{
		if(data3 & 0b10000000)
		{
			HC595Data3High();
		}
		else
		{
			HC595Data3Low();
		}
		HC595Pulse3();
		data3=data3<<1;
	}
	//This function writes the 3rd shift register
}

void Write4(uint8_t data4)
{
	for(uint8_t l=0;l<8;l++)
	{
		if(data4 & 0b10000000)
		{
			HC595Data4High();
		}
		else
		{
			HC595Data4Low();
		}
		HC595Pulse4();
		data4=data4<<1;
	}
	//This function writes the 4th shift register
}



void HC595WRITE()
{
	/****  
	Sends time data to each 7_segment
	****/
	
	Write1(led_pattern[rtcTime[2]]);
	Write2(led_pattern[rtcTime[3]]);
	Write3(led_pattern[rtcTime[4]]);
	Write4(led_pattern[rtcTime[5]]);
	HC595Latch();
}

void Convert(void)
{
	/****** This converts the RTC Data from BCD to Decimal, as well as controls
	                          AM and PM indicators ******/
	
	if (((rtcData[2] > 0b00010010) & (rtcData[2] < 0b00100000)) | (rtcData[2] > 0b00100001))
	{
		rtcHour = (rtcData[2] % 0b00010010);
		PORTC = 0x04; // PM indicator
	}
	else if (rtcData[2] == 0b00000000)
	{
		rtcHour = 0b00010010;
		PORTC = 0x02; //AM indicator
	}
	else if (rtcData[2] == 0b00100000)
	{
		rtcHour = 0b00001000;
		PORTC = 0x04; // PM indicator
	}
	else if (rtcData[2] == 0b00100001)
	{
		rtcHour = 0b00001001;
		PORTC = 0x04; // PM indicator
	}
     else if (rtcData[2] == 0b00010010)
    {
        PORTC = 0x04; //PM indicator
    }
	else
	{ 
		rtcHour = rtcData[2];
		PORTC = 0x02;
	}
	
	rtcTime[0]   =  ( rtcData[0] & 0b00001111);  //second
	rtcTime[1]   = (( rtcData[0] >> 4) & 0b00000111);
	
	rtcTime[2]   =  ( rtcData[1] & 0b00001111);  //minute
	rtcTime[3]   = (( rtcData[1] >> 4) & 0b00000111);
	
	rtcTime[4]   =  ( rtcHour & 0b00001111);     //Hour
	rtcTime[5]   = (( rtcHour >> 4) & 0b00000111);
	
	rtcTime[6]   =  ( rtcData[3] & 0b00001111);  //day of week
	rtcTime[7]   = (( rtcData[3] >> 4) & 0b00000111);
	
	rtcTime[8]   =  ( rtcData[4] & 0b00001111);  //date
	rtcTime[9]   = (( rtcData[4] >> 4) & 0b00000111);
	
	rtcTime[10]  =  ( rtcData[5] & 0b00001111);  //month
	rtcTime[11]  = (( rtcData[5] >> 4) & 0b00000111);
	
	rtcTime[12]  =  ( rtcData[6] & 0b00001111);  //year
	rtcTime[13]  = (( rtcData[6] >> 4) & 0b00000111);

}

void LcdDisplay(void)
{
	/******** Displays Initialization protocols
	as well as Day, Month, Date and Year of the RTC *******/
	
	display.writeText(0,3,DOW[0][rtcTime[6]]);
	
	if (rtcTime[10]==0 && rtcTime[11]==1)
	{
		display.writeText(1,0,MONTH[0][10]);
	} 
	else if(rtcTime[10]==1 && rtcTime[11]==1)
	{
		display.writeText(1,0,MONTH[0][11]);
	}
	else if (rtcTime[10]==2 && rtcTime[11]==1)
	{
		display.writeText(1,0,MONTH[0][12]);
	}
	else
	{
		display.writeText(1,0,MONTH[0][rtcTime[10]]);
	}
	
	display.writeDec(1,9,rtcTime[9]);
	display.writeDec(1,10,rtcTime[8]);
	display.writeText(1,11,",");
	display.writeDec(1,12,Year);
	display.writeDec(1,14,rtcTime[13]);
	display.writeDec(1,15,rtcTime[12]);
	_delay_ms(10);
}



int main(void)
{
	DDRC  = 0xFF;
	PORTC = 0x00;
	
	rtcDs1307.init();//Initialize RTC
	display.init(); //Initialize LCD
	HC595Init();   //Initialize 595
	_delay_ms(100);
	
	display.writeText(0,0,"SYSTEM STARTUP..");
	delay_ms(1000);
	display.clear();
	for (uint8_t a=0;a<101;a++)
	{
		display.writeText(0,0,"UPDATING TIME...");
		display.writeDec(1,7,a);
		display.writeText(1,10,"%");
		_delay_ms(50);
	}
	delay_ms(1000);
	display.clear();
	_delay_ms(500);
	
	sei();
	
	TCCR1B = (1<<CS12);
	TIMSK =  (1<<OCIE1A);
	
	OCR1A = 62500;
	
	
	/* Replace with your application code */
	while (1)
	{
		key = readKeypad4X4();
		if(key == 3)
		{
			key = readKeypad4X4();
			while(t == 0 || t < 23) SetProtocol();
		}
		else
		{
			key = readKeypad4X4();
			
			rtcDs1307.readBytes(0x0000, rtcData, 8);
			
			Convert();
			
			HC595WRITE();
			
			LcdDisplay();
			
			_delay_ms(1);
		}
	}
}

ISR(TIMER1_COMPA_vect)
{	
	if (((rtcData[2] > 0b00010010) & (rtcData[2] < 0b00100000)) | (rtcData[2] > 0b00100001))
	{
		rtcHour = (rtcData[2] % 0b00010010);
		PORTC = 0x04; // PM indicator
	}
	else if (rtcData[2] == 0b00000000)
	{
		rtcHour = 0b00010010;
		PORTC = 0x02;
	}
	else if (rtcData[2] == 0b00100000)
	{
		rtcHour = 0b00001000;
		PORTC = 0x04; // PM indicator
	}
	else if (rtcData[2] == 0b00100001)
	{
		rtcHour = 0b00001001;
		PORTC = 0x04; // PM indicator
	}
    else if (rtcData[2] == 0b00010010)
    {
        PORTC = 0x04;
    }
	else
	{ 
		rtcHour = rtcData[2];
		PORTC = 0x02;
	}
	
	PORTC ^= 0x01;
	_delay_ms(500);
}