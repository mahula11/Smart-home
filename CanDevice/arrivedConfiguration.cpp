#include "arrivedConfiguration.h"


ArrivedConfiguration::ArrivedConfiguration() {
	clean();
	_confCount = 0;
}

ArrivedConfiguration::~ArrivedConfiguration() {
}

bool ArrivedConfiguration::isComplet() {
	return (_numberOfAddedConf == _confCount);
}

uint8_t ArrivedConfiguration::getCount() {
	return _confCount;
}

void ArrivedConfiguration::setCount(uint8_t count) {
	_pConf = SmartHouse::newConf(count);
	_confCount = count;
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

void ArrivedConfiguration::addConf(CDataBase * pConfDataBase) {
	_pConf->ppConfData[_numberOfAddedConf++] = pConfDataBase;
}