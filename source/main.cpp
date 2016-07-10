//Include AVR definitions
#include <avr/io.h>
//Include AVR sleep functions
#include <avr/sleep.h>
//Include C standard header stdio.h
#include <stdio.h>
//Include C standard header stdlib.h
#include <stdlib.h>
//Include C standard header string.h
#include <string.h>
//Include tedavr ic_hd44780.h
#include "../tedavr/include/tedavr/ic_hd44780.h"
//Inclide tedavr button.h
#include "../tedavr/include/tedavr/button.h"
//Include ic_ds1307.h
#include "../include/ic_ds1307.h"
//Include timer.h
#include "../include/timer.h"
//Include colour.h
#include "../include/colour.h"
//Include neopixel light_ws2812.h
#include "../light_ws2812/light_ws2812_AVR/Light_WS2812/light_ws2812.h"

//An empty ISR used to wake the device from sleep mode
EMPTY_INTERRUPT(INT0_vect);

//This function calculates a bitrate value for the TWI. Don't worry about it.
constexpr uint8_t calculate_twbr(float const scl_freq, float const prescale = 1, float const cpu_freq = F_CPU) {
	return(static_cast<uint8_t>((cpu_freq / (2 * scl_freq * prescale)) - (8 / prescale)));
}

//Initialise special function registers
void sfr_init() {
	//---Inputs/Output Setup---//

	//Set DDRD (2 = power button)
	DDRD = 0b11111011;
	//Set DDRB
	DDRB = 0b11111111;
	//Set DDRB(4/5 = TWI lines, 2/3 = generic inputs, 1 = neopixel out, 0 = ADC)
	DDRC = 0b000010;
	//Set PORTD (2 = pullup for power)
	PORTD = 0b00000100;
	//Set PORTB
	PORTB = 0;
	//Set PORTC (2/3 = pullup for generic inputs)
	PORTC = 0b001100;

	//---TWI Interface Setup---//

	//Calculte TWBR (a bitrate for the TWI) for 100kHz
	TWBR = calculate_twbr(100000);
	//Enable the TWI
	twi::enable();


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
	//Initialise special function registers (calls (goes to) the function just above this one)
	sfr_init();

	//Create an instance of the button class called 'power'. We will use this as the power button.
	//When referring to it, we will use 'power' in the code.
	Button power;
	//Initialise the power button
	button_defaultSetup(&power);
	//Power is located on PORTD2
	power.data_port_p = &PIND;
	power.data_shift_portBit = 2;
	//Call the button update function for power
	button_update(&power);

	//Create an instance of the C++ class IC_HD44780 (the chip that drives the display).This will allow us to interface with the display.
	//We call our display 'disp', so when we want it we will use 'disp' in the code.
	IC_HD44780 disp;

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
	//The scope resolution operator '::' allows us to access the things 'inside' the name on the left.
	//EG instr::*whatever* allows us to access the things in instr.
	//So hd::instr::init_4bit means the display (hd) instruction (instr) to initialise to 4-bit mode (init_4bit)
	//'using namespace *whatever*' allows us to not type the *whatever*.
	//G 'using namespace hd;' means that instead of typeing hd::instr::init_4bit, we can just type instr::init_4bit.
	//We use the display by doing 'disp << *what we want*'
	using namespace hd;

	//Initalise the display to 4bit mode.
	//This means that we only need 7 IO pins on out MCU, rather than 11
	disp << instr::init_4bit;

	//This sets the function of the display
	disp << instr::function_set;
	//Set display to 4_bit. These needs to be send twice (already did one)
	//(note the 'function_set::'. This means we are elaborating on the function_set command);
	disp << function_set::datalength_4;
	//We want 5x8 character font (allows for more stuff)
	disp << function_set::font_5x8;
	//Our display has two lines.
	disp << function_set::lines_2;

	//Next we do the display power instruction
	disp << instr::display_power;
	//Turn display on
	disp << display_power::display_on;
	//Turn off cursor (part of display power instruction)
	disp << display_power::cursor_off;
	//Turn off cursor blinking (part of display power instruction)
	disp << display_power::cursorblink_off;

	//We don't have to do things one after the other. We can chain the instructions and values by just adding a '<<'
	//This is the instruction to set the entry mode, we want it so that the cursor moves right, and we don't want the display to shift with it.
	disp << instr::entry_mode_set << entry_mode_set::cursormove_right << entry_mode_set::displayshift_disable;

	//Clear the display
	disp << instr::clear_display;

	
	//Create an instance of a DS1307 real-time-clock (the real-time-clock that we are using)
	//We call it clock, so from now on we will use 'clock' to refer to it.
	IC_DS1307 clock;
	
	//Create a character array that's 4 characters long.
	char day_string[4];
	//Create a character array that's (16*2)+2 characters long.
	//Each line of the display is 16 characters, and we have 2 lines. The +2 for the newline character '\n' and the terminating character '\0'
	char time_string[(16 * 2) + 2];

	IC_DS1307::RegData regData_old;

	//Increase this for decreased colour variation along the neopixel strip (set to 1 for identical)
	constexpr float led_similarity = 15;
	//Create an offset stating the hue difference between each neopixel
	constexpr float led_offset = 360 / led_similarity;
	//The amount of neopixels on the strip
	constexpr uint8_t led_amount = 5;
	//Create an array of cRGB lights (the neopixels).
	cRGB led[led_amount];
	//Create an array of floating point values, representing the hue of each neopixel
	float hue[led_amount];
	for (uint8_t i = 0; i < led_amount; i++) {
		//Increase each LEDs hue slightly relative to the next
		hue[i] = static_cast<size_t>(i * led_offset) % 360;
	}
	//Create a brightness float that will be used throughout the program for brightness
	float brightness = 0;
	//Create a brightness float that will be used for determining of there was a difference in the brightness
	float brightness_old = 0;
	
	//Initialise the timeout timer functions
	//The 0.001 is the 'interval' parameter, which determines how many seconds it should take for a single 'tick' to elapse in timers.
	//In this case we set it to 0.001 seconds, or 1ms. Therefore a tick is 1ms.
	timer::init(0.001);

	//Create a timeout timer to use for the neopizels colour change speed
	Timer neopixel_timer;
	//Set it to timeout in 1 tick, or 1ms.
	neopixel_timer = 1;
	//Start the timer
	neopixel_timer.start();
	
	//The main program loop
	while (true) {
		//---Power button---//
		//Update the state of the power button
		button_update(&power);
		//If somebody has pushed and released the power button
		if (power.flag_state_released) {
			//Disable global interrupts
			cli();
			//Set display brightness to zero
			OCR0A = 0;
			//Set power brightness to zero
			OCR0B = 0;
			//Set all of the leds/neopixels to zero
			for (uint8_t i = 0; i < led_amount; i++) {
				led[i].r = 0;
				led[i].g = 0;
				led[i].b = 0;
			}
			//Set the neopixels
			ws2812_setleds(led, led_amount);
			//Turn off the display
			disp << instr::display_power << display_power::display_off << display_power::cursorblink_off << display_power::cursor_off;
			//Disable the TWI (need to do this for some reason, or it wont work on wake)
			twi::disable();
			//Enable external interrupt 0 (connected to power button, used to wake device from sleep)
			EIMSK |= _BV(INT0);
			//Set the sleep mode to power down
			set_sleep_mode(SLEEP_MODE_PWR_DOWN);
			//Enable sleep mode
			sleep_enable();
			//Enable global interrupts (allowing the device to wake)
			sei();
			//Go to sleep
			sleep_cpu();
			//The device will wait here until the power button is pushed, enableing the external interrupt 0 and waking the device.
			
			//Disable global interrupts
			cli();
			//Disable sleep mode
			sleep_disable();
			//Disable external interrupt 0
			EIMSK &= ~_BV(INT0);
			//Enable the TWI
			twi::enable();

			//Wait for the power button to be released
			while (true) {
				button_update(&power);
				if (power.flag_state_released)
					break;
			}
			//Tuwn on the display
			disp << instr::display_power << display_power::display_on << display_power::cursorblink_off << display_power::cursor_off;

			//Set the display brightness to the brightness value
			OCR0A = brightness;
			//Set the power brightness to the brightness value
			OCR0B = brightness;
			//Set the years 1s digit to zero, forcing a display update
			regData_old.year1 = 0;
			//Enable global interrupts
			sei();
		}

		//---Brightness---//
		
		//Start ADC conversion
		ADCSRA |= _BV(ADSC);
		//Wait for ADC conversion to finish
		while (!(ADCSRA & _BV(ADIF)));
		//Copy the result into brightness (the float we created earlier)
		brightness = ADCH;
		//Set the display brightness
		OCR0A = brightness;
		//Set the power button brightness
		OCR0B = brightness;
		//Change the brightness from 0-255 to 0-100. (used with the neopixels)
		brightness = (brightness / 255) * 100;

		//Store whether the timer has elapsed in a bool.
		//This is becuase a change during the next segment could desync the neopixels.
		bool timer_elapsed = neopixel_timer;
		//If there has been a change in brightness or the timer has elapsed
		if ((brightness != brightness_old) || timer_elapsed) {
			for (uint8_t i = 0; i < led_amount; i++) {
				//Convert HSV to RGB
				RGBColor x = hsv2rgb(hue[i], 100, brightness);
				//Copy over the data
				led[i].r = x.r;
				led[i].g = x.g;
				led[i].b = x.b;
				//If the timer has elapsed (indicating a need to change the neopixels)
				if(timer_elapsed)
					//Change the neopixels colour values
					hue[i]++;
				hue[i] = static_cast<size_t>(hue[i]) % 360;
			}
			//Disable global interrupts
			cli();
			//Set the neopixels
			ws2812_setleds(led, led_amount);
			//Enable global interrupts
			sei();
			if (timer_elapsed) {
				//Reset the timer
				neopixel_timer.reset();
				//Set the timer to 1ms
				neopixel_timer = 1;
				//Start the timer
				neopixel_timer.start();
			}
		}
		//Copy over the brightness value for next loop comparison
		brightness_old = brightness;

		//---Clock---//

		//Update the clock
		clock.get_all();

		//If the clock has remained the same since the last loop
		if (clock.regData == regData_old)
			//Restart the loop (continue skips the rest of the loop and goes to the start again)
			continue;
		//If there has been a time difference, copy the new time into the old time
		regData_old = clock.regData;

		//Determine the day string
		switch (clock.regData.day) {
		case(1):
			strcpy(day_string, "Sun");
			break;
		case(2):
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
		default:
			break;
		}
		//Print the time string into the 'time_string' character array (Google printf for details).
		sprintf(time_string, "%u%u:%u%u:%u%u%s\n%s %u%u/%u%u/20%u%u",
			clock.regData.hour1, clock.regData.hour0, clock.regData.minute1, clock.regData.minute0,
			clock.regData.second1, clock.regData.second0, clock.regData.ampm_hour1 ? "PM" : "AM",
			day_string, clock.regData.date1, clock.regData.date0, clock.regData.month1, clock.regData.month0,
			clock.regData.year1, clock.regData.year0);
		//Display the time string (return_home will set the position to the start of the display)
		disp << instr::return_home << time_string;
	}
}
