#pragma once

#include "Arduino.h"
#include <smartHouse.h>

class ArrivedConfiguration {
private:
	uint8_t _count;
	//* tato premenna je menena pridavanim configuracie a zaroven sa kontroluje v loope, ci uz sa moze konfiguracia zapisat do eeprom (preto volatile)
	volatile uint8_t _numberOfAddedConf;
	CONF_MESSAGE * _pConf;
public:
	ArrivedConfiguration();
	~ArrivedConfiguration();

	bool isComplet();
	void setCount(uint8_t count);
	uint8_t getCount();
	void clean();
	const CONF_MESSAGE * getConf(uint8_t index);
	void addConf(const CONF_MESSAGE & msg);
};

