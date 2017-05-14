#include "configuration.h"


Configuration::Configuration() {
	_modified = false;
}


Configuration::~Configuration() {
}

byte Configuration::getCount() {
	return _pConf->count;
}

void Configuration::setConfiguration(CONF * pConf) {
	_pConf = pConf;
	_pValues = new VALUE[pConf->count];
}

const CONF_MESSAGE * Configuration::getConf(byte index) {
	if (_pConf->count != 0) {
		return (_pConf->pMsgs + index);
	} else {
		Serial << F("Configuration is empty");
	}
}

//* 0 + 2 bytes
//* vrati adresu zariadenia
uint16_t Configuration::getMacAddress() {
	return _pConf->macAddress;
}

bool Configuration::isDeviceLight(byte index) {
	//* Type is in first position in the DATA	
	return (DEVICE_TYPE_LIGHT == _pConf->pMsgs[index]._confData[LIGHT_ADDR_IN_CONF_TYPE]);
}

uint16_t Configuration::getLightsSwitchCanID(byte index) {
	return _pConf->pMsgs[index]._confData[LIGHT_ADDR_IN_CONF_SWITCH_CANID];
}

byte Configuration::getLightsSwitchGPIO(byte index) {
	return _pConf->pMsgs[index]._confData[LIGHT_ADDR_IN_CONF_SWITCH_GPIO];
}

void Configuration::setConfValue(byte index, byte value, bool modifyFlag) {
	//if (index > 7)
	_pValues[index]._value = value;
	_pValues[index]._modified = modifyFlag;
	_modified = true;
}

VALUE Configuration::getConfValue(byte index) {
	return _pValues[index];
}

bool Configuration::isModifiedValue() {
	return _modified;
}