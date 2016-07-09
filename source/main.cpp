#include <avr/io.h>															//Include AVR definitions (EG PORTD)
#include <avr/sleep.h>
#include <stdio.h>															//Include things like sprintf
#include <stdlib.h>
#include <string.h>															//Include things like strcpy
#include "../tedavr/include/tedavr/ic_hd44780.h"							//Include display class
#include "../tedavr/include/tedavr/button.h"								//Include button functions
#include "../include/ic_ds1307.h"											//Include clock class
#include "../include/timer.h"
#include "../include/colour.h"
#include "../light_ws2812/light_ws2812_AVR/Light_WS2812/light_ws2812.h"		//Include ws2812 (neopixel) functions (not written by Teddy)

#ifndef __INTELLISENSE__
#include <util/atomic.h>
#endif

//#define DS1307_IMPLEMENT
//define PRINT_TIME

EMPTY_INTERRUPT(INT0_vect);	//Used to wake the device from sleep mode

//This function calculates a bitrate value for the TWI. Don't worry about it.
constexpr uint8_t calculate_twbr(float const scl_freq, float const prescale = 1, float const cpu_freq = F_CPU) {
	return(static_cast<uint8_t>((cpu_freq / (2 * scl_freq * prescale)) - (8 / prescale)));
}

//Initialise special function registers
void sfr_init() {
	//---Inputs/Output Setup---//

	DDRD = 0b11111011;					//Input for power button
	DDRB = 0b11111111;					//Set all 8 PORTD pins to outputs
	DDRC = 0b000010;					//Sets two PORTC pins to inputs (the the clock), and the two genertic inputs, and the brightness pot.
	PORTD = 0b00000100;					//Set the outpus on PORTD to 0, and enables pullup resistor for input 2
	PORTB = 0;							//Set the outputs on PORTB to 0
	PORTC = 0b001100;					//Enable pullup resistors for the two genertic inputs

	//---TWI Interface Setup---//

	TWBR = calculate_twbr(100000);		//This is for communitcation with the clock. It wants a 100kHz TWI (two wire) interface.
										//(continue) This calls the function at the top of the file.
										//(continue) We tell it we want 100 000 Hz (the number in brackets) and it works out what value needs to go in the TWI bit-rate register to give us that.
	twi::enable();						//This enables the TWI interface. This means that we can't use the 2 PORTC pins for anything other than TWI for now.


	//---PWM Setup---//

	//Phase correct PWM (8bit)
	TCCR0A |= _BV(WGM00);
	TCCR0A &= ~_BV(WGM01);
	TCCR0B &= ~_BV(WGM02);

	//Display
	//Clear on compare match up, set on compare match down
	TCCR0A &= ~_BV(COM0A0);
	TCCR0A |= _BV(COM0A1);

	//Power LED
	//Clear on compare match up, set on compare match down
	TCCR0A &= ~_BV(COM0B0);
	TCCR0A |= _BV(COM0B1);

	//Display brightness to 0
	OCR0A = 0;
	//Power LED brightness to 0
	OCR0B = 0;

	//This gives the timer a clock source, with prescale 1
	TCCR0B |= _BV(CS00);
	TCCR0B &= ~(_BV(CS02) | _BV(CS01));

	//---ADC Setup---//

	//This selects the reference voltage to use. We are using AVCC (basically the source volatage)
	ADMUX |= _BV(REFS0);	//REFS0 = 1
	ADMUX &= ~_BV(REFS1);	//REFS1 = 0
	
	//This left shifts the result, since we only need 8 bit precision (default is 10)
	ADMUX |= _BV(ADLAR);	//ADLAR = 1

	//This selects what pin to use. We are using ADC0
	ADMUX &= ~(_BV(MUX3) | _BV(MUX2) | _BV(MUX1) | _BV(MUX0));	//MUX3-0 = 0x00

	//This enables the ADC (doesn't start a conversion though)
	ADCSRA |= _BV(ADEN);	//ADEN = 1

	//---External Interrupt Setup (used for waking from low power mode)---//
	//ATmega8 specifc stuf here
	EICRA &= ~(_BV(ISC01) | _BV(ISC00));	//Interrput on low
	sei();									//Enable interrupts
}

//Remember, we always start the program at main (so start here)
int main() {
	sfr_init();							//Initialise special function registers (calls (goes to) the function just above this one)))

	Button power;						//Create an instance of the button class called 'power'. We will use this as the power button. When referring to it, we will use 'power' in the code
	button_defaultSetup(&power);		//Initialise the power button
	power.data_port_p = &PIND;			//Power is located on PORTD
	power.data_shift_portBit = 2;		//Power is located on PORTD2
	button_update(&power);				//Call the button update function for power

	IC_HD44780 disp;					//Create an instance of the C++ class IC_HD44780 (the chip that drives the display). This will allow us to interface with the display.
										//(continue) We call our display 'disp', so when we want it we will use 'disp' in the code.

	//These tell disp the pin data direction registers for the respective pins. This is so that when it needs to, the MCU can switch them between inputs and outputs.
	disp.pin.ddr_data0 = &DDRB;
	disp.pin.ddr_data1 = &DDRB;
	disp.pin.ddr_data2 = &DDRB;
	disp.pin.ddr_data3 = &DDRB;
	
	//These tell disp the pin input registers for the respective pins. This is so that the MCU can read from the display data pins.
	disp.pin.port_in_data0 = &PINB;
	disp.pin.port_in_data1 = &PINB;
	disp.pin.port_in_data2 = &PINB;
	disp.pin.port_in_data3 = &PINB;

	//These tell disp the pin output registers for the respective pins. This is so that the MCU can output to the display pins.
	disp.pin.port_out_data0 = &PORTB;
	disp.pin.port_out_data1 = &PORTB;
	disp.pin.port_out_data2 = &PORTB;
	disp.pin.port_out_data3 = &PORTB;
	disp.pin.port_out_rs = &PORTD;
	disp.pin.port_out_rw = &PORTD;
	disp.pin.port_out_en = &PORTD;
	
	//These tell disp the bits that the pins are located on the respective ports. EG data0 in located on PORTD-0, and since we set it to PORTD above we need to set it to 0 here.
	disp.pin.shift_data0 = 0;
	disp.pin.shift_data1 = 3;
	disp.pin.shift_data2 = 4;
	disp.pin.shift_data3 = 5;
	disp.pin.shift_rs = 3;
	disp.pin.shift_en = 7;
	disp.pin.shift_rw = 4;

	//Note: 'instr' is short for instruction
	//Note: The scope resolution operator '::' allows us to access the things 'inside' the name on the left. EG instr::*whatever* allows us to access the things in instr.
	//Note: So hd::instr::init_4bit means the display (hd) instruction (instr) to initialise to 4-bit mode (init_4bit)
	//Note: 'using namespace *whatever*' allows us to not type the *whatever*. EG 'using namespace hd;' means that instead of typeing hd::instr::init_4bit, we can just type instr::init_4bit.
	//Note: We use the display by doing 'disp << *what we want*'
	using namespace hd;
	disp << instr::init_4bit;							//Initalise the display to 4bit mode.
														//(continue) This means that we only need 7 IO pins on out MCU, rather than 11

	disp << instr::function_set;						//This sets the function of the display																		
	disp << function_set::datalength_4;					//(note the 'function_set::'. This means we are elaborating on the function_set command); Set display to 4_bit. These needs to be send twice (already did one)
	disp << function_set::font_5x8;						//We want 5x8 character font (allows for more stuff)
	disp << function_set::lines_2;						//Our display has two lines.

	disp << instr::display_power;						//Next we do the display power instruction
	disp << display_power::display_on;					//Turn display on
	disp << display_power::cursor_off;					//Turn cusor on (part of display power instruction)
	disp << display_power::cursorblink_off;				//Turnon cursor blinking (part of display power instruction)

	//We don't have to do things one after the other. We can chain the instructions and values by just adding a '<<'
	//This is the instruction to set the entry mode, we want it so that the cursor moves right, and we don't want the display to shift with it.
	disp << instr::entry_mode_set << entry_mode_set::cursormove_right << entry_mode_set::displayshift_disable;

	//Clear the display
	disp << instr::clear_display;

	timer::init(0.001);
	
	IC_DS1307 clock;	//Create an instance of a DS1307 real-time-clock (the real-time-clock that we are using)
						//(continue) We call it clock, so from now on we will use 'clock' to refer to it.
	//This one is easier to use. All we need to do is call the clock function 'get_all'. This is done using the '.' operator to access the function within the clock object.
	clock.get_all();
	char day_string[4];
	char time_string[(16 * 2) + 2];	//Create a character array that's (16*2)+1 characters long. This is because each line of the display is 16 characters, and we have 2 lines. The +2 for the newline character '\n' and the terminating character '\0'

	//That's all good, but it only happens once. So we put it in a loop (and we would get rid of the above, because we don't need it twice. But we'll keep it for educational purposes)
	//while (true) means it will loop forever, since true is always true
	//We create a new regData called 'regData_old'. We use this to compare whether there has been a change in the time since the last loop.
	//If there is a change, we re-write the time to the display. If we hadn't done that, it would re-write the time every loop, which sort of looks strange on the display.
	IC_DS1307::RegData regData_old;

	constexpr float led_offset = 360 / 15;	//Increase this for less 
	cRGB led[5];
	float hue[5] = { 0, 1 * led_offset, 2 * led_offset, 3 * led_offset, 4 * led_offset };
	float brightness = 0;
	
	Timer timer;
	timer = 1;
	timer.start();
	
	while (true) {
		//Start of the loop.

		//---Power button---//
		button_update(&power);						//Update the state of power (read it)
		if (power.flag_state_released) {			//If power was pushed
#ifndef __INTELLISENSE__
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
#endif
				OCR0A = 0;	//Turn off display backlight
				OCR0B = 0;	//Turn off power backlight
				for (uint8_t i = 0; i < 5; i++) {
					led[i].r = 0;
					led[i].g = 0;
					led[i].b = 0;
				}
				ws2812_setleds(led, 5);
				disp << instr::display_power << display_power::display_off << display_power::cursorblink_off << display_power::cursor_off;	//Turn off display
				twi::disable();
				EIMSK |= _BV(INT0);						//Enable EXT INT0
				set_sleep_mode(SLEEP_MODE_PWR_DOWN);	//Set the sleep mode to PWR down
				sleep_enable();							//Enable sleep mode
				sei();									//Enable global interrupts
				sleep_cpu();							//Sleep
				sleep_disable();						//Wake up from sleep here
				cli();									//Disable global interrupts
				EIMSK &= ~_BV(INT0);						//Disable EXT INT0
				twi::enable();

				while (true) {
					button_update(&power);
					if (power.flag_state_released)
						break;
				}

				disp << instr::display_power << display_power::display_on << display_power::cursorblink_off << display_power::cursor_off;		//Turn on display
				OCR0A = 0xff;
				OCR0B = 0xff;
				regData_old.year1 = 9;				//Force it to update the display (by setting the year to 9x, it's bound to be different)
#ifndef __INTELLISENSE__
			}
#endif
		}
		//---Brightness---//
		/*
		ADCSRA |= _BV(ADSC);					//Start ADC conversion to get brightness
		while (!(ADCSRA & _BV(ADIF)));			//Wait for conversion to finish
		uint8_t br_u8 = ADCH;
		OCR0A = brightness;							//Set brightness with read ADC value
		OCR0B = brightness;
		*/

			if (timer) {
				for (uint8_t i = 0; i < 5; i++) {
					RGBColor x = hsv2rgb(hue[i], 100, 100);
					led[i].r = x.r;
					led[i].g = x.g;
					led[i].b = x.b;
					hue[i]++;
					if (hue[i] == 360) {
						hue[i] = 0;
					}
				}

				timer.reset();
				timer = 1;
				timer.start();
			}
		cli();
		ws2812_setleds(led, 5);
		sei();

		//---Clock---//
		clock.get_all();						//Update the clock.

		if (clock.regData == regData_old)		//If regData and regData_old are the same
			continue;							//Skip the rest of the loop, and start again (continue skips the rest of the loop)
		regData_old = clock.regData;			//Copy the data over since there was a change

		switch (clock.regData.day) {			//We need to change the 'day' from a number into a readable text string.
		case(1):								//We 'switched' the day. That means that it goes through to check if any of these 'cases' are equal to day. 'case(1):' means 'if (clock.regData.day == 1)' then do whatever there is until the next case statement
			strcpy(day_string, "Sun");			//Copy "Sun" into our array 'day_string'. 'day_string' is what will be put on the display.
			break;								//We use break to exit the switch statement, becuase we found what say it is
		case(2):								//etc...
			strcpy(day_string, "Mon");
			break;
		case(3):
			strcpy(day_string, "Tue");
			break;
		case(4):
			strcpy(day_string, "Wed");
			break;
		case(5):
			strcpy(day_string, "Thu");
			break;
		case(6):
			strcpy(day_string, "Fri");
			break;
		case(7):
			strcpy(day_string, "Sat");
			break;
		default:								//This is what happenes if something unexpected enters the switch statement. (AKA if it's not covered by the case statements)
			break;
		}
		sprintf(time_string, "%u%u:%u%u:%u%u%s\n%s %u%u/%u%u/20%u%u", clock.regData.hour1, clock.regData.hour0, clock.regData.minute1, clock.regData.minute0, clock.regData.second1, clock.regData.second0, clock.regData.ampm_hour1 ? "PM" : "AM",
			day_string, clock.regData.date1, clock.regData.date0, clock.regData.month1, clock.regData.month0, clock.regData.year1, clock.regData.year0);	//Make our string to output
		disp << instr::return_home << time_string;	//And output it after returning home (so that it starts from the start).

		//This is the end of the loop (note the closing brace '}'). So we go back to the opening brace '{'.
	}
}
