#include "eepromConf.h"


EepromConf::EepromConf() {
}


EepromConf::~EepromConf() {
}

//* 0 + 2 bytes
uint16_t EepromConf::getMacAddress() {
	return EEPROM.readInt(EEPROM_ADDRESS__MAC_ADDRESS);
}

void EepromConf::setCountOfConf(uint8_t count) {
	EEPROM.writeByte(EEPROM_ADDRESS__COUNT, count);
}

uint8_t EepromConf::getCountOfConf() {
	return EEPROM.readByte(EEPROM_ADDRESS__COUNT);
}

uint8_t EepromConf::getSizeOfMsg(const CONF_MESSAGE * msg) {
	//* size of msg = 2bytes address + lenght of data (2bytes) + data (n bytes)
	return 2 + 1 + msg->_length;
}

void EepromConf::writeConf(uint8_t index, const CONF_MESSAGE * msg) {
	uint8_t address = EEPROM_ADDRESS__CONFS + (index * getSizeOfMsg(msg));
	uint8_t dataSize = 0;
	EEPROM.writeBlock<uint8_t>(address, (uint8_t*)msg, dataSize);
}

const CONF_MESSAGE * EepromConf::readConf(uint8_t index) {
	//memcpy(&_msg,  //* tricko na test
	; EEPROM.readBlock(index, &_msg, 2);
}