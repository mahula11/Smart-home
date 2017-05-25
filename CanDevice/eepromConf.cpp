#include "eepromConf.h"


EepromConf::EepromConf() {
}


EepromConf::~EepromConf() {
}

//* 0 + 2 bytes
//* vrati adresu zariadenia
uint16_t EepromConf::getMacAddress() {
	return EEPROM.readInt(EEPROM_ADDRESS__MAC_ADDRESS);
}

//* nastavi pocet sprav, ktore sa idu zapisat do eeprom
void EepromConf::setCountOfConf(uint8_t count) {
	EEPROM.writeByte(EEPROM_ADDRESS__COUNT, count);
}

//* vrati pocet sprav v eeprom
uint8_t EepromConf::getCountOfConf() {
	return EEPROM.readByte(EEPROM_ADDRESS__COUNT);
}

//* 
//uint8_t EepromConf::getSizeOfMsgs(const CONF_MESSAGE * msg) {
//	//* size of msg = 2bytes address + lenght of data (2bytes) + data (n bytes)
//	return 2 + 1 + msg->_length;
//}

//* zapise vsetky spravy do eeprom
void EepromConf::writeConf(const CONF * pConf) {
	setCountOfConf(pConf->count);
	EEPROM.writeBlock(EEPROM_ADDRESS__CONFS, pConf->pMsgs, pConf->count);
}

//* vrati novu strukturu s konfiguraciou
const CONF * EepromConf::readConf() {
	uint8_t count = getCountOfConf();
	//_pConf = new CONF_MESSAGE[count];
	_pConf = SmartHouse::newConf(count, getMacAddress());
	EEPROM.readBlock(EEPROM_ADDRESS__CONFS, *_pConf->ppConfData, count);
	return _pConf;
}