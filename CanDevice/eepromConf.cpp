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

//* write whole configuration to eeprom
void EepromConf::writeConf(const CONF * pConf) {
	//* write number of confs
	setCountOfConf(pConf->count);
	//* eeprom writing start at EEPROM_ADDRESS__CONFS
	short address = EEPROM_ADDRESS__CONFS;
	byte data[10];
	byte size;
	//* write particular confs
	for (byte i = 0; i < pConf->count; i++) {
		//* serialize selected device
		pConf->ppConfData[i]->serialize(data);
		size = pConf->ppConfData[i]->getSize();
		//* write to eeprom
		eeprom_write_block(&data, (void*)address, size);
		//* increase address
		address += size;
	}
}

//* read whole configuration from eeprom
const CONF * EepromConf::readConf() {
	//* from eeprom read number of confs
	DEBUG("readConf 1" << endl);
	uint8_t count = getCountOfConf();
	DEBUG("readConf 2" << endl);
	//* make new configuration
	_pConf = SmartHouse::newConf(count, getMacAddress());
	//* eeprom reading starting on address EEPROM_ADDRESS__CONFS
	short address = EEPROM_ADDRESS__CONFS;
	CDataBase * pConfData;
	byte size;
	byte data[10];
	DEBUG(F("Number of configurations in EEPROM = ") << count << endl);
	//* read particular conf
	for (byte i = 0; i < count; i++) {
		//* get type of device
		byte deviceType = EEPROM.readByte(address);
		DEBUG(F("deviceType = ") << deviceType << endl);
		switch (CDataBase::getType(&deviceType)) {
			case DEVICE_TYPE_LIGHT:
				pConfData = new CConfDataLight;
				break;
			case DEVICE_TYPE_SWITCH:
				pConfData = new CConfDataSwitch;				
				break;
		}
		size = pConfData->getSize();		
		//* read from eeprom
		eeprom_read_block(&data, (const void*)address, size);
		pConfData->deserialize(data);
		//* increase address to next conf
		address += size;
		//* insert to global structure
		_pConf->ppConfData[i] = pConfData;
	}
	return _pConf;
}