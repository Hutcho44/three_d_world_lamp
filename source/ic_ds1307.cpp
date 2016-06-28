#include "../include/ic_ds1307.h"

void twi::enable() {
	TWCR |= _BV(TWEN);
}

void twi::disable() {
	TWCR &= ~_BV(TWEN);
}

void twi::wait() {
	while (!(TWCR & _BV(TWINT)));
}

bool twi::start() {
	TWCR = _BV(TWSTA) | _BV(TWINT) | _BV(TWEN);								//Send start condition
	twi::wait();															//Wait
	if (((TWSR & twi::SR_MASK_STATUS) != twi::SR_VALUE_STATUS_START) && ((TWSR & twi::SR_MASK_STATUS) != twi::SR_VALUE_STATUS_REPEATEDSTART)) {		//If a start condition wasn't sent
		return(true);
	}
	return(false);
}

bool twi::stop() {
	TWCR = _BV(TWSTO) | _BV(TWINT) | _BV(TWEN);					//Send stop condition
	return(false);
}

bool twi::address(uint8_t const addr, bool const read) {
	TWDR = (addr << 1) | (read ? 1 : 0);									//Load ds1307 address into data register (shift left one because last lsb is R/W bit, leaving clear means W)
	TWCR = _BV(TWINT) | _BV(TWEN);											//Clear TWINT flag
	twi::wait();															//Wait
	if (read) {
		if ((TWSR & twi::SR_MASK_STATUS) != twi::SR_VALUE_STATUS_SLAR_ACK) {	//If the slave didn't acknowlege
			return(true);
		}
	}
	else {
		if ((TWSR & twi::SR_MASK_STATUS) != twi::SR_VALUE_STATUS_SLAW_ACK) {	//If the slave didn't acknowlege
			return(true);
		}
	}
	return(false);
}

bool twi::recv_packet(uint8_t buffer[], uint8_t const len, bool const nack) {
	for (uint8_t i = 0; i < len; i++) {
		bool nack_time = false;
		if ((i == len - 1) && nack) {
			TWCR &= ~_BV(TWEA);	//Generate NACK for last byte
			nack_time = true;
		}
		else {
			TWCR |= _BV(TWEA);	//Generate ACK
		}
		TWCR |= _BV(TWINT);	//Start receiving
		twi::wait();
		if ((((TWSR & twi::SR_MASK_STATUS) != SR_VALUE_STATUS_RECV_ACK) && (nack_time == false)) || (((TWSR & twi::SR_MASK_STATUS) != SR_VALUE_STATUS_RECV_NACK) && nack_time)) {
			return(true);
		}
		buffer[i] = TWDR;
	}
	return(false);
}

bool twi::send_packet(uint8_t buffer[], uint8_t const len) {
	for (uint8_t i = 0; i < len; i++) {
		TWDR = buffer[i];														//Load TWDR with byte to transmit
		TWCR |= _BV(TWINT);														//Clear TWINT flag
		twi::wait();															//Wait
		if ((TWSR & twi::SR_MASK_STATUS) != twi::SR_VALUE_STATUS_SEND_ACK) {	//If the slave didn't acknowlege
			return(true);
		}
	}
	return(false);
}

bool IC_DS1307::RegData::operator==(RegData const &p0) {
	return((p0.second1 == second1) && (p0.second0 == second0) &&
		(p0.minute1 == minute1) && (p0.minute0 == minute0) &&
		(p0.hour1 == hour1) && (p0.ampm_hour1 == ampm_hour1) && (p0.hour0 == hour0) &&
		(p0.day == day) &&
		(p0.date1 == date1) && (p0.date0 == date0) &&
		(p0.month1 == month1) && (p0.month0 == month0) &&
		(p0.hour_12 == hour_12) &&
		(p0.year1 == year1) && (p0.year0 == year0) &&
		(p0.out == out) && (p0.rs == rs) && (p0.sqwe == sqwe));
}

uint8_t IC_DS1307::update() {
	return(get_all());
}

uint8_t IC_DS1307::get_all() {
	uint8_t raw_data[8];
	uint8_t result;
	result = get_raw_data(raw_data);
	if (result)
		return(result);
	regData.second1 = raw_data[0] >> 4;
	regData.second0 = raw_data[0] & 0x0f;
	regData.minute1 = raw_data[1] >> 4;
	regData.minute0 = raw_data[1] & 0x0f;
	regData.hour_12 = raw_data[2] >> 6;
	regData.ampm_hour1 = (raw_data[2] >> 5) & 0x01;
	regData.hour1 = (raw_data[2] >> 4) & 0x01;
	regData.hour0 = raw_data[2] & 0x0f;
	regData.day = raw_data[3] & 0x07;
	regData.date1 = raw_data[4] >> 4;
	regData.date0 = raw_data[4] & 0x0f;
	regData.month1 = raw_data[5] >> 4;
	regData.month0 = raw_data[5] & 0x0f;
	regData.year1 = raw_data[6] >> 4;
	regData.year0 = raw_data[6] & 0x0f;
	regData.out = raw_data[7] >> 7;
	regData.sqwe = (raw_data[7] >> 4) & 0x01;
	regData.rs = raw_data[7] & 0x03;
	return(0);
}

uint8_t IC_DS1307::set_all() const {
	uint8_t raw_data[8];
	uint8_t result;
	raw_data[0] = (regData.second1 << 4) | regData.second0;
	raw_data[1] = (regData.minute1 << 4) | regData.minute0;
	raw_data[2] = (regData.hour_12 << 6) | (regData.ampm_hour1 << 5) | (regData.hour1 << 4) | regData.hour0;
	raw_data[3] = regData.day;
	raw_data[4] = (regData.date1 << 4) | regData.date0;
	raw_data[5] = (regData.month1 << 4) | regData.month0;
	raw_data[6] = (regData.year1 << 4) | regData.year0;
	raw_data[7] = (regData.out << 7) | (regData.sqwe << 4) | regData.rs;
	result = set_raw_data(raw_data);
	if (result)
		return(result);
	return(0);
}

IC_DS1307::IC_DS1307() {
}

IC_DS1307::IC_DS1307(uint8_t const ntwi_address) : twi_address(ntwi_address) {
}

uint8_t IC_DS1307::get_raw_data(uint8_t data[]) const {
	uint8_t read_addr[1] = { 0x00 };
	if (twi::start()) {
		//Error
		return(1);
	}
	if (twi::address(twi_address, false)) {
		//Error
		return(2);
	}
	if (twi::send_packet(read_addr, 1)) {
		//Error
		return(3);
	}
	if (twi::start()) {
		//Error
		return(4);
	}
	if (twi::address(twi_address, true)) {
		//Error
		return(5);
	}
	if (twi::recv_packet(data, 8)) {
		//Error
		return(6);
	}
	if (twi::stop()) {
		//Error
		return(7);
	}
	return(0);
}

uint8_t IC_DS1307::set_raw_data(uint8_t data[]) const {
	uint8_t add_addr_data[9];
	add_addr_data[0] = 0x00;	//Address to start setting in ds1307
	memcpy(add_addr_data + 1, data, 8);
	if (twi::start()) {
		//Error
		return(1);
	}
	if (twi::address(twi_address, false)) {
		//Error
		return(2);
	}
	if (twi::send_packet(add_addr_data, 9)) {
		//Error
		return(3);
	}
	if(twi::stop()) {
		//Error
		return(4);
	}
	return(0);
}
