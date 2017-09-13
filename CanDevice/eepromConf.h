#pragma once

#include <EEPROMex.h>

#include <Arduino.h>

#include <smartHouse.h>
#include <Streaming.h>

//* eeprom map
//*  address  |     lenght      | description
//* -------------------------------------------
//*    0      |       2         | mac address
//*    2      |       1         | enum WATCHDOG_TIMEOUT
//*    3      |       1         | enum of times for automatic reset
//*    4      |       1         | count of confs
//*    5      |       1         | can bus speed
//*    6      | n * sizeof(msg) | configurations, n - count of confs(from address 2)

#define EEPROM_ADDRESS__MAC_ADDRESS      0
#define EEPROM_ADDRESS__WATCHDOG_TIMEOUT 2
#define EEPROM_ADDRESS__AUTO_RESET_TIME  3
#define EEPROM_ADDRESS__CONF_COUNT       4
#define EEPROM_ADDRESS__CAN_BUS_SPEED	 5
#define EEPROM_ADDRESS__CONFS            6

class EepromConf {
private:
	CONF * _pConf;
	void setCountOfConf(uint8_t count);		
public:
	EepromConf();
	~EepromConf();

	uint16_t getMacAddress();
	uint8_t getCountOfConf();
	void writeConf(uint8_t confCount, const CONF * pConf);
	CONF * readConf();

	uint8_t getWatchdogTimeout();
	void setWatchdogTimeout(uint8_t to);
	uint8_t getAutoResetTime();
	void setAutoResetTime(uint8_t ar);
	uint8_t getCanBusSpeed();
	void setCanBusSpeed(uint8_t speed);
};

