#include "arrivedConfiguration.h"


ArrivedConfiguration::ArrivedConfiguration() {
	clean();
}

ArrivedConfiguration::~ArrivedConfiguration() {
}

bool ArrivedConfiguration::isComplet() {
	return (_numberOfAddedConf == _pConf->count);
}

uint8_t ArrivedConfiguration::getCount() {
	return _pConf->count;
}

void ArrivedConfiguration::setCount(uint8_t count) {
	_pConf = SmartHouse::newConf(count);
}

void ArrivedConfiguration::clean() {
	//_count = 0;
	_numberOfAddedConf = 0;
	//if (_pConf) {
	//	delete[] _pConf;
	//}
	_pConf = nullptr;
}

const CONF * ArrivedConfiguration::getConf() {
	return _pConf;
}

void ArrivedConfiguration::addConf(DATA_BASE * pConfDataBase) {
	_pConf->ppConfData[_numberOfAddedConf++] = pConfDataBase;
}