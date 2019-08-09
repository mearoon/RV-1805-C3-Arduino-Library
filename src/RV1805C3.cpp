/*
	Arduino Library for RV-1805-C3
	
	Copyright (c) 2018 Macro Yau

	https://github.com/MacroYau/RV-1805-C3-Arduino-Library
*/

#include "RV1805C3.h"

RV1805C3::RV1805C3() {

}

bool RV1805C3::begin(TwoWire &wirePort) {
	// Wire.begin() should be called in the application code in advance
	_i2cPort = &wirePort;

	// Checks part number
	uint8_t partNumber[2] = { 0, 0 };
	if (!readBytesFromRegisters(REG_ID0, partNumber, 2)) {
		return false;
	}

	if (partNumber[0] == PART_NUMBER_MSB && partNumber[1] == PART_NUMBER_LSB) {
		enableOscillatorSwitching();
		reduceLeakage();
		return true;
	} else {
		return false;
	}
}

void RV1805C3::reset() {
	writeByteToRegister(REG_CONFIG_KEY, CONFKEY_RESET);
}

void RV1805C3::enableCrystalOscillator() {
	uint8_t value = readByteFromRegister(REG_OSC_CONTROL);
	value &= 0b00011111;
	writeByteToRegister(REG_CONFIG_KEY, CONFKEY_OSC_CONTROL);
	writeByteToRegister(REG_OSC_CONTROL, value);
}

void RV1805C3::disableCrystalOscillator() {
	uint8_t value = readByteFromRegister(REG_OSC_CONTROL);
	value &= 0b00011111;
	value |= (1 << 7); // Uses RC oscillator all the time to minimize power usage
	value |= (0b11 << 5); // Enables autocalibration every 512 seconds (22 nA consumption)
	writeByteToRegister(REG_CONFIG_KEY, 0xA1);
	writeByteToRegister(REG_OSC_CONTROL, value);
}

void RV1805C3::enableOscillatorSwitching() {
	uint8_t value = readByteFromRegister(REG_OSC_CONTROL);
	value &= 0b11100000;
	value |= (1 << 4); // Switches to RC oscillator when using backup power
	value |= (1 << 3); // Switches to RC oscillator when the XT one fails
	writeByteToRegister(REG_OSC_CONTROL, value);
}

void RV1805C3::reduceLeakage() {
	uint8_t value;

	value = readByteFromRegister(REG_CONTROL2);
	value &= 0b11011111; // Set X to 0
	writeByteToRegister(REG_CONTROL2, value);

	// Disable I2C interface when using backup power source
	writeByteToRegister(REG_CONFIG_KEY, CONFKEY_REGISTERS);
	writeByteToRegister(REG_IO_BATMODE, 0x00);

	// Disable WDI, !RST, and CLK/!INT when powered by backup source or in sleep mode
	writeByteToRegister(REG_CONFIG_KEY, CONFKEY_REGISTERS);
	writeByteToRegister(REG_OUTPUT_CONTROL, 0x30);
}

char* RV1805C3::getCurrentDateTime() {
	// Updates RTC date time value to array
	readBytesFromRegisters(REG_TIME_HUNDREDTHS, _dateTime, DATETIME_COMPONENTS);

	// Returns ISO 8601 date time string
	static char iso8601[23];
	sprintf(iso8601, "20%02d-%02d-%02dT%02d:%02d:%02d", 
			convertToDecimal(_dateTime[DATETIME_YEAR]),
			convertToDecimal(_dateTime[DATETIME_MONTH]),
			convertToDecimal(_dateTime[DATETIME_DAY_OF_MONTH]),
			convertToDecimal(_dateTime[DATETIME_HOUR]),
			convertToDecimal(_dateTime[DATETIME_MINUTE]),
			convertToDecimal(_dateTime[DATETIME_SECOND]));
	return iso8601;
}

void RV1805C3::setDateTimeFromISO8601(String iso8601) {
	return setDateTimeFromISO8601(iso8601.c_str());
}

void RV1805C3::setDateTimeFromISO8601(const char *iso8601) {
	// Assumes the input is in the format of "2018-01-01T08:00:00" (hundredths and time zone, if applicable, will be neglected)
	char components[3] = { '0', '0', '\0' };
	uint8_t buffer[6];

	for (uint8_t i = 2, j = 0; i < 20; i += 3, j++) {
		components[0] = iso8601[i];
		components[1] = iso8601[i + 1];
		buffer[j] = atoi(components);
	}

	// Since ISO 8601 date string does not indicate day of week, it is set to 0 (Sunday) and is no longer correct
	setDateTime(2000 + buffer[0], buffer[1], buffer[2], SUN, buffer[3], buffer[4], buffer[5], 0);
}

void RV1805C3::setDateTimeFromHTTPHeader(String str) {
	return setDateTimeFromHTTPHeader(str.c_str());
}

void RV1805C3::setDateTimeFromHTTPHeader(const char* str) {
	char components[3] = { '0', '0', '\0' };
	
	// Checks whether the string begins with "Date: " prefix
	uint8_t counter = 0;
	if (str[0] == 'D') {
		counter = 6;
	}
	
	// Day of week
	uint8_t dayOfWeek = 0;
	if (str[counter] == 'T') {
		// Tue or Thu
		if (str[counter + 1] == 'u') {
			// Tue
			dayOfWeek = 2;
		} else {
			dayOfWeek = 4;
		}
	} else if (str[counter] == 'S') {
		// Sat or Sun
		if (str[counter + 1] == 'a') {
			dayOfWeek = 6;
		} else {
			dayOfWeek = 0;
		}
	} else if (str[counter] == 'M') {
		// Mon
		dayOfWeek = 1;
	} else if (str[counter] == 'W') {
		// Wed
		dayOfWeek = 3;
	} else {
		// Fri
		dayOfWeek = 5;
	}

	// Day of month
	counter += 5;
	components[0] = str[counter];
	components[1] = str[counter + 1];
	uint8_t dayOfMonth = atoi(components);

	// Month
	counter += 3;
	uint8_t month = 0;
	if (str[counter] == 'J') {
		// Jan, Jun, or Jul
		if (str[counter + 1] == 'a') {
			// Jan
			month = 1;
		} else if (str[counter + 2] == 'n') {
			// Jun
			month = 6;
		} else {
			// Jul
			month = 7;
		}
	} else if (str[counter] == 'F') {
		// Feb
		month = 2;
	} else if (str[counter] == 'M') {
		// Mar or May
		if (str[counter + 2] == 'r') {
			// Mar
			month = 3;
		} else {
			// May
			month = 5;
		}
	} else if (str[counter] == 'A') {
		// Apr or Aug
		if (str[counter + 1] == 'p') {
			// Apr
			month = 4;
		} else {
			// Aug
			month = 8;
		}
	} else if (str[counter] == 'S') {
		// Sep
		month = 9;
	} else if (str[counter] == 'O') {
		// Oct
		month = 10;
	} else if (str[counter] == 'N') {
		// Nov
		month = 11;
	} else {
		month = 12;
	}

	// Year
	counter += 6;
	components[0] = str[counter];
	components[1] = str[counter + 1];
	uint16_t year = 2000 + atoi(components);

	// Time of day
	counter += 3;
	uint8_t buffer[3];
	for (uint8_t i = counter, j = 0; j < 3; i += 3, j++) {
		components[0] = str[i];
		components[1] = str[i + 1];
		buffer[j] = atoi(components);
	}
	
	setDateTime(year, month, dayOfMonth, static_cast<DayOfWeek_t>(dayOfWeek), buffer[0], buffer[1], buffer[2], 0);
}

bool RV1805C3::setDateTime(uint16_t year, uint8_t month, uint8_t dayOfMonth, DayOfWeek_t dayOfWeek, uint8_t hour, uint8_t minute, uint8_t second, uint8_t hundredth) {
	// Year 2000 AD is the earliest allowed year in this implementation
	if (year < 2000) { return false; }
	// Century overflow is not considered yet (i.e., only supports year 2000 to 2099)
	_dateTime[DATETIME_YEAR] = convertToBCD(year - 2000);

	if (month < 1 || month > 12) { return false; }
	_dateTime[DATETIME_MONTH] = convertToBCD(month);

	if (dayOfMonth < 1 || dayOfMonth > 31) { return false; }
	_dateTime[DATETIME_DAY_OF_MONTH] = convertToBCD(dayOfMonth);

	if (dayOfWeek > 6) { return false; }
	_dateTime[DATETIME_DAY_OF_WEEK] = convertToBCD(dayOfWeek);

	// Uses 24-hour notation by default
	if (hour > 23) { return false; }
	_dateTime[DATETIME_HOUR] = convertToBCD(hour);

	if (minute > 59) { return false; }
	_dateTime[DATETIME_MINUTE] = convertToBCD(minute);

	if (second > 59) { return false; }
	_dateTime[DATETIME_SECOND] = convertToBCD(second);

	if (hundredth > 99) { return false; }
	_dateTime[DATETIME_HUNDREDTH] = convertToBCD(hundredth);

	return true;
}

void RV1805C3::setDateTimeComponent(DateTimeComponent_t component, uint8_t value) {
	// Updates RTC date time value to array
	readBytesFromRegisters(REG_TIME_HUNDREDTHS, _dateTime, DATETIME_COMPONENTS);

	_dateTime[component] = convertToBCD(value);
}

bool RV1805C3::synchronize() {
	return writeBytesToRegisters(REG_TIME_HUNDREDTHS, _dateTime, DATETIME_COMPONENTS);
}

bool RV1805C3::setAlarmMode(AlarmMode_t mode) {
	uint8_t value = readByteFromRegister(REG_COUNTDOWN_CONTROL);
	value &= 0b11100011;

	if (mode == ALARM_ONCE_PER_TENTH) {
		writeByteToRegister(REG_ALARM_HUNDREDTHS, 0xF0);
		value |= (7 << 2);
	} else if (mode == ALARM_ONCE_PER_HUNDREDTH) {
		writeByteToRegister(REG_ALARM_HUNDREDTHS, 0xFF);
		value |= (7 << 2);
	} else {
		value |= (mode << 2);
	}

	writeByteToRegister(REG_COUNTDOWN_CONTROL, value);
}

bool RV1805C3::setAlarmFromISO8601(String iso8601) {
	return setAlarmFromISO8601(iso8601.c_str());
}

bool RV1805C3::setAlarmFromISO8601(const char *iso8601) {
	setDateTimeFromISO8601(iso8601);
	return writeBytesToRegisters(REG_ALARM_HUNDREDTHS, _dateTime, DATETIME_COMPONENTS);
}

bool RV1805C3::setAlarmFromHTTPHeader(String str) {
	return setAlarmFromHTTPHeader(str.c_str());
}

bool RV1805C3::setAlarmFromHTTPHeader(const char *str) {
	setDateTimeFromHTTPHeader(str);
	return writeBytesToRegisters(REG_ALARM_HUNDREDTHS, _dateTime, DATETIME_COMPONENTS);
}

bool RV1805C3::setAlarm(uint16_t year, uint8_t month, uint8_t dayOfMonth, DayOfWeek_t dayOfWeek, uint8_t hour, uint8_t minute, uint8_t second, uint8_t hundredth) {
	if (!setDateTime(year, month, dayOfMonth, dayOfWeek, hour, minute, second, hundredth)) {
		return false;
	}

	return writeBytesToRegisters(REG_ALARM_HUNDREDTHS, _dateTime, DATETIME_COMPONENTS);
}

void RV1805C3::setCountdownTimer(uint8_t period, CountdownUnit_t unit, bool repeat, bool interruptAsPulse) {
	if (period == 0) { return; }

	// Sets timer value
	writeByteToRegister(REG_COUNTDOWN_TIMER, period - 1);
	writeByteToRegister(REG_TIMER_INIT_VALUE, period - 1);

	// Enables timer
	uint8_t value = readByteFromRegister(REG_COUNTDOWN_CONTROL);
	value &= 0b00011100; // Clears countdown timer bits while preserving ARPT
	value |= unit; // Sets clock frequency
	value |= (!interruptAsPulse << 6);
	value |= (repeat << 5);
	value |= (1 << 7); // Timer enable
	writeByteToRegister(REG_COUNTDOWN_CONTROL, value);
}

void RV1805C3::enableInterrupt(InterruptType_t type) {
	uint8_t value = readByteFromRegister(REG_INTERRUPT_MASK);
	value |= (1 << type);
	writeByteToRegister(REG_INTERRUPT_MASK, value);
}

void RV1805C3::disableInterrupt(InterruptType_t type) {
	uint8_t value = readByteFromRegister(REG_INTERRUPT_MASK);
	value &= ~(1 << type);
	writeByteToRegister(REG_INTERRUPT_MASK, value);
}

uint8_t RV1805C3::clearInterrupts() {
	uint8_t value = readByteFromRegister(REG_STATUS);
	uint8_t flags = (value & 0b00111110);
	value &= 0b11000001;
	writeByteToRegister(REG_STATUS, value);
	return (flags >> 1);
}

void RV1805C3::sleep(SleepWaitPeriod_t waitPeriod, bool disableInterface) {
	uint8_t value;

	if (disableInterface) {
		value = readByteFromRegister(REG_OSC_CONTROL);
		value &= 0b11111011;
		value |= (1 << 2); // Disables I2C interface during sleep, use with caution!
		writeByteToRegister(REG_CONFIG_KEY, CONFKEY_OSC_CONTROL);
		writeByteToRegister(REG_OSC_CONTROL, value);
	}

	value = readByteFromRegister(REG_SLEEP_CONTROL);
	value |= (1 << 7); // Sleep Request
	value |= waitPeriod; // Sleep Wait
	writeByteToRegister(REG_SLEEP_CONTROL, value);
}

void RV1805C3::setPowerSwitchFunction(PowerSwitchFunction_t function) {
	uint8_t value = readByteFromRegister(REG_CONTROL2);
	value &= 0b11100011;
	value |= (function << 2);
	writeByteToRegister(REG_CONTROL2, value);
}

void RV1805C3::lockPowerSwitch() {
	uint8_t value = readByteFromRegister(REG_OSC_STATUS);
	value |= (1 << 5);
	writeByteToRegister(REG_OSC_STATUS, value);
}

void RV1805C3::unlockPowerSwitch() {
	uint8_t value = readByteFromRegister(REG_OSC_STATUS);
	value &= ~(1 << 5);
	writeByteToRegister(REG_OSC_STATUS, value);
}

void RV1805C3::setStaticPowerSwitchOutput(uint8_t level) {
	uint8_t value = readByteFromRegister(REG_CONTROL1);
	value &= ~(1 << 5);
	value |= ((level == 0 ? 0 : 1) << 5);
	writeByteToRegister(REG_CONTROL1, value);
}

uint8_t RV1805C3::convertToDecimal(uint8_t bcd) {
	return (bcd / 16 * 10) + (bcd % 16);
}

uint8_t RV1805C3::convertToBCD(uint8_t decimal) {
	return (decimal / 10 * 16) + (decimal % 10);
}

bool RV1805C3::readBytesFromRegisters(uint8_t startAddress, uint8_t *destination, uint8_t length) {
	_i2cPort->beginTransmission(RV1805C3_ADDRESS);
	_i2cPort->write(startAddress);
	_i2cPort->endTransmission(false);

	_i2cPort->requestFrom((uint8_t) RV1805C3_ADDRESS, length);
	for (uint8_t i = 0; i < length; i++) {
		destination[i] = _i2cPort->read();
	}
	return (_i2cPort->endTransmission() == 0);
}

bool RV1805C3::writeBytesToRegisters(uint8_t startAddress, uint8_t *values, uint8_t length) {
	_i2cPort->beginTransmission(RV1805C3_ADDRESS);
	_i2cPort->write(startAddress);
	for (uint8_t i = 0; i < length; i++) {
		_i2cPort->write(values[i]);
	}
	return (_i2cPort->endTransmission() == 0);
}

uint8_t RV1805C3::readByteFromRegister(uint8_t address) {
	uint8_t value = 0;

	_i2cPort->beginTransmission(RV1805C3_ADDRESS);
	_i2cPort->write(address);
	_i2cPort->endTransmission(false);

	_i2cPort->requestFrom(RV1805C3_ADDRESS, 1);
	value = _i2cPort->read();
	_i2cPort->endTransmission();

	return value;
}

bool RV1805C3::writeByteToRegister(uint8_t address, uint8_t value) {
	_i2cPort->beginTransmission(RV1805C3_ADDRESS);
	_i2cPort->write(address);
	_i2cPort->write(value);
	return (_i2cPort->endTransmission() == 0);
}
