#include <avr/io.h>
#include "../tedavr/include/tedavr/hd44780.h"
#include "../light_ws2812/light_ws2812_AVR/Light_WS2812/light_ws2812.h"
#include "../ds_rtc_lib/library-gcc/rtc.h"

class Program {
public:
	void update();
	
	HD44780 disp;
	tm rtc;
	tm rtc_prev;
	Program();
};

void Program::update() {
	//Clock
	rtc = *rtc_get_time();
	//Display
	if (rtc.twelveHour != rtc_prev.twelveHour) {
		hd44780_setPosition(&disp, 0);
		hd44780_putNumberi16(&disp, rtc.twelveHour);
	}
	if (rtc.min != rtc_prev.min) {
		hd44780_setPosition(&disp, 3);
		hd44780_putNumberi16(&disp, rtc.min);
	}
	if (rtc.sec != rtc_prev.sec) {
		hd44780_setPosition(&disp, 6);
		hd44780_putNumberi16(&disp, rtc.sec);
	}
	if (rtc.am != rtc_prev.am) {
		hd44780_setPosition(&disp, 8);
		if (rtc.am) {
			hd44780_putText(&disp, "AM");
		}
		else {
			hd44780_putText(&disp, "PM");
		}
	}
	rtc_prev = rtc;
}

Program::Program() {
	hd44780_clear(&disp);
	hd44780Set_line(&disp, HD44780_DATA0, &PORTD, 0);
	hd44780Set_line(&disp, HD44780_DATA1, &PORTD, 1);
	hd44780Set_line(&disp, HD44780_DATA2, &PORTD, 2);
	hd44780Set_line(&disp, HD44780_DATA3, &PORTD, 3);
	hd44780Set_line(&disp, HD44780_REGISTER_SELECT, &PORTD, 4);
	hd44780Set_line(&disp, HD44780_ENABLE, &PORTD, 6);
	hd44780_init(&disp);
	hd44780_turnDisplayOn(&disp, hd44780_calculateCursor(0, 0));
	hd44780_setShift(&disp, hd44780_calculateShift(0, 0));
	hd44780_clearDisplay(&disp);
	hd44780_putText(&(disp), "Hutcho's Lamp");
	hd44780_setPosition(&disp, HD44780_NEWLINE);
	hd44780_putText(&(disp), "Work in progress.");

}

int main() {
	DDRD = 0xff;
	DDRB = 0xff;
	DDRC = 0b01111111;
	PORTD = 0;
	PORTB = 0;
	PORTC = 0b10000000;
	Program prg;


	cRGB led[3];
	for (uint8_t i = 0; i < 3; i++) {
		led[i].r = 255;
		led[i].g = 0;
		led[i].b = 0;
	}
	bool led_solid = false;
	while (1) {
		if (!led_solid) {
			led[2] = led[1];
			led[1] = led[0];
			led[0].r += 8;
			led[0].g -= 10;
			led[0].b += 3;
			_delay_ms(100);
		}
		if (!(PINC & _BV(5))) {
			led_solid = !led_solid;
			_delay_ms(100);
		}
		ws2812_setleds(led, 3);
	}
    return(0);
}
