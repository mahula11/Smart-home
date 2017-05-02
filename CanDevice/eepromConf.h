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
	CONF_MESSAGE _msg;
public:
	EepromConf();
	~EepromConf();

	uint16_t getMacAddress();
	uint8_t getCountOfConf();
	void setCountOfConf(uint8_t count);
	void writeConf(uint8_t index, const CONF_MESSAGE * msg);
	const CONF_MESSAGE * readConf(uint8_t index);
	uint8_t getSizeOfMsg(const CONF_MESSAGE * msg);
};

