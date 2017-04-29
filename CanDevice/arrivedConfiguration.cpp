#include "arrivedConfiguration.h"


ArrivedConfiguration::ArrivedConfiguration() {
	clean();
}

ArrivedConfiguration::~ArrivedConfiguration() {
}

bool ArrivedConfiguration::isComplet() {
	return (_numberOfAddedConf == _count);
}

uint8_t ArrivedConfiguration::getCount() {
	return _count;
}

void ArrivedConfiguration::setCount(uint8_t count) {
	_count = count;
	if (_pConf) {
		delete[] _pConf;
	}
	_pConf = new CONF_MESSAGE[count];
}

void ArrivedConfiguration::clean() {
	_count = 0;
	_numberOfAddedConf = 0;
	if (_pConf) {
		delete[] _pConf;
	}
	_pConf = nullptr;
}

CONF_MESSAGE ArrivedConfiguration::getConf(uint8_t index) {
	return _pConf[index];
}

void ArrivedConfiguration::addConf(const CONF_MESSAGE & msg) {
	_pConf[_numberOfAddedConf++] = msg;
}