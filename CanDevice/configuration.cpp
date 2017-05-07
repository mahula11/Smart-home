#include "configuration.h"


Configuration::Configuration() {
}


Configuration::~Configuration() {
}

byte Configuration::getCount() {
	return _pConf->count;
}

void Configuration::setConfiguration(CONF * pConf) {
	_pConf = pConf;
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
