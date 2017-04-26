#pragma once

#include "Arduino.h"
#include <smartHouse.h>

class ArrivedConfiguration {
private:
	uint8_t _count;
	CONF_MESSAGE * _pConf;
public:
	ArrivedConfiguration();
	~ArrivedConfiguration();

	void isAvailable();
	void setCount(uint8_t count);
	uint8_t getCount();
	void clean();
	void getConf(uint8_t index);
};

