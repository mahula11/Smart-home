#include "configuration.h"


Configuration::Configuration() {
	_modified = false;
	_pConf = nullptr;
	_pValues = nullptr;
}


Configuration::~Configuration() {
}

byte Configuration::getCount() {
	return _confCount;
}

void Configuration::setConfigurationStatic(uint8_t confCount, uint8_t watchdogTimeout, uint8_t autoResetTime) {
	_autoResetTime = autoResetTime;
	_confCount = confCount;
	_watchdogTimeout = watchdogTimeout;
}

void Configuration::setConfiguration(CONF * pConf) {
	_pConf = pConf;
	_pValues = new VALUE[_confCount];
}

CDataBase * Configuration::getConf(byte index) {
	if (_confCount != 0 && index < _confCount) {
		return (_pConf->ppConfData[index]);
	} else {
		DEBUG(F("Configuration is empty"));
		return nullptr;
	}
}

//* 0 + 2 bytes
//* vrati adresu zariadenia
MacID Configuration::getMacAddress() {
	return _macID;
}

void Configuration::setConfValue(byte index, byte value, bool modifyFlag) {
	//if (index > 7)
	_pValues[index]._value = value;
	_pValues[index]._modified = modifyFlag;
	_modified = true;
}

VALUE * Configuration::getConfValue(byte index) {
	return &_pValues[index];
}

bool Configuration::isModifiedValue() {
	return _modified;
}

void Configuration::resetModifiedValue() {
	_modified = false;
}

unsigned long Configuration::getAutoResetTime() {
	
	switch (_autoResetTime) {
		case arDisable:
			return 0UL;
		case ar10s:
			return 10000UL;		//* = 10 * 1000
		case ar1m:
			return 60000UL;		//* = 1 * 60 * 1000
		case ar15m:
			return 900000UL;	//* = 15 * 60 * 1000
		case ar1h:
			return 3600000UL;	//* = 60 * 60 * 1000
		case ar2h:
			return 7200000UL;	//* = 2 * 60 * 60 * 1000
		case ar3h:
			return 10800000UL;	//* = 3 * 60 * 60 * 1000
		case ar4h:
			return 14400000UL;	//* = 4 * 60 * 60 * 1000
		case ar5h:
			return 18000000UL;	//* = 5 * 60 * 60 * 1000
		case ar6h:
			return 21600000UL;	//* = 6 * 60 * 60 * 1000
		case ar7h:
			return 25200000UL;	//* = 7 * 60 * 60 * 1000
		case ar8h:
			return 28800000UL;	//* = 8 * 60 * 60 * 1000
		default:
			DEBUG(F("Unknown value of AutoResetTime"));
			return 0UL;
	}
}

void Configuration::setAutoResetTime(uint8_t val) {
	_autoResetTime = val;
}