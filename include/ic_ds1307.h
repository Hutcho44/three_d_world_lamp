#pragma once

#include <inttypes.h>
#include <string.h>
#include <avr/io.h>

namespace twi {
	constexpr uint8_t SR_MASK_PRESCALE = (_BV(TWPS1) | _BV(TWPS0));
	constexpr uint8_t SR_MASK_STATUS = (_BV(TWS7) | _BV(TWS6) | _BV(TWS5) | _BV(TWS4) | _BV(TWS3));
	constexpr uint8_t SR_VALUE_STATUS_START = 0x08;		//Not shifted, assuming that perscaler bits are 0
	constexpr uint8_t SR_VALUE_STATUS_REPEATEDSTART = 0x10;
	constexpr uint8_t SR_VALUE_STATUS_SLAW_ACK = 0x18;
	constexpr uint8_t SR_VALUE_STATUS_SLAR_ACK = 0x40;
	constexpr uint8_t SR_VALUE_STATUS_SEND_ACK = 0x28;
	constexpr uint8_t SR_VALUE_STATUS_RECV_ACK = 0x50;
	constexpr uint8_t SR_VALUE_STATUS_RECV_NACK = 0x58;

	void enable();
	void disable();
	void wait();
	bool start();
	bool stop();
	bool address(uint8_t const addr, bool const read);
	bool recv_packet(uint8_t buffer[], uint8_t const len, bool const nack = true);
	bool send_packet(uint8_t buffer[], uint8_t const len);
}

class IC_DS1307 {
public:
	struct RegData {
		bool operator==(RegData const &p0);
		uint8_t second1 : 3;	//Seconds 10s digit
		uint8_t second0 : 4;	//Seconds 1s digit
		uint8_t minute1 : 3;	//Minutes 10s digit
		uint8_t minute0 : 4;	//Minutes 1s digit
		uint8_t hour_12 : 1;	//0 = 24 hour mode; 1 = 12 hour mode;
		uint8_t ampm_hour1 : 1;	//24: bit1 of the hour 10s; 12: (0 = AM; 1 = PM);
		uint8_t hour1 : 1;		//bit0 of the hour 10s
		uint8_t hour0 : 4;		//Hours 1s digit
		uint8_t day : 3;		//Day of the week
		uint8_t date1 : 2;		//Date 10s digit
		uint8_t date0 : 4;		//Date 1s digit
		uint8_t month1 : 1;		//Month 10s digit
		uint8_t month0 : 4;		//Month 1s digit
		uint8_t year1 : 4;		//Year 10s digit
		uint8_t year0 : 4;		//Year 1s digit
		uint8_t out : 1;		//(if sqwe = 0): 0 = set sqwe out to 0; 1 = set sqwe out to 1;
		uint8_t sqwe : 1;		//0 = Disble square wave output; 1 = Enable quare wave output;
		uint8_t rs : 2;			//(sqwe frequency) 0 = 1Hz; 1 = 4.096kHz; 2 = 8.192kHz; 3 = 32.768kHz;
	};
	constexpr static uint8_t twi_address_d = 0b1101000;

	//Calls get_all
	uint8_t update();
	//Gets the time and configuration from the ds1307
	uint8_t get_all();
	//Sets the time and configuration on the ds1307 
	uint8_t set_all() const;

	IC_DS1307();
	IC_DS1307(uint8_t const ntwi_address);


	uint8_t twi_address = twi_address_d;

	RegData regData;
protected:
	uint8_t get_raw_data(uint8_t data[]) const;
	uint8_t set_raw_data(uint8_t data[]) const;
};
