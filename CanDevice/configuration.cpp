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

CDataBase * Configuration::getConf(byte index) {
	if (_pConf->count != 0 && index < _pConf->count) {
		return (_pConf->ppConfData[index]);
	} else {
		Serial << F("Configuration is empty");
	}
}

//* 0 + 2 bytes
//* vrati adresu zariadenia
MacID Configuration::getMacAddress() {
	return _pConf->macAddress;
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