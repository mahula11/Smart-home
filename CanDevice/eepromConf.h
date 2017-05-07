#pragma once

#include <EEPROMex.h>

#include <Arduino.h>

#include <smartHouse.h>

//* eeprom map
//*  address  |     lenght      | description
//* -------------------------------------------
//*    0      |       2         | mac address
//*    2      |       1         | count of confs
//*    3      | n * sizeof(msg) | configurations, n - count of confs(from address 2)

#define EEPROM_ADDRESS__MAC_ADDRESS 0
#define EEPROM_ADDRESS__COUNT 2
#define EEPROM_ADDRESS__CONFS 3

class EepromConf {
private:
	CONF * _pConf;
	void setCountOfConf(uint8_t count);		
public:
	EepromConf();
	~EepromConf();

	uint16_t getMacAddress();
	uint8_t getCountOfConf();
	void writeConf(const CONF * pConf);
	const CONF * readConf();
};

