#include <avr/io.h>
#include "../tedavr/include/tedavr/hd44780.h"
#include "../light_ws2812/light_ws2812_AVR/Light_WS2812/light_ws2812.h"
#include "../include/ic_ds1307.h"

constexpr uint8_t calculate_twbr(float const scl_freq, float const prescale = 1, float const cpu_freq = F_CPU) {
	return(static_cast<uint8_t>((cpu_freq / (2 * scl_freq * prescale)) - (8 / prescale)));
}

class Program {
public:
	void update();
	
	HD44780 disp;
	Program();
};

void Program::update() {
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
	/*hd44780_putText(&(disp), const_cast<char *>("Hutcho's Lamp"));
	hd44780_setPosition(&disp, HD44780_NEWLINE);
	hd44780_putText(&(disp), const_cast<char *>("Work in progress."));
	*/

}

void sfr_init() {
	DDRD = 0xff;
	DDRB = 0xff;
	DDRC = 0b001111;
	PORTD = 0;
	PORTB = 0;
	PORTC = 0;

	TWBR = calculate_twbr(100000);	//100kHz TWI
	twi::enable();
}

int main() {
	sfr_init();
	Program prg;
	IC_DS1307 clock;

	IC_DS1307::RegData d;
	d.ampm_hour1 = 1;
	d.date0 = 8;
	d.date1 = 2;
	d.month0 = 6;
	d.month1 = 0;
	d.day = 3;
	d.hour0 = 9;
	d.hour1 = 0;
	d.hour_12 = 1;
	d.minute0 = 1;
	d.minute1 = 4;
	d.out = 0;
	d.rs = 0;
	d.second0 = 0;
	d.second1 = 0;
	d.sqwe = 0;
	d.year0 = 6;
	d.year1 = 1;
	clock.regData = d;
	
	//clock.set_all();
	/*
	hd44780_putNumberui8(&(prg.disp), result);
	hd44780_setPosition(&(prg.disp), HD44780_NEWLINE);
	hd44780_putNumberui8(&(prg.disp), TWSR);
	hd44780_putText(&(prg.disp), " ");
	hd44780_putNumberui8(&(prg.disp), twi::SR_VALUE_STATUS_START);
	hd44780_putText(&(prg.disp), " ");
	hd44780_putNumberui8(&(prg.disp), twi::SR_VALUE_STATUS_START & twi::SR_MASK_STATUS);
	*/
	IC_DS1307::RegData regdata_old;
	while (true) {
		while (regdata_old == clock.regData) {
			clock.update();
		}
		regdata_old = clock.regData;
		hd44780_clearDisplay(&(prg.disp));
		hd44780_putNumberui8(&(prg.disp), clock.regData.hour1);
		hd44780_putNumberui8(&(prg.disp), clock.regData.hour0);
		hd44780_putText(&(prg.disp), ":");
		hd44780_putNumberui8(&(prg.disp), clock.regData.minute1);
		hd44780_putNumberui8(&(prg.disp), clock.regData.minute0);
		hd44780_putText(&(prg.disp), ":");
		hd44780_putNumberui8(&(prg.disp), clock.regData.second1);
		hd44780_putNumberui8(&(prg.disp), clock.regData.second0);
		hd44780_putText(&(prg.disp), clock.regData.ampm_hour1 ? const_cast<char *>("PM") : const_cast<char *>("AM"));
		hd44780_setPosition(&(prg.disp), HD44780_NEWLINE);
		hd44780_putNumberui8(&(prg.disp), 20);
		hd44780_putNumberui8(&(prg.disp), clock.regData.year1);
		hd44780_putNumberui8(&(prg.disp), clock.regData.year0);
		hd44780_putText(&(prg.disp), "/");
		hd44780_putNumberui8(&(prg.disp), clock.regData.month1);
		hd44780_putNumberui8(&(prg.disp), clock.regData.month0);
		hd44780_putText(&(prg.disp), "/");
		hd44780_putNumberui8(&(prg.disp), clock.regData.date1);
		hd44780_putNumberui8(&(prg.disp), clock.regData.date0);
		switch (clock.regData.day) {
		case 1:
			hd44780_putText(&(prg.disp), " Sun");
			break;
		case 2:
			hd44780_putText(&(prg.disp), " Mon");
			break;
		case 3:
			hd44780_putText(&(prg.disp), " Tue");
			break;
		case 4:
			hd44780_putText(&(prg.disp), " Wed");
			break;
		case 5:
			hd44780_putText(&(prg.disp), " Thu");
			break;
		case 6:
			hd44780_putText(&(prg.disp), " Fri");
			break;
		case 7:
			hd44780_putText(&(prg.disp), " Sat");
			break;
		default:
			break;
		}
	}

	/*
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
	*/
    return(0);
}
