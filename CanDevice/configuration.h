#pragma once

#include <smartHouse.h>

#include <Streaming.h>

class Configuration {
private:
	CONF * _pConf;
public:
	Configuration();
	~Configuration();

	byte getCount();
	uint16_t getMacAddress();
	void setConfiguration(CONF * pConf);
	const CONF_MESSAGE * getConf(byte index);
};

