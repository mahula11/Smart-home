#include "eepromConf.h"


EepromConf::EepromConf() {
}


EepromConf::~EepromConf() {
}

uint16_t EepromConf::getMacAddress() {
	return EEPROM.readInt(0);
}