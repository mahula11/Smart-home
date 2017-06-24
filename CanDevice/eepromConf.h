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
//*    3      |       1         | count of confs
//*    4      | n * sizeof(msg) | configurations, n - count of confs(from address 2)

#define EEPROM_ADDRESS__MAC_ADDRESS 0
#define EEPROM_ADDRESS__WATCHDOG_TIMEOUT 2
#define EEPROM_ADDRESS__CONF_COUNT 3
#define EEPROM_ADDRESS__CONFS 4

class EepromConf {
private:
	CONF * _pConf;
	void setCountOfConf(uint8_t count);		
public:
	EepromConf();
	~EepromConf();

	uint16_t getMacAddress();
	uint8_t getWatchdogTimeout();
	void setWatchdogTimeout(uint8_t to);
	uint8_t getCountOfConf();
	void writeConf(const CONF * pConf);
	const CONF * readConf();
};

