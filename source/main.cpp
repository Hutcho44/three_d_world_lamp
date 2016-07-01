#include <avr/io.h>															//Include AVR definitions (EG PORTD)
#include <stdio.h>															//Include things like sprintf
#include <string.h>															//Include things like strcpy
#include "../tedavr/include/tedavr/ic_hd44780.h"							//Include display class
#include "../include/ic_ds1307.h"											//Include clock class
#include "../light_ws2812/light_ws2812_AVR/Light_WS2812/light_ws2812.h"		//Include ws2812 (neopixel) functions

//This function calculates a bitrate value for the TWI. Don't worry about it.
constexpr uint8_t calculate_twbr(float const scl_freq, float const prescale = 1, float const cpu_freq = F_CPU) {
	return(static_cast<uint8_t>((cpu_freq / (2 * scl_freq * prescale)) - (8 / prescale)));
}

//Initialise special function registers
void sfr_init() {
	DDRD = 0b11111111;					//Set all 8 PORTD pins to outputs
	DDRB = 0b11111111;					//Set all 8 PORTD pins to outputs
	DDRC = 0b001111;					//Sets two PORTC pins to inputs (the the clock), and the rest to outputs (PORTC only has 6 pins)
	PORTD = 0;							//Set the outpus on PORTD to 0
	PORTB = 0;							//Set the outputs on PORTB to 0
	PORTC = 0;							//Set the outpus on PORTC to 0

	TWBR = calculate_twbr(100000);		//This is for communitcation with the clock. It wants a 100kHz TWI (two wire) interface.
										//(continue) This calls the function at the top of the file.
										//(continue) We tell it we want 100 000 Hz (the number in brackets) and it works out what value needs to go in the TWI bit-rate register to give us that.
	twi::enable();						//This enables the TWI interface. This means that we can't use the 2 PORTC pins for anything other than TWI for now.
}

//Remember, we always start the program at main (so start here)
int main() {
	sfr_init();							//Initialise special function registers (calls (goes to) the function just above this one)))


	IC_HD44780 disp;					//Create an instance of the C++ class IC_HD44780 (the chip that drives the display). This will allow us to interface with the display.
										//(continue) We call our display 'disp', so when we want it we will use 'disp' in the code.

	//These tell disp the pin data direction registers for the respective pins. This is so that when it needs to, the MCU can switch them between inputs and outputs.
	disp.pin.ddr_data0 = &DDRD;
	disp.pin.ddr_data1 = &DDRD;
	disp.pin.ddr_data2 = &DDRD;
	disp.pin.ddr_data3 = &DDRD;
	
	//These tell disp the pin input registers for the respective pins. This is so that the MCU can read from the display data pins.
	disp.pin.port_in_data0 = &PIND;
	disp.pin.port_in_data1 = &PIND;
	disp.pin.port_in_data2 = &PIND;
	disp.pin.port_in_data3 = &PIND;

	//These tell disp the pin output registers for the respective pins. This is so that the MCU can output to the display pins.
	disp.pin.port_out_data0 = &PORTD;
	disp.pin.port_out_data1 = &PORTD;
	disp.pin.port_out_data2 = &PORTD;
	disp.pin.port_out_data3 = &PORTD;
	disp.pin.port_out_rs = &PORTD;
	disp.pin.port_out_rw = &PORTD;
	disp.pin.port_out_en = &PORTD;
	
	//These tell disp the bits that the pins are located on the respective ports. EG data0 in located on PORTD-0, and since we set it to PORTD above we need to set it to 0 here.
	disp.pin.shift_data0 = 0;
	disp.pin.shift_data1 = 1;
	disp.pin.shift_data2 = 2;
	disp.pin.shift_data3 = 3;
	disp.pin.shift_rs = 4;
	disp.pin.shift_en = 6;
	disp.pin.shift_rw = 5;

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
	disp << display_power::cursor_on;					//Turn cusor on (part of display power instruction)
	disp << display_power::cursorblink_on;				//Turnon cursor blinking (part of display power instruction)

	//We don't have to do things one after the other. We can chain the instructions and values by just adding a '<<'
	//This is the instruction to set the entry mode, we want it so that the cursor moves right, and we don't want the display to shift with it.
	disp << instr::entry_mode_set << entry_mode_set::cursormove_right << entry_mode_set::displayshift_disable;

	//Clear the display
	disp << instr::clear_display;
	//Print "Hello! (newline) World!" to the display.
	//disp << "Hello!\nWorld!";

	//Note: We could have chained all of that:
	//disp << instr::init_4bit << instr::function_set << function_set::datalength_4 << function_set::font_5x8 << function_set::lines_2 << instr::display_power << display_power::display_on << display_power::cursor_on << display_power::cursorblink_on << instr::entry_mode_set << entry_mode_set::cursormove_right << entry_mode_set::displayshift_disable << instr::clear_display << "Waaaaat too\nLooooooooooooong";
	//But why would we do that? ;)
	
	
	IC_DS1307 clock;	//Create an instance of a DS1307 real-time-clock (the real-time-clock that we are using)
						//(continue) We call it clock, so from now on we will use 'clock' to refer to it.
	//This one is easier to use. All we need to do is call the clock function 'get_all'. This is done using the '.' operator to access the function within the clock object.
	clock.get_all();
	//Now the time is stored in 'clock'. Inside clock there is a data stucture (a collection of data) called 'regData' (short for register data) that the time is stored in.
	//To access regData, we use the '.' operator. To access the contents of regData, we also use the '.' operator.
	clock.regData.second0;	//This is the seconds '1s' digit (see ds_1307.h (a file, look for it in the GitHub repository) for more information on all the different values)
	//^^^ But that's pointless. We need to do something with it. We'll use an alternative to the printf function to write the data to an array of char's that can be outputted to the display.
	//I'll explain more about that when I see you.
	char day_string[4];
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
	char time_string[(16 * 2) + 2];	//Create a character array that's (16*2)+1 characters long. This is because each line of the display is 16 characters, and we have 2 lines. The +2 for the newline character '\n' and the terminating character '\0'
	sprintf(time_string, "%u%u:%u%u:%u%u%s\n%s %u%u/%u%u/20%u%u", clock.regData.hour1, clock.regData.hour0, clock.regData.minute1, clock.regData.minute0, clock.regData.second1, clock.regData.second0, clock.regData.ampm_hour1 ? "PM" : "AM",
		day_string, clock.regData.date1, clock.regData.date0, clock.regData.month1, clock.regData.month0, clock.regData.year1, clock.regData.year0);
	//That filled 'time_string' with a time string like the following: hh/mm/ss/AMPM (newline) day dd/mm/yyyy
	disp << time_string;	//Output the time to the display
	
	//That's all good, but it only happens once. So we put it in a loop (and we would get rid of the above, because we don't need it twice. But we'll keep it for educational purposes)
	//while (true) means it will loop forever, since true is always true
	//We create a new regData called 'regData_old'. We use this to compare whether there has been a change in the time since the last loop.
	//If there is a change, we re-write the time to the display. If we hadn't done that, it would re-write the time every loop, which sort of looks strange on the display.
	IC_DS1307::RegData regData_old;
	while (true) {
		//Start of the loop.
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
