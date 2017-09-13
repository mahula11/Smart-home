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

uint8_t EepromConf::getCanBusSpeed() {
	return EEPROM.readByte(EEPROM_ADDRESS__CAN_BUS_SPEED);
}

void EepromConf::setCanBusSpeed(uint8_t speed) {
	EEPROM.writeByte(EEPROM_ADDRESS__CAN_BUS_SPEED, speed);
}

uint8_t EepromConf::getWatchdogTimeout() {
	return EEPROM.readByte(EEPROM_ADDRESS__WATCHDOG_TIMEOUT);
}

void EepromConf::setWatchdogTimeout(uint8_t to) {
	EEPROM.writeByte(EEPROM_ADDRESS__WATCHDOG_TIMEOUT, to);
}

uint8_t EepromConf::getAutoResetTime() {
	return EEPROM.readByte(EEPROM_ADDRESS__AUTO_RESET_TIME);
}

void EepromConf::setAutoResetTime(uint8_t ar) {
	EEPROM.writeByte(EEPROM_ADDRESS__AUTO_RESET_TIME, ar);
}

//* nastavi pocet sprav, ktore sa idu zapisat do eeprom
void EepromConf::setCountOfConf(uint8_t count) {
	EEPROM.writeByte(EEPROM_ADDRESS__CONF_COUNT, count);
}

//* vrati pocet sprav v eeprom
uint8_t EepromConf::getCountOfConf() {
	return EEPROM.readByte(EEPROM_ADDRESS__CONF_COUNT);
}

//* 
//uint8_t EepromConf::getSizeOfMsgs(const CONF_MESSAGE * msg) {
//	//* size of msg = 2bytes address + lenght of data (2bytes) + data (n bytes)
//	return 2 + 1 + msg->_length;
//}

//* write whole configuration to eeprom
void EepromConf::writeConf(uint8_t confCount, const CONF * pConf) {
	//DEBUG(VAR(pConf->count));
	//* write number of confs
	setCountOfConf(confCount);
	//* eeprom writing start at EEPROM_ADDRESS__CONFS
	short address = EEPROM_ADDRESS__CONFS;
	byte data[10];
	byte size;
	//* write particular confs
	for (byte i = 0; i < confCount; i++) {
		//* serialize selected device
		pConf->ppConfData[i]->setModeForEeprom(true);
		pConf->ppConfData[i]->serialize(data);
		//DEBUG(VAR(pConf->ppConfData[i]->_type));
		//DEBUG(VAR(data[0]) << 
		//	VAR(data[1]) << 
		//	VAR(data[2]) << 
		//	VAR(data[3]) << 
		//	VAR(data[4]) << 
		//	VAR(data[5]) << 
		//	VAR(data[6]) << 
		//	VAR(data[7]));		
		size = pConf->ppConfData[i]->getSize();
		//DEBUG(VAR(size));
		//DEBUG(VAR(address));
		//* write to eeprom
		eeprom_write_block(&data, (void*)address, size);
		//* increase address
		address += size;
	}
}

//* read whole configuration from eeprom
CONF * EepromConf::readConf() {
	//* from eeprom read number of confs
	uint8_t count = getCountOfConf();
	//* make new configuration
	_pConf = SmartHouse::newConf(count); // , getMacAddress());
	//_pConf->autoResetTime = EEPROM.readByte(EEPROM_ADDRESS__AUTO_RESET_TIME);
	//* eeprom reading starting on address EEPROM_ADDRESS__CONFS
	short address = EEPROM_ADDRESS__CONFS;
	CDataBase * pConfData = nullptr;
	byte size;
	byte data[10];
	DEBUG(F("Number of configurations in EEPROM = ") << count);
	
	//* read particular conf
	for (byte i = 0; i < count; i++) {
		//* get type of device
		byte deviceType = EEPROM.readByte(address);
		DEBUG(F("deviceType = ") << deviceType);
		switch (deviceType) {
			case DEVICE_TYPE_LIGHT:
				pConfData = new CConfMsg_light;
				break;
			case DEVICE_TYPE_SWITCH:
				pConfData = new CConfMsg_switch;				
				break;
			default:
				DEBUG(F("UNKNOWN DEVICE TYPE!!!"));
				break;
		}
		if (pConfData != nullptr) {
			//pConfData->setModeForEeprom(true);
			size = pConfData->getSize();
			//* incerement address, because we don't need type (first in memory stream)
			address += 1;
			//* read from eeprom
			eeprom_read_block(&data, (const void*)address, size);
			pConfData->deserialize(data);
			//* increase address to next conf
			address += size;
			//* insert to global structure
			_pConf->ppConfData[i] = pConfData;
		}
	}
	return _pConf;
}